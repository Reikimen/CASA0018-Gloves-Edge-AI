/*
 * config.h - Sign Language Glove Project Configuration
 * 
 * Contains all hardware definitions, calibration values and parameter configurations
 */

#ifndef CONFIG_H
#define CONFIG_H

// LCD Support - comment out this line to disable LCD functionality
#define USE_LCD

// Flex sensor pin definitions
#define FLEX_PIN_THUMB A0
#define FLEX_PIN_INDEX A1
#define FLEX_PIN_MIDDLE A2
#define FLEX_PIN_RING A3
#define FLEX_PIN_PINKY A6

// Flex sensor calibration values - as obtained from the calibration program
const int FLEX_THUMB_STRAIGHT_ADC = 330;    // ADC value when straight
const int FLEX_INDEX_STRAIGHT_ADC = 440;    // ADC value when straight
const int FLEX_MIDDLE_STRAIGHT_ADC = 400;   // ADC value when straight
const int FLEX_RING_STRAIGHT_ADC = 430;     // ADC value when straight
const int FLEX_PINKY_STRAIGHT_ADC = 390;    // ADC value when straight

const int FLEX_THUMB_BENT_ADC = 220;    // ADC value when fully bent - ADC decreases when bent
const int FLEX_INDEX_BENT_ADC = 360;    // ADC value when fully bent - ADC decreases when bent
const int FLEX_MIDDLE_BENT_ADC = 320;   // ADC value when fully bent - ADC decreases when bent
const int FLEX_RING_BENT_ADC = 335;     // ADC value when fully bent - ADC decreases when bent
const int FLEX_PINKY_BENT_ADC = 320;    // ADC value when fully bent - ADC decreases when bent

// Integrated into arrays for easier processing
const int FLEX_STRAIGHT_ADC[5] = {
  FLEX_THUMB_STRAIGHT_ADC, 
  FLEX_INDEX_STRAIGHT_ADC, 
  FLEX_MIDDLE_STRAIGHT_ADC, 
  FLEX_RING_STRAIGHT_ADC, 
  FLEX_PINKY_STRAIGHT_ADC
};

const int FLEX_BENT_ADC[5] = {
  FLEX_THUMB_BENT_ADC, 
  FLEX_INDEX_BENT_ADC, 
  FLEX_MIDDLE_BENT_ADC, 
  FLEX_RING_BENT_ADC, 
  FLEX_PINKY_BENT_ADC
};

// LCD Update interval
#define LCD_UPDATE_INTERVAL_MS 200  // Update LCD every 200ms

// Data processing parameters
#define WINDOW_SIZE 50          // Number of samples to collect for statistics
#define FEATURE_COUNT 35        // Total features (5 sensors × 7 statistics)
#define STATS_PER_SENSOR 7      // Number of statistics per sensor

// Sampling and inference configuration
#define SAMPLING_INTERVAL_MS 20 // 50Hz sampling rate
#define INFERENCE_INTERVAL_MS 300  // Perform inference every 300ms
#define CONFIDENCE_THRESHOLD 0.60 // Confidence threshold (0.0-1.0)

// Filtering parameters
#define ALPHA 0.3  // Low-pass filter coefficient

// Gesture labels and descriptions
const char* GESTURE_LABELS[] = {"one", "two", "three", "four", "five", "love"};
const char* GESTURE_DESCRIPTIONS[] = {
  "Number 1 (Index finger extended)",
  "Number 2 (Index and middle fingers extended)",
  "Number 3 (Thumb, index and middle fingers extended)",
  "Number 4 (Four fingers extended, thumb tucked)",
  "Number 5 (All five fingers extended)",
  "Love gesture (Thumb and pinky extended, forming heart shape)"
};

#endif // CONFIG_H