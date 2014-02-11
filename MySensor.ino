/*
 * This sketch reads measurment and sends it over 433 MHz
 *
 *
 */

#define WITHSERIAL 1
//#undef WITHSERIAL
#define WITHDHT 1
#define WITHONEWIRE 1
//#undef WITHONEWIRE

#include <SensorTransmitter.h>

#ifdef WITHSERIAL
#include <SendOnlySoftwareSerial.h>
#endif

#ifdef WITHDHT
#include <dht11.h>
#endif

#ifdef WITHONEWIRE
#include <OneWire.h>
#endif

#define SERIAL_PIN 0
#define ONEWIRE_PIN 2
#define DHT_PIN 1
#define RF_PIN 3

// OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library

#ifdef WITHSERIAL
SendOnlySoftwareSerial mySerial (SERIAL_PIN);  // Tx pin
#endif

#ifdef WITHDHT
dht11 DHT11;
#endif

#ifdef WITHONEWIRE
OneWire  ds(ONEWIRE_PIN);  // on pin 10 (a 4.7K resistor is necessary)
#endif

void setup(void) {
#ifdef WITHSERIAL
  mySerial.begin(9600);
#endif
}

void sendowtemp(byte randid, byte randchan, byte *data) {
  float celsius;
  ThermoHygroTransmitter transmitter(RF_PIN, randid, randchan);

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  byte cfg = (data[4] & 0x60);
  // at lower res, the low bits are undefined, so let's zero them
  if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
  else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
  else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
  //// default is 12 bit resolution, 750 ms conversion time

  celsius = (float)raw / 16.0;
  transmitter.sendTempHumi(round(celsius*10), 0);
}

void loop(void) {
  byte i;
  byte present;
  byte data[12];
  byte addr[8];
  byte tempid, tempchan;
  float h, t;

#ifdef WITHONEWIRE
  if ( !ds.search(addr)) {
#ifdef WITHSERIAL
    mySerial.println("No more addresses.");
#endif
    ds.reset_search();
    delay(250);
  } else {
#ifdef WITHSERIAL
    mySerial.print("ROM =");
    for( i = 0; i < 8; i++) {
      mySerial.write(' ');
      mySerial.print(addr[i], HEX);
    }
#endif
    tempid = 0;
    tempchan = 0;
    for (i = 0; i < 3; i++) {
      tempid ^= addr[(i*2)];
      tempchan ^= addr[(i*2)+1];
    }
#ifdef WITHSERIAL
    mySerial.print("Tempid = ");
    mySerial.println(tempid, HEX);

    mySerial.println();
#endif
    ds.reset();
    ds.select(addr);
    ds.write(0x44, 1);  // start conversion, with parasite power on at the end

    delay(1000);     // maybe 750ms is enough, maybe not
    // we might do a ds.depower() here, but the reset will take care of it.

    present = ds.reset();
    ds.select(addr);
    ds.write(0xBE);         // Read Scratchpad
#ifdef WITHSERIAL
    mySerial.print("  Data = ");
    mySerial.print(present, HEX);
    mySerial.print(" ");
#endif
    for ( i = 0; i < 9; i++) {           // we need 9 bytes
      data[i] = ds.read();
#ifdef WITHSERIAL
      mySerial.print(data[i], HEX);
      mySerial.print(" ");
#endif
    }
#ifdef WITHSERIAL
    mySerial.print(" CRC=");
    mySerial.print(OneWire::crc8(data, 8), HEX);
    mySerial.println();
#endif
    sendowtemp(tempid, tempchan, data);
    delay(500);
  }
#endif

#ifdef WITHDHT
  ThermoHygroTransmitter transmitter(RF_PIN, 0, 1);
  int dhtstatus = DHT11.read(DHT_PIN);
  if (dhtstatus != DHTLIB_OK) {
#ifdef WITHSERIAL
    mySerial.println("Failed DHT");
#endif
  } else  {
#ifdef WITHSERIAL
    mySerial.print("Humidity: ");
    mySerial.println(DHT11.humidity);
    mySerial.print("Dht temp: ");
    mySerial.println(DHT11.temperature);
    mySerial.println("Sending temphum via 433 MHz");
#endif
    transmitter.sendTempHumi(round(DHT11.temperature*10), round(DHT11.humidity));
    delay(5000);
  }
#endif
}
