#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Create an ADS1115 object
Adafruit_ADS1115 ads;

// Resistor values for voltage dividers
float R1 = 10000.0;  // Upper resistor value in ohms
float R2 = 1000.0;   // Lower resistor value in ohms

// DS18B20 Temperature Sensor Setup
#define ONE_WIRE_BUS 2  // DS18B20 data pin connected to D2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ACS712-5A constants
float ACS712_Offset = 2.5;  // Voltage at 0A (approximately 2.5V)
float ACS712_Sensitivity = 0.185;  // Sensitivity in V/A (for 5A version)

void setup() {
  Serial.begin(115200);
  
  // Initialize ADS1115
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS1115!");
    while (1);
  }
  Serial.println("ADS1115 initialized.");
  
  // Initialize DS18B20
  sensors.begin();
  Serial.println("DS18B20 initialized.");
}

void loop() {
  // Check if data is available on the serial port
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');  // Read the incoming command

    if (command == "READ_CHANNEL_0") {
      Serial.print("Channel 0 Voltage: ");
      Serial.println(readChannelVoltage(0));
    } else if (command == "READ_CHANNEL_1") {
      Serial.print("Channel 1 Voltage: ");
      Serial.println(readChannelVoltage(1));
    } else if (command == "READ_CHANNEL_2") {
      Serial.print("Channel 2 Voltage: ");
      Serial.println(readChannelVoltage(2));
    } else if (command == "READ_CURRENT") {
      Serial.print("Current (ACS712): ");
      Serial.print(readCurrent());
      Serial.println(" A");
    } else if (command == "READ_TEMPERATURE") {
      Serial.print("Temperature (DS18B20): ");
      Serial.print(readTemperature());
      Serial.println(" °C");
    } else {
      Serial.println("Unknown command");
    }
  }

  delay(100);  // Short delay to allow for serial processing
}

// Function to read voltage from a specific ADS1115 channel
float readChannelVoltage(int channel) {
  int16_t adcReading;
  float Vout, inputVoltage;

  ads.setGain(GAIN_TWOTHIRDS);
  adcReading = ads.readADC_SingleEnded(channel);
  Vout = (adcReading * 6.144) / 32767.0;
  inputVoltage = Vout * ((R1 + R2) / R2);

  if (inputVoltage < 4.0) {
    ads.setGain(GAIN_ONE);
    adcReading = ads.readADC_SingleEnded(channel);
    Vout = (adcReading * 4.096) / 32767.0;
    inputVoltage = Vout * ((R1 + R2) / R2);
  }

  return inputVoltage;
}

// Function to read current from ACS712
float readCurrent() {
  int16_t currentAdcReading = ads.readADC_SingleEnded(3);
  float currentVout = (currentAdcReading * 6.144) / 32767.0;
  return (currentVout - ACS712_Offset) / ACS712_Sensitivity;
}

// Function to read temperature from DS18B20
float readTemperature() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}