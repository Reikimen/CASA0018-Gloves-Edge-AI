/*
 * ui.h - User Interface and Command Processing
 * 
 * Contains functions for user interface display and command processing
 */

#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include <Sign-Language-Glove_inferencing.h>
#include "config.h"
#include "sensors.h"
#include "lcd_ui.h"  // Added LCD UI header

// Display mode flag
extern bool debugMode;

/**
 * @brief Display current sensor data
 */
void printSensorData();

/**
 * @brief Display list of supported gestures
 */
void printGestureList();

/**
 * @brief Display finger bend angle visualization
 */
void printFingerBending();

/**
 * @brief Display statistical features currently being used
 */
void printFeatures();

/**
 * @brief Process command
 */
void handleCommand(String command);

/**
 * @brief Display welcome message and initialization status
 */
void showWelcomeMessage();

// Implementation section ---------------------------------

// Debug mode flag
bool debugMode = false;

void printSensorData() {
  const char* fingerNames[] = {"Thumb", "Index", "Middle", "Ring", "Pinky"};
  int flexRawValues[5];
  
  // Read raw ADC values
  flexRawValues[0] = analogRead(FLEX_PIN_THUMB);
  flexRawValues[1] = analogRead(FLEX_PIN_INDEX);
  flexRawValues[2] = analogRead(FLEX_PIN_MIDDLE);
  flexRawValues[3] = analogRead(FLEX_PIN_RING);
  flexRawValues[4] = analogRead(FLEX_PIN_PINKY);
  
  Serial.println("\nCurrent Sensor Data:");
  
  Serial.println("Flex Sensor Values:");
  for (int i = 0; i < 5; i++) {
    Serial.print(fingerNames[i]);
    Serial.print(": ADC=");
    Serial.print(flexRawValues[i]);
    Serial.print(", Bend=");
    Serial.print(filteredFlexValues[i]);
    Serial.println("%");
  }
  
  Serial.println("\nIMU Data:");
  Serial.print("Acceleration (g): X=");
  Serial.print(filteredAx);
  Serial.print(", Y=");
  Serial.print(filteredAy);
  Serial.print(", Z=");
  Serial.println(filteredAz);
  
  Serial.print("Gyroscope (dps): X=");
  Serial.print(filteredGx);
  Serial.print(", Y=");
  Serial.print(filteredGy);
  Serial.print(", Z=");
  Serial.println(filteredGz);
  
  // Display on LCD as well if we're in LCD mode
  #ifdef USE_LCD
  char line1[21], line2[21], line3[21], line4[21];
  sprintf(line1, "Flex Sensors:");
  sprintf(line2, "T:%d I:%d", flexRawValues[0], flexRawValues[1]);
  sprintf(line3, "M:%d R:%d", flexRawValues[2], flexRawValues[3]);
  sprintf(line4, "P:%d", flexRawValues[4]);
  showTempMessage(line1, line2, line3, line4, 3000);
  #endif
}

void printGestureList() {
  Serial.println("\nSupported Gestures:");
  
  // Manually list all labels and their descriptions
  for (int i = 0; i < 6; i++) {  // 6 gestures
    Serial.print("  ");
    Serial.print(i + 1);
    Serial.print(". ");
    Serial.print(GESTURE_LABELS[i]);
    Serial.print(" - ");
    Serial.println(GESTURE_DESCRIPTIONS[i]);
  }
  
  // If there are additional labels in the model
  for (size_t i = 6; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    Serial.print("  ");
    Serial.print(i + 1);
    Serial.print(". ");
    Serial.println(ei_classifier_inferencing_categories[i]);
  }
  
  #ifdef USE_LCD
  // Show first set of gestures on LCD
  showTempMessage(
    "Supported Gestures:",
    "1. one - Number 1",
    "2. two - Number 2",
    "3. three - Number 3",
    3000
  );
  
  // Show second set of gestures on LCD
  showTempMessage(
    "Supported Gestures:",
    "4. four - Number 4",
    "5. five - Number 5",
    "6. love - Love gesture",
    3000
  );
  #endif
}

void printFingerBending() {
  Serial.println("\nFinger Bend Angle Visualization:");
  Serial.println("0%=Fully straight, 100%=Fully bent");
  Serial.println("T: Thumb, I: Index, M: Middle, R: Ring, P: Pinky");
  
  // Draw scale
  Serial.println("  0%      25%      50%      75%     100%");
  Serial.println("  |        |        |        |        |");
  
  // Generate visualization bar for each finger
  const char* fingerLabels[] = {"T:", "I:", "M:", "R:", "P:"};
  for (int i = 0; i < 5; i++) {
    // Finger label
    Serial.print(fingerLabels[i]);
    
    // Calculate position (0-50 characters)
    int position = map(filteredFlexValues[i], 0, 100, 0, 50);
    
    // Draw bar
    for (int j = 0; j < 50; j++) {
      if (j == position) {
        Serial.print("O"); // Current position
      } else if (j < position) {
        Serial.print("-"); // Bent portion
      } else {
        Serial.print(" "); // Unbent portion
      }
    }
    Serial.println();
  }
  
  #ifdef USE_LCD
  // Display bend values on LCD
  char line1[21], line2[21], line3[21], line4[21];
  sprintf(line1, "Bend Percentages:");
  sprintf(line2, "T:%.0f%% I:%.0f%%", filteredFlexValues[0], filteredFlexValues[1]);
  sprintf(line3, "M:%.0f%% R:%.0f%%", filteredFlexValues[2], filteredFlexValues[3]);
  sprintf(line4, "P:%.0f%%", filteredFlexValues[4]);
  showTempMessage(line1, line2, line3, line4, 3000);
  #endif
}

void printFeatures() {
  const char* statNames[] = {"Average", "Minimum", "Maximum", "RMS", "StdDev", "Skewness", "Kurtosis"};
  const char* fingerNames[] = {"Thumb", "Index", "Middle", "Ring", "Pinky"};
  
  Serial.println("\nCurrent Statistical Features:");
  Serial.println("These 35 values are used as input to the Edge Impulse model:");
  
  // First make sure features are up-to-date
  prepareFeatures();
  
  // Print all feature values by finger and statistic
  int featureIndex = 0;
  for (int finger = 0; finger < 5; finger++) {
    Serial.print("\n");
    Serial.print(fingerNames[finger]);
    Serial.println(" Statistics:");
    
    for (int stat = 0; stat < STATS_PER_SENSOR; stat++) {
      Serial.print("  ");
      Serial.print(statNames[stat]);
      Serial.print(": ");
      Serial.println(features[featureIndex++], 4);
    }
  }
  
  #ifdef USE_LCD
  // Show summary on LCD
  char line1[21], line2[21];
  sprintf(line1, "Features Calculated");
  sprintf(line2, "35 values for model");
  showTempMessage(line1, line2, "See serial output", "for details", 3000);
  #endif
}

void handleCommand(String command) {
  if (command == "info") {
    // Display current sensor data
    printSensorData();
  } else if (command == "raw") {
    // Display raw ADC values
    Serial.println("Raw ADC values:");
    Serial.print("Thumb: ");
    Serial.println(analogRead(FLEX_PIN_THUMB));
    Serial.print("Index: ");
    Serial.println(analogRead(FLEX_PIN_INDEX));
    Serial.print("Middle: ");
    Serial.println(analogRead(FLEX_PIN_MIDDLE));
    Serial.print("Ring: ");
    Serial.println(analogRead(FLEX_PIN_RING));
    Serial.print("Pinky: ");
    Serial.println(analogRead(FLEX_PIN_PINKY));
    
    #ifdef USE_LCD
    // Show on LCD
    int t = analogRead(FLEX_PIN_THUMB);
    int i = analogRead(FLEX_PIN_INDEX);
    int m = analogRead(FLEX_PIN_MIDDLE);
    int r = analogRead(FLEX_PIN_RING);
    int p = analogRead(FLEX_PIN_PINKY);
    
    char line1[21], line2[21], line3[21];
    sprintf(line1, "Raw ADC Values:");
    sprintf(line2, "T:%d I:%d", t, i);
    sprintf(line3, "M:%d R:%d P:%d", m, r, p);
    
    showTempMessage(line1, line2, line3, "", 3000);
    #endif
  } else if (command == "list") {
    // Display list of supported gestures
    printGestureList();
  } else if (command == "finger") {
    // Display finger bend angle visualization
    printFingerBending();
  } else if (command == "features") {
    // Display current feature values
    printFeatures();
  } else if (command == "debug") {
    // Toggle debug mode
    debugMode = !debugMode;
    Serial.print("Debug mode ");
    Serial.println(debugMode ? "ON" : "OFF");
    
    #ifdef USE_LCD
    // Show confirmation on LCD
    char message[21];
    sprintf(message, "Debug mode: %s", debugMode ? "ON" : "OFF");
    showTempMessage("Status Change", message, "", "", 1500);
    #endif
  } 
  #ifdef USE_LCD
  else if (command == "lcd") {
    // Toggle LCD backlight
    bool backlight = toggleLCDBacklight();
    Serial.print("LCD backlight ");
    Serial.println(backlight ? "ON" : "OFF");
  }
  #endif
  else if (command == "help") {
    // Display help
    Serial.println("\nAvailable commands:");
    Serial.println("  info - Display current sensor data");
    Serial.println("  raw - Display raw ADC values");
    Serial.println("  list - Display list of supported gestures");
    Serial.println("  finger - Display finger bend angle visualization");
    Serial.println("  features - Display statistical features used by the model");
    Serial.println("  debug - Toggle debug mode");
    #ifdef USE_LCD
    Serial.println("  lcd - Toggle LCD backlight");
    #endif
    Serial.println("  help - Display this help message");
    
    #ifdef USE_LCD
    // Show on LCD
    showTempMessage(
      "Available Commands:",
      "info, raw, list,",
      "finger, features,",
      "debug, lcd, help",
      3000
    );
    #endif
  } else {
    Serial.println("Unknown command. Type 'help' for available commands.");
    
    #ifdef USE_LCD
    showTempMessage("Unknown Command", "Type 'help' for", "available commands", "", 2000);
    #endif
  }
}

void showWelcomeMessage() {
  Serial.println("==================================================");
  Serial.println("|      Sign Language Glove - Gesture Recognition |");
  Serial.println("==================================================");
  Serial.println("\nIMU initialized successfully");
  Serial.print("Accelerometer sample rate: ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.print("Gyroscope sample rate: ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");
  
  // Display model information
  Serial.println("\nEdge Impulse Model Information:");
  Serial.print("Model name: ");
  Serial.println(EI_CLASSIFIER_PROJECT_NAME);
  Serial.print("Model ID: ");
  Serial.println(EI_CLASSIFIER_PROJECT_ID);
  Serial.print("Number of supported gestures: ");
  Serial.println(EI_CLASSIFIER_LABEL_COUNT);
  
  // Display feature information
  Serial.println("\nFeature configuration:");
  Serial.print("Using ");
  Serial.print(FEATURE_COUNT);
  Serial.println(" statistical features (7 statistics for 5 flex sensors)");
  Serial.print("Data window size: ");
  Serial.print(WINDOW_SIZE);
  Serial.println(" samples");
  
  // Display all supported gestures
  printGestureList();
  
  Serial.println("\nReady to recognize gestures...");
  Serial.println("--------------------------------------------------");
  Serial.println("Command Menu:");
  Serial.println("  info - Display current sensor data");
  Serial.println("  raw - Display raw ADC values");
  Serial.println("  list - Display list of supported gestures");
  Serial.println("  finger - Display finger bend angle visualization");
  Serial.println("  features - Display statistical features used by the model");
  #ifdef USE_LCD
  Serial.println("  lcd - Toggle LCD backlight");
  #endif
  Serial.println("  debug - Toggle debug mode");
  Serial.println("  help - Display all available commands");
  Serial.println("--------------------------------------------------");
  
  #ifdef USE_LCD
  // Show welcome message on LCD
  showLCDWelcomeMessage();
  #endif
}

#endif // UI_H