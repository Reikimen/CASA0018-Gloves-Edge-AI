/*
 * sensors.h - Sensor Data Processing
 * 
 * Contains functions for sensor initialization, reading, and data processing
 */

#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino_LSM9DS1.h>
#include <Sign-Language-Glove_inferencing.h>
#include "config.h"

// Store filtered sensor values
extern float filteredFlexValues[5];
extern float filteredAx, filteredAy, filteredAz;
extern float filteredGx, filteredGy, filteredGz;

// Data windows for statistical features
extern float thumbWindow[WINDOW_SIZE];
extern float indexWindow[WINDOW_SIZE];
extern float middleWindow[WINDOW_SIZE];
extern float ringWindow[WINDOW_SIZE];
extern float pinkyWindow[WINDOW_SIZE];
extern int windowIndex;
extern bool windowFilled;

// Model input feature buffer
extern float features[FEATURE_COUNT];

/**
 * @brief Initialize all sensors
 * @return Whether initialization was successful
 */
bool initSensors();

/**
 * @brief Bend percentage calculation function - handles both increasing and decreasing ADC values
 */
float calculateBendPercentage(int adcValue, int straightAdc, int bentAdc);

/**
 * @brief Low-pass filter function
 */
float lowPassFilter(float currentValue, float previousFilteredValue, float alpha);

/**
 * @brief Read all sensor data
 */
void readAllSensors();

/**
 * @brief Update the data window with latest sensor readings
 */
void updateDataWindow();

/**
 * @brief Calculate statistics for a sensor data window
 * @param window Pointer to the data window
 * @param stats Array to store the 7 statistics
 */
void calculateStatistics(float* window, float* stats);

/**
 * @brief Prepare feature data for inference
 */
void prepareFeatures();

/**
 * @brief Define static callback function for getting feature data
 */
int get_signal_data(size_t offset, size_t length, float *out_ptr);

// Implementation section -----------------------

// Store filtered sensor values
float filteredFlexValues[5] = {0};
float filteredAx = 0, filteredAy = 0, filteredAz = 0;
float filteredGx = 0, filteredGy = 0, filteredGz = 0;

// Data windows for statistical features
float thumbWindow[WINDOW_SIZE] = {0};
float indexWindow[WINDOW_SIZE] = {0};
float middleWindow[WINDOW_SIZE] = {0};
float ringWindow[WINDOW_SIZE] = {0};
float pinkyWindow[WINDOW_SIZE] = {0};
int windowIndex = 0;
bool windowFilled = false;

// Model input feature buffer
float features[FEATURE_COUNT];

bool initSensors() {
  return IMU.begin();
}

float calculateBendPercentage(int adcValue, int straightAdc, int bentAdc) {
  // Ensure ADC value is within range to prevent extreme cases
  adcValue = constrain(adcValue, min(straightAdc, bentAdc), max(straightAdc, bentAdc));
  
  // Calculate bend percentage (0% = straight, 100% = fully bent)
  float bendPercentage;
  
  // Handle both cases: ADC values that increase or decrease with bending
  if (bentAdc > straightAdc) {
    // Standard case: ADC value increases when bent
    bendPercentage = map(adcValue, straightAdc, bentAdc, 0, 100);
  } else {
    // Reverse case: ADC value decreases when bent
    bendPercentage = map(adcValue, straightAdc, bentAdc, 0, 100);
  }
  
  // Ensure result is within 0-100 range
  return constrain(bendPercentage, 0, 100);
}

float lowPassFilter(float currentValue, float previousFilteredValue, float alpha) {
  return previousFilteredValue + alpha * (currentValue - previousFilteredValue);
}

void readAllSensors() {
  // Read flex sensor data
  int flexRawValues[5];
  flexRawValues[0] = analogRead(FLEX_PIN_THUMB);
  flexRawValues[1] = analogRead(FLEX_PIN_INDEX);
  flexRawValues[2] = analogRead(FLEX_PIN_MIDDLE);
  flexRawValues[3] = analogRead(FLEX_PIN_RING);
  flexRawValues[4] = analogRead(FLEX_PIN_PINKY);
  
  // Convert ADC values to bend percentages and apply filtering
  for (int i = 0; i < 5; i++) {
    float bendPercentage = calculateBendPercentage(
      flexRawValues[i], 
      FLEX_STRAIGHT_ADC[i], 
      FLEX_BENT_ADC[i]
    );
    
    // Apply low-pass filter
    filteredFlexValues[i] = lowPassFilter(bendPercentage, filteredFlexValues[i], ALPHA);
  }
  
  // Read IMU data
  float ax, ay, az, gx, gy, gz;
  
  if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
    IMU.readAcceleration(ax, ay, az);
    IMU.readGyroscope(gx, gy, gz);
    
    // Apply low-pass filter
    filteredAx = lowPassFilter(ax, filteredAx, ALPHA);
    filteredAy = lowPassFilter(ay, filteredAy, ALPHA);
    filteredAz = lowPassFilter(az, filteredAz, ALPHA);
    filteredGx = lowPassFilter(gx, filteredGx, ALPHA);
    filteredGy = lowPassFilter(gy, filteredGy, ALPHA);
    filteredGz = lowPassFilter(gz, filteredGz, ALPHA);
  }
}

void updateDataWindow() {
  // Add latest data to the window
  thumbWindow[windowIndex] = filteredFlexValues[0];
  indexWindow[windowIndex] = filteredFlexValues[1];
  middleWindow[windowIndex] = filteredFlexValues[2];
  ringWindow[windowIndex] = filteredFlexValues[3];
  pinkyWindow[windowIndex] = filteredFlexValues[4];
  
  // Update window index
  windowIndex = (windowIndex + 1) % WINDOW_SIZE;
  
  // Mark window as filled after first complete cycle
  if (windowIndex == 0) {
    windowFilled = true;
  }
}

void calculateStatistics(float* window, float* stats) {
  float sum = 0, sum2 = 0;
  float min = 1000, max = -1000;
  
  // Calculate basic statistics
  for (int i = 0; i < WINDOW_SIZE; i++) {
    float val = window[i];
    sum += val;
    sum2 += val * val;
    if (val < min) min = val;
    if (val > max) max = val;
  }
  
  // Average (mean)
  float mean = sum / WINDOW_SIZE;
  stats[0] = mean;
  
  // Minimum
  stats[1] = min;
  
  // Maximum
  stats[2] = max;
  
  // Root-mean square
  stats[3] = sqrt(sum2 / WINDOW_SIZE);
  
  // Calculate variance for remaining statistics
  float variance = 0, skewSum = 0, kurtSum = 0;
  
  for (int i = 0; i < WINDOW_SIZE; i++) {
    float diff = window[i] - mean;
    float diff2 = diff * diff;
    variance += diff2;
    skewSum += diff * diff2;
    kurtSum += diff2 * diff2;
  }
  
  variance /= WINDOW_SIZE;
  
  // Standard deviation
  float stdev = sqrt(variance);
  stats[4] = stdev;
  
  // Skewness - Avoid division by zero
  stats[5] = (stdev > 0.0001) ? (skewSum / (WINDOW_SIZE * stdev * stdev * stdev)) : 0;
  
  // Kurtosis - Avoid division by zero
  stats[6] = (variance > 0.0001) ? (kurtSum / (WINDOW_SIZE * variance * variance)) - 3 : 0;
}

void prepareFeatures() {
  // Only prepare features if window has been filled
  if (!windowFilled) return;
  
  // Arrays to hold statistics for each finger
  float thumbStats[STATS_PER_SENSOR];
  float indexStats[STATS_PER_SENSOR];
  float middleStats[STATS_PER_SENSOR];
  float ringStats[STATS_PER_SENSOR];
  float pinkyStats[STATS_PER_SENSOR];
  
  // Calculate statistics for each finger
  calculateStatistics(thumbWindow, thumbStats);
  calculateStatistics(indexWindow, indexStats);
  calculateStatistics(middleWindow, middleStats);
  calculateStatistics(ringWindow, ringStats);
  calculateStatistics(pinkyWindow, pinkyStats);
  
  // Fill feature array in the correct order for Edge Impulse model
  // Features order: [all thumb stats] [all index stats] [all middle stats] [all ring stats] [all pinky stats]
  int featureIndex = 0;
  
  // Copy thumb statistics
  for (int i = 0; i < STATS_PER_SENSOR; i++) {
    features[featureIndex++] = thumbStats[i];
  }
  
  // Copy index finger statistics
  for (int i = 0; i < STATS_PER_SENSOR; i++) {
    features[featureIndex++] = indexStats[i];
  }
  
  // Copy middle finger statistics
  for (int i = 0; i < STATS_PER_SENSOR; i++) {
    features[featureIndex++] = middleStats[i];
  }
  
  // Copy ring finger statistics
  for (int i = 0; i < STATS_PER_SENSOR; i++) {
    features[featureIndex++] = ringStats[i];
  }
  
  // Copy pinky finger statistics
  for (int i = 0; i < STATS_PER_SENSOR; i++) {
    features[featureIndex++] = pinkyStats[i];
  }
}

int get_signal_data(size_t offset, size_t length, float *out_ptr) {
  memcpy(out_ptr, features + offset, length * sizeof(float));
  return 0;
}

#endif // SENSORS_H