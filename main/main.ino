//
// https://github.com/icrnjavic/ArduDAQ
// Arduino shield for data aquisiton of voltage, current and temperature measurements with available IO from teh ebase board.
//



#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>

Adafruit_ADS1115 ads;

// array of voltage resistor values for individual channels 1-4 from left to right
float R1[4] = {10000.0, 10000.0, 10000.0, 10000.0};
float R2[4] = {1000.0, 1000.0, 1000.0, 1000.0};

// ds18b20 Temperature Sensor Setup
#define ONE_WIRE_BUS 2  // DS18B20 to pin D2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// acs712-5A setup
float ACS712_Offset = 2.5;  
float ACS712_Sensitivity = 0.185;  // sensitivity for 5A version
float currentDeadband = 0.05;  // deadband noise filter

bool continuousMode = false;
unsigned long lastMeasurementTime = 0;
const unsigned long measurementInterval = 10;

#define SAMPLES_COUNT 64
#define SAMPLE_DELAY 0

// check if its a valid ads1115 channel(0-3)
bool isValidChannel(int channel) {
  return (channel >= 0 && channel < 4);
}

void setup() {
  Serial.begin(115200);
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS1115!");
    while (1);
  }
  //Serial.println("ADS1115 initialized.");
  //Serial.println("DS18B20 initialized.");
  sensors.begin();

  // set shield pins as outputs
  pinMode(0, OUTPUT); // wont work due to it being a serial pin -> usable for Tx/Rx usage
  pinMode(1, OUTPUT); // wont work due to it being a serial pin -> usable for Tx/Rx usage
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);

  // set all output pins to LOW by default
  digitalWrite(0, LOW);
  digitalWrite(1, LOW);
  digitalWrite(3, LOW);
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);

  // calibrate acs712 offset
  calibrateACS712Offset();

  ads.setGain(GAIN_ONE);    // ±4.096V
  
  // increase to maximum sample rate of 860 SPS (default is 128 SPS)
  ads.setDataRate(RATE_ADS1115_860SPS);
  
  ads.begin();
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    processCommand(command);
  }

  if (continuousMode && (millis() - lastMeasurementTime >= measurementInterval)) {
    sendContinuousMeasurements();
    lastMeasurementTime = millis();
  }
}

void processCommand(String command) {
  if (command.startsWith("OUTP")) {
    String pinStr = command.substring(9, 11); // Extract from index 9 to 11
    int pin = pinStr.toInt(); // Convert to integer
    Serial.println(command);
    Serial.println(pin);

    if (isValidPin(pin)) {
      if (command.endsWith("ON")) {
        digitalWrite(pin, HIGH); // set extracted pin to HIGH
        Serial.print("Pin D");
        Serial.print(pin);
        Serial.println(" set to HIGH");
      } else if (command.endsWith("OFF")) {
        digitalWrite(pin, LOW); // set the extracted pin to LOW
        Serial.print("Pin D");
        Serial.print(pin);
        Serial.println(" set to LOW");
      } else {
        Serial.println("Invalid command");
      }
    } else {
      Serial.println("Invalid pin");
    }
  } else if (command.equals("info")) {
    Serial.println("SET_<pin_number>_ON - Sets the specified pin to HIGH.");
    Serial.println("SET_<pin_number>_OFF - Sets the specified pin to LOW.");
    Serial.println("READ_CHANNEL_<channel_number> - Reads the voltage of the specified channel(1-4).");
    Serial.println("READ_TEMPERATURE - Reads the temperature from the onboard sensor.");
    Serial.println("START_CONTINUOUS - Starts continuous measurements.");
    Serial.println("STOP_CONTINUOUS - Stops continuous measurements.");
    Serial.println("READ_CURRENT - Reads the current from the single channel.");
  } else if (command.equals("START_CONTINUOUS")) {
    continuousMode = true;
    Serial.println("Continuous mode started");
  } else if (command.equals("STOP_CONTINUOUS")) {
    continuousMode = false;
    Serial.println("Continuous mode stopped");
  } else if (command.equals("MEAS:VOLT:CHAN1?")) {
    Serial.print("Channel 0 Voltage: ");
    Serial.println(readChannelVoltage(0), 4);
  } else if (command.equals("MEAS:VOLT:CHAN2?")) {
    Serial.print("Channel 1 Voltage: ");
    Serial.println(readChannelVoltage(1), 4);
  } else if (command.equals("MEAS:VOLT:CHAN3?")) {
    Serial.print("Channel 2 Voltage: ");
    Serial.println(readChannelVoltage(2), 4);
  } else if (command.equals("MEAS:VOLT:CHAN4?")) {
    Serial.print("Channel 3 Voltage: ");
    Serial.println(readChannelVoltage(3), 4);
  } else if (command.equals("MEAS:CURR?")) {
    float averagedCurrent = readCurrentAverage();
    Serial.println(averagedCurrent, 4);
  } else if (command.startsWith("READ_CHANNEL_")) {
    int channel = command.charAt(13) - '0';
    if (isValidChannel(channel)) {
      float voltage = readChannelVoltage(channel);
      Serial.print("Channel ");
      Serial.print(channel);
      Serial.print(": ");
      Serial.print(voltage, 4);
      Serial.println(" V");
    } else {
      Serial.println("Invalid channel");
    }
  } else if (command.equals("MEAS:TEMP?")) {
    float temperature = readTemperature();
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
  }
}

void sendContinuousMeasurements() {
  String response = "";
  for (int i = 0; i < 4; i++) {
    response += "CH_";
    response += String(i);
    response += ": ";
    response += String(readChannelVoltage(i), 4);
    response += "V, ";
  }
  response.trim();
  Serial.println(response);
}

// ads1115 measurement of individual channels
float readChannelVoltage(int channel) {
  int16_t adcReading;
  float Vout, inputVoltage;

  ads.setGain(GAIN_TWOTHIRDS);
  adcReading = ads.readADC_SingleEnded(channel);
  Vout = (adcReading * 6.144) / 32767.0;
  inputVoltage = Vout * ((R1[channel] + R2[channel]) / R2[channel]);

  if (inputVoltage < 4.0) {
    ads.setGain(GAIN_ONE);
    adcReading = ads.readADC_SingleEnded(channel);
    Vout = (adcReading * 4.096) / 32767.0;
    inputVoltage = Vout * ((R1[channel] + R2[channel]) / R2[channel]);
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
  float currentVout = (currentAdcReading * 5.0) / 1023.0;  // TO DO: for V2 use a second ads1115 instead of internal adc for better resolution
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
  //Serial.print("Calibrated ACS712 Offset: ");
  //Serial.println(ACS712_Offset);
}

// temperature reding
float readTemperature() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

// check if the selected output pin is one of the valid shield pins
bool isValidPin(int pin) {
  return (pin == 0 || pin == 1 || pin == 3 || pin == 4 || pin == 5 || pin == 6 || pin == 7);
}
