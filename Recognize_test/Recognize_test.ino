/*
 * 手语翻译手套 - 手势识别程序(one-five & love)
 * 
 * 功能:
 * 1. 使用校准的ADC值读取弯曲传感器和IMU数据
 * 2. 使用Edge Impulse导出的模型识别数字和特殊手势
 * 3. 在串口显示识别结果
 * 
 * 使用方法:
 * 1. 从Edge Impulse导出Arduino库并添加到项目
 * 2. 替换下面的Sign-Language-Glove_inferencing.h为您的实际项目名
 * 3. 上传此代码到Arduino Nano 33 BLE
 * 4. 打开串口监视器(115200波特率)查看识别结果
 */

#include <Arduino_LSM9DS1.h>
// 替换为您从Edge Impulse导出的库的头文件
#include <Sign-Language-Glove_inferencing.h> // ← 根据需要修改为您的实际项目名称

// 弯曲传感器引脚定义
#define FLEX_PIN_THUMB A0
#define FLEX_PIN_INDEX A1
#define FLEX_PIN_MIDDLE A2
#define FLEX_PIN_RING A3
#define FLEX_PIN_PINKY A4

// 弯曲传感器校准值 - 平放状态ADC值(STRAIGHT_ADC)
const int FLEX_THUMB_STRAIGHT_ADC = 330;    // 替换为您的实际值
const int FLEX_INDEX_STRAIGHT_ADC = 440;    // 替换为您的实际值
const int FLEX_MIDDLE_STRAIGHT_ADC = 400;   // 替换为您的实际值
const int FLEX_RING_STRAIGHT_ADC = 430;     // 替换为您的实际值
const int FLEX_PINKY_STRAIGHT_ADC = 390;    // 替换为您的实际值

// 弯曲传感器校准值 - 最大弯曲状态ADC值(BENT_ADC)
const int FLEX_THUMB_BENT_ADC = 220;    // 替换为您的实际值 - 注意这里假设弯曲时ADC值减小
const int FLEX_INDEX_BENT_ADC = 360;    // 替换为您的实际值 - 注意这里假设弯曲时ADC值减小
const int FLEX_MIDDLE_BENT_ADC = 320;   // 替换为您的实际值 - 注意这里假设弯曲时ADC值减小
const int FLEX_RING_BENT_ADC = 335;     // 替换为您的实际值 - 注意这里假设弯曲时ADC值减小
const int FLEX_PINKY_BENT_ADC = 320;    // 替换为您的实际值 - 注意这里假设弯曲时ADC值减小

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

// 采样与推理配置
#define INFERENCE_INTERVAL_MS   300  // 每300ms进行一次推理
#define CONFIDENCE_THRESHOLD    0.60 // 识别置信度阈值(0.0-1.0)

// 滤波参数
const float ALPHA = 0.3;  // 低通滤波系数

// 存储滤波后的传感器值
float filteredFlexValues[5] = {0};
float filteredAx = 0, filteredAy = 0, filteredAz = 0;
float filteredGx = 0, filteredGy = 0, filteredGz = 0;

// 时间记录
unsigned long lastInferenceTime = 0;

// 模型输入特征缓冲区
float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

// 手势识别状态变量
String lastRecognizedGesture = "";
int stableCount = 0;  // 稳定计数器，用于减少识别抖动
int noGestureCount = 0; // 计数器，用于检测没有手势的状态

// 手势标签和描述
const char* GESTURE_LABELS[] = {"one", "two", "three", "four", "five", "love"};
const char* GESTURE_DESCRIPTIONS[] = {
  "数字1 (食指伸出)",
  "数字2 (食指和中指伸出)",
  "数字3 (拇指、食指和中指伸出)",
  "数字4 (四指伸出，拇指收起)",
  "数字5 (五指全部伸开)",
  "爱心手势 (拇指和食指伸出成心形)"
};

// 定义静态回调函数用于获取特征数据
static int get_signal_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

/**
 * @brief 弯曲度计算函数 - 处理ADC值增大或减小的情况
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
 * @brief 准备特征数据用于推理
 */
void prepareFeatures() {
  // 重要: 确保特征顺序与Edge Impulse中相同
  features[0] = filteredFlexValues[0]; // 拇指
  features[1] = filteredFlexValues[1]; // 食指
  features[2] = filteredFlexValues[2]; // 中指
  features[3] = filteredFlexValues[3]; // 无名指
  features[4] = filteredFlexValues[4]; // 小指
  features[5] = filteredAx;
  features[6] = filteredAy;
  features[7] = filteredAz;
  features[8] = filteredGx;
  features[9] = filteredGy;
  features[10] = filteredGz;
}

/**
 * @brief 获取手势的友好描述
 */
const char* getGestureDescription(const char* label) {
  // 判断是否为预定义的手势，并返回对应描述
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
    // 如果不是已知手势，返回原始标签
    return label;
  }
}

/**
 * @brief 运行推理并处理结果
 */
void runInference() {
  // 准备输入特征
  prepareFeatures();
  
  // 存储推理结果
  ei_impulse_result_t result;
  
  // 创建signal_t结构
  signal_t signal;
  signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
  signal.get_data = &get_signal_data;
  
  // 执行推理 - 使用signal_t结构
  EI_IMPULSE_ERROR ei_error = run_classifier(&signal, &result, false);
  
  // 如果推理成功，处理结果
  if (ei_error == EI_IMPULSE_OK) {
    // 找出置信度最高的手势
    float maxScore = 0;
    size_t maxIndex = 0;
    
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
      if (result.classification[i].value > maxScore) {
        maxScore = result.classification[i].value;
        maxIndex = i;
      }
    }
    
    // 超过置信度阈值才认为是有效手势
    if (maxScore > CONFIDENCE_THRESHOLD) {
      String gesture = String(result.classification[maxIndex].label);
      
      // 连续识别同一手势2次后才输出结果
      if (gesture == lastRecognizedGesture) {
        stableCount++;
        if (stableCount >= 2 && stableCount % 2 == 0) {  // 每2次稳定更新输出一次
          // 获取手势描述
          const char* gestureDesc = getGestureDescription(gesture.c_str());
          
          // 显示识别结果
          Serial.print("识别到手势: ");
          Serial.print(gesture);
          Serial.print(" - ");
          Serial.print(gestureDesc);
          Serial.print(" (");
          Serial.print(maxScore * 100);
          Serial.println("%)");
          
          // LED闪烁表示成功识别
          digitalWrite(LED_BUILTIN, HIGH);
          delay(50);
          digitalWrite(LED_BUILTIN, LOW);
          
          // 重置无手势计数器
          noGestureCount = 0;
        }
      } else {
        // 新手势，重置稳定计数
        lastRecognizedGesture = gesture;
        stableCount = 1;
      }
    } else {
      // 低于阈值，可能是噪声或过渡状态
      noGestureCount++;
      
      // 如果连续10次没有识别到手势，清除上一次识别的结果
      if (noGestureCount > 10) {
        if (lastRecognizedGesture != "") {
          Serial.println("手势已释放");
          lastRecognizedGesture = "";
        }
        stableCount = 0;
      }
    }
  } else {
    Serial.println("推理错误");
  }
}

/**
 * @brief 显示当前传感器数据
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
 * @brief 显示支持的手势列表
 */
void printGestureList() {
  Serial.println("\n支持的手势列表:");
  
  // 手动列出所有标签及其描述
  for (int i = 0; i < 6; i++) {  // 更新为6个手势
    Serial.print("  ");
    Serial.print(i + 1);
    Serial.print(". ");
    Serial.print(GESTURE_LABELS[i]);
    Serial.print(" - ");
    Serial.println(GESTURE_DESCRIPTIONS[i]);
  }
  
  // 如果还有其他标签(以防模型中有额外标签)
  for (size_t i = 6; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    Serial.print("  ");
    Serial.print(i + 1);
    Serial.print(". ");
    Serial.println(ei_classifier_inferencing_categories[i]);
  }
}

/**
 * @brief 初始化函数
 */
void setup() {
  // 初始化串口通信
  Serial.begin(115200);
  delay(3000); // 等待串口连接，固定延时3秒
  
  // 初始化LED
  pinMode(LED_BUILTIN, OUTPUT);
  
  // 初始化IMU
  if (!IMU.begin()) {
    Serial.println("IMU初始化失败!");
    while (1) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  }
  
  Serial.println("==================================================");
  Serial.println("|      手语翻译手套 - 手势识别程序              |");
  Serial.println("==================================================");
  Serial.println("\nIMU初始化成功");
  Serial.print("加速度计采样率: ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.print("陀螺仪采样率: ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");
  
  // 显示模型信息
  Serial.println("\nEdge Impulse模型信息:");
  Serial.print("模型名称: ");
  Serial.println(EI_CLASSIFIER_PROJECT_NAME);
  Serial.print("模型ID: ");
  Serial.println(EI_CLASSIFIER_PROJECT_ID);
  Serial.print("支持的手势类别数: ");
  Serial.println(EI_CLASSIFIER_LABEL_COUNT);
  
  // 显示所有支持的手势
  printGestureList();
  
  // 进行初始化读取，确保滤波器有初始值
  for (int i = 0; i < 10; i++) {
    readAllSensors();
    delay(10);
  }
  
  Serial.println("\n准备就绪，开始识别手势...");
  Serial.println("--------------------------------------------------");
  Serial.println("命令菜单:");
  Serial.println("  info - 显示当前传感器数据");
  Serial.println("  raw - 显示原始ADC值");
  Serial.println("  list - 显示支持的手势列表");
  Serial.println("  finger - 显示指尖弯曲角度示意图");
  Serial.println("--------------------------------------------------");
  
  // 短闪LED表示初始化完成
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

/**
 * @brief 显示指尖弯曲角度可视化
 */
void printFingerBending() {
  Serial.println("\n指尖弯曲角度可视化:");
  Serial.println("0%=完全伸直, 100%=完全弯曲");
  Serial.println("T: 拇指, I: 食指, M: 中指, R: 无名指, P: 小指");
  
  // 绘制刻度
  Serial.println("  0%      25%      50%      75%     100%");
  Serial.println("  |        |        |        |        |");
  
  // 为每个手指生成可视化条
  const char* fingerLabels[] = {"T:", "I:", "M:", "R:", "P:"};
  for (int i = 0; i < 5; i++) {
    // 指尖标签
    Serial.print(fingerLabels[i]);
    
    // 计算位置(0-50个字符)
    int position = map(filteredFlexValues[i], 0, 100, 0, 50);
    
    // 绘制条
    for (int j = 0; j < 50; j++) {
      if (j == position) {
        Serial.print("O"); // 当前位置
      } else if (j < position) {
        Serial.print("-"); // 已弯曲部分
      } else {
        Serial.print(" "); // 未弯曲部分
      }
    }
    Serial.println();
  }
}

/**
 * @brief 主循环函数
 */
void loop() {
  // 读取所有传感器数据
  readAllSensors();
  
  // 定期执行推理
  unsigned long currentMillis = millis();
  if (currentMillis - lastInferenceTime >= INFERENCE_INTERVAL_MS) {
    lastInferenceTime = currentMillis;
    
    // 运行推理并处理结果
    runInference();
  }
  
  // 处理来自串口的命令
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "info") {
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
    } else if (command == "list") {
      // 显示支持的手势列表
      printGestureList();
    } else if (command == "finger") {
      // 显示指尖弯曲角度可视化
      printFingerBending();
    }
  }
}