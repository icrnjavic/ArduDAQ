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
float ACS712_Offset = 2.5;  // Voltage at 0A (approximately 2.5V for 5V supply)
float ACS712_Sensitivity = 0.185;  // Sensitivity in V/A (for 5A version)
float currentDeadband = 0.05;  // Small deadband to filter out noise

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

  // Calibrate ACS712 offset
  calibrateACS712Offset();
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
    } else if (command == "READ_CHANNEL_3") {
      Serial.print("Channel 3 Voltage: ");
      Serial.println(readChannelVoltage(3));
    } else if (command == "READ_CURRENT") {
      Serial.print("Averaged Current (ACS712): ");
      float averagedCurrent = readCurrentAverage();
      
      // Apply deadband to ignore small readings
      if (abs(averagedCurrent) < currentDeadband) {
        averagedCurrent = 0.0;
      }
      Serial.print(averagedCurrent);
      Serial.println(" A");
    } else if (command == "READ_TEMPERATURE") {
      Serial.print("Temperature (DS18B20): ");
      Serial.print(readTemperature());
      Serial.println(" °C");
    } 
  }

  delay(10);  // Short delay to allow for serial processing
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

// Function to take 20 burst measurements and average them for ACS712
float readCurrentAverage() {
  const int numMeasurements = 20;  // Number of measurements to take
  float totalCurrent = 0;

  for (int i = 0; i < numMeasurements; i++) {
    totalCurrent += readCurrent();
    delay(5);  // Small delay between measurements
  }

  return totalCurrent / numMeasurements;  // Return the average current
}

// Function to read current from ACS712 connected to A3 (using Arduino's built-in ADC)
float readCurrent() {
  int16_t currentAdcReading = analogRead(A3);  // Use Arduino's built-in ADC on A3
  float currentVout = (currentAdcReading * 5.0) / 1023.0;  // Convert ADC value to voltage (5V reference for Arduino)
  return (currentVout - ACS712_Offset) / ACS712_Sensitivity;
}

// Function to calibrate the ACS712 offset voltage
void calibrateACS712Offset() {
  // Measure the average voltage when no current is flowing
  float total = 0;
  const int numReadings = 100;  // Take multiple readings for stability

  for (int i = 0; i < numReadings; i++) {
    total += (analogRead(A3) * 5.0) / 1023.0;
    delay(5);  // Small delay between readings
  }

  ACS712_Offset = total / numReadings;  // Average voltage as the new offset
  Serial.print("Calibrated ACS712 Offset: ");
  Serial.println(ACS712_Offset);
}

// Function to read temperature from DS18B20
float readTemperature() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}
