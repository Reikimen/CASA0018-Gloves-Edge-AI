/*
 * 手语翻译手套 - 进阶阶段代码 (Edge Impulse数据收集)
 * 
 * 功能:
 * 1. 从所有传感器收集数据(内置IMU和弯曲传感器)
 * 2. 数据预处理和格式化
 * 3. 通过串口将数据发送到Edge Impulse Studio进行模型训练
 * 4. 支持采样模式和连续流模式
 * 
 * 硬件连接:
 * - 弯曲传感器1-5: A0-A4 (对应五根手指)
 * - IMU: 内置在Nano 33 BLE板上
 * - LED指示灯: 内置LED (PIN 13)
 * 
 * 使用方法:
 * 1. 将此代码上传到Arduino Nano 33 BLE
 * 2. 在Edge Impulse Studio创建新项目
 * 3. 在数据采集页面选择"Arduino"作为设备
 * 4. 按照Edge Impulse指南连接设备
 * 5. 开始收集手语动作的数据样本
 */

/* Edge Impulse Arduino 示例
 * 为Arduino Nano 33 BLE Sense修改
 */

// 如果在Windows上运行，取消下一行的注释以避免行尾问题
// #define ARDUINO_ARCH_WINDOWS

#include <Arduino_LSM9DS1.h>

// 弯曲传感器引脚定义
#define FLEX_PIN_THUMB A0
#define FLEX_PIN_INDEX A1
#define FLEX_PIN_MIDDLE A2
#define FLEX_PIN_RING A3
#define FLEX_PIN_PINKY A4

// 弯曲传感器校准值
const float FLEX_STRAIGHT_RESISTANCE = 10000.0; // 平直时电阻值
const float FLEX_BENT_RESISTANCE = 40000.0;     // 弯曲时电阻值
const float VCC = 3.3;                          // 供电电压
const float R_DIV = 10000.0;                    // 分压电阻值

// 采样配置
#define FREQUENCY_HZ        5    // 数据采样频率
#define INTERVAL_MS         (1000 / FREQUENCY_HZ)
#define CONVERT_G_TO_MS2    9.80665f

// 数据包大小定义
#define MAX_ACCEPTED_RANGE  2.0f  // 加速度计, G
#define SCALE_MULTIPLIER    4096.0f

// 传感器数据数组大小 (5个弯曲传感器 + 6轴IMU数据)
#define SENSOR_DATA_SIZE    11

// 功能标志
bool isDebugMode = true;  // 是否输出调试信息

// 时间记录
unsigned long lastSampleTime = 0;
unsigned long lastPrintTime = 0;

// 滤波参数
const float ALPHA = 0.2;  // 低通滤波系数

// 存储滤波后的传感器值
float filteredFlexValues[5] = {0};
float filteredAx = 0, filteredAy = 0, filteredAz = 0;
float filteredGx = 0, filteredGy = 0, filteredGz = 0;

// 数据缓冲区
typedef struct {
    float flex_thumb;
    float flex_index;
    float flex_middle;
    float flex_ring;
    float flex_pinky;
    float acc_x;
    float acc_y;
    float acc_z;
    float gyr_x;
    float gyr_y;
    float gyr_z;
} SensorData;

// 存储传感器读数
SensorData sensorData;

/**
 * @brief 弯曲度计算函数
 * 将模拟读数转换为弯曲百分比
 */
float calculateBendPercentage(int adcValue, float straightResistance, float bentResistance) {
  // 将ADC值转换为电阻值
  float Vflex = adcValue * VCC / 1023.0;
  float Rflex = R_DIV * (VCC / Vflex - 1.0);
  
  // 计算弯曲百分比 (0% = 直, 100% = 完全弯曲)
  float bendPercentage = map(Rflex, straightResistance, bentResistance, 0, 100);
  
  // 限制在0-100范围内
  if (bendPercentage < 0) bendPercentage = 0;
  if (bendPercentage > 100) bendPercentage = 100;
  
  return bendPercentage;
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
      FLEX_STRAIGHT_RESISTANCE, 
      FLEX_BENT_RESISTANCE
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
  
  // 更新传感器数据结构
  sensorData.flex_thumb = filteredFlexValues[0];
  sensorData.flex_index = filteredFlexValues[1];
  sensorData.flex_middle = filteredFlexValues[2];
  sensorData.flex_ring = filteredFlexValues[3];
  sensorData.flex_pinky = filteredFlexValues[4];
  sensorData.acc_x = filteredAx;
  sensorData.acc_y = filteredAy;
  sensorData.acc_z = filteredAz;
  sensorData.gyr_x = filteredGx;
  sensorData.gyr_y = filteredGy;
  sensorData.gyr_z = filteredGz;
}

/**
 * @brief 发送数据到Edge Impulse Studio
 */
void sendDataToEdgeImpulse() {
  // 创建数据缓冲区
  float buffer[SENSOR_DATA_SIZE];
  
  // 将传感器数据填充到缓冲区
  buffer[0] = sensorData.flex_thumb;
  buffer[1] = sensorData.flex_index;
  buffer[2] = sensorData.flex_middle;
  buffer[3] = sensorData.flex_ring;
  buffer[4] = sensorData.flex_pinky;
  buffer[5] = sensorData.acc_x;
  buffer[6] = sensorData.acc_y;
  buffer[7] = sensorData.acc_z;
  buffer[8] = sensorData.gyr_x;
  buffer[9] = sensorData.gyr_y;
  buffer[10] = sensorData.gyr_z;
  
  // 发送数据到Edge Impulse
  // 按照Edge Impulse数据收集格式化
  Serial.print("{'flex_thumb':");
  Serial.print(buffer[0]);
  Serial.print(", 'flex_index':");
  Serial.print(buffer[1]);
  Serial.print(", 'flex_middle':");
  Serial.print(buffer[2]);
  Serial.print(", 'flex_ring':");
  Serial.print(buffer[3]);
  Serial.print(", 'flex_pinky':");
  Serial.print(buffer[4]);
  Serial.print(", 'acc_x':");
  Serial.print(buffer[5]);
  Serial.print(", 'acc_y':");
  Serial.print(buffer[6]);
  Serial.print(", 'acc_z':");
  Serial.print(buffer[7]);
  Serial.print(", 'gyr_x':");
  Serial.print(buffer[8]);
  Serial.print(", 'gyr_y':");
  Serial.print(buffer[9]);
  Serial.print(", 'gyr_z':");
  Serial.print(buffer[10]);
  Serial.println("}");
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
    Serial.println("手语翻译手套 - Edge Impulse数据收集");
    Serial.println("IMU初始化成功");
    Serial.print("加速度计采样率: ");
    Serial.print(IMU.accelerationSampleRate());
    Serial.println(" Hz");
    Serial.print("陀螺仪采样率: ");
    Serial.print(IMU.gyroscopeSampleRate());
    Serial.println(" Hz");
    Serial.println("等待数据接收命令...");
  }
  
  // 进行简单校准
  if (isDebugMode) {
    Serial.println("进行传感器校准，请将手保持平放...");
  }
  digitalWrite(LED_BUILTIN, HIGH);
  for (int i = 0; i < 20; i++) {
    readAllSensors();  // 读取数据进行初始化
    delay(100);
  }
  digitalWrite(LED_BUILTIN, LOW);
  
  if (isDebugMode) {
    Serial.println("校准完成，可以开始数据收集");
    Serial.println("等待Edge Impulse Studio指令...");
  }
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
  
  // 处理来自Edge Impulse的命令
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    // 可以添加对特定命令的响应，如设置采样率等
    if (command == "STATUS") {
      Serial.println("{'status':'OK'}");
    }
  }
}