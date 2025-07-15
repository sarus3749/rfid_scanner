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

// Configuration des broches pour D1 Mini ESP8266
#define RST_PIN         D3    // GPIO 0  (D3)
#define SS_PIN          D8    // GPIO 15 (D8)
// SPI utilise les broches par défaut:
// MOSI = D7 (GPIO 13)
// MISO = D6 (GPIO 12)
// SCK  = D5 (GPIO 14)

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
String lastCardInfo = "Aucune carte";

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
    
    // Vérification du module
    mfrc522.PCD_DumpVersionToSerial();
    
    // Désactiver le WiFi par défaut pour économiser l'énergie
    if (!otaEnabled) {
        WiFi.mode(WIFI_OFF);
    }
	else
	{
		while (!wifiConnected) {
        connectToWiFi();
		setupOTA();
    	}
	}
}

void loop() {
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
        webServer.handleClient();
        MDNS.update();
    }
    
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
    // Recherche de nouvelles cartes
    if (!mfrc522.PICC_IsNewCardPresent()) {
        return;
    }
    // Sélection de la carte
    if (!mfrc522.PICC_ReadCardSerial()) {
        return;
    }
    Serial.println("\n=== Carte détectée ===");
    // Affichage de l'UID
    Serial.print("UID: ");
    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
        uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println();
    // Affichage du type de carte
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.print("Type: ");
    Serial.println(mfrc522.PICC_GetTypeName(piccType));
    // Opération selon le mode
    String cardContent = "UID: " + uid + "\nType: " + String(mfrc522.PICC_GetTypeName(piccType)) + "\n";
    if (mode == "READ") {
        // Lecture de plusieurs blocs (secteurs 1 et 2)
        for (byte sector = 1; sector <= 2; sector++) {
            for (byte block = 0; block < 3; block++) {
                byte blockAddr = sector * 4 + block;
                byte buffer[18];
                byte size = sizeof(buffer);
                MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
                    MFRC522::PICC_CMD_MF_AUTH_KEY_A, 
                    blockAddr, 
                    &key, 
                    &(mfrc522.uid)
                );
                if (status != MFRC522::STATUS_OK) {
                    cardContent += "Bloc " + String(blockAddr) + ": Auth échouée\n";
                    continue;
                }
                status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
                if (status != MFRC522::STATUS_OK) {
                    cardContent += "Bloc " + String(blockAddr) + ": Lecture échouée\n";
                    continue;
                }
                cardContent += "Bloc " + String(blockAddr) + ": ";
                for (byte i = 0; i < 16; i++) {
                    if (buffer[i] < 0x10) cardContent += " 0";
                    else cardContent += " ";
                    cardContent += String(buffer[i], HEX);
                }
                cardContent += " | ";
                for (byte i = 0; i < 16; i++) {
                    if (buffer[i] >= 32 && buffer[i] <= 126) cardContent += (char)buffer[i];
                    else cardContent += ".";
                }
                cardContent += "\n";
            }
        }
        lastCardInfo = cardContent;
        readCard();
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

void readCard() {
    Serial.println("--- Lecture des données ---");
    
    // Lecture de plusieurs blocs
    for (byte sector = 1; sector <= 2; sector++) {
        for (byte block = 0; block < 3; block++) {
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
            
            if (status != MFRC522::STATUS_OK) {
                Serial.print("Auth échouée bloc ");
                Serial.print(blockAddr);
                Serial.print(": ");
                Serial.println(mfrc522.GetStatusCodeName(status));
                continue;
            }
            
            // Lecture du bloc
            status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
            if (status != MFRC522::STATUS_OK) {
                Serial.print("Lecture échouée bloc ");
                Serial.print(blockAddr);
                Serial.print(": ");
                Serial.println(mfrc522.GetStatusCodeName(status));
                continue;
            }
            
            // Affichage des données
            Serial.print("Bloc ");
            Serial.print(blockAddr);
            Serial.print(" (S");
            Serial.print(sector);
            Serial.print("B");
            Serial.print(block);
            Serial.print("): ");
            
            // Données en hexadécimal
            for (byte i = 0; i < 16; i++) {
                Serial.print(buffer[i] < 0x10 ? " 0" : " ");
                Serial.print(buffer[i], HEX);
            }
            
            // Données en texte
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
    }
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
    Serial.println("=== Connexion WiFi ===");
    Serial.print("Connexion à ");
    Serial.println(WIFI_SSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
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
    } else {
        Serial.println();
        Serial.println("Échec de connexion WiFi");
        WiFi.mode(WIFI_OFF);
        wifiConnected = false;
    }
    
    Serial.println("=====================");
}

// Configuration OTA
void setupOTA() {
    if (!wifiConnected) {
        Serial.println("WiFi requis pour OTA. Utilisez la commande WIFI d'abord.");
        return;
    }
    
    Serial.println("=== Configuration OTA ===");
    
    // Configuration du nom d'hôte
    ArduinoOTA.setHostname("ESP8266-RFID");
    ArduinoOTA.setPassword(OTA_PASSWORD);
    
    // Callbacks OTA
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_SPIFFS
            type = "filesystem";
        }
        Serial.println("Début mise à jour " + type);
        // Arrêter les opérations RFID pendant la mise à jour
        continuousMode = false;
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\nMise à jour terminée");
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
    if (MDNS.begin("esp8266-rfid")) {
        Serial.println("mDNS démarré");
        MDNS.addService("http", "tcp", 80);
    }
    
    // Configuration serveur web pour interface OTA
    setupWebServer();
    
    otaEnabled = true;
    
    Serial.println("OTA activé!");
    Serial.println("Nom d'hôte: ESP8266-RFID");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.println("Interface web: http://" + WiFi.localIP().toString());
    Serial.println("========================");
}

// Configuration du serveur web pour interface OTA
void setupWebServer() {
    // Page principale
    webServer.on("/", []() {
        String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP8266 RFID - Interface OTA</title>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <style>
        body { font-family: Arial; margin: 20px; background: #f0f0f0; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { text-align: center; color: #333; margin-bottom: 30px; }
        .status { background: #e8f5e8; padding: 15px; border-radius: 5px; margin: 15px 0; }
        .button { padding: 12px 24px; margin: 8px; background: #4CAF50; color: white; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }
        .button:hover { background: #45a049; }
        .button.danger { background: #f44336; }
        .button.danger:hover { background: #da190b; }
        .info { background: #e3f2fd; padding: 15px; border-radius: 5px; margin: 15px 0; }
        input[type=text] { padding: 10px; width: 200px; border: 1px solid #ddd; border-radius: 4px; }
        .upload-form { background: #fff3cd; padding: 15px; border-radius: 5px; margin: 15px 0; }
    </style>
</head>
<body>
    <div class='container'>
        <div class='header'>
            <h1>🔧 ESP8266 RFID Reader/Writer</h1>
            <h2>Interface de Mise à Jour OTA</h2>
        </div>
        
        <div class='status'>
            <h3>📊 État du système</h3>
            <p><strong>Mode:</strong> <span id='mode'>Chargement...</span></p>
            <p><strong>Mémoire libre:</strong> <span id='memory'>Chargement...</span></p>
            <p><strong>Uptime:</strong> <span id='uptime'>Chargement...</span></p>
            <p><strong>Signal WiFi:</strong> <span id='rssi'>Chargement...</span></p>
        </div>
        
        <div class='info'>
            <h3>🎛️ Commandes RFID</h3>
            <button class='button' onclick='sendCommand("READ")'>📖 Mode Lecture</button>
            <button class='button' onclick='sendCommand("STOP")'>⏹️ Arrêter</button>
            <button class='button' onclick='sendCommand("INFO")'>ℹ️ Informations</button>
            <br><br>
            <input type='text' id='writeData' placeholder='Données à écrire'>
            <button class='button' onclick='writeData()'>✏️ Écrire</button>
        </div>
        
        <div class='upload-form'>
            <h3>🔄 Mise à jour du firmware</h3>
            <p><strong>⚠️ Attention:</strong> La mise à jour interrompra temporairement les opérations RFID.</p>
            <form method='POST' action='/update' enctype='multipart/form-data'>
                <input type='file' name='update' accept='.bin'>
                <br><br>
                <input type='submit' value='📤 Téléverser' class='button'>
            </form>
        </div>
        
        <div class='info'>
            <h3>🔧 Actions système</h3>
            <button class='button danger' onclick='restartESP()'>🔄 Redémarrer</button>
        </div>
        
        <div id='cardInfo' class='status' style='display:none'>
            <h3>💳 Dernière carte détectée</h3>
            <p id='cardDetails'>Aucune carte</p>
        </div>
    </div>
    
    <script>
        function sendCommand(cmd) {
            fetch('/api/command?cmd=' + cmd)
                .then(response => response.text())
                .then(data => {
                    alert('Commande envoyée: ' + cmd);
                    updateStatus();
                });
        }
        
        function writeData() {
            const data = document.getElementById('writeData').value;
            if (data) {
                fetch('/api/write?data=' + encodeURIComponent(data))
                    .then(response => response.text())
                    .then(data => {
                        alert('Mode écriture activé: ' + data);
                        updateStatus();
                    });
            } else {
                alert('Veuillez entrer des données à écrire');
            }
        }
        
        function updateStatus() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('mode').textContent = data.mode;
                    document.getElementById('memory').textContent = data.memory + ' bytes';
                    document.getElementById('uptime').textContent = data.uptime + ' secondes';
                    document.getElementById('rssi').textContent = data.rssi + ' dBm';
                });
        }
        
        function updateCardInfo() {
            fetch('/api/lastcard')
                .then(response => response.text())
                .then(data => {
                    document.getElementById('cardDetails').textContent = data;
                    document.getElementById('cardInfo').style.display = (data && data !== 'Aucune carte') ? '' : 'none';
                });
        }
        
        function restartESP() {
    if(confirm("Redémarrer l'ESP8266?")) {
        fetch('/restart')
            .then(response => response.text())
            .then(data => {
                alert('Redémarrage en cours...');
                setTimeout(function(){ location.reload(); }, 12000);
            });
    }
}
        
        // Mise à jour automatique du statut toutes les 5 secondes
        setInterval(updateStatus, 5000);
        setInterval(updateCardInfo, 2000);
        updateStatus(); // Première mise à jour
        updateCardInfo(); // Première mise à jour des infos carte
    </script>
</body>
</html>
        )";
        webServer.send(200, "text/html", html);
    });
    
    // API pour les commandes
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
    
    // API pour l'écriture
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
    
    // API pour le statut
    webServer.on("/api/status", []() {
        String json = "{";
        json += "\"mode\":\"" + mode + "\",";
        json += "\"memory\":" + String(ESP.getFreeHeap()) + ",";
        json += "\"uptime\":" + String(millis() / 1000) + ",";
        json += "\"rssi\":" + String(WiFi.RSSI());
        json += "}";
        webServer.send(200, "application/json", json);
    });
    // API pour la dernière carte scannée
    webServer.on("/api/lastcard", []() {
        webServer.send(200, "text/plain", lastCardInfo);
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
    
    webServer.begin();
    Serial.println("Serveur web démarré");
}