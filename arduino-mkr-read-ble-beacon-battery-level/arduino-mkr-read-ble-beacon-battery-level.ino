#include <ArduinoBLE.h>


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

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("starting BLE");
  if (!BLE.begin()) {
    delayBlink(10000);
    while (1);
  }
  Serial.println("starting scan");
  BLE.scanForName("papiertonne");

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

void loop() {
  BLEDevice peripheral = BLE.available();

  if (peripheral) {
    BLE.stopScan();
    int batteryLevel = readBatteryLevel(peripheral);
    Serial.println((String)"Battery Level: " + batteryLevel);
  }
}
