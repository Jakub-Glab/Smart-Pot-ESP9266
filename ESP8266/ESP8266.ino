// Include necessary libraries
#include <FS.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp8266.h>
#include <BH1750.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Math.h>
#include <ESP8266HTTPClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <time.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ctime>
#include <EasyButton.h>


// Screen settings
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin
#define SCREEN_ADDRESS 0x3C
#define BUTTON_PIN 0  // Pin for Flash button

// Global objects and variables
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
BH1750 lightMeter;
Adafruit_BMP280 bme;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pl.pool.ntp.org", 7200);
EasyButton button(BUTTON_PIN);
WiFiManager wifiManager;
 
char token[10];
char deviceName[41];
char deviceID[10];
bool shouldSaveConfig = false;

int buttonDuration = 5000;

unsigned long previousMillis = 0;    // will store last time DHT was updated
const long interval = 2000;          // Updates DHT readings every 2 seconds

String currentDate;

float press;
float lux;
float temp;

const int AirValue = 780;   
const int WaterValue = 390;  
int intervals = (AirValue - WaterValue)/3;
int soilMoistureValue = 0;
float moisturePrcnt = 0.0;

const char* serverUrl = "http://5dfe-83-20-164-34.ngrok-free.app/api/v1/plants/update-plant"; // URL of the server endpoint
 

void setup()
{
  Wire.begin();
  Serial.begin(9600);
  button.begin();
  button.onPressedFor(buttonDuration, onPressed);

  readStoredInfo();

  WiFiManagerParameter api_token("token", "your token", token, 40);
  WiFiManagerParameter device_name("device_name", "your device name", deviceName, 40);
  WiFiManagerParameter device_id("device_id", "your device id", deviceID, 40);

  WiFiManager wifiManager;

  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.addParameter(&api_token);
  wifiManager.addParameter(&device_name);
  wifiManager.addParameter(&device_id);

  wifiManager.setTimeout(180);

  if (!wifiManager.autoConnect("AutoConnectAP")) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    // Reset and try again, or maybe put it to deep sleep.
    ESP.reset();
    delay(5000);
  }

  Serial.println("Connected to WiFi.");

  strcpy(token, api_token.getValue());
  strcpy(deviceName, device_name.getValue());
  strcpy(deviceID, device_id.getValue());

  Serial.print("Token: ");
  Serial.println(token);
  Serial.print("Device Name: ");
  Serial.println(deviceName);

  saveData();

  // Initialize Sensors
  initializeSensors();

  // Initialize display settings
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  timeClient.begin();

  while(!timeClient.update()) {
    timeClient.forceUpdate();
    delay(500);
  }
}

void loop()
{
  button.read();
  timeClient.update();
  currentDate = getCurrentDateTime();
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    readFromSensors();
    printToSerial();
    printToDisplay();
    sendToServer();
  }
}

 
 void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void onPressed() {
  Serial.println("Flash button has been pressed. Resetting WiFi settings...");
  wifiManager.resetSettings();
  SPIFFS.format();
  
  delay(1000);

  Serial.println("Restarting ESP...");
  ESP.restart();
}

String getCurrentDateTime() {
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  
  char buffer[30];
  strftime(buffer, 30, "%Y-%m-%dT%H:%M:%SZ", ptm);
  
  return String(buffer);
}

void initializeSensors()
{
 
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
    for(;;); 
  }
}
 
void sendToServer() {
  if (WiFi.status() == WL_CONNECTED) { 
    
    WiFiClient client; 
    HTTPClient http;    

    // Create JSON payload
    String payload = "{"
                     "\"device_id\":\"" + String(deviceID) + "\","  // Add the device_id field
                     "\"sensors\":{"
                     "\"humidity\":" + String(moisturePrcnt) + ","
                     "\"lux\":" + String(lux) + ","
                     "\"temperature\":" + String(temp) + "},"
                     "\"device_token\":\"" + String(token) + "\","
                     "\"name\":\"" + String(deviceName) + "\","  // Add the name field
                     "\"last_updated\":\"" + String(currentDate) + "\""
                     "}";

    http.begin(client, serverUrl);
    http.addHeader("Content-Type", "application/json");  
    
    int httpCode = http.PATCH(payload); 

    Serial.println("HTTP Response code: " + String(httpCode));

    if (httpCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response: " + response);
    }
    
    http.end();
  } else {
    Serial.println("Error in WiFi connection");
  }
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
    Serial.println("====================================");
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

void readStoredInfo()
{
  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

 #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( ! deserializeError ) {
#else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
#endif
          Serial.println("\nparsed json");
          strcpy(token, json["token"]);
          strcpy(deviceName, json["deviceName"]);
          strcpy(deviceID, json["deviceID"]);
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }

}

void saveData() {
  if (shouldSaveConfig) {
    Serial.println("saving config");
#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    DynamicJsonDocument json(1024);
#else
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
#endif
    json["token"] = token;
    json["deviceName"] = deviceName;
    json["deviceID"] = deviceID; 

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    serializeJson(json, Serial);
    serializeJson(json, configFile);
#else
    json.printTo(Serial);
    json.printTo(configFile);
#endif
    configFile.close();
  }
}

 

