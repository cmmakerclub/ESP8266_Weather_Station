#include <WiFiConnector.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

/*===== START INCLUDE =====*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
/*===== DISPLAY =====*/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <ESP_Adafruit_SSD1306.h>
/*===== DHT22 =====*/
#include "DHT.h"
#include <ESP8266WiFi.h>
/*===== END INCLUDE =====*/

/*===== START DEFINE =====*/

/*===== DISPLAY =====*/
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2
#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16
/*===== DHT22 =====*/
#define DHTPIN 12     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define DEBUG
#define DEBUG_PRINTER Serial
#ifdef DEBUG
#define DEBUG_PRINT(...) { DEBUG_PRINTER.print(__VA_ARGS__); }
#define DEBUG_PRINTLN(...) { DEBUG_PRINTER.println(__VA_ARGS__); }
#else
#define DEBUG_PRINT(...) {}
#define DEBUG_PRINTLN(...) {}
#endif
/*===== END DEFINE =====*/

/*===== BEFORE SETUP =====*/
/*===== DHT22 =====*/

DHT *dht;

void initDht(DHT **dht, uint8_t pin, uint8_t dht_type);
void readDht(DHT *dht, float *temp, float *humid);
/*===== DISPLAY =====*/
static const unsigned char logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000
};
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif
/*===== END BEFORE =====*/

long prevMillis = 0;

byte value_pir;


/*=== Timer Display ===*/
long preMillis = 0;
int t = 10;
int v = 0;
/*=====================*/

const char* ssid     = "NAT.WRTNODE";
const char* pass     = "devicenetwork";

WiFiConnector wifi(ssid, pass);

void init_wifi()
{
  wifi.on_connecting([](const void* message)
  {
    Serial.println("CONNECTING...");
    showDHT();
  });

  wifi.on_connected([](const void* message)
  {
    Serial.println ((char*)message);
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  });

  //    wifi.begin();
}


void setup() {
  Serial.println("BEGIN");
  Serial.begin(115200);
  delay(10);
  /*===== DISPLAY =====*/

  display.begin(SSD1306_SWITCHCAPVCC, 0x78 >> 1);
  display.println("begin");
  display.display();

  

  /*=== power ===*/



  delay(30);
  
  init_wifi();

  initDht(&dht, DHTPIN, DHTTYPE);

}

void print_display(String text) {
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(2, 20);
  display.clearDisplay();

  display.print(text.c_str());

  display.println();
  display.display();
}

void loop() {
  static float t_dht;
  static float h_dht;

  readDht(dht, &t_dht, &h_dht);

  if (millis() % 1000 == 0) {
    DEBUG_PRINTLN(t_dht);;
  }


  showDHT();

  wifi.loop();
}


void initDht(DHT **dht, uint8_t pin, uint8_t dht_type) {
  *dht = new DHT(pin, dht_type, 30);
  (*dht)->begin();
  DEBUG_PRINTLN(F("DHTxx test!"))  ;
}


void readDht(DHT *dht, float *temp, float *humid) {

  if (dht == NULL) {
    DEBUG_PRINTLN(F("[dht22] is not initialised. please call initDht() first."));
    return;
  }

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht->readHumidity();

  // Read temperature as Celsius
  float t = dht->readTemperature();
  // Read temperature as Fahrenheit
  float f = dht->readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    DEBUG_PRINTLN("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index
  // Must send in temp in Fahrenheit!
  float hi = dht->computeHeatIndex(f, h);
  *temp = t;
  *humid = f;
}

/* DISPLAY */

void showDHT(void) {
  float t = dht->readTemperature();
  float h = dht->readHumidity();

  print_display(String(t) + String(" C"));
}
