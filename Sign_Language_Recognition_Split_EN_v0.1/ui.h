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
  } else if (command == "list") {
    // Display list of supported gestures
    printGestureList();
  } else if (command == "finger") {
    // Display finger bend angle visualization
    printFingerBending();
  } else if (command == "features") {
    // Display current feature values
    printFeatures();
  } else if (command == "help") {
    // Display help
    Serial.println("\nAvailable commands:");
    Serial.println("  info - Display current sensor data");
    Serial.println("  raw - Display raw ADC values");
    Serial.println("  list - Display list of supported gestures");
    Serial.println("  finger - Display finger bend angle visualization");
    Serial.println("  features - Display statistical features used by the model");
    Serial.println("  help - Display this help message");
  } else {
    Serial.println("Unknown command. Type 'help' for available commands.");
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
  Serial.println("  help - Display all available commands");
  Serial.println("--------------------------------------------------");
}

#endif // UI_H