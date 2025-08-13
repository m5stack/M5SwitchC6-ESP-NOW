# M5-SwitchC6 库

## 概述

M5-SwitchC6 是一个基于ESP-NOW协议的智能开关控制库，用于控制和管理 Switch-C6 开关设备。该库提供了完整的设备控制API，支持开关控制、状态查询、版本查询、响应管理等功能。

### 主要特性

- **开关控制**: 远程开启/关闭Switch-C6设备
- **状态查询**: 实时查询设备状态和电压信息
- **版本查询**: 获取设备固件版本信息
- **响应管理**: 支异步通信，带超时机制
- **信道管理**: 支持设置主机ESP-NOW通信信道
- **设备发现**: 自动发现和管理多个设备
- **信号监控**: 获取设备信号强度（RSSI）信息
- **广播接收**: 自动接收设备按钮广播数据
- **多设备支持**: 支持同时管理多个Switch-C6设备
- **设备清理**: 智能清理离线设备和过期数据，支持自动清理
- **线程安全**: 使用FreeRTOS互斥锁确保线程安全操作

### 支持平台

1. **ESP32系列** (推荐)
2. **M5Stack系列设备**

### 完善的示例项目：https://gitlab.m5stack.com/moyongming/m5iot-switchc6-demo-pio

## 快速开始

### 基础使用

```cpp
#include <M5Switchc6.h>

M5SwitchC6 switchC6(10);
//填入你的SwitchC6设备MAC地址
const String deviceMac = "E4B3-2386-18C4";

void setup() {
    Serial.begin(115200);
  
    // 设置WiFi为Station模式
    WiFi.mode(WIFI_STA);
    WiFi.begin();
  
    // 初始化Switch-C6控制器
    if (switchC6.begin()) {
        Serial.println("Switch-C6 初始化成功");
    } else {
        Serial.println("Switch-C6 初始化失败");
    }
}

void loop() {
    // 执行自动清理检查
    switchC6.checkAutoCleanup();
  
    // 开启设备
    if (switchC6.sendSwitchCommand(deviceMac, true)) {
        Serial.println("开启命令发送成功");
    }
    delay(5000);
  
    // 查询设备状态
    SwitchC6ParsedData_t response;
    if (switchC6.sendStatusQueryWithResponse(deviceMac, 3000, &response)) {
        Serial.printf("设备状态: %s\n", response.switchState ? "开启" : "关闭");
        Serial.printf("设备电压: %.2fV\n", response.voltage);
        Serial.printf("设备MAC: %s\n", response.macAddress.c_str());
    } else {
        Serial.println("状态查询失败或超时");
    }
  
    delay(5000);
  
    // 关闭设备
    if (switchC6.sendSwitchCommand(deviceMac, false)) {
        Serial.println("关闭命令发送成功");
    }
    delay(5000);
}
```

### 调试输出激活

在调试模式下，可以启用详细日志输出。需要在代码中定义以下宏，并在主函数初始化该串口：

```cpp
#define M5_MODULE_DEBUG_SERIAL Serial0
```

### M5Stack示例

```cpp
#include <M5Unified.h>
#include <WiFi.h>
#include <M5Switchc6.h>

M5SwitchC6 switchC6(10);
//填入你的SwitchC6设备MAC地址
const String DEVICE_MAC = "E4B3-2386-18C4";

void setup() {
    Serial.begin(115200);
    M5.begin();
    M5.Display.setRotation(1);
    M5.Display.setTextSize(2);
  
    WiFi.mode(WIFI_STA);
    WiFi.begin();
  
    M5.Display.clear();
    M5.Display.setCursor(10, 10);
    M5.Display.println("Switch-C6 Demo");
  
    if (switchC6.begin()) {
        M5.Display.setCursor(10, 60);
        M5.Display.println("A:ON B:OFF C:STATUS");
    }
}

void loop() {
    M5.update();
  
    if (M5.BtnA.wasPressed()) {
        // 开启设备
        if (switchC6.sendSwitchCommandWithResponse(DEVICE_MAC, true, 10000)) {
            Serial.println("开启命令发送成功");
            M5.Display.fillRect(10, 100, 300, 30, BLACK);
            M5.Display.setCursor(10, 100);
            M5.Display.println("Device ON sent");
        }
    }
  
    if (M5.BtnB.wasPressed()) {
        // 关闭设备
        if (switchC6.sendSwitchCommandWithResponse(DEVICE_MAC, false, 10000)) {
            Serial.println("关闭命令发送成功");
            M5.Display.fillRect(10, 100, 300, 30, BLACK);
            M5.Display.setCursor(10, 100);
            M5.Display.println("Device OFF sent");
        }
    }
  
    if (M5.BtnC.wasPressed()) {
        // 查询状态
        SwitchC6ParsedData_t response;
        if (switchC6.sendStatusQueryWithResponse(DEVICE_MAC, 10000, &response)) {
            Serial.printf("设备状态: %s, 电压: %.2fV\n", 
                         response.switchState ? "开启" : "关闭", response.voltage);
            M5.Display.fillRect(10, 100, 300, 30, BLACK);
            M5.Display.setCursor(10, 100);
            M5.Display.printf("S:%s V:%.2fV", 
                            response.switchState ? "ON" : "OFF", response.voltage);
        } else {
            M5.Display.fillRect(10, 100, 300, 30, BLACK);
            M5.Display.setCursor(10, 100);
            M5.Display.println("Query failed");
        }
    }
  
    delay(100);
}
```

## API 参考

### 初始化和配置

#### `M5SwitchC6(uint16_t max_broadcast_peers)`

构造函数，创建M5SwitchC6实例

- **参数**:
  - `max_broadcast_peers` - 支持的最大广播设备数量

#### `bool begin()`

初始化模块，设置ESP-NOW通信

- **返回值**: `true` 成功，`false` 失败

#### `bool setChannel(uint8_t channel)`

设置ESP-NOW通信信道（仅高级用户使用）

- **参数**:
  - `channel` - WiFi信道 (1-14)
- **返回值**: `true` 成功，`false` 失败

> **注意**: 默认信道已优化，普通用户无需调用此方法。

#### `uint8_t getChannel()`

获取当前通信信道（仅用于调试）

- **返回值**: 当前WiFi信道号

### 设备控制

#### `bool sendSwitchCommand(const String& deviceMac, bool switchOn, bool needResponse = false)`

发送开关控制命令

- **参数**:
  - `deviceMac` - 设备MAC地址 (格式: "XXXX-XXXX-XXXX")
  - `switchOn` - `true` 开启，`false` 关闭
  - `needResponse` - 是否需要响应
- **返回值**: `true` 发送成功，`false` 发送失败

#### `bool sendStatusQuery(const String& deviceMac)`

发送状态查询命令

- **参数**:
  - `deviceMac` - 设备MAC地址
- **返回值**: `true` 发送成功，`false` 发送失败

#### `bool sendVersionQuery(const String& deviceMac)`

发送版本查询命令

- **参数**:
  - `deviceMac` - 设备MAC地址
- **返回值**: `true` 发送成功，`false` 发送失败

#### `bool sendCustomMessage(const String& message)`

发送自定义消息

- **参数**:
  - `message` - 自定义消息字符串
- **返回值**: `true` 发送成功，`false` 发送失败

### 响应管理

#### `bool sendSwitchCommandWithResponse(const String& deviceMac, bool switchOn, unsigned long timeoutMs = 5000, SwitchC6ParsedData_t* response = nullptr)`

发送开关命令并等待响应

- **参数**:
  - `deviceMac` - 设备MAC地址
  - `switchOn` - 开关状态
  - `timeoutMs` - 超时时间（毫秒），默认5000ms
  - `response` - 响应数据指针（可选）
- **返回值**: `true` 收到响应，`false` 超时或失败

#### `bool sendStatusQueryWithResponse(const String& deviceMac, unsigned long timeoutMs = 5000, SwitchC6ParsedData_t* response = nullptr)`

发送状态查询并等待响应

- **参数**:
  - `deviceMac` - 设备MAC地址
  - `timeoutMs` - 超时时间（毫秒），默认5000ms
  - `response` - 响应数据指针（可选）
- **返回值**: `true` 收到响应，`false` 超时或失败

#### `bool sendVersionQueryWithResponse(const String& deviceMac, unsigned long timeoutMs = 5000, SwitchC6ParsedData_t* response = nullptr)`

发送版本查询并等待响应

- **参数**:
  - `deviceMac` - 设备MAC地址
  - `timeoutMs` - 超时时间（毫秒），默认5000ms
  - `response` - 响应数据指针（可选）
- **返回值**: `true` 收到响应，`false` 超时或失败

#### `bool isWaitingForResponse()`

检查是否有等待中的响应请求

- **返回值**: `true` 有等待中的请求，`false` 无等待

#### `bool getResponseWaitInfo(const String& deviceMac, ResponseWaitInfo* waitInfo)`

获取指定设备的响应等待信息

- **参数**:
  - `deviceMac` - 设备MAC地址
  - `waitInfo` - 等待信息结构体指针
- **返回值**: `true` 设备正在等待响应，`false` 无等待

### 设备管理

#### `uint16_t getPeerCount()`

获取检测到的设备数量

- **返回值**: 设备数量

#### `bool getPeerInfo(uint16_t index, broadcast_packet_with_mac* peerInfo)`

根据索引获取设备信息

- **参数**:
  - `index` - 设备索引 (0 到 getPeerCount()-1)
  - `peerInfo` - 设备信息结构体指针
- **返回值**: `true` 获取成功，`false` 索引无效

### 高级配置

#### `bool setResendInterval(unsigned long intervalMs)`

设置重发间隔

- **参数**:
  - `intervalMs` - 重发间隔（毫秒，1-10000ms）
- **返回值**: `true` 设置成功，`false` 参数超出范围

#### `unsigned long getResendInterval()`

获取当前重发间隔

- **返回值**: 重发间隔（毫秒）

### 数据管理

#### `bool clearPeersData()`

清空所有已发现的设备数据

- **功能**: 清空所有存储的设备信息，包括MAC地址、最后接收的数据、信号强度和状态信息
- **返回值**: `true` 清空成功，`false` 清空失败

#### `bool clearResponseWaits()`

清空所有等待中的响应请求

- **功能**: 取消所有等待中的响应请求并清空等待队列
- **返回值**: `true` 清空成功，`false` 清空失败

#### `bool clearAllData()`

清空所有历史数据

- **功能**: 执行完整的数据清理，包括已发现的设备和等待中的响应请求
- **返回值**: `true` 全部清空成功，`false` 部分清空失败

#### `bool clearDeviceData(const String& deviceMac)`

清除指定设备的数据

- **参数**:
  - `deviceMac` - 设备MAC地址 (格式: "XX:XX:XX:XX:XX:XX" 或 "XXXX-XXXX-XXXX")
- **功能**: 根据MAC地址从设备列表中移除指定设备，同时清除该设备的等待响应
- **返回值**: `true` 设备找到并清除成功，`false` 设备未找到或清除失败

#### `uint16_t clearExpiredDevices(unsigned long timeoutMs = 300000)`

清除过期设备

- **参数**:
  - `timeoutMs` - 超时时间（毫秒），默认300000ms（5分钟）
- **功能**: 移除指定时间内未发送任何数据的设备
- **返回值**: 被移除的设备数量

#### `bool setAutoCleanup(unsigned long intervalMs, unsigned long deviceTimeoutMs = 300000)`

设置自动清理

- **参数**:
  - `intervalMs` - 清理检查间隔（毫秒，设为0禁用自动清理）
  - `deviceTimeoutMs` - 设备超时时间（毫秒），默认300000ms（5分钟）
- **功能**: 启用定期自动清理过期设备的功能
- **返回值**: `true` 设置成功，`false` 参数无效

#### `uint16_t checkAutoCleanup()`

执行自动清理检查

- **功能**: 检查并执行自动清理，应在loop()中定期调用
- **返回值**: 本次清理移除的设备数量，如果未到清理时间则返回0

### 数据解析

#### `SwitchC6ParsedData parseDataStructured(const uint8_t *data, int len)`

解析接收到的数据并返回结构化结果

- **参数**:
  - `data` - 数据缓冲区指针
  - `len` - 数据长度
- **返回值**: 解析后的数据结构

## 数据结构

### SwitchC6ParsedData_t

设备解析数据结构

```cpp
struct SwitchC6ParsedData_t {
    SwitchC6CommandType_t commandType; // 命令类型
    String macAddress;               // 设备MAC地址字符串
    bool switchState;                // 开关状态 (true=开, false=关)
    float voltage;                   // 电压值 (例如: 3.30V)
    String version;                  // 版本号字符串
    bool isValid;                    // 解析是否成功
    String errorMessage;             // 错误信息
};
```

### broadcast_packet_with_mac

广播设备信息结构

```cpp
struct broadcast_packet_with_mac {
    uint8_t senderMac[6];            // 发送者MAC地址
    String lastData;                 // 最新数据
    uint8_t channel;                 // 接收数据的信道
    int rssi;                        // 信号强度
    unsigned long lastReceiveTime;   // 最后接收时间
    bool status;                     // 状态标志 (0:关断 1:开启)
};
```

### SwitchC6CommandType_t

命令类型枚举

```cpp
enum SwitchC6CommandType_t {
    COMMAND_UNKNOWN = 0,         // 未知命令
    COMMAND_STATUS_RETURN = 1,   // 状态返回数据
    COMMAND_BROADCAST = 2,       // 按钮广播数据
    COMMAND_VERSION_RETURN = 3   // 版本返回数据
};
```

### ResponseWaitInfo

响应等待信息结构

```cpp
struct ResponseWaitInfo {
    String deviceMac;                // 等待响应的设备MAC地址
    unsigned long requestTime;       // 请求发送时间
    unsigned long timeoutMs;         // 超时时间（毫秒）
    bool isWaiting;                  // 是否正在等待响应
    bool hasResponse;                // 是否已收到响应
    SwitchC6ParsedData_t response;     // 收到的响应数据
};
```

## 高级使用

### 设备清理功能

```cpp
M5SwitchC6 switchC6(20);

void setup() {
    WiFi.mode(WIFI_STA);
    WiFi.begin();

    switchC6.begin();
  
    // 启用自动清理：每30秒检查一次，移除5分钟未响应的设备
    switchC6.setAutoCleanup(30000, 300000);
}

void loop() {
    // 执行自动清理检查
    uint16_t removed = switchC6.checkAutoCleanup();  //不可少了这行，否则不会运行清理函数
    if (removed > 0) {
        Serial.printf("自动清理移除了 %d 个设备\n", removed);
    }
  
    // 手动清理特定设备
    if (needToClearDevice) {
        if (switchC6.clearDeviceData("AA:BB:CC:DD:EE:FF")) {
            Serial.println("指定设备已清除");
        }
    }
  
    // 手动清理过期设备
    if (needToClearExpired) {
        uint16_t count = switchC6.clearExpiredDevices(60000); // 清除1分钟未响应的设备
        Serial.printf("清除了 %d 个过期设备\n", count);
    }
  
    delay(1000);
}
```

### 多设备管理

```cpp
M5SwitchC6 switchC6(20);  // 支持20个设备

void setup() {
    WiFi.mode(WIFI_STA);
    WiFi.begin();

    switchC6.begin();
}

void loop() {
    // 获取所有检测到的设备
    uint16_t deviceCount = switchC6.getPeerCount();
    Serial.printf("检测到 %d 个设备\n", deviceCount);
  
    for (uint16_t i = 0; i < deviceCount; i++) {
        broadcast_packet_with_mac peerInfo;
        if (switchC6.getPeerInfo(i, &peerInfo)) {
            Serial.printf("设备 %d: MAC=%02X:%02X:%02X:%02X:%02X:%02X, "
                         "状态=%s, RSSI=%d, 信道=%d\n",
                         i,
                         peerInfo.senderMac[0], peerInfo.senderMac[1],
                         peerInfo.senderMac[2], peerInfo.senderMac[3],
                         peerInfo.senderMac[4], peerInfo.senderMac[5],
                         peerInfo.status ? "开启" : "关闭",
                         peerInfo.rssi, peerInfo.channel);
        }
    }
    delay(5000);
}
```

### 错误处理和重试机制

```cpp
bool controlDeviceWithRetry(const String& deviceMac, bool switchOn, int maxRetries = 3) {
    for (int i = 0; i < maxRetries; i++) {
        SwitchC6ParsedData_t response;
        if (switchC6.sendSwitchCommandWithResponse(deviceMac, switchOn, 3000, &response)) {
            if (response.isValid && response.switchState == switchOn) {
                Serial.printf("设备控制成功，第 %d 次尝试\n", i + 1);
                return true;
            }
        }
        Serial.printf("第 %d 次尝试失败，重试中...\n", i + 1);
        delay(1000);  // 等待1秒后重试
    }
    Serial.println("设备控制失败，已达到最大重试次数");
    return false;
}

void loop() {
    // 使用重试机制控制设备
    if (controlDeviceWithRetry("E4B3-2386-18C4", true)) {
        Serial.println("设备成功开启");
    }
    delay(10000);
}
```

### 异步操作示例

```cpp
void setup() {
    WiFi.mode(WIFI_STA);
    WiFi.begin();

    switchC6.begin();
}

void loop() {
    // 发送命令但不等待响应
    switchC6.sendSwitchCommand("E4B3-2386-18C4", true, true);
  
    // 继续执行其他任务
    doOtherTasks();
  
    // 检查是否有等待中的响应
    if (switchC6.isWaitingForResponse()) {
        Serial.println("等待设备响应中...");
    }
  
    delay(100);
}

void doOtherTasks() {
    // 执行其他不依赖Switch-C6响应的任务
    Serial.println("执行其他任务...");
    delay(200);
}
```

### 数据管理示例

```cpp
void setup() {
    WiFi.mode(WIFI_STA);
    WiFi.begin();

    switchC6.begin();
  
    // 设置自动清理：每2分钟检查，移除10分钟未响应的设备
    switchC6.setAutoCleanup(120000, 600000);
}

void loop() {
    // 自动清理会在checkAutoCleanup()中执行
    switchC6.checkAutoCleanup();
  
    // 显示当前设备信息
    uint16_t deviceCount = switchC6.getPeerCount();
    Serial.printf("当前检测到 %d 个设备\n", deviceCount);
  
    // 监控设备状态，清理长时间无响应的设备
    for (uint16_t i = 0; i < deviceCount; i++) {
        broadcast_packet_with_mac peerInfo;
        if (switchC6.getPeerInfo(i, &peerInfo)) {
            unsigned long timeSinceLastReceive = millis() - peerInfo.lastReceiveTime;
  
            // 如果设备超过15分钟未响应，手动清理
            if (timeSinceLastReceive > 900000) { // 15分钟
                String deviceMac = "";
                for (int j = 0; j < 6; j++) {
                    if (j > 0) deviceMac += ":";
                    deviceMac += String(peerInfo.senderMac[j], HEX);
                }
  
                Serial.printf("清理长时间未响应的设备: %s\n", deviceMac.c_str());
                switchC6.clearDeviceData(deviceMac);
                break; // 一次只清理一个，避免影响循环
            }
        }
    }
  
    delay(5000);
}
```

### 智能设备管理

```cpp
void smartDeviceManagement() {
    // 1. 检查设备数量，避免过多设备占用内存
    uint16_t deviceCount = switchC6.getPeerCount();
    if (deviceCount > 15) {
        Serial.println("设备数量过多，清理最旧的设备");
        switchC6.clearExpiredDevices(180000); // 清理3分钟未响应的设备
    }
  
    // 2. 定期全面清理
    static unsigned long lastFullCleanup = 0;
    if (millis() - lastFullCleanup > 1800000) { // 30分钟执行一次全面清理
        Serial.println("执行定期全面清理");
        switchC6.clearAllData();
        lastFullCleanup = millis();
    }
  
    // 3. 根据信号强度清理弱信号设备
    for (uint16_t i = 0; i < switchC6.getPeerCount(); i++) {
        broadcast_packet_with_mac peerInfo;
        if (switchC6.getPeerInfo(i, &peerInfo)) {
            if (peerInfo.rssi < -90) { // 信号强度过弱
                unsigned long timeSinceLastReceive = millis() - peerInfo.lastReceiveTime;
                if (timeSinceLastReceive > 300000) { // 且5分钟未响应
                    String deviceMac = "";
                    for (int j = 0; j < 6; j++) {
                        if (j > 0) deviceMac += ":";
                        deviceMac += String(peerInfo.senderMac[j], HEX);
                    }
                    Serial.printf("清理弱信号设备: %s (RSSI: %d)\n", deviceMac.c_str(), peerInfo.rssi);
                    switchC6.clearDeviceData(deviceMac);
                    break;
                }
            }
        }
    }
}
```

## 协议支持

### ESP-NOW通信协议

支持完整的Switch-C6 ESP-NOW控制协议：

#### 控制命令格式

- **开启设备**: `"1"` 或 `"1;"` (带响应)
- **关闭设备**: `"0"` 或 `"0;"` (带响应)
- **状态查询**: `"?"` 或 `"?;"` (带响应)
- **版本查询**: `"V"` 或 `"V;"` (带响应)

#### 响应数据格式

- **状态响应**: `";状态;电压"` (例如: ";1;3.30")
- **版本响应**: `";版本号"` (例如: ";1.0.0")
- **广播数据**: 设备按钮按下时的广播信息

#### MAC地址格式

- **标准格式**: `"XXXX-XXXX-XXXX"` (例如: "E4B3-2386-18C4")
- **广播地址**: `"FFFF-FFFF-FFFF"` (接收所有设备广播)

### 通信特性

- **信道范围**: 1-14 (推荐使用6或11)
- **最大设备数**: 可配置，推荐不超过20个
- **通信距离**: 室内约30-100米，室外约100-300米
- **数据包大小**: 最大250字节
- **重发机制**: 可配置重发间隔和次数

## 故障排除

### 常见问题

#### 1. 设备初始化失败

```cpp
if (!switchC6.begin()) {
    Serial.println("初始化失败，请检查:");
    Serial.println("1. WiFi是否设置为STA模式");
    Serial.println("2. ESP-NOW是否支持");
    Serial.println("3. 内存是否充足");
}
```

#### 2. 设备响应超时

```cpp
SwitchC6ParsedData_t response;
if (!switchC6.sendStatusQueryWithResponse(deviceMac, 5000, &response)) {
    Serial.println("响应超时，可能原因:");
    Serial.println("1. 设备不在线或信号弱");
    Serial.println("2. MAC地址错误");
    Serial.println("3. 信道不匹配");
    Serial.println("4. 设备忙碌");
}
```

#### 3. MAC地址格式错误

```cpp
String deviceMac = "E4B3-2386-18C4";  // 正确格式
// String deviceMac = "E4:B3:23:86:18:C4";  // 错误格式
```

#### 4. 信道配置问题

```cpp
Serial.printf("当前信道: %d\n", switchC6.getChannel());
```

> **注意**: 由于SwitchC6使用的是广播包，且会监听所有信道，故信道设置不重要，如果环境某信道干扰过大，再做更改。
