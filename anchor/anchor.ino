#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

#define WIFI_SSID "NETGEAR31"
#define WIFI_PASSWD "fluffywind2904"
// #define SOFTAP_MODE       //The comment will be connected to the specified ssid


unsigned long sendInterval = 1;
unsigned long sendIntervalMillis = sendInterval * 1000;

unsigned long nextSend = millis();
int lastSendSec = -1;

String macAddress;
String ipAddress;

#include "soc/rtc_wdt.h"

#include <SPI.h>
#include "DW1000Ranging.h"

#define ANCHOR_ADD "86:17:5B:D5:A9:9A:E2:9C"

#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4

// connection pins
const uint8_t PIN_RST = 27;  // reset pin
const uint8_t PIN_IRQ = 34;  // irq pin
const uint8_t PIN_SS = 4;    // spi select pin



WebSocketsClient webSocket;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED: {
      Serial.printf("[WSc] Connected to url: %s\n", payload);
    }
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text: %s\n", payload);
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      break;
    case WStype_PING:
        // pong will be send automatically
        Serial.printf("[WSc] get ping\n");
        break;
    case WStype_PONG:
        // answer to a ping we send
        Serial.printf("[WSc] get pong\n");
        break;
    }

}

void setupNetwork() {
  macAddress = "";
#ifdef SOFTAP_MODE
  WiFi.mode(WIFI_AP);
  macAddress += WiFi.softAPmacAddress().substring(0, 5);
  WiFi.softAP(macAddress.c_str());
  ipAddress = WiFi.softAPIP().toString();
#else
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  ipAddress = WiFi.localIP().toString();
  macAddress = String(WiFi.macAddress());
#endif
}


void setup() {
  Serial.begin(115200);
  rtc_wdt_protect_off();  // Turns off the automatic wdt service

  setupNetwork();

  webSocket.begin("mocap.local", 3000, "/uwb32");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);

  delay(1000);

  //init the configuration
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ);  //Reset, CS, IRQ pin

  //define the sketch as anchor. It will be great to dynamically change the type of module
  DW1000Ranging.attachNewRange(newRange);
  DW1000Ranging.attachBlinkDevice(newBlink);
  DW1000Ranging.attachInactiveDevice(inactiveDevice);
  //Enable the filter to smooth the distance
  //DW1000Ranging.useRangeFilter(true);

  //we start the module as an anchor
  // DW1000Ranging.startAsAnchor("82:17:5B:D5:A9:9A:E2:9C", DW1000.MODE_LONGDATA_RANGE_ACCURACY);

  // DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_LONGDATA_RANGE_LOWPOWER, false);
  // DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_SHORTDATA_FAST_LOWPOWER);
  // DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_LONGDATA_FAST_LOWPOWER);
  // DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_SHORTDATA_FAST_ACCURACY);
  // DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_LONGDATA_FAST_ACCURACY);
  DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_LONGDATA_RANGE_ACCURACY);

  rtc_wdt_enable();  // Turn it on manually
  rtc_wdt_set_time(RTC_WDT_STAGE0, 20000);
}

void loop() {
  DW1000Ranging.loop();
  webSocket.loop();
  uint64_t now = millis();


  rtc_wdt_feed();
}

void newRange() {
  Serial.print("from: ");
  Serial.print(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
  Serial.print("\t Range: ");
  Serial.print(DW1000Ranging.getDistantDevice()->getRange());
  Serial.print(" m");
  Serial.print("\t RX power: ");
  Serial.print(DW1000Ranging.getDistantDevice()->getRXPower());
  Serial.println(" dBm");
  String message = String(millis());
  webSocket.sendTXT(message);
}

void newBlink(DW1000Device *device) {
  Serial.print("blink; 1 device added ! -> ");
  Serial.print(" short:");
  Serial.println(device->getShortAddress(), HEX);
}

void inactiveDevice(DW1000Device *device) {
  Serial.print("delete inactive device: ");
  Serial.println(device->getShortAddress(), HEX);
}
