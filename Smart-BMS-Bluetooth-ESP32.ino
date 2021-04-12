/**
  Program to read out and display
  data from xiaoxiang Smart BMS
  over Bluetooth Low Energy
  https://www.lithiumbatterypcb.com/
  Tested with original BLE module provided.
*/

#define TRACE
#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"
#include "BLEDevice.h"
#include "mydatatypes.h"
#include <SPI.h>
//#include <Wire.h>
#include <PubSubClient.h>

#include <WiFi.h>
#include <WiFiClient.h>

WiFiClient wifiClient;
PubSubClient client(wifiClient);

HardwareSerial commSerial(0);
HardwareSerial bmsSerial(1);

//---- global variables ----

static boolean doConnect = false;
static boolean BLE_client_connected = false;
static boolean doScan = false;

packBasicInfoStruct packBasicInfo; // here shall be the latest data got from BMS
packEepromStruct packEeprom;	     // here shall be the latest data got from BMS
packCellInfoStruct packCellInfo;   // here shall be the latest data got from BMS

const byte cBasicInfo3 = 3; // type of packet 3 = basic info
const byte cCellInfo4 =  4; // type of packet 4 = individual cell info

unsigned long previousMillis = 0;
const long interval = 2000;

bool toggle = false;
bool newPacketReceived = false;

char  buffer[20];
char result[20];

void setup()
{
  commSerial.println("Starting network...");
  networkStartup();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  commSerial.begin(115200, SERIAL_8N1, 3, 1);
  bmsSerial.begin(9600, SERIAL_8N1, 21, 22);
  commSerial.println("Starting bluetooth scan ...");
  bleStartup();
}
//---------------------main loop------------------
void loop()
{
  bleRequestData();
  if (newPacketReceived == true)
  {
    printBasicInfo();
    printCellInfo();
    MQTTsend();
    delay(10000);
  }
}
//---------------------/ main loop------------------
