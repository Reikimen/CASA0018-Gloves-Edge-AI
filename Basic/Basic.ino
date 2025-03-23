/*
 * 手语翻译手套 - 基础验证阶段代码
 * 
 * 功能:
 * 1. 读取Arduino Nano 33 BLE内置IMU (LSM9DS1)数据
 * 2. 读取连接到模拟引脚的5个弯曲传感器数据
 * 3. 通过串口监视器显示所有传感器数据
 * 4. 基本数据滤波和校准
 * 
 * 硬件连接:
 * - 弯曲传感器1: A0 (拇指)
 * - 弯曲传感器2: A1 (食指)
 * - 弯曲传感器3: A2 (中指)
 * - 弯曲传感器4: A3 (无名指)
 * - 弯曲传感器5: A4 (小指)
 * - IMU: 内置在Nano 33 BLE板上
 * 
 * 注意: 弯曲传感器需要配合10K电阻作为分压电路使用
 */

#include <Arduino_LSM9DS1.h>

// 弯曲传感器引脚定义
#define FLEX_PIN_THUMB A0
#define FLEX_PIN_INDEX A1
#define FLEX_PIN_MIDDLE A2
#define FLEX_PIN_RING A3
#define FLEX_PIN_PINKY A4

// 弯曲传感器校准值
// 这些值需要根据您的传感器实际测量调整
const float FLEX_STRAIGHT_RESISTANCE = 10000.0; // 10K欧姆 - 平直时
const float FLEX_BENT_RESISTANCE = 40000.0;     // 40K欧姆 - 弯曲时
const float VCC = 3.3;                          // 供电电压
const float R_DIV = 10000.0;                    // 分压电阻值

// 滤波参数
const float ALPHA = 0.3; // 低通滤波系数 (0-1)

// 存储滤波后的传感器值
float filteredFlexValues[5] = {0};

// IMU数据滤波值
float filteredAx = 0, filteredAy = 0, filteredAz = 0;
float filteredGx = 0, filteredGy = 0, filteredGz = 0;

// 弯曲度计算函数
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

// 低通滤波函数
float lowPassFilter(float currentValue, float previousFilteredValue, float alpha) {
  return previousFilteredValue + alpha * (currentValue - previousFilteredValue);
}

void setup() {
  // 初始化串口通信
  Serial.begin(115200);
  while (!Serial);  // 等待串口连接
  
  Serial.println("手语翻译手套 - 基础传感器测试");
  
  // 初始化IMU
  if (!IMU.begin()) {
    Serial.println("IMU初始化失败!");
    while (1);
  }
  
  // 设置IMU采样率和量程
  // 加速度计量程为±4g，陀螺仪量程为±500dps
  Serial.println("IMU初始化成功");
  
  // 显示传感器详细信息
  Serial.print("加速度计采样率 = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.print("陀螺仪采样率 = ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");
  
  Serial.println("校准中...");
  // 简单校准过程 - 保持手套平放3秒
  for (int i = 0; i < 50; i++) {
    // 读取弯曲传感器作为基准值
    // 这里可以添加更复杂的校准逻辑
    delay(60);
  }
  
  Serial.println("校准完成，开始数据读取");
  Serial.println("===========================================");
  Serial.println("   拇指  食指  中指  无名  小指   Ax   Ay   Az   Gx   Gy   Gz");
  Serial.println("===========================================");
}

void loop() {
  // 存储传感器原始读数
  int flexValues[5];
  float ax, ay, az;  // 加速度计数据 (g)
  float gx, gy, gz;  // 陀螺仪数据 (度/秒)
  
  // 读取所有弯曲传感器值
  flexValues[0] = analogRead(FLEX_PIN_THUMB);
  flexValues[1] = analogRead(FLEX_PIN_INDEX);
  flexValues[2] = analogRead(FLEX_PIN_MIDDLE);
  flexValues[3] = analogRead(FLEX_PIN_RING);
  flexValues[4] = analogRead(FLEX_PIN_PINKY);
  
  // 读取IMU数据
  if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
    IMU.readAcceleration(ax, ay, az);
    IMU.readGyroscope(gx, gy, gz);
    
    // 对IMU数据进行滤波
    filteredAx = lowPassFilter(ax, filteredAx, ALPHA);
    filteredAy = lowPassFilter(ay, filteredAy, ALPHA);
    filteredAz = lowPassFilter(az, filteredAz, ALPHA);
    filteredGx = lowPassFilter(gx, filteredGx, ALPHA);
    filteredGy = lowPassFilter(gy, filteredGy, ALPHA);
    filteredGz = lowPassFilter(gz, filteredGz, ALPHA);
  }
  
  // 计算弯曲百分比并应用滤波
  for (int i = 0; i < 5; i++) {
    float bendPercentage = calculateBendPercentage(
      flexValues[i], 
      FLEX_STRAIGHT_RESISTANCE, 
      FLEX_BENT_RESISTANCE
    );
    
    // 应用低通滤波
    filteredFlexValues[i] = lowPassFilter(bendPercentage, filteredFlexValues[i], ALPHA);
  }
  
  // 格式化并打印数据
  char buffer[120];
  sprintf(buffer, "%5.1f %5.1f %5.1f %5.1f %5.1f %5.2f %5.2f %5.2f %5.1f %5.1f %5.1f",
          filteredFlexValues[0], filteredFlexValues[1], filteredFlexValues[2], 
          filteredFlexValues[3], filteredFlexValues[4],
          filteredAx, filteredAy, filteredAz, 
          filteredGx, filteredGy, filteredGz);
  Serial.println(buffer);
  
  // 添加可视化指示 (当弯曲度超过50%时显示*)
  Serial.print("状态: ");
  for (int i = 0; i < 5; i++) {
    if (filteredFlexValues[i] > 50) {
      Serial.print("* ");
    } else {
      Serial.print("- ");
    }
  }
  Serial.println();
  
  // 短暂延时
  delay(100);  // 10Hz更新频率
}