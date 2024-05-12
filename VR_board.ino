#include <HX711_ADC.h>
#include <Joystick.h>

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_GAMEPAD,
  1, 0,                  // Button Count, Hat Switch Count
  true, true, false,     // X and Y, but no Z Axis
  false, false, false,   // No Rx, Ry, or Rz
  false, false,          // No rudder or throttle
  false, false, false);  // No accelerator, brake, or steering

#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

// Pins for the load cells

const int PIN_R4 = 5;
const int PIN_R3 = 6;
const int PIN_R2 = 7;
const int PIN_R1 = 8;

const int PIN_B4 = 3;
const int PIN_B3 = 10;
const int PIN_B2 = 11;
const int PIN_B1 = 12;

const int PIN_SCK = 13; 

// First HX711 instance
HX711_ADC R4_LoadCell(PIN_R4, PIN_SCK);
HX711_ADC R3_LoadCell(PIN_R3, PIN_SCK);
HX711_ADC R2_LoadCell(PIN_R2, PIN_SCK);
HX711_ADC R1_LoadCell(PIN_R1, PIN_SCK);

HX711_ADC B4_LoadCell(PIN_B4, PIN_SCK);
HX711_ADC B3_LoadCell(PIN_B3, PIN_SCK);
HX711_ADC B2_LoadCell(PIN_B2, PIN_SCK);
HX711_ADC B1_LoadCell(PIN_B1, PIN_SCK);

const int calVal_eepromAdress = 0;
const int tareOffsetVal_eepromAdress = 4;
unsigned long t = 0;

float mapValue(float value, float minInput, float maxInput, float minOutput, float maxOutput) {
    return (value - minInput) * (maxOutput - minOutput) / (maxInput - minInput) + minOutput;
}

void setup() {
  Serial.begin(57600);
  delay(10);
  Serial.println();
  Serial.println("Starting...");

  R4_LoadCell.begin();
  R3_LoadCell.begin();
  R2_LoadCell.begin();
  R1_LoadCell.begin();

  B4_LoadCell.begin();
  B3_LoadCell.begin();
  B2_LoadCell.begin();
  B1_LoadCell.begin();

  float calibrationValue = 48400;

#if defined(ESP8266)|| defined(ESP32)
  EEPROM.begin(512);
#endif

  long tare_offset = 0;

  EEPROM.get(tareOffsetVal_eepromAdress, tare_offset);
  R4_LoadCell.setTareOffset(tare_offset);

  EEPROM.get(tareOffsetVal_eepromAdress + sizeof(long), tare_offset);
  R3_LoadCell.setTareOffset(tare_offset);

  EEPROM.get(tareOffsetVal_eepromAdress + sizeof(long), tare_offset);
  R2_LoadCell.setTareOffset(tare_offset);

  EEPROM.get(tareOffsetVal_eepromAdress + sizeof(long), tare_offset);
  R1_LoadCell.setTareOffset(tare_offset);


  EEPROM.get(tareOffsetVal_eepromAdress + sizeof(long), tare_offset);
  R1_LoadCell.setTareOffset(tare_offset);

  EEPROM.get(tareOffsetVal_eepromAdress + sizeof(long), tare_offset);
  B3_LoadCell.setTareOffset(tare_offset);

  EEPROM.get(tareOffsetVal_eepromAdress + sizeof(long), tare_offset);
  B2_LoadCell.setTareOffset(tare_offset);

  EEPROM.get(tareOffsetVal_eepromAdress + sizeof(long), tare_offset);
  B1_LoadCell.setTareOffset(tare_offset);

  boolean _tare = false; // set this to false as the value has been restored from EEPROM

  unsigned long stabilizingtime = 2000;
  R4_LoadCell.start(stabilizingtime, _tare);
  R3_LoadCell.start(stabilizingtime, _tare);
  R2_LoadCell.start(stabilizingtime, _tare);
  R1_LoadCell.start(stabilizingtime, _tare);

  B4_LoadCell.start(stabilizingtime, _tare);
  B3_LoadCell.start(stabilizingtime, _tare);
  B2_LoadCell.start(stabilizingtime, _tare);
  B1_LoadCell.start(stabilizingtime, _tare);

  if (R4_LoadCell.getTareTimeoutFlag() || R3_LoadCell.getTareTimeoutFlag() || R2_LoadCell.getTareTimeoutFlag() || R1_LoadCell.getTareTimeoutFlag() /*|| B4_LoadCell.getTareTimeoutFlag()*/ || B3_LoadCell.getTareTimeoutFlag() || B2_LoadCell.getTareTimeoutFlag() || B1_LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    R4_LoadCell.setCalFactor(calibrationValue);
    R3_LoadCell.setCalFactor(calibrationValue);
    R2_LoadCell.setCalFactor(calibrationValue);
    R1_LoadCell.setCalFactor(calibrationValue);

    B4_LoadCell.setCalFactor(calibrationValue);
    B3_LoadCell.setCalFactor(calibrationValue);
    B2_LoadCell.setCalFactor(calibrationValue);
    B1_LoadCell.setCalFactor(calibrationValue);

    Serial.println("Startup is complete");
  }

  // Initialize Joystick Library
  Joystick.begin();
  Joystick.setXAxisRange(-100, 100);
  Joystick.setYAxisRange(-100, 100);

  refreshOffsetValuesAndSaveToEEPROM();
}

void loop() {
  // Flags to track data readiness for each load cell
  static boolean newDataReady_R4 = false;
  static boolean newDataReady_R3 = false;
  static boolean newDataReady_R2 = false;
  static boolean newDataReady_R1 = false;

  static boolean newDataReady_B4 = false;
  static boolean newDataReady_B3 = false;
  static boolean newDataReady_B2 = false;
  static boolean newDataReady_B1 = false;

  // Interval for serial print (milliseconds)
  const int serialPrintInterval = 0;

  // Update flags for load cells
  newDataReady_R4 |= R4_LoadCell.update();
  newDataReady_R3 |= R3_LoadCell.update();
  newDataReady_R2 |= R2_LoadCell.update();
  newDataReady_R1 |= R1_LoadCell.update();

  newDataReady_B4 |= B4_LoadCell.update();
  newDataReady_B3 |= B3_LoadCell.update();
  newDataReady_B2 |= B2_LoadCell.update();
  newDataReady_B1 |= B1_LoadCell.update();

  // If all load cells have new data
  if (newDataReady_R4 && newDataReady_R3 && newDataReady_R2 && newDataReady_R1 &&
      newDataReady_B4 && newDataReady_B3 && newDataReady_B2 && newDataReady_B1) {
    // If enough time has passed for serial print interval
    if (millis() > t + serialPrintInterval) {
      // Read current values of load cells
      float i_R4 = abs(R4_LoadCell.getData());
      float i_R3 = abs(R3_LoadCell.getData());
      float i_R2 = abs(R2_LoadCell.getData());
      float i_R1 = abs(R1_LoadCell.getData());

      float i_B4 = abs(B4_LoadCell.getData());
      float i_B3 = abs(B3_LoadCell.getData());
      float i_B2 = abs(B2_LoadCell.getData());
      float i_B1 = abs(B1_LoadCell.getData());

      // Print load cell values
      // Serial.print("Load_cell R4 output val: ");
      // Serial.println(i_R4);
      // Serial.print("Load_cell R3 output val: ");
      // Serial.println(i_R3);
      // Serial.print("Load_cell R2 output val: ");
      // Serial.println(i_R2);
      // Serial.print("Load_cell R1 output val: ");
      // Serial.println(i_R1);

      // Serial.print("Load_cell B4 output val: ");
      // Serial.println(i_B4);
      // Serial.print("Load_cell B3 output val: ");
      // Serial.println(i_B3);
      // Serial.print("Load_cell B2 output val: ");
      // Serial.println(i_B2);
      // Serial.print("Load_cell B1 output val: ");
      // Serial.println(i_B1);

      // Reset data readiness flags
      newDataReady_R4 = false;
      newDataReady_R3 = false;
      newDataReady_R2 = false;
      newDataReady_R1 = false;

      newDataReady_B4 = false;
      newDataReady_B3 = false;
      newDataReady_B2 = false;
      newDataReady_B1 = false;

      // Update time
      t = millis();

      // Calculate joystick axes
      float X = ((i_B4 + (i_R1/2) + (i_B3/2)) - (i_R4 + (i_R3/2) + (i_B1/2)))*30;
      float Y = ((i_R2 + (i_R1/2) + (i_R3/2)) - (i_B2 + (i_B1/2) + (i_B3/2)))*30;
      
      if (X < -100 || X > 100 || Y < -100 || Y > 100) Joystick.setButton(0, true);
      else Joystick.setButton(0, false);
      
      // Print of the coordinates
      Serial.print("X coordinate: ");
      Serial.println(X);
      Serial.print("Y coordinate: ");
      Serial.println(Y);

      // Set joystick axes
      Joystick.setXAxis(X);
      Joystick.setYAxis(Y);
    }
  }

  // Check for serial input
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') refreshOffsetValuesAndSaveToEEPROM();
  }
}





void refreshOffsetValuesAndSaveToEEPROM() {
  long _offset_R4 = 0;
  long _offset_R3 = 0;
  long _offset_R2 = 0;
  long _offset_R1 = 0;

  long _offset_B4 = 0;
  long _offset_B3 = 0;
  long _offset_B2 = 0;
  long _offset_B1 = 0;

  Serial.println("Calculating tare offset values...");

  R4_LoadCell.tare();
  _offset_R4 = R4_LoadCell.getTareOffset();

  R3_LoadCell.tare();
  _offset_R3 = R3_LoadCell.getTareOffset();

  R2_LoadCell.tare();
  _offset_R2 = R2_LoadCell.getTareOffset();

  R1_LoadCell.tare();
  _offset_R1 = R1_LoadCell.getTareOffset();


  B4_LoadCell.tare();
  _offset_B4 = B4_LoadCell.getTareOffset();

  B3_LoadCell.tare();
  _offset_B3 = B3_LoadCell.getTareOffset();

  B2_LoadCell.tare();
  _offset_B2 = B2_LoadCell.getTareOffset();

  B1_LoadCell.tare();
  _offset_B1 = B1_LoadCell.getTareOffset();

  

  EEPROM.put(tareOffsetVal_eepromAdress, _offset_R4);
  EEPROM.put(tareOffsetVal_eepromAdress + sizeof(long), _offset_R3);
  EEPROM.put(tareOffsetVal_eepromAdress + sizeof(long), _offset_R2);
  EEPROM.put(tareOffsetVal_eepromAdress + sizeof(long), _offset_R1);

  EEPROM.put(tareOffsetVal_eepromAdress + sizeof(long), _offset_B4);
  EEPROM.put(tareOffsetVal_eepromAdress + sizeof(long), _offset_B3);
  EEPROM.put(tareOffsetVal_eepromAdress + sizeof(long), _offset_B2);
  EEPROM.put(tareOffsetVal_eepromAdress + sizeof(long), _offset_B1);



#if defined(ESP8266) || defined(ESP32)
  EEPROM.commit();
#endif

  R4_LoadCell.setTareOffset(_offset_R4);
  R3_LoadCell.setTareOffset(_offset_R3);
  R2_LoadCell.setTareOffset(_offset_R2);
  R1_LoadCell.setTareOffset(_offset_R1);

  Serial.print("New tare offset values: ");

  Serial.print(_offset_R4);
  Serial.print(", ");
  Serial.print(_offset_R3);
  Serial.print(", ");
  Serial.print(_offset_R3);
  Serial.print(", ");
  Serial.print(_offset_R2);
  Serial.print(", ");
  Serial.print(_offset_R1);
  Serial.print(", ");

  Serial.print(_offset_B4);
  Serial.print(", ");
  Serial.print(_offset_B3);
  Serial.print(", ");
  Serial.print(_offset_B2);
  Serial.print(", ");
  Serial.print(_offset_B1);

  Serial.println(", saved to EEPROM");
}
