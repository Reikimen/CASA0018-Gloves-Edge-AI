/*
 * 手语翻译手套 - Edge Impulse数据收集程序(校准版)
 * 
 * 功能:
 * 1. 使用前面校准程序获取的ADC上下限值计算准确的弯曲度
 * 2. 适配弯曲时ADC值减小的传感器特性
 * 3. 从所有传感器收集数据(内置IMU和弯曲传感器)
 * 4. 数据预处理和格式化
 * 5. 通过串口将数据发送到Edge Impulse Studio进行模型训练
 * 
 * 使用方法:
 * 1. 将下面的FLEX_XXX_STRAIGHT_ADC和FLEX_XXX_BENT_ADC替换为校准程序输出的值
 * 2. 上传此代码到Arduino Nano 33 BLE
 * 3. 在Edge Impulse Studio创建新项目
 * 4. 在数据采集页面选择"Arduino"作为设备
 * 5. 按照Edge Impulse指南连接设备并开始收集数据
 */

#include <Arduino_LSM9DS1.h>

// 弯曲传感器引脚定义
#define FLEX_PIN_THUMB A0
#define FLEX_PIN_INDEX A1
#define FLEX_PIN_MIDDLE A2
#define FLEX_PIN_RING A3
#define FLEX_PIN_PINKY A4

// ===============================================================
// 【重要】替换为校准程序获得的ADC上下限值
// ===============================================================
// 弯曲传感器校准值 - 平放状态ADC值(STRAIGHT_ADC)
const int FLEX_THUMB_STRAIGHT_ADC = 340;    // 替换为您的实际值
const int FLEX_INDEX_STRAIGHT_ADC = 280;    // 替换为您的实际值
const int FLEX_MIDDLE_STRAIGHT_ADC = 405;   // 替换为您的实际值
const int FLEX_RING_STRAIGHT_ADC = 390;     // 替换为您的实际值
const int FLEX_PINKY_STRAIGHT_ADC = 390;    // 替换为您的实际值

// 弯曲传感器校准值 - 最大弯曲状态ADC值(BENT_ADC)
const int FLEX_THUMB_BENT_ADC = 240;    // 替换为您的实际值 - 注意这里假设弯曲时ADC值减小
const int FLEX_INDEX_BENT_ADC = 220;    // 替换为您的实际值 - 注意这里假设弯曲时ADC值减小
const int FLEX_MIDDLE_BENT_ADC = 360;   // 替换为您的实际值 - 注意这里假设弯曲时ADC值减小
const int FLEX_RING_BENT_ADC = 345;     // 替换为您的实际值 - 注意这里假设弯曲时ADC值减小
const int FLEX_PINKY_BENT_ADC = 335;    // 替换为您的实际值 - 注意这里假设弯曲时ADC值减小
// ===============================================================

// 整合到数组中方便处理
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

// 采样配置
#define FREQUENCY_HZ        50    // 数据采样频率
#define INTERVAL_MS         (1000 / FREQUENCY_HZ)

// 滤波参数
const float ALPHA = 0.2;  // 低通滤波系数

// 存储滤波后的传感器值
float filteredFlexValues[5] = {0};
float filteredAx = 0, filteredAy = 0, filteredAz = 0;
float filteredGx = 0, filteredGy = 0, filteredGz = 0;

// 时间记录
unsigned long lastSampleTime = 0;

// 功能标志
bool isDebugMode = true;  // 是否输出调试信息
bool isCalibrated = false;  // 是否已经校准

/**
 * @brief 弯曲度计算函数 - 正确处理ADC值增大或减小的情况
 */
float calculateBendPercentage(int adcValue, int straightAdc, int bentAdc) {
  // 确保ADC值在范围内，防止极端情况
  adcValue = constrain(adcValue, min(straightAdc, bentAdc), max(straightAdc, bentAdc));
  
  // 计算弯曲百分比 (0% = 直, 100% = 完全弯曲)
  float bendPercentage;
  
  // 处理ADC值增大或减小的情况
  if (bentAdc > straightAdc) {
    // 标准情况：弯曲时ADC值增大
    bendPercentage = map(adcValue, straightAdc, bentAdc, 0, 100);
  } else {
    // 反向情况：弯曲时ADC值减小
    bendPercentage = map(adcValue, straightAdc, bentAdc, 0, 100);
  }
  
  // 确保结果在0-100范围内
  return constrain(bendPercentage, 0, 100);
}

/**
 * @brief 将float映射到指定范围
 */
float map_float(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/**
 * @brief 低通滤波函数
 */
float lowPassFilter(float currentValue, float previousFilteredValue, float alpha) {
  return previousFilteredValue + alpha * (currentValue - previousFilteredValue);
}

/**
 * @brief 读取所有传感器数据
 */
void readAllSensors() {
  // 读取弯曲传感器数据
  int flexRawValues[5];
  flexRawValues[0] = analogRead(FLEX_PIN_THUMB);
  flexRawValues[1] = analogRead(FLEX_PIN_INDEX);
  flexRawValues[2] = analogRead(FLEX_PIN_MIDDLE);
  flexRawValues[3] = analogRead(FLEX_PIN_RING);
  flexRawValues[4] = analogRead(FLEX_PIN_PINKY);
  
  // 将ADC值转换为弯曲百分比并应用滤波
  for (int i = 0; i < 5; i++) {
    float bendPercentage = calculateBendPercentage(
      flexRawValues[i], 
      FLEX_STRAIGHT_ADC[i], 
      FLEX_BENT_ADC[i]
    );
    
    // 应用低通滤波
    filteredFlexValues[i] = lowPassFilter(bendPercentage, filteredFlexValues[i], ALPHA);
  }
  
  // 读取IMU数据
  float ax, ay, az, gx, gy, gz;
  
  if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
    IMU.readAcceleration(ax, ay, az);
    IMU.readGyroscope(gx, gy, gz);
    
    // 应用低通滤波
    filteredAx = lowPassFilter(ax, filteredAx, ALPHA);
    filteredAy = lowPassFilter(ay, filteredAy, ALPHA);
    filteredAz = lowPassFilter(az, filteredAz, ALPHA);
    filteredGx = lowPassFilter(gx, filteredGx, ALPHA);
    filteredGy = lowPassFilter(gy, filteredGy, ALPHA);
    filteredGz = lowPassFilter(gz, filteredGz, ALPHA);
  }
}

/**
 * @brief 发送数据到Edge Impulse Studio - 使用CSV格式
 */
void sendDataToEdgeImpulse() {
  // 创建并发送CSV格式数据
  // 格式: timestamp,value1,value2,...,valueN
  unsigned long timestamp = millis();
  
  // Serial.print(timestamp);
  // Serial.print(",");
  Serial.print(filteredFlexValues[0]);
  Serial.print(",");
  Serial.print(filteredFlexValues[1]);
  Serial.print(",");
  Serial.print(filteredFlexValues[2]);
  Serial.print(",");
  Serial.print(filteredFlexValues[3]);
  Serial.print(",");
  Serial.print(filteredFlexValues[4]);
  Serial.print(",");
  Serial.print(filteredAx);
  Serial.print(",");
  Serial.print(filteredAy);
  Serial.print(",");
  Serial.print(filteredAz);
  Serial.print(",");
  Serial.print(filteredGx);
  Serial.print(",");
  Serial.print(filteredGy);
  Serial.print(",");
  Serial.println(filteredGz);
}

/**
 * @brief 显示传感器校准状态
 */
void printCalibrationStatus() {
  Serial.println("\n当前使用的校准值:");
  
  Serial.println("\n弯曲传感器平放状态ADC值:");
  const char* fingerNames[] = {"拇指(Thumb)", "食指(Index)", "中指(Middle)", "无名指(Ring)", "小指(Pinky)"};
  
  for (int i = 0; i < 5; i++) {
    Serial.print("手指 ");
    Serial.print(i);
    Serial.print(" (");
    Serial.print(fingerNames[i]);
    Serial.print("): ");
    Serial.print(FLEX_STRAIGHT_ADC[i]);
    
    if (FLEX_BENT_ADC[i] < FLEX_STRAIGHT_ADC[i]) {
      Serial.println(" (弯曲时ADC值减小)");
    } else {
      Serial.println(" (弯曲时ADC值增大)");
    }
  }
  
  Serial.println("\n弯曲传感器最大弯曲状态ADC值:");
  for (int i = 0; i < 5; i++) {
    Serial.print("手指 ");
    Serial.print(i);
    Serial.print(" (");
    Serial.print(fingerNames[i]);
    Serial.print("): ");
    Serial.println(FLEX_BENT_ADC[i]);
  }
  
  Serial.println("\n如果这些校准值不准确，请运行校准程序重新校准。");
  Serial.println("数据流将在5秒后开始发送...");
}

/**
 * @brief 显示当前传感器数据和映射百分比
 */
void printSensorData() {
  const char* fingerNames[] = {"Thumb", "Index", "Middle", "Ring", "Pinky"};
  int flexRawValues[5];
  
  // 读取原始ADC值
  flexRawValues[0] = analogRead(FLEX_PIN_THUMB);
  flexRawValues[1] = analogRead(FLEX_PIN_INDEX);
  flexRawValues[2] = analogRead(FLEX_PIN_MIDDLE);
  flexRawValues[3] = analogRead(FLEX_PIN_RING);
  flexRawValues[4] = analogRead(FLEX_PIN_PINKY);
  
  Serial.println("\n当前传感器数据:");
  
  Serial.println("弯曲传感器值:");
  for (int i = 0; i < 5; i++) {
    Serial.print(fingerNames[i]);
    Serial.print(": ADC=");
    Serial.print(flexRawValues[i]);
    Serial.print(", 弯曲度=");
    Serial.print(filteredFlexValues[i]);
    Serial.println("%");
  }
  
  Serial.println("\nIMU数据:");
  Serial.print("加速度 (g): X=");
  Serial.print(filteredAx);
  Serial.print(", Y=");
  Serial.print(filteredAy);
  Serial.print(", Z=");
  Serial.println(filteredAz);
  
  Serial.print("陀螺仪 (dps): X=");
  Serial.print(filteredGx);
  Serial.print(", Y=");
  Serial.print(filteredGy);
  Serial.print(", Z=");
  Serial.println(filteredGz);
}

/**
 * @brief 初始化函数
 */
void setup() {
  // 初始化串口通信
  Serial.begin(115200);
  
  // 初始化LED
  pinMode(LED_BUILTIN, OUTPUT);
  
  // 等待2秒以便Edge Impulse客户端连接
  delay(2000);
  
  // 初始化IMU
  if (!IMU.begin()) {
    if (isDebugMode) {
      Serial.println("IMU初始化失败!");
    }
    while (1) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  }
  
  if (isDebugMode) {
    Serial.println("==================================================");
    Serial.println("| 手语翻译手套 - Edge Impulse数据收集程序(校准版) |");
    Serial.println("==================================================");
    Serial.println("\nIMU初始化成功");
    Serial.print("加速度计采样率: ");
    Serial.print(IMU.accelerationSampleRate());
    Serial.println(" Hz");
    Serial.print("陀螺仪采样率: ");
    Serial.print(IMU.gyroscopeSampleRate());
    Serial.println(" Hz");
    
    // 显示当前校准状态
    printCalibrationStatus();
  }
  
  // 进行初始化读取，确保滤波器有初始值
  for (int i = 0; i < 10; i++) {
    readAllSensors();
    delay(10);
  }
  
  // 5秒倒计时
  for (int i = 5; i > 0; i--) {
    Serial.print(i);
    Serial.println("...");
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }
  
  if (isDebugMode) {
    Serial.println("\n开始发送数据到Edge Impulse...");
    Serial.println("当前采样频率: " + String(FREQUENCY_HZ) + " Hz");
    Serial.println("请在Edge Impulse Studio开始采样。");
    Serial.println("==================================================");
  }
  
  // 表示准备就绪
  isCalibrated = true;
}

/**
 * @brief 主循环函数
 */
void loop() {
  unsigned long currentMillis = millis();
  
  // 按照固定频率采样
  if (currentMillis - lastSampleTime >= INTERVAL_MS) {
    lastSampleTime = currentMillis;
    
    // 采集传感器数据
    readAllSensors();
    
    // 发送数据到Edge Impulse
    sendDataToEdgeImpulse();
    
    // LED闪烁表示数据发送
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
  
  // 处理来自Edge Impulse或串口的命令
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    // 处理特定命令
    if (command == "STATUS") {
      Serial.println("{'status':'OK'}");
    } else if (command == "info") {
      // 显示当前传感器数据
      printSensorData();
    } else if (command == "raw") {
      // 显示原始ADC值
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
    }
  }
}