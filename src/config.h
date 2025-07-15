
#define OTA_PASSWORD "password123"
#define HOSTNAME "RFID-SCANNER"


// Configuration des broches pour D1 Mini ESP8266
#define RST_PIN         D3    // GPIO 0  (D3)
#define SS_PIN          D8    // GPIO 15 (D8)
// SPI utilise les broches par défaut:
// MOSI = D7 (GPIO 13)
// MISO = D6 (GPIO 12)
// SCK  = D5 (GPIO 14)

#define LED_PIN D2 // GPIO 4  (D2)

#define EEPROM_SIZE 256
#define API_URL_ADDR 0
#define API_URL_MAXLEN 200

#define EEPROM_SIZE 256
#define API_URL_ADDR 0
#define API_URL_MAXLEN 200
#define WIFI_SSID_ADDR (API_URL_ADDR + API_URL_MAXLEN)
#define WIFI_SSID_MAXLEN 32
#define WIFI_PASS_ADDR (WIFI_SSID_ADDR + WIFI_SSID_MAXLEN)
#define WIFI_PASS_MAXLEN 64
#define AP_SSID "RFID-Config"
#define AP_PASS "12345678"