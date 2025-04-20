---
typora-root-url: ./
---

# CASA0018-Gloves-Edge-AI

# Sign Language Recognition Glove

This project implements a wearable device that translates sign language gestures into text using flex sensors, an IMU, and edge ML inference.

## Project Overview

The Sign Language Recognition Glove aims to develop a wearable device that translates sign language by recognizing hand positions in real-time. This device uses flex sensors to detect finger bending and an IMU to capture hand orientation, enabling it to recognize specific sign language gestures. The system processes this data using a machine learning model deployed directly on an Arduino microcontroller, providing immediate feedback through an LCD display.

<img src="/img/love.jpg" alt="love" style="zoom:10%;" />

### Features

- Real-time recognition of 6 sign language gestures (one, two, three, four, five, love)
- LCD display for visual feedback and status information
- Serial command interface with rich debugging features
- Statistical feature extraction for robust gesture recognition
- Low-latency processing suitable for real-time interaction

## Hardware Requirements

- Arduino Nano 33 BLE
- 5× Flex sensors (one for each finger)
- LCD2004 I2C display (address 0x27 by default)
- Breadboard/PCB for circuit connections
- Glove for mounting sensors

## Software Dependencies

- Arduino IDE
- [Arduino_LSM9DS1 library](https://www.arduino.cc/en/Reference/ArduinoLSM9DS1)
- [LiquidCrystal_I2C library](https://github.com/johnrickman/LiquidCrystal_I2C)
- Edge Impulse exported model library for Sign Language Recognition

## Installation

1. Clone this repository:

   ```
   git clone https://github.com/Reikimen/CASA0018-Gloves-Edge-AI.git
   ```

2. Open the Arduino sketch (`Sign_Language_Recognition_Split_EN_v0.2.ino`) in the Arduino IDE.

3. Install the required libraries through the Arduino Library Manager:

   - Arduino_LSM9DS1
   - LiquidCrystal_I2C

4. Export the machine learning model from Edge Impulse as an Arduino library.

   - Follow the [Edge Impulse documentation](https://docs.edgeimpulse.com/docs/deployment/arduino-library) for exporting the model
   - Add the exported library to your Arduino libraries folder

5. Connect the hardware components according to the pin definitions in `config.h`.

6. Upload the sketch to your Arduino Nano 33 BLE.

## Usage

Once the system is powered on and initialized:

1. The LCD will display the welcome message and initial sensor status.
2. Make a sign language gesture with the glove.
3. The system will recognize the gesture and display it on the LCD.
4. The built-in LED will flash to indicate successful recognition.

### Serial Commands

Connect to the serial monitor (115200 baud) to access these commands:

- `info` - Display current sensor data
- `raw` - Display raw ADC values
- `list` - Display list of supported gestures
- `finger` - Display finger bend angle visualization
- `features` - Display statistical features used by the model
- `debug` - Toggle debug mode
- `lcd` - Toggle LCD backlight
- `help` - Display this help message

## System Architecture

The system is organized into several modular components (`Sign_Language_Recognition_Split_EN_v0.2`):

- **config.h** - Configuration parameters, pin definitions, and calibration values
- **sensors.h** - Sensor data acquisition and processing
- **gestures.h** - Gesture recognition and inference
- **lcd_ui.h** - LCD display interface
- **ui.h** - User interface and command processing
- **Sign_Language_Recognition_Split_EN_v0.2.ino** - Main program

### Data Processing Pipeline

1. Flex sensors read finger bending angles and IMU reads hand orientation
2. Raw data is filtered and normalized
3. Statistical features are extracted from a sliding window of samples
4. The machine learning model performs inference on the features
5. Recognition results are displayed on LCD and via serial output

## Model Training

The model was trained using Edge Impulse with the following workflow:

1. Data collection using the same hardware setup

   <img src="/img/Collected Data.jpg" alt="Collected Data" style="zoom:15%;" />

2. Feature extraction: 7 statistical features (mean, min, max, RMS, StdDev, skewness, kurtosis) for each sensor

   <img src="/img/Flatten.jpg" alt="Flatten" style="zoom:15%;" />

3. Neural network training with several dense layers

   <img src="/img/Classifer NNS.jpg" alt="Classifer NNS" style="zoom:18%;" />

4. Model optimization

   ![versions](/img/versions.jpg)

5. Quantization to reduce model size and improve inference speed

   ![Quantized](/img/Quantized.jpg)



## Calibration

The system uses calibration values for flex sensors defined in `config.h`:

```cpp
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
```

If needed, update these values based on your specific sensor characteristics. (Use the calabration programe)

## Performance

The system:

- Samples sensor data at 50Hz
- Performs inference every 300ms
- Updates LCD every 200ms
- Uses a confidence threshold of 0.60 for gesture detection
- Implements stability detection to prevent jitter in recognition results

<img src="/img/love example.jpg" alt="love example" style="zoom:25%;" />

## Troubleshooting

- If the LCD doesn't display anything, check the I2C address (default is 0x27)
- If gestures aren't recognized correctly, consider recalibrating the flex sensors
- Ensure the flex sensors are properly positioned on each finger
- Check serial output for debugging information when issues occur

## Future Improvements

- Add support for more sign language gestures
- Implement adaptive calibration to handle different users
- Add Bluetooth communication for mobile app integration
- Support for dynamic gestures (not just static positions)
- Improve model architecture for better accuracy with less computation

## Acknowledgments

- UCL Centre for Advanced Spatial Analysis (CASA)
- Edge Impulse for the embedded machine learning platform
- Arduino community for libraries and support

## Author

Dankao Chen - [GitHub](https://github.com/Reikimen)

------

Project created as part of the CASA0018 - Deep Learning for Sensor Networks course at University College London.