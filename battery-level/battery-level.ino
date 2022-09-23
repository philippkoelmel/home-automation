//#include <Battery.h>

//Battery battery(3400, 4200, A0);

void setup() {
  Serial.begin(9600);
  //  battery.begin(4200, 1.0);
}

int readBatteryCapacity(int pin) {
  float measuredvbat = analogRead(pin);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  int milliVolt = measuredvbat * 1000.0;
  int capacityPercent = map(milliVolt, 3600, 4200, 0, 100);
  return capacityPercent;
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
  Serial.print(" %: " ); Serial.println(readBatteryCapacity(A0));
}
