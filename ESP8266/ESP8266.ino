#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL93-1UBiM"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "****"


#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <BH1750.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Math.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for SSD1306 display connected using I2C
#define OLED_RESET     -1 // Reset pin
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "****";
char pass[] = "****";

BH1750 lightMeter;
Adafruit_BMP280 bme;


unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates DHT readings every 2 seconds
const long interval = 2000;

const int AirValue = 780;   //you need to replace this value with Value_1
const int WaterValue = 390;  //you need to replace this value with Value_2
int intervals = (AirValue - WaterValue)/3;
int soilMoistureValue = 0;
float moisturePrcnt = 0.0;

float press;
float lux;
float temp;


void initializeSensors()
{
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

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
	Serial.println(F("SSD1306 allocation failed"));
	for(;;); // Don't proceed, loop forever
  }
}

BLYNK_WRITE(V1)  // Reset
{
  if (param.asInt()==1) {
  delay(100);
  ESP.restart();
    } 
}

void writeToBlynk()
{
  Blynk.virtualWrite(V8, lux);
  Blynk.virtualWrite(V6, temp);
  Blynk.virtualWrite(V7, press);
  Blynk.virtualWrite(V9, moisturePrcnt);
}

void printToSerial()
{
    Serial.println("================================");
    Serial.print("Light: ");
    Serial.print(lux);
    Serial.println(" lx");
    Serial.print("Temperature = ");
    Serial.print(temp);
    Serial.println(" *C");
    Serial.print("Pressure = ");
    Serial.print(press);
    Serial.println(" Pa");
    if(soilMoistureValue > WaterValue && soilMoistureValue < (WaterValue + intervals))
    {
      Serial.println("Soil Moisture: Very Wet");
    }
    else if(soilMoistureValue > (WaterValue + intervals) && soilMoistureValue < (AirValue - intervals))
    {
      Serial.println("Soil Moisture: Wet");
    }
    else if(soilMoistureValue < AirValue && soilMoistureValue > (AirValue - intervals))
    {
      Serial.println("Soil Moisture: Dry");
    }
    Serial.print("====================================");
}

void printToDisplay()
{
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("=====================");
  display.setCursor(0,8);
  display.print("Light: ");
  display.print(lux);
  display.println(" lx");
  display.setCursor(0,16);
  display.print("Temperature = ");
  display.print(temp);
  display.print((char)247);
  display.println("C");
  display.setCursor(0,32);
  display.print("Pressure = ");
  display.print(press);
  display.println(" Pa");
  display.setCursor(0,40);
  display.print("Soil moisture = ");
  display.print(moisturePrcnt, 1);
  display.println("%");
  display.setCursor(0,48);
  display.println("=====================");
  display.display();
}

void readFromSensors()
{
  temp = bme.readTemperature();
  press = bme.readPressure();
  press = press/100;
  soilMoistureValue = analogRead(A0);  //put Sensor insert into soil
  moisturePrcnt = ((soilMoistureValue - AirValue)*100/(WaterValue-AirValue));
  lux = lightMeter.readLightLevel();
}

void setup()
{
  // Debug console
  Wire.begin();
  Serial.begin(9600);
  initializeSensors();
  // Clear the buffer.
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
}

void loop()
{
  Blynk.run();
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    readFromSensors();
    printToSerial();
    printToDisplay();
    writeToBlynk();
  }
}

