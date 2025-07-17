/*
 * Programme ESP8266 (D1 Mini) pour lecture/écriture RFID
 * Module: RC522 connecté en SPI
 * 
 * Fonctionnalités:
 * - Lecture de tags RFID/NFC
 * - Écriture sur des tags RFID/NFC
 * - Interface série pour commandes
 * - Gestion d'erreurs
 */
#include <config.h>
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>
#include <webpage.h>
#include <login_page.h>


// Création des instances
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
ESP8266WebServer webServer(80);

// Variables globales
String mode = "READ"; // READ, WRITE
String dataToWrite = "";
bool continuousMode = true;
bool otaEnabled = true;
bool wifiConnected = false;
bool otaInProgress = false;
String lastCardInfo = "Aucune carte";
String apiUrl = "";
String wifiSsid = "";
String wifiPass = "";

// === Paramètre délai entre scans RFID ===
#define SCAN_DELAY_ADDR  (WIFI_PASS_ADDR + WIFI_PASS_MAXLEN) // placer après le WiFi
#define SCAN_DELAY_SIZE  4
unsigned long scanDelayMs = 3000; // 3 secondes par défaut
unsigned long lastScanTime = 0;

// === Historique des envois à l'API ===
#define API_LOG_SIZE 32
struct ApiLogEntry {
    unsigned long timestamp;
    String uid;
    int httpCode;
    String url; // Ajouté pour journaliser l'URL utilisée
};
ApiLogEntry apiLog[API_LOG_SIZE];
int apiLogIndex = 0;

// === Code d'accès à l'interface web ===
#define WEB_CODE_ADDR (SCAN_DELAY_ADDR + SCAN_DELAY_SIZE)
#define WEB_CODE_MAXLEN 16
String webAccessCode = "admin";

// === Prototypes des fonctions ===
void setup();
void loop();
void handleSerialCommands();
void handleRFIDOperations();
void readCard();
void writeCard();
void formatCard();
void backupCard();
void showSystemInfo();
void printHex(byte *buffer, byte bufferSize);
void printText(byte *buffer, byte bufferSize);
void testRFIDModule();
void connectToWiFi();
void setupOTA();
void setupWebServer();
void loadApiUrl();
void saveApiUrl(const String& url);
void blinkLed(int times = 2, int duration = 100);
void sendUidToApi(const String& uid);
void loadWifiConfig();
void saveWifiConfig(const String& ssid, const String& pass);
void startConfigAP();
void loadScanDelay();
void saveScanDelay(unsigned long val);
void logApiSend(const String& uid, int httpCode, const String& url);
void loadWebAccessCode();
void saveWebAccessCode(const String& code);
String getCardDump();

void setup() {
    Serial.begin(115200);
    while (!Serial);
    // Initialisation SPI
    SPI.begin();
    // Initialisation du module RFID
    mfrc522.PCD_Init();
    // Préparation de la clé par défaut
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
    Serial.println("=== ESP8266 D1 Mini RFID Reader/Writer ===");
    Serial.println("Module RC522 initialisé");
    Serial.println("Commandes disponibles:");
    Serial.println("- READ: Mode lecture");
    Serial.println("- WRITE <data>: Mode écriture");
    Serial.println("- SCAN: Scan continu");
    Serial.println("- STOP: Arrêter le scan");
    Serial.println("- INFO: Informations système");
    Serial.println("- FORMAT: Formater une carte");
    Serial.println("- BACKUP: Sauvegarder une carte");
    Serial.println("- OTA: Activer les mises à jour OTA");
    Serial.println("- WIFI: Se connecter au WiFi");
    Serial.println("========================================");
    mfrc522.PCD_DumpVersionToSerial();
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // Éteint la LED (inversée sur ESP8266)
    loadApiUrl();
    loadWifiConfig();
    loadScanDelay();
    loadWebAccessCode();
    if (!otaEnabled) {
        WiFi.mode(WIFI_OFF);
    } else {
        if (wifiSsid.length() == 0 || wifiPass.length() == 0) {
            startConfigAP();
        } else {
            connectToWiFi();
            if (wifiConnected) {
                setupOTA();
            }
        }
    }
}

void loop() {
    if (otaInProgress) {
        ArduinoOTA.handle();
        return;
    }
    // Gestion des commandes série
    if (Serial.available()) {
        handleSerialCommands();
    }
    
    // Mode continu
    if (continuousMode) {
        handleRFIDOperations();
    }
    
    // Gestion OTA si activé
    if (otaEnabled && wifiConnected) {
        ArduinoOTA.handle();
        MDNS.update();
    }
    // Toujours gérer le serveur web, même en AP
    webServer.handleClient();
    delay(100);
}

void handleSerialCommands() {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toUpperCase();
    
    if (command == "READ") {
        mode = "READ";
        continuousMode = true;
        Serial.println("Mode lecture activé");
    }
    else if (command.startsWith("WRITE ")) {
        mode = "WRITE";
        dataToWrite = command.substring(6);
        continuousMode = true;
        Serial.println("Mode écriture activé - Données: " + dataToWrite);
    }
    else if (command == "SCAN") {
        continuousMode = true;
        Serial.println("Scan continu activé");
    }
    else if (command == "STOP") {
        continuousMode = false;
        Serial.println("Scan arrêté");
    }
    else if (command == "INFO") {
        showSystemInfo();
    }
    else if (command == "FORMAT") {
        mode = "FORMAT";
        continuousMode = true;
        Serial.println("Mode formatage activé - Approchez une carte");
    }
    else if (command == "BACKUP") {
        mode = "BACKUP";
        continuousMode = true;
        Serial.println("Mode sauvegarde activé - Approchez une carte");
    }
    else if (command == "OTA") {
        setupOTA();
    }
    else if (command == "WIFI") {
        connectToWiFi();
    }
    else {
        Serial.println("Commande inconnue: " + command);
        Serial.println("Commandes: READ, WRITE <data>, SCAN, STOP, INFO, FORMAT, BACKUP, OTA, WIFI");
    }
}

void handleRFIDOperations() {
    // Délai entre scans
    if (millis() - lastScanTime < scanDelayMs) {
        return;
    }
    // Recherche de nouvelles cartes
    if (!mfrc522.PICC_IsNewCardPresent()) {
        return;
    }
    // Sélection de la carte
    if (!mfrc522.PICC_ReadCardSerial()) {
        return;
    }
    lastScanTime = millis();
    blinkLed();
    Serial.println("\n=== Carte détectée ===");
    // Affichage de l'UID
    Serial.print("UID: ");
    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
        if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
        uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println();
    // Affichage du type de carte
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.print("Type: ");
    Serial.println(mfrc522.PICC_GetTypeName(piccType));
    // Opération selon le mode
    String cardContent = "UID: " + uid + "\nType: " + String(mfrc522.PICC_GetTypeName(piccType)) + "<br/>\n";
    if (mode == "READ") {
        // Vérification du type de carte
        if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
            piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
            piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
            cardContent += "<b>Type de carte non supporté pour la lecture mémoire (" + String(mfrc522.PICC_GetTypeName(piccType)) + ")</b>";
            lastCardInfo = cardContent;
            sendUidToApi(uid);
            return;
        }
        // Lecture complète des secteurs
        String dump = getCardDump();
        lastCardInfo = cardContent + dump;
        sendUidToApi(uid);
    } else if (mode == "WRITE") {
        lastCardInfo = cardContent + "(Mode écriture)";
        writeCard();
    } else if (mode == "FORMAT") {
        lastCardInfo = cardContent + "(Mode formatage)";
        formatCard();
    } else if (mode == "BACKUP") {
        lastCardInfo = cardContent + "(Mode sauvegarde)";
        backupCard();
    }
    // Arrêt de la communication avec la carte
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    Serial.println("===================\n");
}

String getCardDump() {
    Serial.println("--- Lecture complète de la carte ---");
    String sectorDump = "<b>Lecture des secteurs RFID :</b><br/>";
    for (byte sector = 1; sector < 16; sector++) {
        Serial.print("Secteur ");
        Serial.print(sector);
        Serial.println(":");
        sectorDump += "Secteur " + String(sector) + ":<br/>";
        for (byte block = 0; block < 3; block++) {
            byte blockAddr = sector * 4 + block;
            byte buffer[18] = {0};
            byte size = sizeof(buffer);
            // Réinitialisation de la clé à chaque lecture (clé 0xFF)
            for (byte k = 0; k < 6; k++) key.keyByte[k] = 0xFF;
            Serial.print("  Bloc ");
            Serial.print(blockAddr);
            Serial.print(" | Clé utilisée: ");
            for (byte k = 0; k < 6; k++) Serial.print(key.keyByte[k], HEX);
            Serial.print(" | ");
            delay(50); // Délai plus long avant authentification
            MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
                MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                blockAddr,
                &key,
                &(mfrc522.uid)
            );
            String hexStr = "";
            String txtStr = "";
            if (status == MFRC522::STATUS_OK) {
                status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
                if (status == MFRC522::STATUS_OK) {
                    for (byte i = 0; i < 16; i++) {
                        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
                        Serial.print(buffer[i], HEX);
                        hexStr += (buffer[i] < 0x10 ? " 0" : " ");
                        hexStr += String(buffer[i], HEX);
                    }
                    Serial.print(" | ");
                    for (byte i = 0; i < 16; i++) {
                        if (buffer[i] >= 32 && buffer[i] <= 126) {
                            Serial.print((char)buffer[i]);
                            txtStr += (char)buffer[i];
                        } else {
                            Serial.print(".");
                            txtStr += ".";
                        }
                    }
                    Serial.println();
                } else {
                    Serial.print("  Bloc ");
                    Serial.print(blockAddr);
                    Serial.print(": Lecture échouée: ");
                    Serial.println(mfrc522.GetStatusCodeName(status));
                    hexStr = "(Lecture échouée)";
                    txtStr = "(Lecture échouée)";
                }
            } else {
                Serial.print("  Bloc ");
                Serial.print(blockAddr);
                Serial.print(": Auth échouée: ");
                Serial.println(mfrc522.GetStatusCodeName(status));
                if (status == MFRC522::STATUS_TIMEOUT) {
                    Serial.println("[AIDE] Vérifiez le câblage SPI, l'alimentation du module RC522, et la position de la carte.");
                }
                hexStr = "(Auth échouée)";
                txtStr = "(Auth échouée)";
            }
            sectorDump += "&nbsp;&nbsp;Bloc " + String(blockAddr) + ": " + hexStr + " | " + txtStr + "<br/>";
        }
    }
    return sectorDump;
}

void writeCard() {
    Serial.println("--- Écriture des données ---");
    
    byte sector = 1;
    byte blockAddr = 4;
    byte buffer[16];
    
    // Préparation des données
    memset(buffer, 0, sizeof(buffer));
    dataToWrite.getBytes(buffer, min(dataToWrite.length() + 1, (unsigned int)16));
    
    // Authentification
    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A, 
        blockAddr, 
        &key, 
        &(mfrc522.uid)
    );
    
    if (status != MFRC522::STATUS_OK) {
        Serial.print("Authentification échouée: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }
    
    // Écriture du bloc
    status = mfrc522.MIFARE_Write(blockAddr, buffer, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print("Écriture échouée: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }
    
    Serial.println("Données écrites avec succès!");
    Serial.print("Contenu écrit: ");
    for (byte i = 0; i < 16; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
    Serial.print(" | ");
    for (byte i = 0; i < 16; i++) {
        if (buffer[i] >= 32 && buffer[i] <= 126) {
            Serial.print((char)buffer[i]);
        } else {
            Serial.print(".");
        }
    }
    Serial.println();
}

void formatCard() {
    Serial.println("--- Formatage de la carte ---");
    Serial.println("ATTENTION: Cette opération effacera toutes les données!");
    
    byte emptyBlock[16] = {0};
    int blocksFormatted = 0;
    
    // Formatage des secteurs 1 à 15 (éviter le secteur 0)
    for (byte sector = 1; sector < 16; sector++) {
        for (byte block = 0; block < 3; block++) { // Éviter le bloc trailer
            byte blockAddr = sector * 4 + block;
            
            // Authentification
            MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
                MFRC522::PICC_CMD_MF_AUTH_KEY_A, 
                blockAddr, 
                &key, 
                &(mfrc522.uid)
            );
            
            if (status == MFRC522::STATUS_OK) {
                // Écriture du bloc vide
                status = mfrc522.MIFARE_Write(blockAddr, emptyBlock, 16);
                if (status == MFRC522::STATUS_OK) {
                    blocksFormatted++;
                    if (blocksFormatted % 10 == 0) {
                        Serial.print(".");
                    }
                }
            }
        }
    }
    
    Serial.println();
    Serial.print("Formatage terminé! ");
    Serial.print(blocksFormatted);
    Serial.println(" blocs formatés.");
}

void backupCard() {
    Serial.println("--- Sauvegarde de la carte ---");
    
    // Affichage de l'UID
    Serial.print("UID sauvegardé: ");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println();
    
    // Sauvegarde des données importantes
    Serial.println("--- Contenu des secteurs ---");
    
    for (byte sector = 0; sector < 16; sector++) {
        Serial.print("Secteur ");
        Serial.print(sector);
        Serial.println(":");
        
        for (byte block = 0; block < 4; block++) {
            byte blockAddr = sector * 4 + block;
            byte buffer[18];
            byte size = sizeof(buffer);
            
            // Authentification
            MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
                MFRC522::PICC_CMD_MF_AUTH_KEY_A, 
                blockAddr, 
                &key, 
                &(mfrc522.uid)
            );
            
            if (status == MFRC522::STATUS_OK) {
                // Lecture du bloc
                status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
                if (status == MFRC522::STATUS_OK) {
                    Serial.print("  B");
                    Serial.print(block);
                    Serial.print(": ");
                    for (byte i = 0; i < 16; i++) {
                        Serial.print(buffer[i] < 0x10 ? "0" : "");
                        Serial.print(buffer[i], HEX);
                    }
                    Serial.println();
                }
            }
        }
    }
    
    Serial.println("Sauvegarde terminée!");
}

void showSystemInfo() {
    Serial.println("\n=== Informations système ===");
    Serial.println("Modèle: ESP8266 D1 Mini");
    Serial.println("Module RFID: RC522");
    Serial.println("Fréquence: 13.56 MHz");
    Serial.println("Connexions SPI:");
    Serial.println("  RST: D3 (GPIO 0)");
    Serial.println("  SS:  D8 (GPIO 15)");
    Serial.println("  SCK: D5 (GPIO 14)");
    Serial.println("  MOSI:D7 (GPIO 13)");
    Serial.println("  MISO:D6 (GPIO 12)");
    Serial.print("Mode actuel: ");
    Serial.println(mode);
    Serial.print("Scan continu: ");
    Serial.println(continuousMode ? "Activé" : "Désactivé");
    Serial.print("Mémoire libre: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    Serial.print("Fréquence CPU: ");
    Serial.print(ESP.getCpuFreqMHz());
    Serial.println(" MHz");
    Serial.print("WiFi: ");
    Serial.println(wifiConnected ? "Connecté" : "Déconnecté");
    if (wifiConnected) {
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    }
    Serial.print("OTA: ");
    Serial.println(otaEnabled ? "Activé" : "Désactivé");
    Serial.println("============================\n");
}

// Fonction utilitaire pour afficher les données en hexadécimal
void printHex(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

// Fonction utilitaire pour afficher les données en texte
void printText(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        if (buffer[i] >= 32 && buffer[i] <= 126) {
            Serial.print((char)buffer[i]);
        } else {
            Serial.print(".");
        }
    }
}

// Fonction de test de connectivité
void testRFIDModule() {
    Serial.println("=== Test du module RFID ===");
    
    // Test de communication SPI
    byte version = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
    Serial.print("Version du firmware: 0x");
    Serial.println(version, HEX);
    
    if (version == 0x00 || version == 0xFF) {
        Serial.println("ERREUR: Aucune communication avec le module RC522!");
        Serial.println("Vérifiez les connexions SPI.");
    } else {
        Serial.println("Module RC522 détecté et fonctionnel.");
    }
    
    Serial.println("===========================\n");
}

// Fonction de connexion WiFi
void connectToWiFi() {
    loadWifiConfig();
    if (wifiSsid.length() == 0 || wifiPass.length() == 0) {
        startConfigAP();
        return;
    }
    Serial.println("=== Connexion WiFi ===");
    Serial.print("Connexion à ");
    Serial.println(wifiSsid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
    int attempts = 0;
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < 20000) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.println();
        Serial.println("WiFi connecté!");
        Serial.print("Adresse IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("Signal: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        // Attendre que le serveur web et mDNS soient prêts avant d'allumer la LED
        delay(200); // Laisser le temps au WiFi
        setupWebServer();
        bool mdnsReady = false;
        for (int i = 0; i < 10; i++) { // Attendre jusqu'à 2s max
            if (MDNS.begin(HOSTNAME)) {
                mdnsReady = true;
                break;
            }
            delay(200);
        }
        if (mdnsReady) {
            MDNS.addService("http", "tcp", 80);
        }
        digitalWrite(LED_PIN, LOW); // Allume la LED (inversée sur ESP8266)
    } else {
        Serial.println();
        Serial.println("Échec de connexion WiFi, démarrage AP config");
        wifiConnected = false;
        startConfigAP();
    }
    Serial.println("=====================");
}

void startConfigAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("AP de configuration démarré. SSID: ");
    Serial.println(AP_SSID);
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());
    setupWebServer(); // Démarre le serveur web en mode AP
}

// Configuration OTA
void setupOTA() {
    if (!wifiConnected) {
        Serial.println("WiFi requis pour OTA. Utilisez la commande WIFI d'abord.");
        return;
    }
    Serial.println("=== Configuration OTA ===");
    ArduinoOTA.setHostname(HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else {
            type = "filesystem";
        }
        Serial.println("Début mise à jour " + type);
        continuousMode = false;
        otaInProgress = true;
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nMise à jour terminée");
        otaInProgress = false;
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progression: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Erreur[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Échec d'authentification");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Échec de début");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Échec de connexion");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Échec de réception");
        } else if (error == OTA_END_ERROR) {
            Serial.println("Échec de fin");
        }
        // Reprendre les opérations RFID en cas d'erreur
        continuousMode = true;
    });
    ArduinoOTA.begin();
    
    // Configuration mDNS
    if (MDNS.begin(HOSTNAME)) {
        Serial.println("mDNS démarré");
        MDNS.addService("http", "tcp", 80);
    }
    
    // Configuration serveur web pour interface OTA
    setupWebServer();
    
    otaEnabled = true;
    
    Serial.println("OTA activé!");
    Serial.println("Nom d'hôte: " + String(HOSTNAME));
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.println("Interface web: http://" + WiFi.localIP().toString());
    Serial.println("========================");
}

// Configuration du serveur web pour interface OTA
void setupWebServer() {
    static bool started = false;
    if (started) return;
    // Page principale protégée par code
    webServer.on("/", HTTP_GET, []() {
        if (!webServer.hasArg("code") || webServer.arg("code") != webAccessCode) {
            webServer.send(401, "text/html", LOGIN_PAGE);
            return;
        }
        String page = WEB_PAGE;
        String msg = "";
        if (webServer.hasArg("msg")) msg = webServer.arg("msg");
        page.replace("%MSG%", msg);
        webServer.send(200, "text/html", page);
    });
    // Modification du code d'accès via POST sur la page principale
    webServer.on("/", HTTP_POST, []() {
        if (!webServer.hasArg("code") || webServer.arg("code") != webAccessCode) {
            webServer.send(401, "text/html", LOGIN_PAGE);
            return;
        }
        if (webServer.hasArg("newcode")) {
            saveWebAccessCode(webServer.arg("newcode"));
            String page = WEB_PAGE;
            page.replace("%MSG%", "Code modifié !");
            webServer.send(200, "text/html", page);
        } else {
            String page = WEB_PAGE;
            page.replace("%MSG%", "Erreur : paramètre manquant.");
            webServer.send(400, "text/html", page);
        }
    });
    // Toutes les routes API sont publiques
    webServer.on("/api/command", []() {
        if (webServer.hasArg("cmd")) {
            String cmd = webServer.arg("cmd");
            cmd.toUpperCase();
            if (cmd == "READ") {
                mode = "READ";
                continuousMode = true;
            } else if (cmd == "STOP") {
                continuousMode = false;
            } else if (cmd == "INFO") {
                showSystemInfo();
            }
            webServer.send(200, "text/plain", "Commande exécutée: " + cmd);
        } else {
            webServer.send(400, "text/plain", "Paramètre 'cmd' manquant");
        }
    });
    webServer.on("/api/write", []() {
        if (webServer.hasArg("data")) {
            dataToWrite = webServer.arg("data");
            mode = "WRITE";
            continuousMode = true;
            webServer.send(200, "text/plain", dataToWrite);
        } else {
            webServer.send(400, "text/plain", "Paramètre 'data' manquant");
        }
    });
    webServer.on("/api/status", []() {
        String json = "{";
        json += "\"mode\":\"" + mode + "\",";
        json += "\"memory\":" + String(ESP.getFreeHeap()) + ",";
        json += "\"uptime\":" + String(millis() / 1000) + ",";
        json += "\"rssi\":" + String(WiFi.RSSI());
        json += "}";
        webServer.send(200, "application/json", json);
    });
    webServer.on("/api/lastcard", []() {
        webServer.send(200, "text/plain", lastCardInfo);
    });
    
    // API pour l'URL de l'API
    webServer.on("/api/apiurl", []() {
        webServer.send(200, "text/plain", apiUrl);
    });
    webServer.on("/api/setapiurl", []() {
        if (webServer.hasArg("url")) {
            saveApiUrl(webServer.arg("url"));
            webServer.send(200, "text/plain", "OK");
        } else {
            webServer.send(400, "text/plain", "Paramètre 'url' manquant");
        }
    });
    
    // API pour la config WiFi
    webServer.on("/api/wificonfig", []() {
        String json = "{";
        json += "\"ssid\":\"" + wifiSsid + "\",";
        json += "\"pass\":\"" + wifiPass + "\"}";
        webServer.send(200, "application/json", json);
    });
    webServer.on("/api/setwificonfig", []() {
        if (webServer.hasArg("ssid") && webServer.hasArg("pass")) {
            saveWifiConfig(webServer.arg("ssid"), webServer.arg("pass"));
            webServer.send(200, "text/plain", "OK");
        } else {
            webServer.send(400, "text/plain", "Paramètres 'ssid' ou 'pass' manquants");
        }
    });
    
    // API pour le délai entre scans RFID
    webServer.on("/api/scandelay", []() {
        if (webServer.method() == HTTP_GET) {
            webServer.send(200, "text/plain", String(scanDelayMs));
        } else if (webServer.method() == HTTP_POST) {
            if (webServer.hasArg("delay")) {
                unsigned long val = webServer.arg("delay").toInt();
                if (val < 500) val = 500;
                saveScanDelay(val);
                webServer.send(200, "text/plain", "OK");
            } else {
                webServer.send(400, "text/plain", "Paramètre 'delay' manquant");
            }
        }
    });
    
    // API pour l'historique des envois à l'API
    webServer.on("/api/apilog", []() {
        String json = "[";
        int count = 0;
        // Afficher les entrées dans l'ordre chronologique (de la plus ancienne à la plus récente)
        for (int i = 0; i < API_LOG_SIZE; i++) {
            int idx = (apiLogIndex + i) % API_LOG_SIZE;
            if (apiLog[idx].uid.length() == 0) continue;
            if (count > 0) json += ",";
            json += "{\"t\":" + String(apiLog[idx].timestamp)
                + ",\"uid\":\"" + apiLog[idx].uid
                + "\",\"code\":" + String(apiLog[idx].httpCode)
                + ",\"url\":\"" + apiLog[idx].url + "\"}";
            count++;
        }
        json += "]";
        webServer.send(200, "application/json", json);
    });
    
    // Redémarrage
    webServer.on("/restart", []() {
        webServer.send(200, "text/html", "<h1>Redémarrage en cours...</h1><script>setTimeout(function(){location.href='/';}, 10000);</script>");
        delay(1000);
        ESP.restart();
    });
    
    // Gestion de la mise à jour OTA via web
    webServer.on("/update", HTTP_POST, []() {
        webServer.sendHeader("Connection", "close");
        webServer.send(200, "text/html", (Update.hasError()) ? 
            "<h1>❌ Échec de la mise à jour</h1><a href='/'>Retour</a>" : 
            "<h1>✅ Mise à jour réussie</h1><p>Redémarrage en cours...</p><script>setTimeout(function(){location.href='/';}, 5000);</script>");
        ESP.restart();
    }, []() {
        HTTPUpload& upload = webServer.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("Mise à jour: %s\n", upload.filename.c_str());
            if (!Update.begin(upload.contentLength)) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("Mise à jour réussie: %u\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
        }
    });
    
    // API pour faire clignoter le buzzer
    webServer.on("/api/buzzer", []() {
        int times = 1;
        int duration = 100;
        if (webServer.hasArg("times")) times = webServer.arg("times").toInt();
        if (webServer.hasArg("duration")) duration = webServer.arg("duration").toInt();
        if (times < 1) times = 1;
        if (duration < 10) duration = 10;
        for (int i = 0; i < times; i++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(duration);
            digitalWrite(BUZZER_PIN, LOW);
            delay(duration);
        }
        webServer.send(200, "text/plain", "Buzzer OK");
    });
    
    // API pour le code d'accès web
    webServer.on("/api/webcode", []() {
        if (webServer.method() == HTTP_GET) {
            webServer.send(200, "text/plain", webAccessCode);
        } else if (webServer.method() == HTTP_POST) {
            if (webServer.hasArg("code")) {
                saveWebAccessCode(webServer.arg("code"));
                webServer.send(200, "text/plain", "OK");
            } else {
                webServer.send(400, "text/plain", "Paramètre 'code' manquant");
            }
        }
    });
    
    webServer.begin();
    started = true;
    Serial.print("Serveur web démarré sur IP: ");
    Serial.println(WiFi.softAPIP());
}

// Fonction pour charger l'URL de l'API depuis l'EEPROM
void loadApiUrl() {
    EEPROM.begin(EEPROM_SIZE);
    char buf[API_URL_MAXLEN+1];
    for (int i = 0; i < API_URL_MAXLEN; i++) {
        buf[i] = EEPROM.read(API_URL_ADDR + i);
        if (buf[i] == '\0') break;
    }
    buf[API_URL_MAXLEN] = '\0';
    apiUrl = String(buf);
    EEPROM.end();
    if (apiUrl.length() == 0) apiUrl = "http://";
}

// Fonction pour sauvegarder l'URL de l'API dans l'EEPROM
void saveApiUrl(const String& url) {
    EEPROM.begin(EEPROM_SIZE);
    for (int i = 0; i < API_URL_MAXLEN; i++) {
        if (i < url.length()) EEPROM.write(API_URL_ADDR + i, url[i]);
        else EEPROM.write(API_URL_ADDR + i, 0);
    }
    EEPROM.commit();
    EEPROM.end();
    apiUrl = url;
}

void loadWifiConfig() {
    EEPROM.begin(EEPROM_SIZE);
    char ssid[WIFI_SSID_MAXLEN+1] = {0};
    char pass[WIFI_PASS_MAXLEN+1] = {0};
    bool ssidValid = false;
    bool passValid = false;
    for (int i = 0; i < WIFI_SSID_MAXLEN; i++) {
        byte b = EEPROM.read(WIFI_SSID_ADDR + i);
        if (b == 0xFF || b == 0) break;
        ssid[i] = b;
        ssidValid = true;
    }
    ssid[WIFI_SSID_MAXLEN] = '\0';
    for (int i = 0; i < WIFI_PASS_MAXLEN; i++) {
        byte b = EEPROM.read(WIFI_PASS_ADDR + i);
        if (b == 0xFF || b == 0) break;
        pass[i] = b;
        passValid = true;
    }
    pass[WIFI_PASS_MAXLEN] = '\0';
    wifiSsid = ssidValid ? String(ssid) : "";
    wifiPass = passValid ? String(pass) : "";
    wifiSsid.trim();
    wifiPass.trim();
    EEPROM.end();
}

void saveWifiConfig(const String& ssid, const String& pass) {
    EEPROM.begin(EEPROM_SIZE);
    for (int i = 0; i < WIFI_SSID_MAXLEN; i++) {
        if (i < ssid.length()) EEPROM.write(WIFI_SSID_ADDR + i, ssid[i]);
        else EEPROM.write(WIFI_SSID_ADDR + i, 0);
    }
    for (int i = 0; i < WIFI_PASS_MAXLEN; i++) {
        if (i < pass.length()) EEPROM.write(WIFI_PASS_ADDR + i, pass[i]);
        else EEPROM.write(WIFI_PASS_ADDR + i, 0);
    }
    EEPROM.commit();
    EEPROM.end();
    wifiSsid = ssid;
    wifiPass = pass;
}

// Fonction pour charger le délai entre scans RFID depuis l'EEPROM
void loadScanDelay() {
    EEPROM.begin(EEPROM_SIZE);
    unsigned long val = 0;
    bool valid = true;
    for (int i = 0; i < SCAN_DELAY_SIZE; i++) {
        byte b = EEPROM.read(SCAN_DELAY_ADDR + i);
        if (b == 0xFF) valid = false;
        val |= ((unsigned long)b) << (8 * i);
    }
    if (!valid || val < 500) {
        // Écrire la valeur par défaut 3000 ms en EEPROM
        val = 3000;
        for (int i = 0; i < SCAN_DELAY_SIZE; i++) {
            EEPROM.write(SCAN_DELAY_ADDR + i, (val >> (8 * i)) & 0xFF);
        }
        EEPROM.commit();
    }
    EEPROM.end();
    scanDelayMs = val;
}

// Fonction pour sauvegarder le délai entre scans RFID dans l'EEPROM
void saveScanDelay(unsigned long val) {
    EEPROM.begin(EEPROM_SIZE);
    for (int i = 0; i < SCAN_DELAY_SIZE; i++) {
        EEPROM.write(SCAN_DELAY_ADDR + i, (val >> (8 * i)) & 0xFF);
    }
    EEPROM.commit();
    EEPROM.end();
    scanDelayMs = val;
}

// Fonction pour faire clignoter la LED
void blinkLed(int times, int duration) {
    for (int i = 0; i < times; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(duration);
        digitalWrite(BUZZER_PIN, LOW);
        delay(duration);
    }
}

// Fonction pour envoyer l'UID à l'API
void sendUidToApi(const String& uid) {
    String url = apiUrl;
    Serial.println("[API] Préparation envoi UID: " + uid + " vers " + url);
    if (WiFi.status() == WL_CONNECTED && url.startsWith("http")) {
        HTTPClient http;
        bool beginOk = false;
        if (url.startsWith("https://")) {
            #include <WiFiClientSecure.h>
            WiFiClientSecure client;
            client.setInsecure(); // Pour ignorer la vérification du certificat (à sécuriser en prod)
            beginOk = http.begin(client, url);
        } else {
            WiFiClient client;
            beginOk = http.begin(client, url);
        }
        if (!beginOk) {
            Serial.println("[API] Erreur http.begin()");
            logApiSend(uid, -2, url);
            return;
        }
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        Serial.println("[API] Envoi POST...");
        int httpCode = http.POST("uid=" + uid);
        Serial.print("[API] Code HTTP: ");
        Serial.println(httpCode);
        if (httpCode > 0) {
            String payload = http.getString();
            Serial.print("[API] Réponse: ");
            Serial.println(payload);
        } else {
            Serial.println("[API] Erreur POST: " + String(http.errorToString(httpCode)));
        }
        logApiSend(uid, httpCode, url);
        http.end();
    } else {
        Serial.println("[API] WiFi non connecté ou URL invalide");
        logApiSend(uid, -1, url);
    }
}

void logApiSend(const String& uid, int httpCode, const String& url) {
    apiLog[apiLogIndex].timestamp = millis() / 1000;
    apiLog[apiLogIndex].uid = uid;
    apiLog[apiLogIndex].httpCode = httpCode;
    apiLog[apiLogIndex].url = url;
    apiLogIndex = (apiLogIndex + 1) % API_LOG_SIZE;
}

// Fonction pour charger le code d'accès à l'interface web depuis l'EEPROM
void loadWebAccessCode() {
    EEPROM.begin(EEPROM_SIZE);
    char buf[WEB_CODE_MAXLEN+1];
    for (int i = 0; i < WEB_CODE_MAXLEN; i++) {
        buf[i] = EEPROM.read(WEB_CODE_ADDR + i);
        if (buf[i] == '\0') break;
    }
    buf[WEB_CODE_MAXLEN] = '\0';
    webAccessCode = String(buf);
    EEPROM.end();
    if (webAccessCode.length() == 0) webAccessCode = "admin";
}

// Fonction pour sauvegarder le code d'accès à l'interface web dans l'EEPROM
void saveWebAccessCode(const String& code) {
    EEPROM.begin(EEPROM_SIZE);
    for (int i = 0; i < WEB_CODE_MAXLEN; i++) {
        if (i < code.length()) EEPROM.write(WEB_CODE_ADDR + i, code[i]);
        else EEPROM.write(WEB_CODE_ADDR + i, 0);
    }
    EEPROM.commit();
    EEPROM.end();
    webAccessCode = code;
}