#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

#if SOFTWARE_SERIAL_AVAILABLE
#include <SoftwareSerial.h>
#endif

//#define DEBUG_MODE

#define VBATPIN A7

#define FACTORYRESET_ENABLE      1

#define MANUFACTURER_APPLE         "0x004C"
#define MANUFACTURER_NORDIC        "0x0059"

//#define PAPIERTONNE
#define RESTTONNE
//#define BIOTONNE
//#define GELBETONNE


#ifdef PAPIERTONNE
#define BEACON_NAME                "papiertonne"
#define BEACON_UUID                "03-10-23-34-45-56-67-78-89-9A-AB-BC-CD-DE-EF-F0"
#endif

#ifdef RESTTONNE
#define BEACON_NAME                "resttonne"
#define BEACON_UUID                "03-11-23-34-45-56-67-78-89-9A-AB-BC-CD-DE-EF-F0"
#endif

#ifdef BIOTONNE
#define BEACON_NAME                "biotonne"
#define BEACON_UUID                "03-12-23-34-45-56-67-78-89-9A-AB-BC-CD-DE-EF-F0"
#endif

#ifdef GELBETONNE
#define BEACON_NAME                "gelbetonne"
#define BEACON_UUID                "03-13-23-34-45-56-67-78-89-9A-AB-BC-CD-DE-EF-F0"
#endif

#define BEACON_MANUFACTURER_ID     MANUFACTURER_APPLE
#define BEACON_MAJOR               "0x0000"
#define BEACON_MINOR               "0x0000"
#define BEACON_RSSI_1M             "-54"

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
int previousBatteryLevel = 0;

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  Serial.println("halting.");
  while (1);
}

void setup(void)
{
#ifdef DEBUG_MODE
  while (!Serial);
#endif
  delay(500);

  Serial.begin(115200);

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ) {
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  Serial.println(F("Setting beacon configuration details: "));

  // AT+BLEBEACON=0x004C,01-12-23-34-45-56-67-78-89-9A-AB-BC-CD-DE-EF-F0,0x0000,0x0000,-54
  ble.print("AT+BLEBEACON="        );
  ble.print(BEACON_MANUFACTURER_ID ); ble.print(',');
  ble.print(BEACON_UUID            ); ble.print(',');
  ble.print(BEACON_MAJOR           ); ble.print(',');
  ble.print(BEACON_MINOR           ); ble.print(',');
  ble.print(BEACON_RSSI_1M         );
  ble.println(); // print line causes the command to execute

  // check response status
  if (! ble.waitForOK() ) {
    error(F("Didn't get the OK on getting info"));
  }

  if (! ble.sendCommandCheckOK(F(((String)"AT+GAPDEVNAME=" + BEACON_NAME).c_str())) ) {
    error(F("Could not set device name?"));
  }

  if (! ble.sendCommandCheckOK(F("AT+BLEBATTEN=1")) ) {
    error(F("Could not BLEBATTEN"));
  }

  if (! ble.sendCommandCheckOK(F("ATZ")) ) {
    error(F("Could not BLEBATTVAL"));
  }

  /*  if (! ble.sendCommandCheckOK(F("AT+BLEBATTVAL=100")) ) {
      error(F("Could not BLEBATTVAL"));
    }
  */
  Serial.println();
  Serial.println(F("Open your beacon app to test"));
}

void loop(void)
{
  delay(1000);

  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage

  int milliVolt = measuredvbat * 1000.0;
  int batterylevel = map(milliVolt, 3600, 4200, 0, 100);
  batterylevel = constrain(batterylevel, 0, 100);

  Serial.println((String)"batterylevel: " + batterylevel + " millivolt: " + milliVolt + " measuredvbat: " + measuredvbat);

  if (batterylevel != previousBatteryLevel) {
    previousBatteryLevel = batterylevel;
    if (! ble.sendCommandCheckOK(F(((String)"AT+BLEBATTVAL=" + batterylevel).c_str()))) {
      Serial.println(F(((String)"Could not AT+BLEBATTVAL=" + batterylevel).c_str()));
    }
  }

#ifndef DEBUG_MODE
  delay(10 * 60000);
#endif
}
