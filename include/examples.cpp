/*
 * Exemples d'utilisation avancée du programme RFID ESP8266 D1 Mini
 * 
 * Ces exemples montrent comment étendre les fonctionnalités
 * du programme principal pour ESP8266
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

// Variables pour les exemples avancés
ESP8266WebServer server(80);
bool webServerEnabled = false;

// Exemple 1: Interface Web pour contrôle RFID
void setupWebInterface() {
    Serial.println("=== Configuration Interface Web ===");
    
    // Configuration WiFi en mode AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP8266-RFID", "12345678");
    
    Serial.print("Point d'accès créé: ESP8266-RFID");
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());
    
    // Routes web
    server.on("/", handleRoot);
    server.on("/read", handleWebRead);
    server.on("/write", handleWebWrite);
    server.on("/status", handleStatus);
    
    server.begin();
    webServerEnabled = true;
    
    Serial.println("Serveur web démarré sur http://192.168.4.1");
}

void handleRoot() {
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP8266 RFID Control</title>
    <meta charset='utf-8'>
    <style>
        body { font-family: Arial; margin: 20px; }
        .button { padding: 10px 20px; margin: 5px; background: #4CAF50; color: white; border: none; cursor: pointer; }
        .button:hover { background: #45a049; }
        .status { background: #f0f0f0; padding: 10px; margin: 10px 0; }
        input[type=text] { padding: 8px; width: 200px; }
    </style>
</head>
<body>
    <h1>ESP8266 RFID Reader/Writer</h1>
    
    <div class='status'>
        <h3>État du système</h3>
        <p id='status'>Chargement...</p>
    </div>
    
    <h3>Commandes</h3>
    <button class='button' onclick='sendCommand("READ")'>Mode Lecture</button>
    <button class='button' onclick='sendCommand("STOP")'>Arrêter</button>
    <br><br>
    
    <input type='text' id='writeData' placeholder='Données à écrire'>
    <button class='button' onclick='writeData()'>Écrire</button>
    
    <h3>Dernière carte détectée</h3>
    <div id='cardInfo'>Aucune carte détectée</div>
    
    <script>
        function sendCommand(cmd) {
            fetch('/status?cmd=' + cmd)
                .then(response => response.text())
                .then(data => updateStatus(data));
        }
        
        function writeData() {
            const data = document.getElementById('writeData').value;
            if (data) {
                fetch('/write?data=' + encodeURIComponent(data))
                    .then(response => response.text())
                    .then(data => updateStatus(data));
            }
        }
        
        function updateStatus(data) {
            document.getElementById('status').innerHTML = data;
        }
        
        // Mise à jour automatique du statut
        setInterval(() => {
            fetch('/status')
                .then(response => response.text())
                .then(data => updateStatus(data));
        }, 2000);
    </script>
</body>
</html>
    )";
    
    server.send(200, "text/html", html);
}

void handleWebRead() {
    mode = "READ";
    continuousMode = true;
    server.send(200, "text/plain", "Mode lecture activé");
}

void handleWebWrite() {
    if (server.hasArg("data")) {
        dataToWrite = server.arg("data");
        mode = "WRITE";
        continuousMode = true;
        server.send(200, "text/plain", "Mode écriture activé: " + dataToWrite);
    } else {
        server.send(400, "text/plain", "Paramètre 'data' manquant");
    }
}

void handleStatus() {
    if (server.hasArg("cmd")) {
        String cmd = server.arg("cmd");
        if (cmd == "READ") {
            mode = "READ";
            continuousMode = true;
        } else if (cmd == "STOP") {
            continuousMode = false;
        }
    }
    
    String status = "Mode: " + mode + "<br>";
    status += "Scan: " + String(continuousMode ? "Actif" : "Arrêté") + "<br>";
    status += "Mémoire libre: " + String(ESP.getFreeHeap()) + " bytes<br>";
    status += "Uptime: " + String(millis() / 1000) + " secondes";
    
    server.send(200, "text/html", status);
}

// Exemple 2: Stockage des UIDs en mémoire flash
void saveUIDToFlash(String uid) {
    if (!LittleFS.begin()) {
        Serial.println("Erreur: Impossible de monter LittleFS");
        return;
    }
    
    File file = LittleFS.open("/uids.txt", "a");
    if (file) {
        String timestamp = String(millis());
        file.println(timestamp + "," + uid);
        file.close();
        Serial.println("UID sauvegardé: " + uid);
    } else {
        Serial.println("Erreur: Impossible d'ouvrir le fichier");
    }
}

void listStoredUIDs() {
    if (!LittleFS.begin()) {
        Serial.println("Erreur: Impossible de monter LittleFS");
        return;
    }
    
    File file = LittleFS.open("/uids.txt", "r");
    if (file) {
        Serial.println("=== UIDs stockés ===");
        while (file.available()) {
            String line = file.readStringUntil('\n');
            int commaIndex = line.indexOf(',');
            if (commaIndex > 0) {
                String timestamp = line.substring(0, commaIndex);
                String uid = line.substring(commaIndex + 1);
                Serial.println("Temps: " + timestamp + "ms, UID: " + uid);
            }
        }
        file.close();
        Serial.println("==================");
    } else {
        Serial.println("Aucun UID stocké");
    }
}

// Exemple 3: Mode apprentissage pour cloner des cartes
struct CardData {
    byte uid[10];
    byte uidSize;
    byte sectors[16][64]; // 16 secteurs, 64 bytes par secteur
    bool isValid;
};

CardData learnedCard;

void learnCard() {
    Serial.println("=== Mode apprentissage ===");
    
    // Copie de l'UID
    learnedCard.uidSize = mfrc522.uid.size;
    for (byte i = 0; i < learnedCard.uidSize; i++) {
        learnedCard.uid[i] = mfrc522.uid.uidByte[i];
    }
    
    // Lecture de tous les secteurs
    bool success = true;
    for (byte sector = 0; sector < 16; sector++) {
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
                    memcpy(&learnedCard.sectors[sector][block * 16], buffer, 16);
                } else {
                    success = false;
                }
            } else {
                success = false;
            }
        }
    }
    
    learnedCard.isValid = success;
    
    if (success) {
        Serial.println("Carte apprise avec succès!");
        Serial.print("UID: ");
        for (byte i = 0; i < learnedCard.uidSize; i++) {
            Serial.print(learnedCard.uid[i] < 0x10 ? " 0" : " ");
            Serial.print(learnedCard.uid[i], HEX);
        }
        Serial.println();
    } else {
        Serial.println("Erreur lors de l'apprentissage");
    }
}

void cloneCard() {
    if (!learnedCard.isValid) {
        Serial.println("Aucune carte apprise!");
        return;
    }
    
    Serial.println("=== Clonage de carte ===");
    Serial.println("ATTENTION: Cette opération peut endommager la carte cible!");
    
    // Écriture des données (éviter le secteur 0 et les blocs trailer)
    int blocksWritten = 0;
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
                // Écriture du bloc
                status = mfrc522.MIFARE_Write(blockAddr, &learnedCard.sectors[sector][block * 16], 16);
                if (status == MFRC522::STATUS_OK) {
                    blocksWritten++;
                }
            }
        }
    }
    
    Serial.print("Clonage terminé: ");
    Serial.print(blocksWritten);
    Serial.println(" blocs écrits");
}

// Exemple 4: Détection de mouvement de carte
void cardPresenceDetection() {
    static bool cardPresent = false;
    static unsigned long lastDetection = 0;
    static String lastUID = "";
    
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        // Construction de l'UID
        String currentUID = "";
        for (byte i = 0; i < mfrc522.uid.size; i++) {
            currentUID += String(mfrc522.uid.uidByte[i], HEX);
        }
        
        if (!cardPresent || currentUID != lastUID) {
            Serial.println("Nouvelle carte détectée: " + currentUID);
            cardPresent = true;
            lastUID = currentUID;
            
            // Sauvegarde automatique de l'UID
            saveUIDToFlash(currentUID);
        }
        
        lastDetection = millis();
    } else {
        if (cardPresent && (millis() - lastDetection > 3000)) {
            Serial.println("Carte retirée: " + lastUID);
            cardPresent = false;
            lastUID = "";
        }
    }
}

// Exemple 5: Système d'authentification simple
struct AuthUser {
    String uid;
    String name;
    byte accessLevel;
};

AuthUser authorizedUsers[] = {
    {"A1B2C3D4", "Admin", 3},
    {"E5F6G7H8", "User1", 2},
    {"I9J0K1L2", "User2", 1}
};

void checkAccess() {
    // Construction de l'UID
    String cardUID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        cardUID += String(mfrc522.uid.uidByte[i], HEX);
    }
    cardUID.toUpperCase();
    
    // Vérification de l'autorisation
    bool authorized = false;
    String userName = "";
    byte accessLevel = 0;
    
    for (int i = 0; i < sizeof(authorizedUsers) / sizeof(AuthUser); i++) {
        if (authorizedUsers[i].uid == cardUID) {
            authorized = true;
            userName = authorizedUsers[i].name;
            accessLevel = authorizedUsers[i].accessLevel;
            break;
        }
    }
    
    if (authorized) {
        Serial.println("=== ACCÈS AUTORISÉ ===");
        Serial.println("Utilisateur: " + userName);
        Serial.println("Niveau d'accès: " + String(accessLevel));
        Serial.println("UID: " + cardUID);
        
        // Actions selon le niveau d'accès
        switch (accessLevel) {
            case 3:
                Serial.println("Accès administrateur accordé");
                break;
            case 2:
                Serial.println("Accès utilisateur accordé");
                break;
            case 1:
                Serial.println("Accès limité accordé");
                break;
        }
    } else {
        Serial.println("=== ACCÈS REFUSÉ ===");
        Serial.println("Carte non autorisée: " + cardUID);
    }
    
    Serial.println("=====================");
}

// Exemple 6: Monitoring de performance
void performanceMonitoring() {
    static unsigned long lastCheck = 0;
    static int cardCount = 0;
    static unsigned long totalReadTime = 0;
    
    if (millis() - lastCheck > 60000) { // Toutes les minutes
        Serial.println("=== Statistiques de performance ===");
        Serial.print("Cartes lues: ");
        Serial.println(cardCount);
        
        if (cardCount > 0) {
            Serial.print("Temps moyen de lecture: ");
            Serial.print(totalReadTime / cardCount);
            Serial.println(" ms");
        }
        
        Serial.print("Mémoire libre: ");
        Serial.print(ESP.getFreeHeap());
        Serial.println(" bytes");
        
        Serial.print("Uptime: ");
        Serial.print(millis() / 1000);
        Serial.println(" secondes");
        
        Serial.println("=================================");
        
        lastCheck = millis();
        cardCount = 0;
        totalReadTime = 0;
    }
}

// Fonction utilitaire pour mesurer le temps de lecture
unsigned long measureReadTime() {
    unsigned long startTime = millis();
    
    // Simulation d'une opération de lecture
    readCard();
    
    return millis() - startTime;
}