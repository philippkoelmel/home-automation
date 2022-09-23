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
  Serial.begin(115200);
  while (!Serial);

  Serial.println("starting BLE");
  if (!BLE.begin()) {
    delayBlink(10000);
    while (1);
  }
  Serial.println("starting scan");
  BLE.scanForAddress("DB:1A:7F:DC:33:10");

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
      Serial.println(uuid);
      Serial.println(String(major, HEX));
      Serial.println(String(minor, HEX));
      switch(major) {
        case 0x0001: // resttonne
          return 1;
        case 0x0002: // papiertonne
          return 2;
        case 0x0003: // biotonne
          return 3;
        case 0x0004: // gelbetonne
          return 4;
        default:
          return -1;
      }
    }else{
      return -2;
    }
  }else{
    Serial.println("not found");
    return -3;
  }
}

void loop() {
  BLEDevice peripheral = BLE.available();

  if (peripheral) {
    BLE.stopScan();
    Serial.println("result: " + String(determineTonneForPeripheral(peripheral)));
  }
}
