/*
 * gestures.h - Gesture Recognition Processing
 * 
 * Contains functions related to gesture recognition and inference
 */

#ifndef GESTURES_H
#define GESTURES_H

#include <Arduino.h>
#include <Sign-Language-Glove_inferencing.h>
#include "config.h"
#include "sensors.h"
#ifdef USE_LCD
#include "lcd_ui.h"
#endif

// Gesture recognition state variables
extern String lastRecognizedGesture;
extern int stableCount;
extern int noGestureCount;

/**
 * @brief Get friendly description for a gesture
 */
const char* getGestureDescription(const char* label);

/**
 * @brief Run inference and process results
 */
void runInference();

// Implementation section ---------------------------------

// Gesture recognition state variables
String lastRecognizedGesture = "";
int stableCount = 0;  // Stability counter to reduce recognition jitter
int noGestureCount = 0; // Counter to detect when no gesture is present

const char* getGestureDescription(const char* label) {
  // Check if the label matches any predefined gesture and return its description
  if (strcmp(label, "one") == 0) {
    return GESTURE_DESCRIPTIONS[0];
  } else if (strcmp(label, "two") == 0) {
    return GESTURE_DESCRIPTIONS[1];
  } else if (strcmp(label, "three") == 0) {
    return GESTURE_DESCRIPTIONS[2];
  } else if (strcmp(label, "four") == 0) {
    return GESTURE_DESCRIPTIONS[3];
  } else if (strcmp(label, "five") == 0) {
    return GESTURE_DESCRIPTIONS[4];
  } else if (strcmp(label, "love") == 0) {
    return GESTURE_DESCRIPTIONS[5];
  } else {
    // If not a known gesture, return the original label
    return label;
  }
}

void runInference() {
  // Create signal_t structure for Edge Impulse
  signal_t signal;
  signal.total_length = FEATURE_COUNT;
  signal.get_data = &get_signal_data;
  
  // Result storage
  ei_impulse_result_t result;
  
  // Execute inference
  EI_IMPULSE_ERROR ei_error = run_classifier(&signal, &result, false);
  
  // Process results if inference was successful
  if (ei_error == EI_IMPULSE_OK) {
    // Find gesture with highest confidence
    float maxScore = 0;
    size_t maxIndex = 0;
    
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
      if (result.classification[i].value > maxScore) {
        maxScore = result.classification[i].value;
        maxIndex = i;
      }
    }
    
    // Only consider as valid gesture if above confidence threshold
    if (maxScore > CONFIDENCE_THRESHOLD) {
      String gesture = String(result.classification[maxIndex].label);
      
      // Only output result after consecutive identical recognitions
      if (gesture == lastRecognizedGesture) {
        stableCount++;
        if (stableCount >= 2 && stableCount % 2 == 0) {  // Output every 2 stable updates
          // Get gesture description
          const char* gestureDesc = getGestureDescription(gesture.c_str());
          
          // Display recognition result
          Serial.print("Recognized gesture: ");
          Serial.print(gesture);
          Serial.print(" - ");
          Serial.print(gestureDesc);
          Serial.print(" (");
          Serial.print(maxScore * 100);
          Serial.println("%)");
          
          // LED flash to indicate successful recognition
          digitalWrite(LED_BUILTIN, HIGH);
          delay(50);
          digitalWrite(LED_BUILTIN, LOW);
          
          #ifdef USE_LCD
          // Update LCD with latest gesture (done in main loop)
          #endif
          
          // Reset no-gesture counter
          noGestureCount = 0;
        }
      } else {
        // New gesture, reset stability counter
        lastRecognizedGesture = gesture;
        stableCount = 1;
      }
    } else {
      // Below threshold, might be noise or transition state
      noGestureCount++;
      
      // If no gesture detected for 10 consecutive times, clear last recognized gesture
      if (noGestureCount > 10) {
        if (lastRecognizedGesture != "") {
          Serial.println("Gesture released");
          
          #ifdef USE_LCD
          // Update LCD to show ready state
          updateLCD("");
          #endif
          
          lastRecognizedGesture = "";
        }
        stableCount = 0;
      }
    }
  } else {
    Serial.println("Inference error");
    
    #ifdef USE_LCD
    // Display error on LCD
    showTempMessage("Error:", "Inference failed", "", "", 1000);
    #endif
  }
}

#endif // GESTURES_H