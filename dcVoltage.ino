#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Create an ADS1115 object
Adafruit_ADS1115 ads;

// Resistor values for voltage dividers
float R1 = 19910.0;  // Upper resistor value in ohms
float R2 = 2006.0;   // Lower resistor value in ohms

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
  // Array to store the input voltages from all three channels
  float inputVoltages[3];
  float currentMeasurement = 0.0;  // Variable to store current reading

  // Iterate over voltage channels (AIN0 to AIN2)
  for (int channel = 0; channel < 3; channel++) {
    int16_t adcReading;
    float Vout, inputVoltage;

    // Set default gain (GAIN_TWOTHIRDS) to measure up to ±6.144V
    ads.setGain(GAIN_TWOTHIRDS);  
    adcReading = ads.readADC_SingleEnded(channel);  // Read from the current channel
    Vout = (adcReading * 6.144) / 32767.0;  // Convert to voltage for ±6.144V range
    inputVoltage = Vout * ((R1 + R2) / R2);

    // If the measured voltage is less than 4V, switch to higher gain for better resolution
    if (inputVoltage < 4.0) {
      ads.setGain(GAIN_ONE);  // Switch to ±4.096V range
      adcReading = ads.readADC_SingleEnded(channel);
      Vout = (adcReading * 4.096) / 32767.0;  // Convert to voltage for ±4.096V range
      inputVoltage = Vout * ((R1 + R2) / R2);
    }

    // Store the calculated input voltage for this channel
    inputVoltages[channel] = inputVoltage;
  }

  // Read current from ACS712 on AIN3
  int16_t currentAdcReading = ads.readADC_SingleEnded(3);
  float currentVout = (currentAdcReading * 6.144) / 32767.0;  // Convert to voltage (assuming ±6.144V range)
  
  // Calculate the current based on ACS712 characteristics
  currentMeasurement = (currentVout - ACS712_Offset) / ACS712_Sensitivity;

  // Request temperature from DS18B20
  sensors.requestTemperatures();  
  float temperatureC = sensors.getTempCByIndex(0);  // Read temperature in Celsius

  // Print the measured voltages for all three channels
  for (int channel = 0; channel < 3; channel++) {
    Serial.print("Measured Voltage on Channel ");
    Serial.print(channel);
    Serial.print(": ");
    Serial.println(inputVoltages[channel]);
  }

  // Print the measured current
  Serial.print("Measured Current (ACS712): ");
  Serial.print(currentMeasurement);
  Serial.println(" A");

  // Print the temperature
  Serial.print("Measured Temperature (DS18B20): ");
  Serial.print(temperatureC);
  Serial.println(" °C");

  delay(1000);  // Wait for 1 second before taking the next set of readings
}
