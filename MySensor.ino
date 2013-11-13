/*
 * This sketch reads measurment and sends it over 433 MHz
 *
 *
 */


#include <SensorTransmitter.h>
#include <DHT.h>
#include <OneWire.h>


// OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library

OneWire  ds(10);  // on pin 10 (a 4.7K resistor is necessary)
DHT dht(2, DHT11);
bool dhtread = true;


void setup(void) {
  Serial.begin(9600);
  if (dhtread) {
    dht.begin();
  }
}

void sendowtemp(byte randid, byte *data) {
  float celsius;
  ThermoHygroTransmitter transmitter(11, randid, 1);

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
  Serial.println("Sending via 433 MHz");
  transmitter.sendTempHumi(round(celsius*10), 0);
  Serial.println("My conversion");
  Serial.println(celsius);
}

void loop(void) {
  byte i;
  byte present;
  byte data[12];
  byte addr[8];
  byte tempid;
  float h, t;
  bool owread = true;
  if (owread) {
    if ( !ds.search(addr)) {
      Serial.println("No more addresses.");
      Serial.println();
      ds.reset_search();
      delay(250);
    } else {

      Serial.print("ROM =");
      for( i = 0; i < 8; i++) {
        Serial.write(' ');
        Serial.print(addr[i], HEX);
      }

      tempid = 0;
      for (i = 1; i < 7; i++) {
        tempid ^= addr[i];
      }
      Serial.print("Tempid = ");
      Serial.println(tempid, HEX);

      Serial.println();

      ds.reset();
      ds.select(addr);
      ds.write(0x44, 1);  // start conversion, with parasite power on at the end

      delay(1000);     // maybe 750ms is enough, maybe not
      // we might do a ds.depower() here, but the reset will take care of it.

      present = ds.reset();
      ds.select(addr);
      ds.write(0xBE);         // Read Scratchpad

      Serial.print("  Data = ");
      Serial.print(present, HEX);
      Serial.print(" ");
      for ( i = 0; i < 9; i++) {           // we need 9 bytes
        data[i] = ds.read();
        Serial.print(data[i], HEX);
        Serial.print(" ");
      }
      Serial.print(" CRC=");
      Serial.print(OneWire::crc8(data, 8), HEX);
      Serial.println();

      sendowtemp(tempid, data);
      delay(500);
    }
  }
  if (dhtread) {
    ThermoHygroTransmitter transmitter(11, 0, 1);
    h = dht.readHumidity();
    t = dht.readTemperature();
    if (isnan(t) || isnan(h)) {
      Serial.println("Failed DHT");
    } else {
      Serial.print("Humidity: ");
      Serial.println(h);
      Serial.print("Dht temp: ");
      Serial.println(t);
      Serial.println("Sending temphum via 433 MHz");
      transmitter.sendTempHumi(round(t*10), round(h));
      delay(500);
   }
  }
}
