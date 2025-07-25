# Programme ESP8266 D1 Mini RFID Reader/Writer

Ce programme permet de lire et écrire des tags RFID/NFC avec un ESP8266 D1 Mini et un module RC522 connecté via SPI.

## Matériel requis

- ESP8266 D1 Mini
- Module RFID RC522
- Câbles de connexion (jumper wires)
- Tags RFID/NFC compatibles MIFARE Classic
- Breadboard (optionnel)

## Câblage ESP8266 D1 Mini

### Connexions RC522 vers ESP8266 D1 Mini:

| RC522 | D1 Mini | GPIO | Description |
|-------|---------|------|-------------|
| VCC   | 3.3V    | -    | Alimentation 3.3V |
| GND   | GND     | -    | Masse |
| RST   | D3      | 0    | Reset |
| SDA   | D8      | 15   | Slave Select (SS) |
| SCK   | D5      | 14   | Serial Clock |
| MOSI  | D7      | 13   | Master Out Slave In |
| MISO  | D6      | 12   | Master In Slave Out |

### Schéma de câblage du lecteur:
```
    D1 Mini                    RC522
   ┌─────────┐              ┌─────────┐
   │   3.3V  │──────────────│   VCC   │
   │   GND   │──────────────│   GND   │
   │   D3    │──────────────│   RST   │
   │   D8    │──────────────│   SDA   │
   │   D5    │──────────────│   SCK   │
   │   D7    │──────────────│   MOSI  │
   │   D6    │──────────────│   MISO  │
   └─────────┘              └─────────┘
```

## Utilisation

### 1. Téléversement du programme
- Connectez votre D1 Mini à l'ordinateur via USB
- Sélectionnez le port série approprié
- Téléversez le programme

### 2. Moniteur série
- Ouvrez le moniteur série (115200 bauds)
- Le programme affichera les informations d'initialisation

### 3. Commandes disponibles

#### Commandes de base:
```
READ                    - Active le mode lecture continu
WRITE <vos_données>     - Mode écriture (ex: WRITE Hello World!)
SCAN                    - Active le scan continu
STOP                    - Arrête le scan continu
INFO                    - Affiche les informations système
```

#### Commandes avancées:
```
FORMAT                  - Formate une carte (efface toutes les données)
BACKUP                  - Sauvegarde complète d'une carte
OTA                     - Active les mises à jour Over-The-Air
WIFI                    - Se connecter au réseau WiFi
```

### 4. Exemple d'utilisation

```
=== ESP8266 D1 Mini RFID Reader/Writer ===
Module RC522 initialisé
Commandes disponibles:
- READ: Mode lecture
- WRITE <data>: Mode écriture
- SCAN: Scan continu
- STOP: Arrêter le scan
- INFO: Informations système
- FORMAT: Formater une carte
- BACKUP: Sauvegarder une carte
========================================

> READ
Mode lecture activé

=== Carte détectée ===
UID:  A1 B2 C3 D4
Type: MIFARE 1KB
===================

> WRITE Bonjour ESP8266!
Mode écriture activé - Données: Bonjour ESP8266!

=== Carte détectée ===
UID:  A1 B2 C3 D4
Type: MIFARE 1KB
===================

> INFO
=== Informations système ===
Modèle: ESP8266 D1 Mini
Module RFID: RC522
Fréquence: 13.56 MHz
Connexions SPI:
  RST: D3 (GPIO 0)
  SS:  D8 (GPIO 15)
  SCK: D5 (GPIO 14)
  MOSI:D7 (GPIO 13)
  MISO:D6 (GPIO 12)
Mode actuel: READ
Scan continu: Activé
Mémoire libre: 45328 bytes
Fréquence CPU: 80 MHz
============================
```
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
## Fonctionnalités

### Fonctionnalités de base:
- **Lecture de tags**: Lit le contenu de plusieurs blocs
- **Écriture de tags**: Écrit jusqu'à 16 caractères par bloc, retiré du code car inutile pour le projet
- **Affichage UID**: Affiche l'identifiant unique de chaque tag
- **Type de carte**: Détecte et affiche le type de carte RFID
- **Interface série**: Commandes interactives via le moniteur série
- **API**: Envoi du tag vers une API web

### Fonctionnalités avancées:
- **Formatage**: Efface complètement une carte
- **Sauvegarde**: Dump complet du contenu d'une carte
- **Scan continu**: Mode de détection automatique
- **Gestion d'erreurs**: Messages d'erreur détaillés
- **Informations système**: État du système et de la mémoire
- **Mises à jour OTA**: Mise à jour du firmware sans fil
- **Interface web**: Contrôle via navigateur web
- **mDNS**: Découverte automatique sur le réseau

## Spécifications techniques

- **Microcontrôleur**: ESP8266 (80/160 MHz)
- **Mémoire Flash**: 4MB
- **RAM**: 80KB
- **Module RFID**: RC522 (13.56 MHz)
- **Interface**: SPI
- **Portée**: 2-3 cm (selon l'antenne)
- **Types supportés**: MIFARE Classic 1K/4K

## Limitations

- Fonctionne uniquement avec les cartes MIFARE Classic
- Écriture limitée à 16 caractères par bloc
- Utilise la clé par défaut (0xFF x 6)
- Portée limitée (quelques centimètres)

## Sécurité

- Les cartes MIFARE Classic utilisent une sécurité basique
- La clé par défaut est utilisée (0xFF x 6)
- Pour une sécurité renforcée, modifiez les clés d'authentification
- Le formatage efface définitivement les données

## Dépannage

### Problèmes courants:

1. **"Aucune communication avec le module RC522"**
   - Vérifiez toutes les connexions SPI
   - Assurez-vous que l'alimentation est stable (3.3V)
   - Vérifiez que les broches correspondent au schéma

2. **"Authentification échouée"**
   - Vérifiez que la carte est compatible MIFARE Classic
   - Assurez-vous que la clé n'a pas été modifiée
   - Rapprochez la carte du lecteur

3. **"Lecture/Écriture échouée"**
   - Maintenez la carte proche du lecteur
   - Évitez les interférences électromagnétiques
   - Vérifiez l'alimentation du module

4. **Module non détecté**
   - Vérifiez les connexions SPI
   - Testez avec un multimètre les tensions
   - Redémarrez l'ESP8266

### Tests de diagnostic:

```
> INFO
```
Cette commande affiche l'état complet du système et peut aider à identifier les problèmes.

## Interface web

Le programme embarque un serveur web accessible depuis le réseau local (WiFi). Pour accéder à l'interface, ouvrez votre navigateur et entrez l'adresse IP affichée dans le moniteur série après connexion WiFi (exemple : http://192.168.1.100).
Le serveur est également accessible à : http://rfid-scanner.local/

- L'accès à la page principale est protégé par un mot de passe.
- Mot de passe par défaut : **admin**
- Vous pouvez le modifier depuis l'interface web.
- L'interface permet de consulter le dernier tag lu, l'état du système, et d'envoyer des commandes (lecture, arrêt, etc).

### Exemple d'accès

1. Connectez l'ESP8266 au WiFi
2. Ouvrez votre navigateur sur l'adresse IP affichée
3. Entrez le mot de passe `admin` pour accéder à la page principale

L'API web propose aussi des routes pour automatiser l'envoi de commandes et récupérer l'état du système.

