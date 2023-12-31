#include <singleLEDLibrary.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PZEM004Tv30.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#define PZEM_SERIAL Serial2
#define NUM_PZEMS 5
int START_INDEX = 11;
#define ledPin 2

const char *ssid = "Weregrin Meter";
const char *password = "WeregrinX";
const char *serverUrl = "https://weregrin.com/v1/push_data/elect";

int timezone = 7 * 3600;
int dst = 0;
const long measureInterval = 500;

PZEM004Tv30 pzems[NUM_PZEMS];
LiquidCrystal_I2C lcd1(0x27, 20, 4);
LiquidCrystal_I2C lcd2(0x26, 20, 4);
sllib LED(ledPin);

unsigned long powerLoggerPreviousMillis = 0;
unsigned long displayPowerDataPreviousMillis = 0;
unsigned long postRequestPowerDataPreviousMillis = 0;
const unsigned long powerLoggerInterval = 500;
const unsigned long displayPowerDataInterval = 1000;
const unsigned long PostRequestPowerDataInterval = 300000;

float voltage[NUM_PZEMS];
float current[NUM_PZEMS];
float power[NUM_PZEMS];
float energy[NUM_PZEMS];
float frequency[NUM_PZEMS];
float pf[NUM_PZEMS];

void setup()
{
  HardwareSetup();
  NetworkSetup();
  configTime(timezone, dst, "pool.ntp.org", "time.nist.gov");
}

void loop()
{
  LED.update();
  checkWifiConnection();
  unsigned long currentMillis = millis();
  if (currentMillis - powerLoggerPreviousMillis >= powerLoggerInterval)
  {
    powerLoggerPreviousMillis = currentMillis;
    readPZEMValues();
    for (int i = 0; i < NUM_PZEMS; i++)
    {
      PowerLogger(i, voltage[i], current[i], power[i], energy[i], frequency[i], pf[i]);
    }
  }

  if (currentMillis - displayPowerDataPreviousMillis >= displayPowerDataInterval)
  {
    displayPowerDataPreviousMillis = currentMillis;
    for (int i = 0; i < NUM_PZEMS; i++)
    {
      if (!isnan(voltage[i]) && !isnan(current[i]) && !isnan(power[i]) && !isnan(energy[i]) && !isnan(frequency[i]) && !isnan(pf[i]))
      {
        displayPowerData(i, voltage[i], current[i], power[i], energy[i]);
      }
    }
  }

  if (currentMillis - postRequestPowerDataPreviousMillis >= PostRequestPowerDataInterval)
  {
    postRequestPowerDataPreviousMillis = currentMillis;
    for (int i = 0; i < NUM_PZEMS; i++)
    {
      if (!isnan(voltage[i]) && !isnan(current[i]) && !isnan(power[i]) && !isnan(energy[i]) && !isnan(frequency[i]) && !isnan(pf[i]))
      {
        sendHttpRequest(generateJsonPayload(voltage, current, power, energy, frequency, pf, getTime(), i + START_INDEX));
      }
    }
  }
}

void FirstCore()
{
}

void readPZEMValues()
{
  LED.breathSingle(1000);
  for (int i = 0; i < NUM_PZEMS; i++)
  {
    voltage[i] = pzems[i].voltage();
    current[i] = pzems[i].current();
    power[i] = pzems[i].power();
    energy[i] = pzems[i].energy();
    frequency[i] = pzems[i].frequency();
    pf[i] = pzems[i].pf();
  }
}

void PowerLogger(int i, float voltage, float current, float power, float energy, float frequency, float pf)
{
  Serial.print("Power meter ");
  Serial.print(i + 1);
  Serial.print(": ");
  Serial.print(pzems[i].getAddress(), HEX);
  Serial.println("===================");
  if (isnan(voltage))
  {
    Serial.println("Error reading voltage");
  }
  else if (isnan(current))
  {
    Serial.println("Error reading current");
  }
  else if (isnan(power))
  {
    Serial.println("Error reading power");
  }
  else if (isnan(energy))
  {
    Serial.println("Error reading energy");
  }
  else if (isnan(frequency))
  {
    Serial.println("Error reading frequency");
  }
  else if (isnan(pf))
  {
    Serial.println("Error reading power factor");
  }
  else
  {
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.println("V");
    Serial.print("Current: ");
    Serial.print(current);
    Serial.println("A");
    Serial.print("Power: ");
    Serial.print(power);
    Serial.println("W");
    Serial.print("Energy: ");
    Serial.print(energy, 3);
    Serial.println("kWh");
    Serial.print("Frequency: ");
    Serial.print(frequency, 1);
    Serial.println("Hz");
    Serial.print("PF: ");
    Serial.println(pf);
  }
  Serial.println("-------------------");
  Serial.println();
}

void displayPowerData(int i, float voltage, float current, float power, float energy)
{
  if (i > 3)
  {
    lcd2.setCursor(0, i - 4);
    lcd2.print(String(voltage, 1));
    lcd2.print("V ");
    lcd2.print(String(power, 1));
    lcd2.print("W ");
    lcd2.print(String(energy, 1));
    lcd2.print("U");
  }
  else
  {
    lcd1.setCursor(0, i);
    lcd1.print(String(voltage, 1));
    lcd1.print("V ");
    lcd1.print(String(power, 1));
    lcd1.print("W ");
    lcd1.print(String(energy, 1));
    lcd1.print("U");
  }
}

void HardwareSetup()
{
  lcd1.init();
  lcd2.init();
  lcd1.backlight();
  lcd2.backlight();
  lcd1.home();
  lcd2.home();
  lcd1.clear();
  lcd2.clear();
  Serial.println("Setting up hardware...");
  Serial.begin(115200);
  for (int i = 0; i < NUM_PZEMS; i++)
  {
    pzems[i] = PZEM004Tv30(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x1 + i);
  }
}

void NetworkSetup()
{
  Serial.println("Setting up network...")
  LED.blinkSingle(500);
  WiFiManager wifiManager;
  wifiManager.autoConnect(ssid, password);
  Serial.println("Connected to WiFi");
  LED.setOffSingle();
}

void checkWifiConnection()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi connection lost!");
    ESP.restart();
  }
}

String getTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    ESP.restart();
    return "";
  }
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(buffer);
}

String generateJsonPayload(float voltage, float current, float power, float energy, float frequency, float pf, String dateTime, int id)
{
  // ตรวจสอบความถูกต้องของข้อมูล
  if (isnan(voltage) || isnan(current) || isnan(power) || isnan(energy) || isnan(frequency) || isnan(pf))
  {
    Serial.println("Error: Invalid data");
    return;
  }

  // สร้าง JSON payload
  String jsonPayload =
      "{"
      "\"voltage\": " +
      String(voltage) + ","
                        "\"current\": " +
      String(current) + ","
                        "\"power\": " +
      String(power) + ","
                      "\"energy\": " +
      String(energy) + ","
                       "\"frequency\": " +
      String(frequency) + ","
                          "\"pf\": " +
      String(pf) + ","
                   "\"datetime\": \"" +
      dateTime + "\","
                 "\"meter_id\": " +
      String(id) + "}";

  return jsonPayload;
}

void sendHttpRequest(String jsonPayload)
{
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(jsonPayload);

  if (httpResponseCode > 0)
  {
    // ตรวจสอบสถานะ HTTP ตอบกลับ
    if (httpResponseCode != 200)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }

    String response = http.getString();
    Serial.println(response);
  }
  else
  {
    Serial.print("Error in HTTP POST request. HTTP Response code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}
