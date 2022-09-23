#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiNINA.h>
#include <sys/time.h>

#include "credentials.h"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

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

void setup() {
  Serial.begin(115200);
  Serial.println("--- Start Serial Monitor ---");
  while (!Serial);

  ensureWifiConnection();

  timeClient.begin();

}

void loop() {
  timeClient.update();

  Serial.println(timeClient.getFormattedTime());
  unsigned long epoch = timeClient.getEpochTime();

  setenv("TZ", "GST-1GDT", 1);
  tzset();
  time_t rawtime = epoch;
  struct tm  ts;
  char       buf[80];
  memset(buf, 0, 80);

  // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
  ts = *localtime(&rawtime);
  strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
  Serial.println(buf);

  delay(1000);
}
