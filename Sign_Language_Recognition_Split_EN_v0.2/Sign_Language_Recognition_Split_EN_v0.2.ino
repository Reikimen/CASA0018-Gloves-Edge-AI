/*
 * Sign Language Glove - Gesture Recognition Program with LCD Support
 * 
 * Features:
 * 1. Reads flex sensor data using calibrated ADC values
 * 2. Calculates statistical features (average, min, max, RMS, stddev, skew, kurtosis)
 * 3. Uses Edge Impulse exported model to recognize digital and special gestures
 * 4. Displays recognition results on serial monitor and LCD2004 display
 * 
 * Usage:
 * 1. Export Arduino library from Edge Impulse and add it to the project
 * 2. Ensure Sign-Language-Glove_inferencing.h is included in the project
 * 3. Place all header files (.h) and main program in the same directory
 * 4. Upload code to Arduino Nano 33 BLE
 * 5. Connect LCD2004 I2C display (default address 0x27)
 * 6. Open serial monitor (115200 baud rate) to view recognition results
 */

#include <Arduino_LSM9DS1.h>
#include <Sign-Language-Glove_inferencing.h>
#include "config.h"
#include "sensors.h"
#include "gestures.h"
#include "ui.h"
#ifdef USE_LCD
#include "lcd_ui.h"
#endif

// Time tracking
unsigned long lastInferenceTime = 0;
unsigned long lastSampleTime = 0;
unsigned long lastLcdUpdateTime = 0;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(3000); // Wait for serial connection
  
  // Initialize LED
  pinMode(LED_BUILTIN, OUTPUT);
  
  #ifdef USE_LCD
  // Initialize LCD
  initLCD();
  #endif
  
  // Initialize sensors
  if (!initSensors()) {
    Serial.println("Sensor initialization failed!");
    #ifdef USE_LCD
    showTempMessage("ERROR:", "Sensor initialization", "failed!", "", 0);
    #endif
    while (1) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  }
  
  // Display welcome message
  showWelcomeMessage();
  
  // Fill initial data windows
  Serial.println("Initializing data window with initial samples...");
  #ifdef USE_LCD
  showTempMessage("Initializing...", "Collecting data", "samples for window", "", 1000);
  #endif
  
  for (int i = 0; i < WINDOW_SIZE; i++) {
    readAllSensors();
    updateDataWindow();
    delay(10);
  }
  Serial.println("Data window initialization complete.");
  
  // Short LED flash to indicate initialization complete
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

void loop() {
  // Current time
  unsigned long currentMillis = millis();
  
  // Sample data at fixed intervals
  if (currentMillis - lastSampleTime >= SAMPLING_INTERVAL_MS) {
    lastSampleTime = currentMillis;
    
    // Read all sensor data
    readAllSensors();
    
    // Update the data window with new readings
    updateDataWindow();
  }
  
  // Periodically perform inference
  if (currentMillis - lastInferenceTime >= INFERENCE_INTERVAL_MS) {
    lastInferenceTime = currentMillis;
    
    // Prepare statistical features for the model
    prepareFeatures();
    
    // Only run inference if we have a full data window
    if (windowFilled) {
      // Run inference and process results
      runInference();
      
      #ifdef USE_LCD
      // Update LCD with current gesture information
      if (currentMillis - lastLcdUpdateTime >= LCD_UPDATE_INTERVAL_MS) {
        lastLcdUpdateTime = currentMillis;
        updateLCD(lastRecognizedGesture.c_str());
      }
      #endif
    }
  }
  
  // Process commands from serial
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    handleCommand(command);
  }
}