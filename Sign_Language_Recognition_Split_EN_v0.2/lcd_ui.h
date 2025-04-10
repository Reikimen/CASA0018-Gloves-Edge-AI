/*
 * lcd_ui.h - LCD Display Interface
 * 
 * Contains functions for managing the LCD display output
 */

#ifndef LCD_UI_H
#define LCD_UI_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"
#include "sensors.h"

// Global LCD object
// Set the LCD address to 0x27 for a 20 chars and 4 line display
// Note: Your LCD address might be different (common addresses: 0x27, 0x3F)
extern LiquidCrystal_I2C lcd;

// Double buffer for LCD - represents the current 4 lines on the LCD
extern char lcdBuffer[4][21]; // 20 chars + null terminator

// Secondary buffer for preparing the next frame
extern char lcdNextBuffer[4][21];

/**
 * @brief Initialize the LCD display
 */
void initLCD();

/**
 * @brief Initialize LCD buffers with spaces
 */
void initBuffers();

/**
 * @brief Write a string to the next buffer at specified position
 */
void writeToBuffer(int row, int col, const char* text);

/**
 * @brief Write a string to the next buffer at specified position
 */
void writeToBuffer(int row, int col, String text);

/**
 * @brief Clear a single line in the next buffer
 */
void clearBufferLine(int row);

/**
 * @brief Clear all lines in the next buffer
 */
void clearBuffer();

/**
 * @brief Commit the next buffer to the LCD, but only update changed lines
 */
void commitBuffer();

/**
 * @brief Update LCD with gesture recognition information
 */
void updateLCD(const char* gesture);

/**
 * @brief Show temporary message on LCD
 */
void showTempMessage(const char* line1, const char* line2, const char* line3, const char* line4, int duration);

/**
 * @brief Toggle LCD backlight
 * @return Current backlight status (true = on, false = off)
 */
bool toggleLCDBacklight();

/**
 * @brief Show welcome message on LCD
 */
void showLCDWelcomeMessage();

// Implementation section ---------------------------------

// Global LCD object
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Double buffer for LCD
char lcdBuffer[4][21];
char lcdNextBuffer[4][21];

void initLCD() {
    Wire.begin();
    lcd.init();      // Initialize the LCD
    lcd.backlight(); // Turn on the backlight
    
    // Initialize LCD buffers
    initBuffers();
}

void initBuffers() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 20; j++) {
            lcdBuffer[i][j] = ' ';
            lcdNextBuffer[i][j] = ' ';
        }
        lcdBuffer[i][20] = '\0';
        lcdNextBuffer[i][20] = '\0';
    }
}

void writeToBuffer(int row, int col, const char* text) {
    if (row < 0 || row > 3 || col < 0 || col > 19) return;
    
    int i = 0;
    while (text[i] != '\0' && col + i < 20) {
        lcdNextBuffer[row][col + i] = text[i];
        i++;
    }
}

void writeToBuffer(int row, int col, String text) {
    writeToBuffer(row, col, text.c_str());
}

void clearBufferLine(int row) {
    if (row < 0 || row > 3) return;
    
    for (int i = 0; i < 20; i++) {
        lcdNextBuffer[row][i] = ' ';
    }
    lcdNextBuffer[row][20] = '\0';
}

void clearBuffer() {
    for (int i = 0; i < 4; i++) {
        clearBufferLine(i);
    }
}

void commitBuffer() {
    bool different = false;
    
    for (int i = 0; i < 4; i++) {
        different = false;
        
        // Check if this line is different
        for (int j = 0; j < 20; j++) {
            if (lcdBuffer[i][j] != lcdNextBuffer[i][j]) {
                different = true;
                break;
            }
        }
        
        // If different, update LCD and current buffer
        if (different) {
            lcd.setCursor(0, i);
            for (int j = 0; j < 20; j++) {
                lcdBuffer[i][j] = lcdNextBuffer[i][j];
                lcd.write(lcdBuffer[i][j]);
            }
        }
    }
}

void updateLCD(const char* gesture) {
    // First line: Title
    clearBufferLine(0);
    writeToBuffer(0, 0, "Sign Language Glove");
    
    // Second line: Current gesture
    clearBufferLine(1);
    if (gesture != NULL && strcmp(gesture, "") != 0 && strcmp(gesture, "unknown") != 0) {
        char gestureLine[21];
        sprintf(gestureLine, "Gesture: %s", gesture);
        writeToBuffer(1, 0, gestureLine);
    } else {
        writeToBuffer(1, 0, "Ready for gestures");
    }
    
    // Get flex sensor data for display
    float bendValues[5];
    for (int i = 0; i < 5; i++) {
        bendValues[i] = filteredFlexValues[i];
    }
    
    // Third line: Flex values for thumb, index, middle
    clearBufferLine(2);
    char line3[21];
    sprintf(line3, "T:%.0f%% I:%.0f%% M:%.0f%%", 
            bendValues[0], bendValues[1], bendValues[2]);
    writeToBuffer(2, 0, line3);
    
    // Fourth line: Flex values for ring, pinky
    clearBufferLine(3);
    char line4[21];
    sprintf(line4, "R:%.0f%% P:%.0f%%", 
            bendValues[3], bendValues[4]);
    writeToBuffer(3, 0, line4);
    
    // Commit changes to the physical LCD
    commitBuffer();
}

void showTempMessage(const char* line1, const char* line2, const char* line3, const char* line4, int duration) {
    // Store current buffer
    char tempBuffer[4][21];
    for (int i = 0; i < 4; i++) {
        strncpy(tempBuffer[i], lcdNextBuffer[i], 21);
    }
    
    // Show message
    clearBuffer();
    if (line1) writeToBuffer(0, 0, line1);
    if (line2) writeToBuffer(1, 0, line2);
    if (line3) writeToBuffer(2, 0, line3);
    if (line4) writeToBuffer(3, 0, line4);
    commitBuffer();
    
    delay(duration);
    
    // Restore buffer
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 21; j++) {
            lcdNextBuffer[i][j] = tempBuffer[i][j];
        }
    }
    commitBuffer();
}

bool toggleLCDBacklight() {
    static bool backlight = true;
    backlight = !backlight;
    
    if (backlight) {
        lcd.backlight();
    } else {
        lcd.noBacklight();
    }
    
    return backlight;
}

void showLCDWelcomeMessage() {
    clearBuffer();
    writeToBuffer(0, 0, "Sign Language Glove");
    writeToBuffer(1, 0, "Initializing...");
    commitBuffer();
    
    delay(1000);
    
    // Show model information
    clearBuffer();
    writeToBuffer(0, 0, "Edge Impulse Model:");
    writeToBuffer(1, 0, EI_CLASSIFIER_PROJECT_NAME);
    char modelInfo[21];
    sprintf(modelInfo, "Gestures: %d", EI_CLASSIFIER_LABEL_COUNT);
    writeToBuffer(2, 0, modelInfo);
    writeToBuffer(3, 0, "Loading...");
    commitBuffer();
    
    delay(2000);
    
    // Show ready message
    clearBuffer();
    writeToBuffer(0, 0, "Sign Language Glove");
    writeToBuffer(1, 0, "Ready for gestures");
    writeToBuffer(2, 0, "");
    writeToBuffer(3, 0, "");
    commitBuffer();
}

#endif // LCD_UI_H