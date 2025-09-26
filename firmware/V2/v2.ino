//
// https://github.com/icrnjavic/ArduDAQ
// Arduino shield for data acquisition of voltage, current and temperature.
// Version 2.0 - Dual ADS1115 support and an extra current measurement input via shunt (8 channels total; 2 inputs will be used for current measurement via shunt)
// Version 2.1.2 - All 8 ads channels usable as analog inputs with 2 being configurable to use for current measurements via onboard shunt
//


const String FIRMWARE_VERSION = "2.1.2";

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>

Adafruit_ADS1115 ads1;  // 0x48
Adafruit_ADS1115 ads2;  // 0x49

// divider values per channel (1–8)
float R1[8] = {10000,10000,10000,10000,10000,10000,10000,10000};
float R2[8] = {1000,1000,1000,1000,1000,1000,1000,1000};

// DS18B20
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ACS712-5A (internal ADC path)
float ACS712_Offset = 2.5;
float ACS712_Sensitivity = 0.185;  // V/A
float currentDeadband = 0.05;      // V (ACS path only)

// --- Shunt setup (default 10 mΩ) ---
// makes it configurable at runtime:
float SHUNT_RESISTANCE = 0.01f;          // Ω
const float ADS1115_LSB_V_GAIN16 = 0.0000078125f; // 7.8125 µV/LSB
float shuntZeroOffset_V = 0.0f;          // V after zeroing
float shuntDeadband_mA  = 0.8f;          // ignore tiny jitter
float shuntScale        = 1.0f;          // multiplicative scale correction
bool  shuntInvert       = false;         // invert sign if wiring flipped

bool continuousMode = false;
unsigned long lastMeasurementTime = 0;
const unsigned long measurementInterval = 10;

bool isValidChannel(int channel) { return channel >= 0 && channel < 8; }
// only allow D3–D9 as digital outputs (avoid D0/D1 serial pins!)
bool isValidPin(int pin) { return (pin == 3 || pin == 4 || pin == 5 || pin == 6 || pin == 7 || pin == 8 || pin == 9); }

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("ArduDAQ v2.1.2 – ready @115200"));

  if (!ads1.begin(0x48)) { Serial.println(F("Failed to init ADS1115 #1 (0x48)!")); while (1) {} }
  if (!ads2.begin(0x49)) { Serial.println(F("Failed to init ADS1115 #2 (0x49)!")); while (1) {} }

  sensors.begin();

  // Configure output pins (D3–D9 only)
  pinMode(3, OUTPUT); pinMode(4, OUTPUT); pinMode(5, OUTPUT); pinMode(6, OUTPUT); pinMode(7, OUTPUT); pinMode(8, OUTPUT); pinMode(9, OUTPUT);
  digitalWrite(3, LOW); digitalWrite(4, LOW); digitalWrite(5, LOW); digitalWrite(6, LOW); digitalWrite(7, LOW); digitalWrite(8, LOW); digitalWrite(9, LOW);

  calibrateACS712Offset();

  // Default gains/rates for voltage channels
  ads1.setGain(GAIN_ONE);  // ±4.096 V
  ads2.setGain(GAIN_ONE);  // ±4.096 V
  ads1.setDataRate(RATE_ADS1115_860SPS);
  ads2.setDataRate(RATE_ADS1115_860SPS);

  calibrateShuntOffset();

  Serial.println(F("Init done. Type 'info' for commands."));
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

static inline String upcase(const String &s) {
  String t = s; t.toUpperCase(); return t;
}

void processCommand(String command) {
  command.trim();
  String U = upcase(command);

  if (U.startsWith("OUTP")) {
    int pIdx = U.indexOf("PIN=");
    if (pIdx >= 0 && pIdx + 6 <= (int)U.length()) {
      int pin = U.substring(pIdx + 4, pIdx + 6).toInt();
      if (!isValidPin(pin)) { Serial.println(F("Invalid pin (use 3,4,5,6,7)")); return; }
      if (U.endsWith("ON"))  { digitalWrite(pin, HIGH); Serial.print(F("Pin D")); Serial.print(pin); Serial.println(F(" HIGH")); }
      else if (U.endsWith("OFF")) { digitalWrite(pin, LOW);  Serial.print(F("Pin D")); Serial.print(pin); Serial.println(F(" LOW")); }
      else { Serial.println(F("Invalid OUTP command")); }
    } else {
      Serial.println(F("Format: OUTP:PIN=03:ON|OFF"));
    }

  } else if (U == "INFO") {
    Serial.println(F("Commands:"));
    Serial.println(F("  OUTP:PIN=03:ON|OFF"));
    Serial.println(F("  MEAS:VOLT:CHAN1? .. :CHAN8?"));
    Serial.println(F("  MEAS:CURR?                 (ACS712, A)"));
    Serial.println(F("  MEAS:CURR:SHUNT?           (mA numeric)"));
    Serial.println(F("  MEAS:CURR:SHUNT:TXT?       (mA with units)"));
    Serial.println(F("  MEAS:CURR:SHUNT:RAW?       (debug raw, µV)"));
    Serial.println(F("  MEAS:CURR:SHUNT:ZERO       (re-zero shunt)"));
    Serial.println(F("  CONF:SHUNT:R=<ohms>        (e.g. 0.01)"));
    Serial.println(F("  CONF:SHUNT:SCALE=<factor>  (e.g. 1.000)"));
    Serial.println(F("  CONF:SHUNT:INV=ON|OFF      (invert sign)"));
    Serial.println(F("  MEAS:TEMP?"));
    Serial.println(F("  START_CONTINUOUS / STOP_CONTINUOUS"));
    Serial.println(F("  VERSION"));

  } else if (U == "VERSION") {
    Serial.print(F("Firmware Version: ")); Serial.println(FIRMWARE_VERSION);

  } else if (U == "START_CONTINUOUS") {
    continuousMode = true;  Serial.println(F("Continuous mode started"));

  } else if (U == "STOP_CONTINUOUS") {
    continuousMode = false; Serial.println(F("Continuous mode stopped"));

  } else if (U == "MEAS:VOLT:CHAN1?") { Serial.println(readChannelVoltage(0), 4);
  } else if (U == "MEAS:VOLT:CHAN2?") { Serial.println(readChannelVoltage(1), 4);
  } else if (U == "MEAS:VOLT:CHAN3?") { Serial.println(readChannelVoltage(2), 4);
  } else if (U == "MEAS:VOLT:CHAN4?") { Serial.println(readChannelVoltage(3), 4);
  } else if (U == "MEAS:VOLT:CHAN5?") { Serial.println(readChannelVoltage(4), 4);
  } else if (U == "MEAS:VOLT:CHAN6?") { Serial.println(readChannelVoltage(5), 4);
  } else if (U == "MEAS:VOLT:CHAN7?") { Serial.println(readChannelVoltage(6), 4);
  } else if (U == "MEAS:VOLT:CHAN8?") { Serial.println(readChannelVoltage(7), 4);

  } else if (U == "MEAS:CURR?") {
    Serial.println(readCurrentAverage(), 4); // Amps (ACS712)

  } else if (U == "MEAS:CURR:SHUNT?") {
    Serial.println(readShuntCurrentAverage_mA(), 3); // mA (shunt)
    /*──────────────────────────────────────────────────────────────────────────────
      SHUNT CURRENT MEASUREMENT – QUICK GUIDE (ADS1115, 10 mΩ default)

      Overview
      - The shunt is read DIFFERENTIALLY (ADS #2, A2–A3) at GAIN_SIXTEEN (±0.256 V).
      - Resolution at this gain is 7.8125 µV/LSB.
      - With a 10 mΩ shunt: 1 mA → 10 µV, so ≈0.781 mA/LSB (7.8125 µV / 10 µV).
        After averaging 32 samples, effective noise is ~0.14–0.2 mA.

      Default Settings (can be changed via serial commands below)
      - SHUNT_RESISTANCE = 0.01 Ω  (10 mΩ)
      - shuntScale       = 1.0      (unitless fine calibration)
      - shuntInvert      = OFF      (polarity normal)
      - shuntZeroOffset  = measured at startup (do MEAS:CURR:SHUNT:ZERO if needed)
      - Deadband         = 0.8 mA   (suppress tiny jitter around 0)

      Commands (type in Serial Monitor @115200 baud)
      - MEAS:CURR:SHUNT?         → numeric current in mA (no units; machine-friendly)
      - MEAS:CURR:SHUNT:TXT?     → current in mA with units (e.g., "59.982 mA")
      - MEAS:CURR:SHUNT:RAW?     → debug: raw counts + microvolts (before/after zero)
      - MEAS:CURR:SHUNT:ZERO     → re-measure & store zero-offset (no load connected)
      - CONF:SHUNT:R=<ohms>      → set shunt value (e.g., 0.01, 0.1, 1.0)
      - CONF:SHUNT:SCALE=<k>     → fine scale factor (e.g., 1.012 for +1.2%)
      - CONF:SHUNT:INV=ON|OFF    → invert sign if wiring is flipped

      One-Minute Calibration Procedure (recommended)
      1) Warm up for 30–60 s. Disconnect load. Send: MEAS:CURR:SHUNT:ZERO
      2) Apply a known current (e.g., 60.00 mA measured on a DMM).
      3) Read MEAS:CURR:SHUNT?  (e.g., returns 58.0)
      4) Set fine scale: CONF:SHUNT:SCALE= (known_mA / measured_mA)
        Example: 60.00 / 58.0 = 1.03448 → CONF:SHUNT:SCALE=1.03448
      5) (Optional) If sign is negative, send: CONF:SHUNT:INV=ON

      When to change R vs SCALE
      - R (ohms) = the physics. Use this if the reading is off by 2×, 10×, 100×
        (e.g., board has 0.1 Ω instead of 0.01 Ω). Command: CONF:SHUNT:R=0.1
      - SCALE (unitless) = small trim (±0–5%) to correct tolerance/gain/layout error
        after R is correct. Command: CONF:SHUNT:SCALE=1.02 (for +2%)

      Using RAW for sanity checks
      - MEAS:CURR:SHUNT:RAW? prints: raw counts, v_raw (µV), v_zero (µV after zero).
      - For a 10 mΩ shunt, each 1 mA corresponds to ~10 µV across the shunt.
        So at ~60 mA you expect ~600 µV (≈ 600 / 7.8125 ≈ 77 counts).
      - If v_zero matches expectation but current looks 10× off, fix R (not SCALE).

      Expected Numbers (10 mΩ reference)
      - 10 mA  → ~100 µV across shunt → ~13 counts → ~10 mA reported
      - 60 mA  → ~600 µV across shunt → ~77 counts → ~60 mA reported
      - 100 mA → ~1,000 µV (~1.0 mV)   → ~128 counts → ~100 mA reported

      Stability Tips
      - Re-zero after thermal changes: MEAS:CURR:SHUNT:ZERO (no load).
      - If you ever see noise spikes at very low currents, increase averaging
        (code uses 32 samples) or increase the deadband slightly.
      - Keep shunt wiring as Kelvin (4-wire) to the ADC inputs for best accuracy.

      Output/Units
      - MEAS:CURR:SHUNT? returns numeric mA (no units) for easy parsing.
      - MEAS:CURR:SHUNT:TXT? returns a human-friendly string with " mA".
    ──────────────────────────────────────────────────────────────────────────────*/


  } else if (U == "MEAS:CURR:SHUNT:TXT?") {
    float mA = readShuntCurrentAverage_mA();
    Serial.print(mA, 3); Serial.println(F(" mA"));

  } else if (U == "MEAS:CURR:SHUNT:RAW?") {
    // One-shot raw debug: counts & microvolts (before/after zero)
    ads2.setGain(GAIN_SIXTEEN);
    int16_t raw = ads2.readADC_Differential_2_3();
    float v_raw = raw * ADS1115_LSB_V_GAIN16;        // V
    float v_zeroed = v_raw - shuntZeroOffset_V;      // V
    Serial.print(F("raw=")); Serial.print(raw);
    Serial.print(F(", v_raw="));   Serial.print(v_raw * 1e6, 1);   Serial.print(F(" uV"));
    Serial.print(F(", v_zero="));  Serial.print(v_zeroed * 1e6, 1); Serial.println(F(" uV"));

  } else if (U == "MEAS:CURR:SHUNT:ZERO") {
    calibrateShuntOffset();
    Serial.print(F("Shunt zero (V): ")); Serial.println(shuntZeroOffset_V, 6);

  } else if (U.startsWith("CONF:SHUNT:R=")) {
    String v = command.substring(13); v.trim();
    float newR = v.toFloat();
    if (newR > 0.0f && newR < 100.0f) { SHUNT_RESISTANCE = newR; Serial.print(F("SHUNT R=")); Serial.println(SHUNT_RESISTANCE, 5); }
    else { Serial.println(F("Invalid R (ohms)")); }

  } else if (U.startsWith("CONF:SHUNT:SCALE=")) {
    String v = command.substring(17); v.trim();
    float s = v.toFloat();
    if (s > 0.01f && s < 100.0f) { shuntScale = s; Serial.print(F("SHUNT SCALE=")); Serial.println(shuntScale, 6); }
    else { Serial.println(F("Invalid SCALE")); }

  } else if (U.startsWith("CONF:SHUNT:INV=")) {
    if (U.endsWith("ON"))  { shuntInvert = true;  Serial.println(F("SHUNT INV=ON")); }
    else if (U.endsWith("OFF")) { shuntInvert = false; Serial.println(F("SHUNT INV=OFF")); }
    else { Serial.println(F("Use CONF:SHUNT:INV=ON|OFF")); }

  } else if (U == "MEAS:TEMP?") {
    Serial.println(readTemperature(), 2);

  } else if (U.startsWith("READ_CHANNEL_")) {
    int channel = command.substring(13).toInt();
    if (isValidChannel(channel)) { Serial.println(readChannelVoltage(channel), 4); }
    else { Serial.println(F("Invalid channel")); }

  } else {
    Serial.println(F("Unknown command. Type 'info'."));
  }
}

void sendContinuousMeasurements() {
  String response = "";
  for (int i = 0; i < 8; i++) {
    if (i) response += ", ";
    response += "CH_";
    response += i;
    response += "=";
    response += String(readChannelVoltage(i), 4);
    response += "V";
  }
  Serial.println(response);
}

// -------- voltage channels (external ADC inputs) --------
float readChannelVoltage(int channel) {
  int16_t adcReading;
  float Vout, inputVoltage;
  Adafruit_ADS1115* ads;
  int adsChannel;

  if (channel < 4) { ads = &ads1; adsChannel = channel; }
  else             { ads = &ads2; adsChannel = channel - 4; }

  ads->setGain(GAIN_TWOTHIRDS);
  adcReading = ads->readADC_SingleEnded(adsChannel);
  Vout = (adcReading * 6.144f) / 32767.0f;
  inputVoltage = Vout * ((R1[channel] + R2[channel]) / R2[channel]);

  if (inputVoltage < 4.0f) {
    ads->setGain(GAIN_ONE);
    adcReading = ads->readADC_SingleEnded(adsChannel);
    Vout = (adcReading * 4.096f) / 32767.0f;
    inputVoltage = Vout * ((R1[channel] + R2[channel]) / R2[channel]);
  }
  return inputVoltage;
}

// -------- ACS712 (suitable  mostly for rough 1-2A loads) --------
float readCurrentAverage() {
  const int numMeasurements = 20;
  float totalCurrent = 0;
  for (int i = 0; i < numMeasurements; i++) {
    totalCurrent += readCurrent();
    delay(5);
  }
  return totalCurrent / numMeasurements; // A
}

float readCurrent() {
  int16_t currentAdcReading = analogRead(A3);
  float currentVout = (currentAdcReading * 5.0f) / 1023.0f;
  return (currentVout - ACS712_Offset) / ACS712_Sensitivity; // A
}

void calibrateACS712Offset() {
  float total = 0;
  const int numReadings = 100;
  for (int i = 0; i < numReadings; i++) {
    total += (analogRead(A3) * 5.0f) / 1023.0f;
    delay(5);
  }
  ACS712_Offset = total / numReadings;
}

// -------- shunt via ADS1115 differential @ high gain --------
float readShuntDiff_V_once() {
  ads2.setGain(GAIN_SIXTEEN); // ±0.256 V
  int16_t raw = ads2.readADC_Differential_2_3(); // A2 - A3
  float v = raw * ADS1115_LSB_V_GAIN16; // V
  return (v - shuntZeroOffset_V);
}

float readShuntCurrent_mA_once() {
  float v = readShuntDiff_V_once();
  float mA = (v / SHUNT_RESISTANCE) * 1000.0f;
  if (shuntInvert) mA = -mA;
  mA *= shuntScale;
  if (fabs(mA) < shuntDeadband_mA) mA = 0.0f;
  return mA;
}

float readShuntCurrentAverage_mA() {
  const int N = 32;
  float sum = 0;
  for (int i = 0; i < N; i++) {
    sum += readShuntCurrent_mA_once();
    delay(3);
  }
  return sum / N;
}

void calibrateShuntOffset() {
  const int N = 64;
  float acc = 0;
  ads2.setGain(GAIN_SIXTEEN);
  for (int i = 0; i < N; i++) {
    int16_t raw = ads2.readADC_Differential_2_3();
    acc += raw * ADS1115_LSB_V_GAIN16;
    delay(3);
  }
  shuntZeroOffset_V = acc / N;
}

// -------- Temperature --------
float readTemperature() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}
