#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiNINA.h>
#include <sys/time.h>
#include <PubSubClient.h>
#include <ArduinoBLE.h>
#include <RTCZero.h>
#include <time.h>

#include "credentials.h"

RTCZero rtc;

#define STATE_TRANSMIT_TO_MQTT  1
#define STATE_START_SCAN        2
#define STATE_STOP_SCAN         3
#define STATE_SCAN_BLE_BEACON   4
#define STATE_STOP_WIFI         5
#define STATE_START_WIFI        6
#define STATE_START_MQTT        7
#define STATE_STOP_MQTT         8
#define STATE_PAUSE             9
#define STATE_COMPUTE_RESULT    10

#define INDEX_GELBE_TONNE       0
#define INDEX_REST_TONNE        1
#define INDEX_BIO_TONNE         2
#define INDEX_PAPIER_TONNE      3

#define PERIPHERAL_ERROR_MAJOR_NOT_IDENTIFIED -1
#define PERIPHERAL_ERROR_UUID_NOT_MATCHED     -2
#define PERIPHERAL_ERROR_NOT_A_BEACON         -3

#define NUMBER_OF_TONNES        4
#define STREAK_THRESHOLD        2
#define TOPIC_CONTACT_TEMPLATE                "home-assistant/%nameTonne%/contact"
#define TOPIC_BATTERY_LEVEL_TEMPLATE          "home-assistant/%nameTonne%/battery_level"
#define TOPIC_HEARTBEAT_TEMPLATE              "home-assistant/%nameTonne%/heartbeat"
#define TOPIC_OWN_BATTERY_LEVEL_TEMPLATE      "home-assistant/muelltonnenscanner/battery_level"
#define TOPIC_OWN_HEARTBEAT_TEMPLATE          "home-assistant/muelltonnenscanner/heartbeat"

// never zero this interval
#define BATTERY_SENSE_INTERVAL  4
#define PAUSE_MINUTES           10
#define USE_DEEPSLEEP
#define BATTERY_PIN             A0

String nameTonne[] = {"gelbetonne", "resttonne", "biotonne", "papiertonne"};
bool foundTonne[] = {false, false, false, false};
int streakFoundTonne[] = {0, 0, 0, 0};
int streakNotFoundTonne[] = {0, 0, 0, 0};
String stateTonne[] = {"OFF", "OFF", "OFF", "OFF"};
bool sendStateTonne[] = {false, false, false, false};
int rgbTonne[][3] = {{255, 255, 0}, {255, 0, 0}, {0, 255, 0}, {0, 0, 255}};
// immediately send battery level
int batteryLevelCounter = BATTERY_SENSE_INTERVAL;
int batteryLevelTonne[] = { -100, -100, -100, -100};

bool isTimeClientSetup = false;

const int red_light_pin = 3;
const int green_light_pin = 4;
const int blue_light_pin = 5;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
WiFiClient wifiClient;
PubSubClient client(wifiClient);

boolean sendStateMessage = false;
String clientIdSpecification = "muelltonnenscanner";

long lastMsg = 0;
int state = STATE_START_SCAN;

void RGB_color(int red_light_value, int green_light_value, int blue_light_value)
{
  analogWrite(red_light_pin, red_light_value);
  analogWrite(green_light_pin, green_light_value);
  analogWrite(blue_light_pin, blue_light_value);
}

int readBatteryCapacity(int pin) {
  float measuredvbat = analogRead(pin);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  int milliVolt = measuredvbat * 1000.0;
  int capacityPercent = map(milliVolt, 3600, 4200, 0, 100);
  capacityPercent = constrain(capacityPercent, 0, 100);

  return capacityPercent;
}

void testRGB()
{
  RGB_color(255, 0, 0); // Red
  delay(1000);
  RGB_color(0, 255, 0); // Green
  delay(1000);
  RGB_color(0, 0, 255); // Blue
  delay(1000);
  RGB_color(255, 255, 125); // Raspberry
  delay(1000);
  RGB_color(0, 255, 255); // Cyan
  delay(1000);
  RGB_color(255, 0, 255); // Magenta
  delay(1000);
  RGB_color(255, 255, 0); // Yellow
  delay(1000);
  RGB_color(255, 255, 255); // White
  delay(1000);
  RGB_color(0, 0, 0); // Black
}

void ensureWifiConnection()
{
  do
  {
    delay(1000);

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED && retryCount < 20)
    {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(50);
      Serial.print(".");
      retryCount++;
    }
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Cannot onnect, retrying.");
    }
  }
  while (WiFi.status() != WL_CONNECTED);

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  delay(1000);
}

void startBLEScan()
{
  delayBlink(1000);
  if (!BLE.begin()) {
    delayBlink(10000);
    while (1);
  }

  // @TODO Disable event handler
  //BLE.setEventHandler(BLEDiscovered, blePeripheralDiscoverHandler);
  BLE.scan();
  lastMsg = millis();

  for (int i = 0; i < NUMBER_OF_TONNES; i++) {
    foundTonne[i] = false;
  }
}

void stopBLEScan()
{
  BLE.stopScan();
  BLE.end();
}

bool ensureMQTTConnection()
{
  // Loop until we're reconnected
  int retries = 3;
  while (!client.connected() && --retries > 0)
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ArduinoClient-";
    clientId += clientIdSpecification;
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqttUsername, mqttPassword))
    {
      Serial.println("connected");
      delay(1000);
    } else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  if (!client.connected()) {
    Serial.println("cannot connect to mqtt, giving up...");
    return false;
  } else {
    return true;
  }
}

void delayBlink(int millis)
{
  int loops = millis / 100;
  for (int i = 0; i < loops; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
  }
}

void delayBlinkSlow(int millis)
{
  int loops = millis / 200;
  for (int i = 0; i < loops; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

void setup() {
  rtc.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(red_light_pin, OUTPUT);
  pinMode(green_light_pin, OUTPUT);
  pinMode(blue_light_pin, OUTPUT);

  Serial.begin(115200);
  Serial.println("--- Start Serial Monitor ---");
  //while (!Serial);

  testRGB();
}

int readBatteryLevel(BLEDevice peripheral)
{
  if (!peripheral.connect()) {
    return -1;
  }

  if (!peripheral.discoverAttributes()) {
    peripheral.disconnect();
    return -2;
  }

  BLECharacteristic batteryLevelCharacterisic = peripheral.characteristic("2a19");
  if (!batteryLevelCharacterisic) { // Battery Level
    peripheral.disconnect();
    return -3;
  }

  byte value = 0;

  batteryLevelCharacterisic.readValue(value);
  int valueInt = value;
  peripheral.disconnect();

  return valueInt;
}

int determineTonneForPeripheral(BLEDevice peripheral) 
{
  bool found_beacon = false;
  int major;
  int minor;
  String uuid;
  for(int i=0; i<peripheral._eirDataLength && peripheral._eirDataLength>=2; ) {
    int eirLength = peripheral._eirData[i++];
    int eirType = peripheral._eirData[i++];
    if( eirType == 0xFF && eirLength == 0x1a ) {
      int company_identifier_minor = peripheral._eirData[i++];
      int company_identifier_major = peripheral._eirData[i++];
      if( company_identifier_major == 0x00 && company_identifier_minor == 0x59 /* Nordic */) {
        i += 2; // skip stuff
        for(int j=0; j<16; j++) {
          uint8_t uuid_part = peripheral._eirData[i++];
          if(uuid_part < 0x10) {
            uuid += "0";
          }
          uuid += String(uuid_part, HEX);
        }
        uint16_t major_major = peripheral._eirData[i++];
        uint16_t major_minor = peripheral._eirData[i++];
        uint16_t minor_major = peripheral._eirData[i++];
        uint16_t minor_minor = peripheral._eirData[i++];
        major = major_major * 256 + major_minor;
        minor = minor_major * 256 + minor_minor;
        i++; // RSSI
        found_beacon = true;
      }else{ 
        // skip to next
        i += eirLength - 4;
      }
    }else if(eirLength == 0) {
      // end of useful data reached
      break;
    }else{
      if( eirLength > 2 ) {
        i += eirLength - 2; 
      }
      i++;  /* length not included in eirLength */
    }
  }
  if(found_beacon) {
    if(uuid == "b1ac64242ee443559079a52ba42b2935") {
      switch(major) {
        case INDEX_GELBE_TONNE:
          return INDEX_GELBE_TONNE;
        case INDEX_REST_TONNE:
          return INDEX_REST_TONNE;
        case INDEX_BIO_TONNE:
          return INDEX_BIO_TONNE;
        case INDEX_PAPIER_TONNE:
          return INDEX_PAPIER_TONNE;
        default:
          return PERIPHERAL_ERROR_MAJOR_NOT_IDENTIFIED;
      }
    }else{
      return PERIPHERAL_ERROR_UUID_NOT_MATCHED;
    }
  }else{
    return PERIPHERAL_ERROR_NOT_A_BEACON;
  }
}

void blePeripheralDiscoverHandler(BLEDevice peripheral) {

  int tonne = determineTonneForPeripheral(peripheral);
  if(tonne >= 0 && !foundTonne[tonne]) {
    foundTonne[tonne] = true;
    batteryLevelTonne[tonne] = 0;
    Serial.println((String)"found beacon for " + nameTonne[tonne]);
  }else{
    Serial.println((String)"not a tonne reason " + String(tonne) + " name: " + peripheral.localName() + " address: " + peripheral.address());
  }
}

void computeResult()
{
  sendStateMessage = false;
  for (int i = 0; i < NUMBER_OF_TONNES; i++) {
    if (foundTonne[i]) {
      streakFoundTonne[i]++;
      streakNotFoundTonne[i] = 0;

      RGB_color(rgbTonne[i][0], rgbTonne[i][1], rgbTonne[i][2]);
      delay(1000);
      RGB_color(0, 0, 0);

      if (streakFoundTonne[i] == STREAK_THRESHOLD) {
        stateTonne[i] = "ON";
        sendStateTonne[i] = true;
        sendStateMessage = true;
      } else {
        sendStateTonne[i] = false;
      }
    } else {
      streakFoundTonne[i] = 0;
      streakNotFoundTonne[i]++;

      if (streakNotFoundTonne[i] == STREAK_THRESHOLD) {
        stateTonne[i] = "OFF";
        sendStateTonne[i] = true;
        sendStateMessage = true;
      } else {
        sendStateTonne[i] = false;
      }
    }
    Serial.println("report " + nameTonne[i] + " streakFound: " + streakFoundTonne[i] + " streakNotFound: " + streakNotFoundTonne[i] + " sendState: " + (sendStateTonne[i] ? "true" : "false"));
  }
  Serial.println((String)"send report? "  + (sendStateMessage ? "true" : "false"));
}

void stopWifi()
{
  WiFi.disconnect();
  WiFi.end();
}

void pause()
{

  batteryLevelCounter++;
#ifdef USE_DEEPSLEEP
  Serial.println((String)"deep sleep for minutes: "  + PAUSE_MINUTES);
  rtc.setTime(0, 0, 0);
  rtc.setDate(1, 1, 2020);
  rtc.setAlarmTime(0, PAUSE_MINUTES, 0);
  rtc.enableAlarm(rtc.MATCH_HHMMSS);
  rtc.attachInterrupt(wakeUp);
  rtc.standbyMode();
#else
  Serial.println((String)"sleep for minutes: "  + PAUSE_MINUTES);
  delay(PAUSE_MINUTES * 60 * 1000);
#endif
}

void wakeUp() {
}

void loop() {
  switch (state) {
    case STATE_START_WIFI:
      ensureWifiConnection();
      client.setServer(mqttServer, 1883);
      state = STATE_START_MQTT;
      Serial.println("NEXT STATE: STATE_START_MQTT");
      break;
    case STATE_START_MQTT:
      if (WiFi.status() != WL_CONNECTED) {
        state = STATE_START_WIFI;
        Serial.println("NEXT STATE: STATE_START_WIFI");
      } else if (ensureMQTTConnection()) {
        state = STATE_TRANSMIT_TO_MQTT;
        lastMsg = millis();
        // @TODO needs reset?
        isTimeClientSetup = false;
        Serial.println("NEXT STATE: STATE_TRANSMIT_TO_MQTT");
      }
      break;
    case STATE_TRANSMIT_TO_MQTT:
      if (WiFi.status() != WL_CONNECTED) {
        state = STATE_START_WIFI;
        Serial.println("NEXT STATE: STATE_START_WIFI");
      } else  if (!client.connected()) {
        state = STATE_START_MQTT;
        Serial.println("NEXT STATE: STATE_START_MQTT");
      } else {
        client.loop();
        long now = millis();
        if (isTimeClientSetup) {
          timeClient.update();
        }
        if (now - lastMsg > 200 && !isTimeClientSetup ) {
          timeClient.begin();
          isTimeClientSetup = true;
        }
        if (now - lastMsg > 1000 && batteryLevelCounter == BATTERY_SENSE_INTERVAL) {
          batteryLevelCounter = 0;

          unsigned long epoch = timeClient.getEpochTime();

          setenv("TZ", "GST-1GDT", 1);
          tzset();
          time_t rawtime = epoch;
          struct tm  ts;
          char       buf[80];
          memset(buf, 0, 80);

          // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
          ts = *localtime(&rawtime);
          strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ts);

          String datetime(buf);

          String topicNameTemplate(TOPIC_OWN_BATTERY_LEVEL_TEMPLATE);
          int ownBatteryLevel = readBatteryCapacity(BATTERY_PIN);
          client.publish(topicNameTemplate.c_str(), ((String)"" + ownBatteryLevel).c_str());

          String heartbeatJson = (String)"{\"last_heartbeat_at\": \"" + datetime + "\"}";

          topicNameTemplate = String(TOPIC_OWN_HEARTBEAT_TEMPLATE);
          client.publish(topicNameTemplate.c_str(), heartbeatJson.c_str());

          for (int i = 0; i < NUMBER_OF_TONNES; i++) {
            if (batteryLevelTonne[i] != -100) {
              topicNameTemplate = String(TOPIC_BATTERY_LEVEL_TEMPLATE);
              topicNameTemplate.replace((String)"%nameTonne%", nameTonne[i]);
              client.publish(topicNameTemplate.c_str(), ((String)"" + batteryLevelTonne[i]).c_str());
              batteryLevelTonne[i] = -100;

              topicNameTemplate = String(TOPIC_HEARTBEAT_TEMPLATE);
              topicNameTemplate.replace((String)"%nameTonne%", nameTonne[i]);

              client.publish(topicNameTemplate.c_str(), heartbeatJson.c_str());
            }
          }
        }
        if (now - lastMsg > 1000 && sendStateMessage)
        {
          sendStateMessage = false;
          for (int i = 0; i < NUMBER_OF_TONNES; i++) {
            if (sendStateTonne[i]) {
              String topicNameTemplate(TOPIC_CONTACT_TEMPLATE);
              topicNameTemplate.replace((String)"%nameTonne%", nameTonne[i]);
              client.publish(topicNameTemplate.c_str(), stateTonne[i].c_str());
            }
          }
        } else if (now - lastMsg > 10000)
        {
          lastMsg = now;
          state = STATE_STOP_MQTT;
          Serial.println("NEXT STATE: STATE_STOP_MQTT");
        }
      }
      break;
    case STATE_STOP_WIFI:
      stopWifi();
      state = STATE_PAUSE;
      Serial.println("NEXT STATE: STATE_PAUSE");
      break;
    case STATE_PAUSE:
      pause();
      state = STATE_START_SCAN;
      Serial.println("NEXT STATE: STATE_START_SCAN");
      break;
    case STATE_STOP_MQTT:
      client.disconnect();
      state = STATE_STOP_WIFI;
      Serial.println("NEXT STATE: STATE_STOP_WIFI");
      break;
    case STATE_START_SCAN:
      startBLEScan();
      state = STATE_SCAN_BLE_BEACON;
      Serial.println("NEXT STATE: STATE_SCAN_BLE_BEACON");
      break;
    case STATE_COMPUTE_RESULT:
      computeResult();
      if (batteryLevelCounter == BATTERY_SENSE_INTERVAL) {
        // time to send battery status
        state = STATE_START_WIFI;
        Serial.println("NEXT STATE: STATE_START_WIFI");
      } else if (sendStateMessage) {
        state = STATE_START_WIFI;
        Serial.println("NEXT STATE: STATE_START_WIFI");
      } else {
        state = STATE_PAUSE;
        Serial.println("NEXT STATE: STATE_PAUSE");
      }
      break;
    case STATE_STOP_SCAN:
      stopBLEScan();
      state = STATE_COMPUTE_RESULT;
      Serial.println("NEXT STATE: STATE_COMPUTE_RESULT");
      break;
    case STATE_SCAN_BLE_BEACON:
      // @TODO Disable event handler
      //BLE.poll(1000);
      BLEDevice peripheral = BLE.available();
      if (peripheral) {
        blePeripheralDiscoverHandler(peripheral);
      }
      long now = millis();
      if (now - lastMsg > 40000)
      {
        lastMsg = now;
        state = STATE_STOP_SCAN;
        Serial.println("NEXT STATE: STATE_STOP_SCAN");
      }
      break;
  }
}
