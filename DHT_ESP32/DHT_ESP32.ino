#include "DHT.h"

#define DHTPIN 32 // what digital pin we're connected to

#define DHTTYPE DHT22 // DHT 11

DHT dht(DHTPIN, DHTTYPE);

void setup() {
 Serial.begin(115200);
 Serial.println("DHTxx test!");

dht.begin();
}

void loop() {
 // Wait a few seconds between measurements.
 delay(2000);
 // Read temperature as Celsius (the default)
 float t = dht.readTemperature();
// Check if any reads failed and exit early (to try again).
 if (isnan(t)) {
 Serial.println("Failed to read from DHT sensor!");
 return;
 }

 Serial.print("Temperature: ");
 Serial.print(t);
 Serial.print(" *C \n");
}
