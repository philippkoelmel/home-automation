#include <MKRIMU.h>
#include <LowPower.h>
#include <WiFiNINA.h>

#define SLEEP_INTERVAL 10000

#include <PubSubClient.h>

#include "credentials.h"


WiFiClient wifiClient;
PubSubClient client(wifiClient);
int ledPin = LED_BUILTIN;
String clientIdSpecification = "muelltonne";
int willQoS = 0;
boolean willRetain = true;
const char willMessage[] = "offline";
const char connectMessage[] = "online";
const float tolerance = 100.0;
boolean isMoving = false;

char pubTopic[] = "home-assistant/muelltonne/moving";
const char willTopic[] = "home-assistant/muelltonne/availability";

boolean didSentConnectMessage = false;

void setup_wifi()
{
  do
  {
    delay(1000);

    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED && retryCount < 20)
    {
      digitalWrite(ledPin, HIGH);
      delay(500);
      digitalWrite(ledPin, LOW);
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

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (true); // halt program
  }
  Serial.println("IMU initialized!");
  setup_wifi();
  client.setServer(mqttServer, 1883);

  Serial.println("MQTT initialized!");
  Serial.print("Gyroscop sample rate = ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");
  Serial.println();
  Serial.println("Gyroscope in degrees/second");
  Serial.println("X\tY\tZ");
}

void delayBlink(int millis)
{
  int loops = millis / 100;
  for (int i = 0; i < loops; i++)
  {
    digitalWrite(ledPin, HIGH);
    delay(50);
    digitalWrite(ledPin, LOW);
    delay(50);
  }
}

void disconnectWifi()
{
  WiFi.disconnect();
}

void reconnect()
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
    if (client.connect(clientId.c_str(), mqttUsername, mqttPassword)) //, willTopic, willQoS, willRetain, willMessage))
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
  }
}

void loop()
{
  float aX, aY, aZ;
  float gX, gY, gZ;
  const char * spacer = ", ";

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Lost WiFi connection retrying...");
    didSentConnectMessage = false;
    setup_wifi();
  } else if (!client.connected())
  {
    Serial.println("Lost MQTT connection retrying...");
    //didSentConnectMessage = false;
    reconnect();
  } else {
    client.loop();
    //if (!didSentConnectMessage) {
    //Serial.println("mark online...");
    //didSentConnectMessage = true;
    //client.publish(willTopic, connectMessage);
    //}
    delayBlink(2000);
    if (IMU.gyroscopeAvailable()) {
      IMU.readGyroscope(gX, gY, gZ);
      String gyroscopeString = (String)"(" + gX + ", " + gY + ", " + gZ + ") ";
      Serial.println(gyroscopeString);

      if (abs(gX) > tolerance || abs(gY) > tolerance || abs(gZ) > tolerance) {
        if (!isMoving) {
          String payLoad = "ON";
          client.publish(pubTopic, payLoad.c_str());
          isMoving = true;
        }
      } else {
        if (isMoving) {
          String payLoad = "OFF";
          client.publish(pubTopic, payLoad.c_str());
          isMoving = false;
        }
      }
    }

    //digitalWrite(ledPin, HIGH);
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    //delay(SLEEP_INTERVAL);
    //digitalWrite(ledPin, LOW);
  }
}
