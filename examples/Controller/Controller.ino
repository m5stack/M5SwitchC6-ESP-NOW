#include <M5Unified.h>
#include <WiFi.h>
#include <M5Switchc6.h>

// 创建 M5SwitchC6 实例，最大节点数量为16，可自定义大小 / Create M5SwitchC6 instance with max peers set to 16, customizable size
M5SwitchC6 switchC6(16);
//填入你的SwitchC6设备MAC地址 / Fill in your SwitchC6 device MAC address
const String DEVICE_MAC = "E4B3-2386-18C4";

//是否启用wifi连接 / Enable Wi-Fi connection 
// #define USE_WIFI
#ifdef USE_WIFI
const char *ssid     = "your_ssid";
const char *password = "your_password";
#endif

// 等待动画控制变量 / Control flag for waiting animation
volatile bool showAnimation = false;
TaskHandle_t animationTaskHandle = NULL;
SemaphoreHandle_t displayMutex = NULL;

// 等待动画任务 / Waiting animation task
void animationTask(void *parameter) {
    while (true) {
        if (showAnimation) {
            // 获取互斥锁 / Acquire mutex
            if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                int centerX = M5.Display.width() / 2;
                int centerY = M5.Display.height() / 2;
                static int dots = 1;
                
                // 清除点号区域 - 只清除动画区域 / Clear dots area (only the animation region)
                M5.Display.fillRect(centerX - 30, centerY, 60, 20, BLACK);
                
                // 显示当前数量的点号 / Draw current number of dots
                M5.Display.setCursor(centerX - dots * 6, centerY);
                M5.Display.setTextColor(WHITE);
                for (int i = 0; i < dots; i++) {
                    M5.Display.print(".");
                }
                
                // 释放互斥锁 / Release mutex
                xSemaphoreGive(displayMutex);
                
                dots++;
                if (dots > 5) {
                    dots = 1;
                }
            }
            
            vTaskDelay(200 / portTICK_PERIOD_MS); // 200ms延迟 / 200 ms delay
        } else {
            // 清除动画时也需要获取锁 / Also need the lock when clearing animation
            if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                int centerX = M5.Display.width() / 2;
                int centerY = M5.Display.height() / 2 + 20;
                
                // 清除动画区域 / Clear animation region
                M5.Display.fillRect(centerX - 30, centerY - 10, 60, 30, BLACK);
                 
                
                xSemaphoreGive(displayMutex);
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}

// 开始等待动画 / Start waiting animation
void startWaitingAnimation() {
    showAnimation = true;
}

// 停止等待动画 / Stop waiting animation
void stopWaitingAnimation() {
    showAnimation = false;
}

void setup() {
    Serial.begin(115200);
    M5.begin();
    // M5.Display.setRotation(1);
    M5.Display.setTextSize(2);
  
    WiFi.mode(WIFI_STA);

#ifdef USE_WIFI
    // 连接到Wi-Fi网络 / Connect to Wi-Fi network
    WiFi.begin(ssid, password);
    if (WiFi.status() != WL_CONNECTED) {
        Serial.print("Connecting to Wi-Fi");
        while (WiFi.status() != WL_CONNECTED) {
            Serial.print(".");
            delay(500);
        }
        Serial.println("\nConnected to Wi-Fi!");
        // 串口打印连接信息 / Print connection info to serial
        Serial.printf("SSID: %s\r\n", WiFi.SSID().c_str());
        Serial.printf("IP Address: %s\r\n", WiFi.localIP().toString().c_str());
        Serial.printf("MAC Address: %s\r\n", WiFi.macAddress().c_str());
        Serial.printf("Channel: %d\r\n", WiFi.channel());

    } else {
        Serial.println("Already connected to Wi-Fi.");
    }
#else
    WiFi.begin();
    switchC6.setChannel(11); // 设置信道为11 / Set channel to 11
#endif
  
    M5.Display.clear();
    
    // 创建互斥锁 / Create mutex
    displayMutex = xSemaphoreCreateMutex();
    
    // 创建canvas，使用正确的屏幕尺寸 / Prepare display with proper size
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(WHITE);
    
    M5.Display.setCursor(10, 10);
    M5.Display.println("M5Switchc6 Controller");
  
    if (switchC6.begin()) {
        M5.Display.setCursor(10, 60);
        M5.Display.println("A:ON B:OFF C:STATUS");
    }
     
    
    // 创建等待动画任务 / Create waiting animation task
    xTaskCreate(
        animationTask,           // 任务函数 / Task function
        "AnimationTask",         // 任务名称 / Task name
        4096,                    // 增加堆栈大小 / Stack size
        NULL,                    // 参数 / Parameter
        1,                       // 优先级 / Priority
        &animationTaskHandle     // 任务句柄 / Task handle
    );
}

void loop() {
    M5.update();
    
    // 先定义按钮变量，在互斥锁外部 / Define button layout variables outside the mutex
    int screenWidth = M5.Display.width();
    int screenHeight = M5.Display.height();
    int buttonWidth = screenWidth / 3;
    int buttonHeight = 40;
    int buttonY = screenHeight - buttonHeight;
    int button3Width = screenWidth - buttonWidth * 2; // 剩余宽度 / Remaining width
    
    // 获取互斥锁后再操作canvas / Lock before drawing on the display
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // 绘制3个按钮 / Draw 3 buttons
        // 按钮A - ON / Button A - ON
        M5.Display.fillRect(0, buttonY, buttonWidth, buttonHeight, GREEN);
        M5.Display.drawRect(0, buttonY, buttonWidth, buttonHeight, WHITE);
        M5.Display.setTextColor(BLACK);
        M5.Display.setCursor(buttonWidth/2 - 10, buttonY + 15);
        M5.Display.println("ON");
        
        // 按钮B - OFF  / Button B - OFF
        M5.Display.fillRect(buttonWidth, buttonY, buttonWidth, buttonHeight, RED);
        M5.Display.drawRect(buttonWidth, buttonY, buttonWidth, buttonHeight, WHITE);
        M5.Display.setTextColor(WHITE);
        M5.Display.setCursor(buttonWidth + buttonWidth/2 - 15, buttonY + 15);
        M5.Display.println("OFF");
        
        // 按钮C - STATUS（使用计算后的宽度避免溢出） / Button C - STATUS (use computed width to avoid overflow)
        M5.Display.fillRect(buttonWidth * 2, buttonY, button3Width, buttonHeight, BLUE);
        M5.Display.drawRect(buttonWidth * 2, buttonY, button3Width, buttonHeight, WHITE);
        M5.Display.setTextColor(WHITE);
        M5.Display.setCursor(buttonWidth * 2 + button3Width/2 - 25, buttonY + 15);
        M5.Display.println("STATUS");
        
        M5.Display.setTextColor(WHITE); // 恢复默认文本颜色 / Restore default text color
        
         
        
        // 释放互斥锁 / Release mutex
        xSemaphoreGive(displayMutex);
    }
    bool deviceOn = false;
    bool deviceOff = false;
    bool deviceStatus = false;
    
    // 检测物理按键（适用于有按键的设备） / Handle physical buttons (if available)
    if (M5.BtnA.wasPressed()) {
        deviceOn = true;
    }
    if (M5.BtnB.wasPressed()) {
        deviceOff = true;
    }
    if (M5.BtnC.wasPressed()) {
        deviceStatus = true;
    }
    
    // 检测触摸事件（适用于有触摸屏的设备） / Handle touch events (if touch screen)
    auto touch = M5.Touch.getDetail();
    if (touch.wasPressed()) {
        int touchX = touch.x;
        int touchY = touch.y;
        
        // 检查是否点击了按钮区域 / Check whether a button area is tapped
        if (touchY >= buttonY) {
            if (touchX < buttonWidth) {
                deviceOn = true;
            } else if (touchX < buttonWidth * 2) {
                deviceOff = true;
            } else {
                deviceStatus = true;
            }
        }
    }
    
    // 执行相应的设备控制逻辑 / Execute device control logic
    if (deviceOn) {
        // 开始等待动画 / Start waiting animation
        startWaitingAnimation();
        
        // 开启设备 / Turn device ON
        if (switchC6.sendSwitchCommandWithResponse(DEVICE_MAC, true, 10000)) {
            Serial.println("开启命令发送成功"); // ON command sent successfully
            stopWaitingAnimation();
            
            // 获取互斥锁后更新状态显示 / Update status display with mutex
            if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                M5.Display.fillRect(10, 160, 300, 30, BLACK);
                M5.Display.setCursor(10, 160);
                M5.Display.setTextColor(WHITE);
                M5.Display.println("Device ON sent");
                 
                xSemaphoreGive(displayMutex);
            }
        }
        else {
            Serial.println("开启命令发送失败"); // Failed to send ON command
            stopWaitingAnimation();
            
            if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                M5.Display.fillRect(10, 160, 300, 30, BLACK);
                M5.Display.setCursor(10, 160);
                M5.Display.setTextColor(WHITE);
                M5.Display.println("Device ON failed");
                 
                xSemaphoreGive(displayMutex);
            }
        }
    }
    
    if (deviceOff) {
        // 开始等待动画 / Start waiting animation
        startWaitingAnimation();
        
        // 关闭设备 / Turn device OFF
        if (switchC6.sendSwitchCommandWithResponse(DEVICE_MAC, false, 10000)) {
            Serial.println("关闭命令发送成功"); // OFF command sent successfully
            stopWaitingAnimation();
            
            if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                M5.Display.fillRect(10, 160, 300, 30, BLACK);
                M5.Display.setCursor(10, 160);
                M5.Display.setTextColor(WHITE);
                M5.Display.println("Device OFF sent");
                 
                xSemaphoreGive(displayMutex);
            }
        }
        else {
            Serial.println("关闭命令发送失败"); // Failed to send OFF command
            stopWaitingAnimation();
            
            if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                M5.Display.fillRect(10, 160, 300, 30, BLACK);
                M5.Display.setCursor(10, 160);
                M5.Display.setTextColor(WHITE);
                M5.Display.println("Device OFF failed");
                 
                xSemaphoreGive(displayMutex);
            }
        }
    }
    
    if (deviceStatus) {
        // 开始等待动画 / Start waiting animation
        startWaitingAnimation();
        
        // 查询状态 / Query status
        SwitchC6ParsedData_t response;
        if (switchC6.sendStatusQueryWithResponse(DEVICE_MAC, 10000, &response)) {
            Serial.printf("设备状态: %s, 电压: %.2fV\n", 
                         response.switchState ? "开启" : "关闭", response.voltage); // Device status and voltage
            stopWaitingAnimation();
            
            if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                M5.Display.fillRect(10, 160, 300, 30, BLACK);
                M5.Display.setCursor(10, 160);
                M5.Display.setTextColor(WHITE);
                M5.Display.printf("S:%s V:%.2fV", 
                                response.switchState ? "ON" : "OFF", response.voltage);
                 
                xSemaphoreGive(displayMutex);
            }
        } else {
            stopWaitingAnimation();
            
            if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                M5.Display.fillRect(10, 160, 300, 30, BLACK);
                M5.Display.setCursor(10, 160);
                M5.Display.setTextColor(WHITE);
                M5.Display.println("Query failed");
                 
                xSemaphoreGive(displayMutex);
            }
        }
    }
    
    delay(20); // 减少循环频率，降低系统负载 / Slow down the loop to reduce load
}