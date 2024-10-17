//
// https://github.com/icrnjavic/ArduDAQ
// Arduino shield for data aquisiton of voltage, current and temperature measurements with available IO from teh ebase board.
//



#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>

Adafruit_ADS1115 ads;

// voltage divider resistor values.
// with 0.1% resistors its fine this way but when using resistors above 1% it would be better using measured resistor values for each channel to achive best accuracy
float R1 = 10000.0;
float R2 = 1000.0;

// ds18b20 Temperature Sensor Setup
#define ONE_WIRE_BUS 2  // DS18B20 to pin D2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// acs712-5A setup
float ACS712_Offset = 2.5;  
float ACS712_Sensitivity = 0.185;  // sensitivity for 5A version
float currentDeadband = 0.05;  // deadband noise filter

void setup() {
  Serial.begin(115200);
  // initialize modules
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS1115!");
    while (1);
  }
  Serial.println("ADS1115 initialized.");
  sensors.begin();
  Serial.println("DS18B20 initialized.");

  // calibrate acs712 offset
  calibrateACS712Offset();
}

void loop() {
  // read commands via serial and return the measurement
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
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
      
      // deadband filter (WIP)
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

  delay(10);  // serial processing
}

// ADS1115 measurement of individual channels
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

// acs712 - sample current draw and return the average
float readCurrentAverage() {
  const int numMeasurements = 20;  // number of samples
  float totalCurrent = 0;

  for (int i = 0; i < numMeasurements; i++) {
    totalCurrent += readCurrent();
    delay(5);
  }

  return totalCurrent / numMeasurements;
}

// read current from acs712 via A3 pin
float readCurrent() {
  int16_t currentAdcReading = analogRead(A3); 
  float currentVout = (currentAdcReading * 5.0) / 1023.0;  // TO DO: for V2 use ads1115 instead of internal adc for better resolution
  return (currentVout - ACS712_Offset) / ACS712_Sensitivity;
}


void calibrateACS712Offset() {
  // measure average voltage with no current draw
  float total = 0;
  const int numReadings = 100;  // number of samples

  for (int i = 0; i < numReadings; i++) {
    total += (analogRead(A3) * 5.0) / 1023.0;
    delay(5);
  }

  ACS712_Offset = total / numReadings;  // calculate new offset
  Serial.print("Calibrated ACS712 Offset: ");
  Serial.println(ACS712_Offset);
}

// temperature reding
float readTemperature() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}
