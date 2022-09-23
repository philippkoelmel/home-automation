#include <WiFiNINA.h>
#include <PubSubClient.h>

#include "credentials.h"

char distancePubTopic[] = "home-assistant/obenmilch_bottle/contact";
char forcePubTopic[] = "home-assistant/obenmilch_bottle/force";
const char willTopic[] = "home-assistant/obenmilch_bottle/availability";
int willQoS = 0;
boolean willRetain = true;
const char willMessage[] = "offline";
const char connectMessage[] = "online";

WiFiClient wifiClient;
PubSubClient client(wifiClient);
long lastMsg = 0;
char msg[50];
int value = 0;
int ledState = 0;

int ledPin = A0;
int distanceSensorPin = 2;
const int forceSensorPin = A1; //pin A0 to read analog input

int previousState = 0;
short distanceSensorValue = 0;
int forceSensorValue = 0;
int forceStreakOn = 0;
int forceStreakOff = 0;
String forceState = "OFF";
bool sendForceState = true;
boolean didSentConnectMessage = false;
int forceLedState = LOW;

String clientIdSpecification = "obenmilch";

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
    if (client.connect(clientId.c_str(), mqttUsername, mqttPassword, willTopic, willQoS, willRetain, willMessage))
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

void setup()
{
  pinMode(ledPin, OUTPUT);
  pinMode(distanceSensorPin, INPUT);
  Serial.begin(115200);
  Serial.println("--- Start Serial Monitor ---");
  delayBlink(1000);
  setup_wifi();
  client.setServer(mqttServer, 1883);
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Lost WiFi connection retrying...");
    didSentConnectMessage = false;
    setup_wifi();
  } else if (!client.connected())
  {
    Serial.println("Lost MQTT connection retrying...");
    didSentConnectMessage = false;
    reconnect();
  } else {
    client.loop();
    if (!didSentConnectMessage) {
      Serial.println("Tell Home Assistant we are online...");
      didSentConnectMessage = true;
      client.publish(willTopic, connectMessage);
    }

    long now = millis();

    if (now - lastMsg > 500)
    {
      if (forceState == "ON") {
        forceLedState = (forceLedState == HIGH) ? LOW : HIGH;
        digitalWrite(ledPin, forceLedState);
      }

      forceSensorValue = analogRead(forceSensorPin);
      String forceSensorValueString = String(forceSensorValue);
      Serial.println("force: " + forceSensorValueString);

      if (forceSensorValue > 100) {
        forceStreakOn++;
        forceStreakOff = 0;
        if (forceStreakOn == 5 && forceState != "ON") {
          forceState = "ON";
          sendForceState = true;
        }
      } else {
        forceStreakOn = 0;
        forceStreakOff++;
        if (forceStreakOff == 5 && forceState != "OFF") {
          forceState = "OFF";
          sendForceState = true;
          digitalWrite(ledPin, LOW);
        }
      }

      if (sendForceState) {
        client.publish(forcePubTopic, forceState.c_str());
        sendForceState = false;
      }

      distanceSensorValue = digitalRead(distanceSensorPin);
      String distanceSensorValueString = String(distanceSensorValue);
      Serial.println("distance: " + distanceSensorValueString);

      if (distanceSensorValue == 0 && previousState == 1)
      {
        Serial.println(distanceSensorValue, DEC);
        digitalWrite(ledPin, LOW);
        previousState = 0;
        String payLoad = "ON";
        client.publish(distancePubTopic, payLoad.c_str());
      }
      if (distanceSensorValue != 0 && previousState == 0)
      {
        Serial.println(distanceSensorValue, DEC);
        digitalWrite(ledPin, HIGH);
        previousState = 1;
        String payLoad = "OFF";
        client.publish(distancePubTopic, payLoad.c_str());
      }
      lastMsg = now;
    }
  }
}
