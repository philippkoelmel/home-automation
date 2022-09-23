#include <RTCZero.h>

RTCZero rtc;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  delay(5000);
  rtc.begin();
  rtc.setTime(0, 0, 0);
  rtc.setDate(1, 1, 2020);
  rtc.setAlarmTime(0, 0, 10);
  rtc.enableAlarm(rtc.MATCH_HHMMSS);
  rtc.attachInterrupt(turnOn);
  rtc.standbyMode();
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}

void turnOn() {
  for (int i = 0; i < 10; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
  }
}
