/*===== START INCLUDE =====*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
/*===== DISPLAY =====*/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <ESP_Adafruit_SSD1306.h>
/*===== DHT22 =====*/
#include "DHT.h"
#include <OneWire.h>
#include <ESP8266WiFi.h>
/*===== END INCLUDE =====*/

/*===== START DEFINE =====*/

#define power_oled 0
#define power_dht22 13
#define read_pir 12

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
#define DS18x20_PIN 4
#define DHTPIN 2     // what pin we're connected to
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
const char* ssid     = "MAKERCLUB-CM";
const char* password = "welcomegogogo";
DHT *dht;
OneWire *ds;  // on pin 2 (a 4.7K resistor is necessary)
void connectWifi();
void reconnectWifiIfLinkDown();
void initDht(DHT **dht, uint8_t pin, uint8_t dht_type);
void initDs18b20(OneWire **ds, uint8_t pin);
void readDs18B20(OneWire *ds, float *temp);
void readDht(DHT *dht, float *temp, float *humid);
void uploadThingsSpeak(float t, float );
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
  B00000000, B00110000 };
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

void setup() {
    Serial.begin(115200);

    /*=== power ===*/
    pinMode(power_oled, OUTPUT);
    pinMode(power_dht22, OUTPUT);
    pinMode(read_pir, INPUT);
    digitalWrite(power_oled, HIGH);
    digitalWrite(power_dht22, HIGH);
    
    delay(10);

    connectWifi();

    initDht(&dht, DHTPIN, DHTTYPE);
    initDs18b20(&ds, DS18x20_PIN);

    /*===== DISPLAY =====*/
    display.begin(SSD1306_SWITCHCAPVCC, 0x78>>1);

}

void loop() {
    static float t_ds;
    static float t_dht;
    static float h_dht;

    readDht(dht, &t_dht, &h_dht);

    DEBUG_PRINTLN(t_ds);
    
    showDHT();

    delay(10 * 100);
}

void reconnectWifiIfLinkDown() {
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("WIFI DISCONNECTED");
        connectWifi();
    }
}

void connectWifi() {
    DEBUG_PRINTLN();
    DEBUG_PRINTLN();
    DEBUG_PRINT("Connecting to ");
    DEBUG_PRINTLN(ssid);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        DEBUG_PRINT(".");
    }

    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("WiFi connected");
    DEBUG_PRINTLN("IP address: ");
    DEBUG_PRINTLN(WiFi.localIP());
}

void initDht(DHT **dht, uint8_t pin, uint8_t dht_type) {
    *dht = new DHT(pin, dht_type, 30);
    (*dht)->begin();
    DEBUG_PRINTLN(F("DHTxx test!"))  ;
}


void initDs18b20(OneWire **ds, uint8_t pin) {
    *ds = new OneWire(pin);
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

    DEBUG_PRINT("Humidity: ");
    DEBUG_PRINT(h);
    DEBUG_PRINT(" %\t");
    DEBUG_PRINT("Temperature: ");
    DEBUG_PRINT(t);
    DEBUG_PRINT(" *C ");
    DEBUG_PRINT(f);
    DEBUG_PRINT(" *F\t");
    DEBUG_PRINT("Heat index: ");
    DEBUG_PRINT(hi);
    DEBUG_PRINTLN(" *F");

    *temp = t;
    *humid = f;

}

void readDs18B20(OneWire *ds, float *temp) {
    if (ds == NULL) {
        DEBUG_PRINTLN(F("[ds18b20] is not initialised. please call initDs18b20() first."));
        return;
    }


    byte i;
    byte present = 0;
    byte type_s;
    byte data[12];
    byte addr[8];
    float celsius, fahrenheit;

    if ( !ds->search(addr)) {
        DEBUG_PRINTLN("No more addresses.");
        DEBUG_PRINTLN();
        ds->reset_search();
        delay(250);
        return;
    }

    DEBUG_PRINT("ROM =");
    for ( i = 0; i < 8; i++) {
        Serial.write(' ');
        DEBUG_PRINT(addr[i], HEX);
    }

    if (OneWire::crc8(addr, 7) != addr[7]) {
        DEBUG_PRINTLN("CRC is not valid!");
        return;
    }
    DEBUG_PRINTLN();

    // the first ROM byte indicates which chip
    switch (addr[0]) {
    case 0x10:
        DEBUG_PRINTLN("  Chip = DS18S20");  // or old DS1820
        type_s = 1;
        break;
    case 0x28:
        DEBUG_PRINTLN("  Chip = DS18B20");
        type_s = 0;
        break;
    case 0x22:
        DEBUG_PRINTLN("  Chip = DS1822");
        type_s = 0;
        break;
    default:
        DEBUG_PRINTLN("Device is not a DS18x20 family device.");
        return;
    }

    ds->reset();
    ds->select(addr);
    ds->write(0x44, 1);        // start conversion, with parasite power on at the end

    delay(1000);     // maybe 750ms is enough, maybe not
    // we might do a ds->depower() here, but the reset will take care of it.

    present = ds->reset();
    ds->select(addr);
    ds->write(0xBE);         // Read Scratchpad

    DEBUG_PRINT("  Data = ");
    DEBUG_PRINT(present, HEX);
    DEBUG_PRINT(" ");
    for ( i = 0; i < 9; i++) {           // we need 9 bytes
        data[i] = ds->read();
        DEBUG_PRINT(data[i], HEX);
        DEBUG_PRINT(" ");
    }
    DEBUG_PRINT(" CRC=");
    DEBUG_PRINT(OneWire::crc8(data, 8), HEX);
    DEBUG_PRINTLN();

    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    int16_t raw = (data[1] << 8) | data[0];
    if (type_s) {
        raw = raw << 3; // 9 bit resolution default
        if (data[7] == 0x10) {
            // "count remain" gives full 12 bit resolution
            raw = (raw & 0xFFF0) + 12 - data[6];
        }
    } else {
        byte cfg = (data[4] & 0x60);
        // at lower res, the low bits are undefined, so let's zero them
        if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
        else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
        //// default is 12 bit resolution, 750 ms conversion time
    }

    celsius = (float)raw / 16.0;
    fahrenheit = celsius * 1.8 + 32.0;

    DEBUG_PRINT("  Temperature = ");
    DEBUG_PRINT(celsius);
    DEBUG_PRINT(" Celsius, ");
    DEBUG_PRINT(fahrenheit);
    DEBUG_PRINTLN(" Fahrenheit");


    *temp = celsius;
}

/* DISPLAY */

void showDHT(void) {
    float t = dht->readTemperature();
    float h = dht->readHumidity();

    value_pir = digitalRead(read_pir);
  
    if (preMillis >= 500 || preMillis <= 0) {
      preMillis = 0;
    }
    
    if (value_pir == HIGH) {

        if (preMillis > 0) {
          preMillis = preMillis - 10;
        }

        v++;
        
    }else if (value_pir == LOW){

        preMillis = preMillis + 100;
        v = 0;
    }

    if (v >= 5) {
        preMillis = 0;
        v = 0;
    }
    
    if (preMillis > 0) {
      Serial.println(preMillis);
      display.clearDisplay();
      
      //Serial.println("Found");
      display.setTextSize(3);
      display.setTextColor(WHITE);
      display.setCursor(2,20);
      display.clearDisplay();
      display.print(t - 5);
      display.print(" C");
      display.println();
      display.display();
      
    }else if (preMillis <= 0){
      Serial.println(preMillis);
      //Serial.println("Not Found");
      display.clearDisplay();
      display.display();

    }

}
