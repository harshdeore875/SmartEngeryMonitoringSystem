#define BLYNK_TEMPLATE_ID "TMPL3dXk4oa4M"
#define BLYNK_TEMPLATE_NAME "IoT Energy Meter"
#define BLYNK_PRINT Serial
 
#include "EmonLib.h"
#include <EEPROM.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD Configuration (20x4, I2C address 0x27)
LiquidCrystal_I2C lcd(0x27, 20, 4);

// I2C Pins for LCD
#define I2C_SDA 21
#define I2C_SCL 22

// Constants for calibration
const float vCalibration = 148.0;
const float currCalibration = 0.55;
 
// Blynk and WiFi credentials
const char auth[] = "2J-qrE2_bcqBHfXQhMa16H_pWa_psNXn";
const char ssid[] = "OnePlus 12";
const char pass[] = "Rudra123456";
 
// EnergyMonitor instance
EnergyMonitor emon;
 
// Timer for regular updates
BlynkTimer timer;
 
// Variables for energy calculation
float kWh = 0.0;
unsigned long lastMillis = millis();
 
// EEPROM addresses for each variable
const int addrVrms = 0;
const int addrIrms = 4;
const int addrPower = 8;
const int addrKWh = 12;
 
// Function prototypes
void sendEnergyDataToBlynk();
void readEnergyDataFromEEPROM();
void saveEnergyDataToEEPROM();
 
 
void setup()
{
  Serial.begin(115200);
  
  // Initialize LCD
  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();
  
  // Display startup message on LCD
  lcd.setCursor(0, 0);
  lcd.print("  IoT Energy Meter  ");
  lcd.setCursor(0, 1);
  lcd.print("   Starting...      ");
  lcd.setCursor(0, 2);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print("  Please Wait...    ");
  
  Serial.println("==========================");
  Serial.println("IoT Energy Meter Starting");
  Serial.println("==========================");
  
  // Connect to Blynk
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  lcd.setCursor(0, 2);
  lcd.print("WiFi: Connecting... ");
  
  Blynk.begin(auth, ssid, pass);
  
  Serial.println("Connected to Blynk!");
  
  lcd.setCursor(0, 2);
  lcd.print("WiFi: Connected!    ");
  delay(1000);
  
  lcd.setCursor(0, 3);
  lcd.print("Blynk: Connected!   ");
  delay(1000);
 
  // Initialize EEPROM with the size of the data to be stored
  EEPROM.begin(32); // Allocate 32 bytes for float values
 
  // Read the stored energy data from EEPROM
  readEnergyDataFromEEPROM();
  Serial.print("Loaded kWh from EEPROM: ");
  Serial.println(kWh, 5);
 
  // Setup voltage and current inputs
  emon.voltage(35, vCalibration, 1.7); // Voltage: input pin, calibration, phase_shift
  emon.current(34, currCalibration);    // Current: input pin, calibration
 
  // Setup a timer for sending data every 5 seconds
  timer.setInterval(5000L, sendEnergyDataToBlynk);
 
  Serial.println("System Ready!");
  Serial.println("==========================");
  
  // Clear LCD and show ready message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready!       ");
  delay(2000);
  
  // Clear LCD for data display
  lcd.clear();
  
  // A small delay for system to stabilize
  delay(1000);
}
 
 
void loop()
{
  Blynk.run();
  timer.run();
}
 
 
void sendEnergyDataToBlynk()
{
  emon.calcVI(20, 2000); // Calculate all. No.of half wavelengths (crossings), time-out
 
  // Calculate energy consumed in kWh
  unsigned long currentMillis = millis();
  kWh += emon.apparentPower * (currentMillis - lastMillis) / 3600000000.0;
  lastMillis = currentMillis;
 
  // Print data to Serial Monitor
  Serial.println("---------------------------");
  Serial.print("Voltage (Vrms): ");
  Serial.print(emon.Vrms, 2);
  Serial.println(" V");
  
  Serial.print("Current (Irms): ");
  Serial.print(emon.Irms, 4);
  Serial.println(" A");
  
  Serial.print("Power: ");
  Serial.print(emon.apparentPower, 4);
  Serial.println(" W");
  
  Serial.print("Energy (kWh): ");
  Serial.print(kWh, 5);
  Serial.println(" kWh");
  Serial.println("---------------------------");
 
  // Update LCD Display
  // Line 1: Voltage
  lcd.setCursor(0, 0);
  lcd.print("V:");
  lcd.print(emon.Vrms, 2);
  lcd.print("V   ");  // Clear extra characters
  
  // Line 2: Current
  lcd.setCursor(0, 1);
  lcd.print("I:");
  lcd.print(emon.Irms, 4);
  lcd.print("A   ");  // Clear extra characters
  
  // Line 3: Power
  lcd.setCursor(0, 2);
  lcd.print("P:");
  lcd.print(emon.apparentPower, 2);
  lcd.print("W   ");  // Clear extra characters
  
  // Line 4: Energy (kWh)
  lcd.setCursor(0, 3);
  lcd.print("E:");
  lcd.print(kWh, 5);
  lcd.print("kWh  ");  // Clear extra characters
 
  // Save the latest values to EEPROM
  saveEnergyDataToEEPROM();
 
  // Send data to Blynk
  Blynk.virtualWrite(V0, emon.Vrms);
  Blynk.virtualWrite(V1, emon.Irms);
  Blynk.virtualWrite(V2, emon.apparentPower);
  Blynk.virtualWrite(V3, kWh);
}
 
 
void readEnergyDataFromEEPROM()
{
  // Read the stored kWh value from EEPROM
  EEPROM.get(addrKWh, kWh);
 
  // Check if the read value is a valid float. If not, initialize it to zero
  if (isnan(kWh))
  {
    kWh = 0.0;
    saveEnergyDataToEEPROM(); // Save initialized value to EEPROM
  }
}
 
 
void saveEnergyDataToEEPROM()
{
  // Write the current kWh value to EEPROM
  EEPROM.put(addrKWh, kWh);
 
  // Commit changes to EEPROM
  EEPROM.commit();
}
