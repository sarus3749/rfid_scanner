# Guide de Configuration OTA pour ESP8266 RFID

## Configuration initiale

### 1. Modification des paramètres WiFi

Dans le fichier `main.cpp`, modifiez ces lignes :

```cpp
// Configuration WiFi (à modifier selon votre réseau)
const char* ssid = "VotreSSID";           // Nom de votre réseau WiFi
const char* password = "VotreMotDePasse"; // Mot de passe WiFi
const char* otaPassword = "password123";     // Mot de passe pour OTA
```

### 2. Première connexion

1. Téléversez le programme via USB
2. Ouvrez le moniteur série (115200 bauds)
3. Tapez `WIFI` pour se connecter au réseau
4. Tapez `OTA` pour activer les mises à jour OTA

## Utilisation de l'OTA

### Via Arduino IDE

1. **Menu Outils → Port**
2. Sélectionnez **ESP8266-RFID at 192.168.x.x (ESP8266)**
3. Téléversez normalement (Ctrl+U)
4. Entrez le mot de passe OTA quand demandé

### Via Interface Web

1. Ouvrez un navigateur
2. Allez à l'adresse IP affichée (ex: `http://192.168.1.100`)
3. Utilisez l'interface pour :
   - Contrôler le lecteur RFID
   - Téléverser un nouveau firmware
   - Redémarrer l'appareil
   - Voir les statistiques système

## Fonctionnalités de l'interface web

### 📊 État du système
- Mode actuel (READ/WRITE/STOP)
- Mémoire libre disponible
- Temps de fonctionnement
- Force du signal WiFi

### 🎛️ Commandes RFID
- **Mode Lecture** : Active la lecture continue
- **Arrêter** : Stoppe les opérations
- **Écrire** : Mode écriture avec données personnalisées
- **Informations** : Affiche les infos système

### 🔄 Mise à jour firmware
- Upload de fichiers .bin
- Progression en temps réel
- Redémarrage automatique

### 🔧 Actions système
- Redémarrage à distance
- Monitoring en temps réel

## Sécurité OTA

### Mot de passe OTA
Le mot de passe par défaut est `password123`. Changez-le dans le code :

```cpp
const char* otaPassword = "VotreNouveauMotDePasse";
```

### Recommandations
- Utilisez un mot de passe fort
- Désactivez l'OTA quand non utilisé
- Surveillez les connexions réseau
- Utilisez un réseau WiFi sécurisé

## Dépannage OTA

### Problème : "Pas de port OTA disponible"
**Solution :**
1. Vérifiez que l'ESP8266 est connecté au WiFi
2. Vérifiez que l'OTA est activé (`OTA` dans le moniteur série)
3. Redémarrez Arduino IDE
4. Vérifiez que l'ESP8266 et l'ordinateur sont sur le même réseau

### Problème : "Authentification OTA échouée"
**Solution :**
1. Vérifiez le mot de passe OTA dans le code
2. Retéléversez le programme via USB si nécessaire

### Problème : "Timeout de connexion"
**Solution :**
1. Vérifiez la stabilité du réseau WiFi
2. Rapprochez l'ESP8266 du routeur
3. Redémarrez l'ESP8266 (`/restart` via web ou bouton reset)

### Problème : "Mise à jour échouée"
**Solution :**
1. Vérifiez que le fichier .bin est valide
2. Assurez-vous que l'ESP8266 a assez de mémoire libre
3. Évitez les opérations RFID pendant la mise à jour

## Commandes série étendues

```
WIFI    - Se connecter au réseau WiFi configuré
OTA     - Activer les mises à jour Over-The-Air
INFO    - Afficher l'état WiFi et OTA
```

## Exemple de session complète

```
=== ESP8266 D1 Mini RFID Reader/Writer ===
Module RC522 initialisé
[...]

> WIFI
=== Connexion WiFi ===
Connexion à MonReseauWiFi
..........
WiFi connecté!
Adresse IP: 192.168.1.100
Signal: -45 dBm
=====================

> OTA
=== Configuration OTA ===
mDNS démarré
Serveur web démarré
OTA activé!
Nom d'hôte: ESP8266-RFID
IP: 192.168.1.100
Interface web: http://192.168.1.100
========================

> INFO
=== Informations système ===
[...]
WiFi: Connecté
IP: 192.168.1.100
OTA: Activé
============================
```
