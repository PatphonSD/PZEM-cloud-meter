#include <PZEM004Tv30.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#define PZEM_SERIAL Serial2
#define NUM_PZEMS 5

PZEM004Tv30 pzems[NUM_PZEMS];
LiquidCrystal_I2C lcd1(0x27, 20, 4);
LiquidCrystal_I2C lcd2(0x26, 20, 4);

unsigned long powerLoggerPreviousMillis = 0;
unsigned long displayPowerDataPreviousMillis = 0;
const unsigned long powerLoggerInterval = 100; // 0.1 วินาที
const unsigned long displayPowerDataInterval = 2000; // 2 วินาที

void setup() {
  lcd1.init();
  lcd2.init();
  lcd1.backlight();
  lcd2.backlight();
  lcd1.home();
  lcd2.home();
  lcd1.clear();
  lcd2.clear();
  Serial.begin(115200);
  for (int i = 0; i < NUM_PZEMS; i++) {
    pzems[i] = PZEM004Tv30(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x1 + i);
  }
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - powerLoggerPreviousMillis >= powerLoggerInterval) {
    powerLoggerPreviousMillis = currentMillis;
    
    for (int i = 0; i < NUM_PZEMS; i++) {
      float voltage = pzems[i].voltage();
      float current = pzems[i].current();
      float power = pzems[i].power();
      float energy = pzems[i].energy();
      float frequency = pzems[i].frequency();
      float pf = pzems[i].pf();
      PowerLogger(i, voltage, current, power, energy, frequency, pf);
    }
  }

  if (currentMillis - displayPowerDataPreviousMillis >= displayPowerDataInterval) {
    displayPowerDataPreviousMillis = currentMillis;

    for (int i = 0; i < NUM_PZEMS; i++) {
      float voltage = pzems[i].voltage();
      float current = pzems[i].current();
      float power = pzems[i].power();
      float energy = pzems[i].energy();
      displayPowerData(i, voltage, current, power, energy);
    }

    Serial.println();
  }
}

void PowerLogger(int i, float voltage, float current, float power, float energy, float frequency, float pf) {
  Serial.print("PZEM ");
  Serial.print(i);
  Serial.print(" - Address:");
  Serial.println(pzems[i].getAddress(), HEX);
  Serial.println("===================");
  if (isnan(voltage)) {
    Serial.println("Error reading voltage");
  } else if (isnan(current)) {
    Serial.println("Error reading current");
  } else if (isnan(power)) {
    Serial.println("Error reading power");
  } else if (isnan(energy)) {
    Serial.println("Error reading energy");
  } else if (isnan(frequency)) {
    Serial.println("Error reading frequency");
  } else if (isnan(pf)) {
    Serial.println("Error reading power factor");
  } else {
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

void displayPowerData(int i, float voltage, float current, float power, float energy) {
  if (i > 3) {
    lcd2.setCursor(0, i - 4);
    lcd2.print(String(voltage, 1));
    lcd2.print("V ");
    lcd2.print(String(power, 1));
    lcd2.print("W ");
    lcd2.print(String(energy, 1));
    lcd2.print("U");
  } else {
    lcd1.setCursor(0, i);
    lcd1.print(String(voltage, 1));
    lcd1.print("V ");
    lcd1.print(String(power, 1));
    lcd1.print("W ");
    lcd1.print(String(energy, 1));
    lcd1.print("U");
  }
}
