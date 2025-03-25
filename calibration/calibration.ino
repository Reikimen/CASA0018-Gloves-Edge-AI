/*
 * 手语翻译手套 - 传感器校准程序
 * 
 * 功能:
 * 1. 读取5个弯曲传感器的ADC原始读数
 * 2. 记录平放状态(未弯曲)和最大弯曲状态的ADC值
 * 3. 将校准结果通过串口输出，用于后续程序中的参数设置
 * 
 * 使用方法:
 * 1. 上传程序到Arduino Nano 33 BLE
 * 2. 打开串口监视器(波特率115200)
 * 3. 按照提示操作，分别记录每个手指的平放和最大弯曲ADC值
 * 4. 记录输出的校准结果，用于数据收集程序
 */

// 弯曲传感器引脚定义
#define FLEX_PIN_THUMB A0
#define FLEX_PIN_INDEX A1
#define FLEX_PIN_MIDDLE A2
#define FLEX_PIN_RING A3
#define FLEX_PIN_PINKY A4

// 手指名称数组，用于输出
const char* FINGER_NAMES[] = {"拇指", "食指", "中指", "无名指", "小指"};

// 弯曲传感器引脚数组
const int FLEX_PINS[] = {FLEX_PIN_THUMB, FLEX_PIN_INDEX, FLEX_PIN_MIDDLE, FLEX_PIN_RING, FLEX_PIN_PINKY};

// 存储校准值的数组
int flexStraightValues[5]; // 平放状态(未弯曲)的ADC值
int flexBentValues[5];     // 最大弯曲状态的ADC值

// 当前校准状态
enum CalibrationState {
  READY_FOR_STRAIGHT,  // 准备记录平放状态
  READY_FOR_BENT,      // 准备记录弯曲状态
  FINISHED             // 校准完成
};

CalibrationState currentState = READY_FOR_STRAIGHT;
int currentFinger = 0; // 当前校准的手指索引

void setup() {
  // 初始化串口通信
  Serial.begin(115200);
  while (!Serial);  // 等待串口连接
  
  // 初始化LED
  pinMode(LED_BUILTIN, OUTPUT);
  
  // 输出欢迎信息
  Serial.println("==============================================");
  Serial.println("| 手语翻译手套 - 弯曲传感器校准程序       |");
  Serial.println("==============================================");
  Serial.println();
  Serial.println("本程序用于校准5个弯曲传感器的ADC读数范围。");
  Serial.println("请按照提示依次完成每个手指的校准过程。");
  Serial.println();
  
  // 开始校准第一个手指的平放状态
  promptForCalibration();
}

void loop() {
  // 读取所有弯曲传感器的当前ADC值并显示
  Serial.println("当前ADC读数:");
  for (int i = 0; i < 5; i++) {
    int value = analogRead(FLEX_PINS[i]);
    Serial.print(FINGER_NAMES[i]);
    Serial.print(": ");
    Serial.println(value);
  }
  Serial.println("------------------------------");
  
  // 检测是否有用户输入
  if (Serial.available() > 0) {
    // 读取用户输入
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    // 判断输入
    if (input == "c" || input == "C") {
      // 捕获当前读数
      captureCurrentReading();
    } else if (input == "r" || input == "R") {
      // 重新开始校准
      resetCalibration();
    }
  }
  
  // 延时500毫秒再读取下一个值
  delay(500);
}

// 显示当前校准步骤的提示
void promptForCalibration() {
  if (currentFinger < 5) {
    if (currentState == READY_FOR_STRAIGHT) {
      Serial.print("请将 ");
      Serial.print(FINGER_NAMES[currentFinger]);
      Serial.println(" 保持平放(未弯曲)状态。");
      Serial.println("准备好后，输入字母'c'并按回车确认。");
    } else if (currentState == READY_FOR_BENT) {
      Serial.print("请将 ");
      Serial.print(FINGER_NAMES[currentFinger]);
      Serial.println(" 弯曲到最大角度。");
      Serial.println("准备好后，输入字母'c'并按回车确认。");
    }
  } else {
    // 所有手指校准完成
    displayCalibrationResults();
    currentState = FINISHED;
  }
}

// 捕获当前读数
void captureCurrentReading() {
  if (currentFinger < 5) {
    int value = analogRead(FLEX_PINS[currentFinger]);
    
    if (currentState == READY_FOR_STRAIGHT) {
      // 记录平放状态读数
      flexStraightValues[currentFinger] = value;
      Serial.print(FINGER_NAMES[currentFinger]);
      Serial.print(" 平放状态ADC值: ");
      Serial.println(value);
      
      // 切换到弯曲状态校准
      currentState = READY_FOR_BENT;
      promptForCalibration();
    } else if (currentState == READY_FOR_BENT) {
      // 记录弯曲状态读数
      flexBentValues[currentFinger] = value;
      Serial.print(FINGER_NAMES[currentFinger]);
      Serial.print(" 最大弯曲状态ADC值: ");
      Serial.println(value);
      
      // 闪烁LED指示此手指校准完成
      flashLED(2);
      
      // 进入下一个手指的校准
      currentFinger++;
      currentState = READY_FOR_STRAIGHT;
      
      Serial.println();
      promptForCalibration();
    }
  }
}

// 重置校准过程
void resetCalibration() {
  Serial.println("重新开始校准过程...");
  currentFinger = 0;
  currentState = READY_FOR_STRAIGHT;
  promptForCalibration();
}

// 显示校准结果
void displayCalibrationResults() {
  Serial.println("\n==============================================");
  Serial.println("| 校准完成! 校准结果如下:                  |");
  Serial.println("==============================================");
  
  Serial.println("\n在您的数据收集程序中，请使用以下校准值:");
  Serial.println("\n// 弯曲传感器校准值 - 平放状态ADC值(STRAIGHT_ADC)");
  for (int i = 0; i < 5; i++) {
    Serial.print("const int FLEX_");
    Serial.print(FINGER_NAMES[i]);
    Serial.print("_STRAIGHT_ADC = ");
    Serial.print(flexStraightValues[i]);
    Serial.println(";");
  }
  
  Serial.println("\n// 弯曲传感器校准值 - 最大弯曲状态ADC值(BENT_ADC)");
  for (int i = 0; i < 5; i++) {
    Serial.print("const int FLEX_");
    Serial.print(FINGER_NAMES[i]);
    Serial.print("_BENT_ADC = ");
    Serial.print(flexBentValues[i]);
    Serial.println(";");
  }
  
  Serial.println("\n请复制这些值到您的数据收集程序中。");
  Serial.println("若需重新校准，输入字母'r'并按回车。");
  
  // 三次闪烁LED指示校准完成
  flashLED(3);
}

// LED闪烁
void flashLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}