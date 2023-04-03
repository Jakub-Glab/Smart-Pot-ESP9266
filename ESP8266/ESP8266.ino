/*************************************************************
  Blynk is a platform with iOS and Android apps to control
  ESP32, Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build mobile and web interfaces for any
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: https://www.blynk.io
    Sketch generator:           https://examples.blynk.cc
    Blynk community:            https://community.blynk.cc
    Follow us:                  https://www.fb.com/blynkapp
                                https://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************
  This example runs directly on ESP8266 chip.

  NOTE: This requires ESP8266 support package:
    https://github.com/esp8266/Arduino

  Please be sure to select the right ESP8266 module
  in the Tools -> Board menu!

  Change WiFi ssid, pass, and Blynk auth token to run :)
  Feel free to apply it to any other example. It's simple!
 *************************************************************/

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

/* Fill in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID "TMPL93-1UBiM"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "tS6OpMT1QUMAF4OifajalmVnykm3nmmx"


#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <BH1750.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "INEA-4376_2.4G";
char pass[] = "H97YS9bF";

BH1750 lightMeter;
Adafruit_BMP280 bme;


unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates DHT readings every 2 seconds
const long interval = 2000;

float press;

void setup()
{
  // Debug console
  Wire.begin();
  Serial.begin(9600);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
   if (lightMeter.begin()) {
    Serial.println(F("BH1750 initialised"));
  }
  else {
    Serial.println(F("Error initialising BH1750"));
  }

  if (!bme.begin(0x76)) { 
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
  }
}

BLYNK_WRITE(V1)  // Reset
{
  if (param.asInt()==1) {
  delay(100);
  ESP.restart();
    } 
}

void loop()
{
  Blynk.run();
  float lux = lightMeter.readLightLevel();
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you updated the DHT values
    previousMillis = currentMillis;

    Serial.print("Light: ");
    Serial.print(lux);
    Serial.println(" lx");
    Blynk.virtualWrite(V8, lux);

    Serial.print("Temperature = ");
    Serial.print(bme.readTemperature());
    Serial.println(" *C");
    Blynk.virtualWrite(V6, bme.readTemperature());

    press = bme.readPressure();
    press = press/100;
    Blynk.virtualWrite(V7, press);
    Serial.print("Pressure = ");
    Serial.print(press);
    Serial.println(" Pa");
    
  }
}

