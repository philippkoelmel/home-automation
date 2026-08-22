#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "secrets.h"

#define RELAY_PIN D1
#define RELAY_ACTIVE_LOW true
#define MAX_ON_MS (3UL * 60UL * 60UL * 1000UL) // safety auto-off if /off is never called
#define WIFI_RETRY_INTERVAL_MS 10000
#define MDNS_HOSTNAME "myto-ir"

ESP8266WebServer server(80);

bool relayOn = false;
unsigned long relayOnSince = 0;
unsigned long lastWifiAttempt = 0;

void applyRelay() {
  bool activeLevel = RELAY_ACTIVE_LOW ? LOW : HIGH;
  bool idleLevel = RELAY_ACTIVE_LOW ? HIGH : LOW;
  digitalWrite(RELAY_PIN, relayOn ? activeLevel : idleLevel);
}

void setRelay(bool on) {
  relayOn = on;
  if (on) {
    relayOnSince = millis();
  }
  applyRelay();
  digitalWrite(LED_BUILTIN, on ? LOW : HIGH); // LED_BUILTIN is active-low on the D1 Mini Pro
  Serial.println(on ? "[relay] ON" : "[relay] OFF");
}

bool checkAuth() {
  if (!server.hasArg("token") || server.arg("token") != AUTH_TOKEN) {
    server.send(401, "text/plain", "unauthorized");
    return false;
  }
  return true;
}

void handleOn() {
  if (!checkAuth()) return;
  setRelay(true);
  server.send(200, "text/plain", "ok");
}

void handleOff() {
  if (!checkAuth()) return;
  setRelay(false);
  server.send(200, "text/plain", "ok");
}

void handleStatus() {
  unsigned long onForMs = relayOn ? millis() - relayOnSince : 0;
  String json = "{\"relay\":\"" + String(relayOn ? "on" : "off") +
                "\",\"on_for_ms\":" + String(onForMs) +
                ",\"wifi_rssi\":" + String(WiFi.RSSI()) +
                ",\"uptime_ms\":" + String(millis()) + "}";
  server.send(200, "application/json", json);
}

void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  if (millis() - lastWifiAttempt < WIFI_RETRY_INTERVAL_MS) return;
  lastWifiAttempt = millis();
  Serial.println("[wifi] reconnecting...");
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void setup() {
  // Relay defaults to off before anything else runs, including before WiFi connects.
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  setRelay(false);

  Serial.begin(115200);
  Serial.println();
  Serial.println("[boot] Myto IR controller starting");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[wifi] connecting to ");
  Serial.print(WIFI_SSID);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[wifi] connected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[wifi] not connected yet, will keep retrying in background");
  }

  MDNS.begin(MDNS_HOSTNAME);

  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/status", handleStatus);
  server.begin();

  MDNS.addService("http", "tcp", 80);
  Serial.println("[http] server started on port 80, hostname " MDNS_HOSTNAME ".local");
}

void loop() {
  ensureWiFi();
  server.handleClient();
  MDNS.update();

  if (relayOn && millis() - relayOnSince > MAX_ON_MS) {
    setRelay(false);
  }
}
