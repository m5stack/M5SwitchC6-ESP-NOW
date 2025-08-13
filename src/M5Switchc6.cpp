/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "M5Switchc6.h"

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
// 静态成员变量定义 / Static member variable definitions
SemaphoreHandle_t M5SwitchC6::mutexLock = nullptr;
M5SwitchC6* M5SwitchC6::instance = nullptr;

// 队列相关静态变量 / Queue-related static variables
QueueHandle_t M5SwitchC6::dataQueue = nullptr;
TaskHandle_t M5SwitchC6::parseTaskHandle = nullptr;
#endif

void M5SwitchC6::acquireMutex()
{

    if (mutexLock == nullptr) {
        mutexLock = xSemaphoreCreateMutex();
    }
    if (mutexLock != nullptr) {
        xSemaphoreTake(mutexLock, portMAX_DELAY);
    }
}

void M5SwitchC6::releaseMutex()
{
    if (mutexLock != nullptr) {
        xSemaphoreGive(mutexLock);
    }
}

// 构造函数，只做基本的成员变量初始化 / Constructor, only performs basic member variable initialization
M5SwitchC6::M5SwitchC6(uint16_t max_broadcast_peers) : max_broadcast_peers(max_broadcast_peers) {
    // 在构造函数中不进行需要串口输出的操作 / Don't perform operations requiring serial output in constructor
    // 所有初始化逻辑移到 begin() 方法中 / Move all initialization logic to begin() method
    peers = nullptr;
    peerCount = 0;
    responseWaitCount = 0;
    
    // 初始化自动清理相关变量 / Initialize auto cleanup related variables
    autoCleanupInterval = 0;  // 默认禁用自动清理 / Default disable auto cleanup
    deviceTimeout = 300000;   // 默认5分钟超时 / Default 5 minute timeout
    lastCleanupTime = 0;
    
    // 初始化响应等待数组 / Initialize response wait array
    for (uint16_t i = 0; i < MAX_RESPONSE_WAITS; i++) {
        responseWaits[i].isWaiting = false;
        responseWaits[i].hasResponse = false;
        responseWaits[i].deviceMac = "";
        responseWaits[i].requestTime = 0;
        responseWaits[i].timeoutMs = 0;
    }
    
    instance = this;  // 设置静态实例指针 / Set static instance pointer
}

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
// 数据解析任务 / Data parsing task
void M5SwitchC6::parseDataTask(void* parameter) {
    QueuedData_t queuedData;
    
    while (true) {
        // 从队列中接收数据，等待最多100ms / Receive data from queue, wait up to 100ms
        if (xQueueReceive(dataQueue, &queuedData, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (instance != nullptr) {
                // 在任务上下文中安全地处理数据（带发送方MAC地址） / Safely process data in task context (with sender MAC address)
                instance->parseData(queuedData.data, queuedData.len, queuedData.senderMac);
            }
            
            // 释放动态分配的内存 / Release dynamically allocated memory
            if (queuedData.data != nullptr) {
                delete[] queuedData.data;
            }
        }
        
        // 让其他任务有机会运行 / Give other tasks a chance to run
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// 将数据加入队列的安全函数 / Safe function to enqueue data
bool M5SwitchC6::enqueueData(const uint8_t* data, int len) {
    if (dataQueue == nullptr || data == nullptr || len <= 0) {
        return false;
    }
    
    // 创建队列数据结构 / Create queue data structure
    QueuedData_t queuedData;
    queuedData.len = len;
    queuedData.data = new uint8_t[len];
    memset(queuedData.senderMac, 0, 6); // 默认清零 / Default to zero
    
    if (queuedData.data == nullptr) {
        return false; // 内存分配失败 / Memory allocation failed
    }
    
    // 复制数据 / Copy data
    memcpy(queuedData.data, data, len);
    
    // 尝试发送到队列（非阻塞） / Try to send to queue (non-blocking)
    if (xQueueSend(dataQueue, &queuedData, 0) != pdTRUE) {
        // 队列满了，释放内存 / Queue is full, release memory
        delete[] queuedData.data;
        return false;
    }
    
    return true;
}

// 将数据加入队列的安全函数（带发送方MAC地址） / Safe function to enqueue data (with sender MAC address)
bool M5SwitchC6::enqueueData(const uint8_t* data, int len, const uint8_t* senderMac) {
    if (dataQueue == nullptr || data == nullptr || len <= 0) {
        return false;
    }
    
    // 创建队列数据结构 / Create queue data structure
    QueuedData_t queuedData;
    queuedData.len = len;
    queuedData.data = new uint8_t[len];
    
    if (queuedData.data == nullptr) {
        return false; // 内存分配失败 / Memory allocation failed
    }
    
    // 复制数据和发送方MAC地址 / Copy data and sender MAC address
    memcpy(queuedData.data, data, len);
    if (senderMac != nullptr) {
        memcpy(queuedData.senderMac, senderMac, 6);
    } else {
        memset(queuedData.senderMac, 0, 6);
    }
    
    // 尝试发送到队列（非阻塞） / Try to send to queue (non-blocking)
    if (xQueueSend(dataQueue, &queuedData, 0) != pdTRUE) {
        // 队列满了，释放内存 / Queue is full, release memory
        delete[] queuedData.data;
        return false;
    }
    
    return true;
}
#endif

// 析构函数，ESP 平台上删除互斥锁 / Destructor, delete mutex lock on ESP platform
M5SwitchC6::~M5SwitchC6() {
    
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    // 停止解析任务 / Stop parsing task
    if (parseTaskHandle != nullptr) {
        vTaskDelete(parseTaskHandle);
        parseTaskHandle = nullptr;
    }
    
    // 删除队列 / Delete queue
    if (dataQueue != nullptr) {
        vQueueDelete(dataQueue);
        dataQueue = nullptr;
    }
#endif
    
    // 释放动态分配的内存 / Release dynamically allocated memory
    if (peers != nullptr) {
        delete[] peers;
        peers = nullptr;
    }
    
    // 清理静态实例指针 / Clear static instance pointer
    if (instance == this) {
        instance = nullptr;
    }
    
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    if (mutexLock != NULL) {
        vSemaphoreDelete(mutexLock);
        mutexLock = NULL;
    }
#endif
}

// 初始化模块 / Initialize module
bool M5SwitchC6::begin() {
    //检查当前是不是 ESP32 或 ESP8266 平台 / Check if current platform is ESP32 or ESP8266
#if !defined(ARDUINO_ARCH_ESP32) && !defined(ARDUINO_ARCH_ESP8266)
    serialPrintln("M5SwitchC6 is only supported on ESP32 or ESP8266 platforms, please check your platform.");
    return false;
#endif

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    // 创建互斥锁 / Create mutex lock
    if (mutexLock == NULL) {
        mutexLock = xSemaphoreCreateMutex();
        if (mutexLock == NULL) {
            serialPrintln("Failed to create mutex lock.");
            return false;
        } else {
            serialPrintln("Mutex lock created successfully.");
        }
    }

    // 创建数据队列（队列大小为10） / Create data queue (queue size is 10)
    if (dataQueue == nullptr) {
        dataQueue = xQueueCreate(10, sizeof(QueuedData_t));
        if (dataQueue == nullptr) {
            serialPrintln("Failed to create data queue.");
            return false;
        } else {
            serialPrintln("Data queue created successfully.");
        }
    }

    // 创建数据解析任务 / Create data parsing task
    if (parseTaskHandle == nullptr) {
        BaseType_t result = xTaskCreate(
            parseDataTask,           // 任务函数 / Task function
            "ParseDataTask",         // 任务名称 / Task name
            4096,                    // 栈大小（字节） / Stack size (bytes)
            nullptr,                 // 任务参数 / Task parameter
            1,                       // 任务优先级 / Task priority
            &parseTaskHandle         // 任务句柄 / Task handle
        );
        
        if (result != pdPASS) {
            serialPrintln("Failed to create parse data task.");
            return false;
        } else {
            serialPrintln("Parse data task created successfully.");
        }
    }

    // 分配内存给peers数组 / Allocate memory for peers array
    if (peers == nullptr) {
        peers = new broadcast_packet_with_mac_t[max_broadcast_peers];
        if (peers == nullptr) {
            serialPrintln("Failed to allocate memory for broadcast peers.");
            return false;
        }
        // 初始化peers数组 / Initialize peers array
        for (uint16_t i = 0; i < max_broadcast_peers; i++) {
            memset(peers[i].senderMac, 0, sizeof(peers[i].senderMac));
            peers[i].lastData = "";
            peers[i].channel = 0;
            peers[i].rssi = 0;
            peers[i].lastReceiveTime = 0;
            peers[i].status = false;
        }
        serialPrintln("M5SwitchC6 initialized with max_broadcast_peers: " + String(max_broadcast_peers));
    }

    // 检查当前esp的wifi是否为 STA 模式 / Check if current ESP WiFi is in STA mode
    if (WiFi.getMode() == WIFI_OFF) {
        serialPrintln("WiFi is off. Please set the WiFi mode to STA or AP_STA and call WiFi.begin() before using the M5SwitchC6.");
        return false;
    }
    else if(WiFi.getMode() == WIFI_AP) {
        serialPrintln("WiFi is in AP mode. Please set the WiFi mode to STA or AP_STA and call WiFi.begin() before using the M5SwitchC6.");
        return false;
    }
    else if(WiFi.getMode() == WIFI_STA || WiFi.getMode() == WIFI_AP_STA) {
        serialPrintln("WiFi is in STA or AP_STA mode. M5SwitchC6 begin now.");
    } else {
        serialPrintln("Unknown WiFi mode. Please set the WiFi mode to STA or AP_STA and call WiFi.begin() before using the M5SwitchC6.");
        return false;
    }

    // 获取互斥锁 / Acquire mutex lock
    acquireMutex();

    // 初始化esp_now / Initialize esp_now
    if (esp_now_init() != ESP_OK) {
        serialPrintln("Failed to initialize ESP-NOW.");
        releaseMutex();
        return false;
    }
    serialPrintln("ESP-NOW initialized successfully.");

    // 注册接收回调函数 / Register receive callback function
    esp_now_register_recv_cb([](const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int len) {
        if (esp_now_info && esp_now_info->src_addr && instance) {
            // 获取互斥锁，保护 peers 操作 / Acquire mutex lock to protect peers operations
            instance->acquireMutex();
            
            const uint8_t *mac = esp_now_info->src_addr;
            char* tempStr = new char[len + 1];
            memcpy(tempStr, data, len);
            tempStr[len] = '\0';
            String dataStr = String(tempStr);
            delete[] tempStr;
            int channel = 0;
            int rssi = 0;
            
            // 获取信道和信号强度信息 / Get channel and signal strength information
            if (esp_now_info->rx_ctrl) {
                channel = esp_now_info->rx_ctrl->channel;
                rssi = esp_now_info->rx_ctrl->rssi;
            }
            
            int idx = -1;
            
            // 查找已有节点 / Find existing node
            for (int i = 0; i < instance->peerCount; ++i) {
                bool same = true;
                for (int j = 0; j < 6; ++j) {
                    if (instance->peers[i].senderMac[j] != mac[j]) { 
                        same = false; 
                        break; 
                    }
                }
                if (same) { 
                    idx = i; 
                    break; 
                }
            }
            
            // 新节点添加 / Add new node
            if (idx < 0 && instance->peerCount < instance->max_broadcast_peers) {
                idx = instance->peerCount;
                memcpy(instance->peers[idx].senderMac, mac, 6);
                instance->peerCount++;
            }
            
            // 更新数据 / Update data
            if (idx >= 0) {
                instance->peers[idx].lastData = dataStr;
                instance->peers[idx].channel = channel;
                instance->peers[idx].rssi = rssi;
                instance->peers[idx].lastReceiveTime = millis();
                //MAC;status;voltage (例如: 1122-AABB-CCGG;1;3.30V) / Example: 1122-AABB-CCGG;1;3.30V
                if (dataStr.indexOf(';') != -1) {
                    // 解析按键状态 / Parse button status
                    int statusIndex = dataStr.indexOf(';');
                    if (statusIndex != -1) {
                        String statusStr = dataStr.substring(statusIndex + 1);
                        instance->peers[idx].status = (statusStr.startsWith("1") || statusStr.startsWith("true"));
                    }
                } else {
                    instance->peers[idx].status = false; // 默认状态为关闭 / Default status is off
                }
            }
            
            // 释放互斥锁 / Release mutex lock
            instance->releaseMutex();
            
            // 构建完整的日志消息，使用字符数组避免String拼接的问题 / Build complete log message using character array to avoid String concatenation issues
            char logBuffer[512] = {0};
            char macStr[18] = {0};
            snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            
            snprintf(logBuffer, sizeof(logBuffer), 
                     "ESPNOW data received [MAC:%s]<%d> Channel:%d RSSI:%ddBm Data: %s",
                     macStr, idx, channel, rssi, dataStr.c_str());
            serialPrintf("%s\r\n", logBuffer);
            
            // 将数据加入队列而不是直接调用parseData（传递发送方MAC地址） / Add data to queue instead of directly calling parseData (pass sender MAC address)
            if (!instance->enqueueData(data, len, mac)) {
                serialPrintf("Warning: Failed to enqueue data for parsing\r\n");
            }
            
        } else {
            char logBuffer[512] = {0};
            snprintf(logBuffer, sizeof(logBuffer), "ESPNOW data received [MAC:UNKNOWN]: %s", 
                     (data && len > 0) ? (const char *)data : "NULL");
            serialPrintf("%s\r\n", logBuffer);
            
            // 即使MAC未知，也尝试解析数据（无发送方MAC） / Even if MAC is unknown, try to parse data (without sender MAC)
            if (instance != nullptr && data != nullptr && len > 0) {
                if (!instance->enqueueData(data, len)) {
                    serialPrintf("Warning: Failed to enqueue unknown MAC data for parsing\r\n");
                }
            }
        }
    });
    
    // 注册发送回调函数 / Register send callback function
    // 兼容新旧ESP-IDF版本 / Compatible with old and new ESP-IDF versions
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)
    // 新版本ESP-IDF (5.5.0+) 使用wifi_tx_info_t / New ESP-IDF version (5.5.0+) uses wifi_tx_info_t
    esp_now_register_send_cb([](const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
        char logBuffer[128];
        char macStr[18];
        const uint8_t *mac = tx_info->des_addr;
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        snprintf(logBuffer, sizeof(logBuffer), "ESPNOW send status: [MAC:%s]: %d", macStr, status);
        serialPrintf("%s\r\n", logBuffer);
    });
#else
    // 旧版本ESP-IDF (<5.5.0) 使用uint8_t* / Old ESP-IDF version (<5.5.0) uses uint8_t*
    esp_now_register_send_cb([](const uint8_t *mac, esp_now_send_status_t status) {
        char logBuffer[128];
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        snprintf(logBuffer, sizeof(logBuffer), "ESPNOW send status: [MAC:%s]: %d", macStr, status);
        serialPrintf("%s\r\n", logBuffer);
    });
#endif

    // 释放互斥锁 / Release mutex lock
    releaseMutex();

#endif
    return true;
}

// 设置WiFi信道 / Set WiFi channel
bool M5SwitchC6::setChannel(uint8_t channel) {
    // 检查信道范围 / Check channel range
    if (channel < 1 || channel > 14) {
        serialPrintln("Invalid channel. Channel must be between 1 and 14.");
        return false;
    }

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    // 获取互斥锁 / Acquire mutex lock
    acquireMutex();

    // 设置WiFi信道 / Set WiFi channel
    if (esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE) == ESP_OK) {
        serialPrintln("WiFi channel set to: " + String(channel));
        releaseMutex();
        return true;
    } else {
        serialPrintln("Failed to set WiFi channel to: " + String(channel));
        releaseMutex();
        return false;
    }
#else
    serialPrintln("setChannel is only supported on ESP32 or ESP8266 platforms.");
    return false;
#endif
}

// 获取当前WiFi信道 / Get current WiFi channel
uint8_t M5SwitchC6::getChannel() {
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    uint8_t primary_channel = 0;
    wifi_second_chan_t second_channel = WIFI_SECOND_CHAN_NONE;
    
    if (esp_wifi_get_channel(&primary_channel, &second_channel) == ESP_OK) {
        String logMsg = "Current WiFi channel: " + String(primary_channel);
        serialPrintf("%s\r\n", logMsg.c_str());
        return primary_channel;
    } else {
        String logMsg = "Failed to get current WiFi channel.";
        serialPrintf("%s\r\n", logMsg.c_str());
        return 0;
    }
#else
    serialPrintln("getChannel is only supported on ESP32 or ESP8266 platforms.");
    return 0;
#endif
}

// 获取节点数量 / Get peer count
uint16_t M5SwitchC6::getPeerCount() {
    return peerCount;
}

// 获取节点信息 / Get peer information
bool M5SwitchC6::getPeerInfo(uint16_t index, broadcast_packet_with_mac_t* peerInfo) {
    if (index >= peerCount || peerInfo == nullptr || peers == nullptr) {
        return false;
    }
    
    // 复制节点信息 / Copy peer information
    memcpy(peerInfo->senderMac, peers[index].senderMac, 6);
    peerInfo->lastData = peers[index].lastData;
    peerInfo->channel = peers[index].channel;
    peerInfo->rssi = peers[index].rssi;
    peerInfo->lastReceiveTime = peers[index].lastReceiveTime;
    peerInfo->status = peers[index].status;

    
    return true;
}

// 数据解析（带发送方MAC地址） / Data parsing (with sender MAC address)
void M5SwitchC6::parseData(const uint8_t *data, int len, const uint8_t *senderMac) {
    // 调用结构化解析函数 / Call structured parsing function
    SwitchC6ParsedData_t parsed = parseDataStructured(data, len);
    
    // 转换发送方MAC为字符串格式 / Convert sender MAC to string format
    String senderMacStr = "";
    if (senderMac != nullptr) {
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X%02X-%02X%02X-%02X%02X",
                 senderMac[0], senderMac[1], senderMac[2], 
                 senderMac[3], senderMac[4], senderMac[5]);
        senderMacStr = String(macStr);
    }
    
    // 打印解析结果 / Print parsing results
    if (parsed.isValid) {
        char logBuffer[512];
        switch (parsed.commandType) {
            case COMMAND_STATUS_RETURN:
                snprintf(logBuffer, sizeof(logBuffer), 
                    "Device Status Response: DataMAC=%s, SenderMAC=%s, State=%s, Voltage=%.2fV",
                    parsed.macAddress.c_str(), senderMacStr.c_str(), 
                    parsed.switchState ? "ON" : "OFF", parsed.voltage);
                break;
            case COMMAND_BROADCAST:
                snprintf(logBuffer, sizeof(logBuffer), 
                    "Device Button Broadcast: DataMAC=%s, SenderMAC=%s, State=%s, Voltage=%.2fV",
                    parsed.macAddress.c_str(), senderMacStr.c_str(),
                    parsed.switchState ? "ON" : "OFF", parsed.voltage);
                break;
            case COMMAND_VERSION_RETURN:
                snprintf(logBuffer, sizeof(logBuffer), 
                    "Device Version Response: DataMAC=%s, SenderMAC=%s, Version=%s",
                    parsed.macAddress.c_str(), senderMacStr.c_str(), parsed.version.c_str());
                break;
            default:
                snprintf(logBuffer, sizeof(logBuffer), "Unknown Device Response Type");
                break;
        }
        serialPrintf("Parsed: %s\r\n", logBuffer);
        
        // 检查是否有等待这个响应的请求（使用发送方MAC） / Check if there's a request waiting for this response (using sender MAC)
        handleReceivedResponse(parsed, senderMacStr);
    } else {
        serialPrintf("Parse Error: %s\r\n", parsed.errorMessage.c_str());
    }
}

// 结构化数据解析函数 / Structured data parsing function
SwitchC6ParsedData_t M5SwitchC6::parseDataStructured(const uint8_t *data, int len) {
    SwitchC6ParsedData_t result;
    result.isValid = false;
    result.commandType = COMMAND_UNKNOWN;
    result.switchState = false;
    result.voltage = 0.0f;
    result.version = "";
    
    // 检查数据有效性 / Check data validity
    if (data == nullptr || len <= 0) {
        result.errorMessage = "Invalid data or length";
        return result;
    }
    
    // 确保字符串以null结尾 / Ensure string is null-terminated
    char* dataStr = new char[len + 1];
    memcpy(dataStr, data, len);
    dataStr[len] = '\0';
    
    String receivedData = String(dataStr);
    delete[] dataStr;
    
    receivedData.trim(); // 去除首尾空白 / Remove leading and trailing whitespace
    
    if (receivedData.length() == 0) {
        result.errorMessage = "Empty data";
        return result;
    }
    
    // 作为主机，我们主要接收从机的状态返回、广播数据和版本查询响应 / As master, we mainly receive slave status returns, broadcast data and version query responses
    // 从机数据格式: / Slave data format:
    // 状态/广播: MAC;status;voltage (MAC为从机地址或FFFF-FFFF-FFFF广播地址) / Status/broadcast: MAC;status;voltage (MAC is slave address or FFFF-FFFF-FFFF broadcast address)
    // 版本响应: MAC;version (例如: 1122-AABB-CCGG;1.0.0) / Version response: MAC;version (e.g.: 1122-AABB-CCGG;1.0.0)
    
    // 检查数据是否包含分号（从机返回数据的特征） / Check if data contains semicolon (characteristic of slave return data)
    if (receivedData.indexOf(';') > 0) {
        // 解析从机返回数据格式 / Parse slave return data format
        int firstSemicolon = receivedData.indexOf(';');
        String macPart = receivedData.substring(0, firstSemicolon);
        String remainingPart = receivedData.substring(firstSemicolon + 1);
        
        if (!isValidMacFormat(macPart)) {
            result.errorMessage = "Invalid MAC address format in device response";
            return result;
        }
        
        result.macAddress = normalizeMacAddress(macPart);
        
        // 检查第二个分号的存在来判断数据类型 / Check for second semicolon to determine data type
        int secondSemicolon = remainingPart.indexOf(';');
        if (secondSemicolon > 0) {
            // 有第二个分号，说明是状态数据格式: MAC;status;voltage / Has second semicolon, indicating status data format: MAC;status;voltage
            String statusStr = remainingPart.substring(0, secondSemicolon);
            String voltageStr = remainingPart.substring(secondSemicolon + 1);
            
            // 检查是否为广播地址（按钮按下时的广播） / Check if broadcast address (broadcast when button is pressed)
            if (isBroadcastAddress(result.macAddress)) {
                result.commandType = COMMAND_BROADCAST;
            } else {
                result.commandType = COMMAND_STATUS_RETURN;
            }
            
            // 解析状态 / Parse status
            if (statusStr == "1") {
                result.switchState = true;
            } else if (statusStr == "0") {
                result.switchState = false;
            } else {
                result.errorMessage = "Invalid status value: " + statusStr;
                return result;
            }
            
            // 解析电压 / Parse voltage
            result.voltage = parseVoltage(voltageStr);
            if (result.voltage < 0) {
                result.errorMessage = "Invalid voltage format: " + voltageStr;
                return result;
            }
            
            result.isValid = true;
        } else {
            // 没有第二个分号，说明是版本数据格式: MAC;version / No second semicolon, indicating version data format: MAC;version
            result.commandType = COMMAND_VERSION_RETURN;
            result.version = remainingPart;
            
            // 简单验证版本格式（至少应该包含数字和点） / Simple version format validation (should at least contain numbers and dots)
            if (result.version.length() == 0) {
                result.errorMessage = "Empty version string";
                return result;
            }
            
            result.isValid = true;
        }
    } else {
        result.errorMessage = "Unrecognized device response format - expecting MAC;status;voltage or MAC;version";
        return result;
    }
    
    return result;
}

// 检查是否为广播地址 / Check if broadcast address
bool M5SwitchC6::isBroadcastAddress(const String& macStr) {
    // 统一成规范化的连字符大写格式后再比较 / Normalize to standardized hyphen uppercase format before comparison
    String norm = normalizeMacAddress(macStr);
    return norm == "FFFF-FFFF-FFFF";
}

// 检查MAC地址格式是否有效 / Check if MAC address format is valid
bool M5SwitchC6::isValidMacFormat(const String& macStr) {
    // 支持两种格式： / Support two formats:
    // 1) 连字符格式："XXXX-XXXX-XXXX"（长度14，4和9位是'-'） / 1) Hyphen format: "XXXX-XXXX-XXXX" (length 14, positions 4 and 9 are '-')
    // 2) 冒号格式：  "XX:XX:XX:XX:XX:XX"（长度17，2/5/8/11/14位是':'） / 2) Colon format: "XX:XX:XX:XX:XX:XX" (length 17, positions 2/5/8/11/14 are ':')
    int len = macStr.length();

    auto isHex = [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    };

    if (len == 14 && macStr.charAt(4) == '-' && macStr.charAt(9) == '-') {
        // 验证连字符格式的十六进制字符 / Validate hexadecimal characters in hyphen format
        for (int i = 0; i < len; i++) {
            if (i == 4 || i == 9) continue;
            if (!isHex(macStr.charAt(i))) return false;
        }
        return true;
    }

    if (len == 17 && macStr.charAt(2) == ':' && macStr.charAt(5) == ':' &&
        macStr.charAt(8) == ':' && macStr.charAt(11) == ':' && macStr.charAt(14) == ':') {
        // 验证冒号格式的十六进制字符 / Validate hexadecimal characters in colon format
        for (int i = 0; i < len; i++) {
            if (i == 2 || i == 5 || i == 8 || i == 11 || i == 14) continue;
            if (!isHex(macStr.charAt(i))) return false;
        }
        return true;
    }

    return false;
}

// 规范化MAC地址格式（转为大写并输出为连字符格式：XXXX-XXXX-XXXX） / Normalize MAC address format (convert to uppercase and output as hyphen format: XXXX-XXXX-XXXX)
String M5SwitchC6::normalizeMacAddress(const String& macStr) {
    // 提取十六进制字符，忽略分隔符（'-'、':' 等） / Extract hexadecimal characters, ignore separators ('-', ':' etc.)
    String hexOnly = "";
    hexOnly.reserve(12);
    for (int i = 0; i < macStr.length(); i++) {
        char c = macStr.charAt(i);
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
            // 转为大写后加入 / Convert to uppercase and add
            if (c >= 'a' && c <= 'f') c = c - 'a' + 'A';
            hexOnly += c;
        }
    }

    if (hexOnly.length() == 12) {
        // 重组为 XXXX-XXXX-XXXX / Reformat as XXXX-XXXX-XXXX
        return hexOnly.substring(0, 4) + "-" + hexOnly.substring(4, 8) + "-" + hexOnly.substring(8, 12);
    }

    // 长度异常时，退化为原字符串的大写并将冒号替换为连字符（以尽量保持兼容） / When length is abnormal, fallback to uppercase original string with colons replaced by hyphens (to maintain compatibility)
    String upper = macStr;
    upper.toUpperCase();
    upper.replace(':', '-');
    return upper;
}

// 发送开关控制命令 / Send switch control command
bool M5SwitchC6::sendSwitchCommand(const String& deviceMac, bool switchOn, bool needResponse) {
#if !defined(ARDUINO_ARCH_ESP32) && !defined(ARDUINO_ARCH_ESP8266)
    serialPrintln("sendSwitchCommand is only supported on ESP32 or ESP8266 platforms.");
    return false;
#endif

    // 获取当前信道 / Get current channel
    uint8_t channel = getChannel();
    if (channel < 1 || channel > 14) {
        serialPrintln("Invalid current channel for sendSwitchCommand. Channel must be between 1 and 14.");
        return false;
    }

    // 检查MAC地址格式 / Check MAC address format
    if (!isValidMacFormat(deviceMac)) {
        serialPrintln("Invalid MAC address format for sendSwitchCommand: " + deviceMac);
        return false;
    }

    // 构建命令字符串 / Build command string
    // 格式: "XXXX-XXXX-XXXX=1;ch=n" 或 "XXXX-XXXX-XXXX=1;ch=n;" / Format: "XXXX-XXXX-XXXX=1;ch=n" or "XXXX-XXXX-XXXX=1;ch=n;"
    String command = normalizeMacAddress(deviceMac) + "=" + (switchOn ? "1" : "0") + ";ch=" + String(channel);
    if (needResponse) {
        command += ";";
    }

    return sendCustomMessage(command);
}

// 发送状态查询命令 / Send status query command
bool M5SwitchC6::sendStatusQuery(const String& deviceMac) {
#if !defined(ARDUINO_ARCH_ESP32) && !defined(ARDUINO_ARCH_ESP8266)
    serialPrintln("sendStatusQuery is only supported on ESP32 or ESP8266 platforms.");
    return false;
#endif

    // 获取当前信道 / Get current channel
    uint8_t channel = getChannel();
    if (channel < 1 || channel > 14) {
        serialPrintln("Invalid current channel for sendStatusQuery. Channel must be between 1 and 14.");
        return false;
    }

    // 检查MAC地址格式 / Check MAC address format
    if (!isValidMacFormat(deviceMac)) {
        serialPrintln("Invalid MAC address format for sendStatusQuery: " + deviceMac);
        return false;
    }

    // 构建查询命令字符串 / Build query command string
    // 格式: "XXXX-XXXX-XXXX=?;ch=n;" / Format: "XXXX-XXXX-XXXX=?;ch=n;"
    String command = normalizeMacAddress(deviceMac) + "=?;ch=" + String(channel) + ";";

    return sendCustomMessage(command);
}

// 发送版本查询命令 / Send version query command
bool M5SwitchC6::sendVersionQuery(const String& deviceMac) {
#if !defined(ARDUINO_ARCH_ESP32) && !defined(ARDUINO_ARCH_ESP8266)
    serialPrintln("sendVersionQuery is only supported on ESP32 or ESP8266 platforms.");
    return false;
#endif

    // 获取当前信道 / Get current channel
    uint8_t channel = getChannel();
    if (channel < 1 || channel > 14) {
        serialPrintln("Invalid current channel for sendVersionQuery. Channel must be between 1 and 14.");
        return false;
    }

    // 检查MAC地址格式 / Check MAC address format
    if (!isValidMacFormat(deviceMac)) {
        serialPrintln("Invalid MAC address format for sendVersionQuery: " + deviceMac);
        return false;
    }

    // 构建版本查询命令字符串 / Build version query command string
    // 格式: "XXXX-XXXX-XXXX=V;ch=n;" / Format: "XXXX-XXXX-XXXX=V;ch=n;"
    String command = normalizeMacAddress(deviceMac) + "=V;ch=" + String(channel) + ";";

    return sendCustomMessage(command);
}

// 发送自定义消息 / Send custom message
bool M5SwitchC6::sendCustomMessage(const String& message) {
#if !defined(ARDUINO_ARCH_ESP32) && !defined(ARDUINO_ARCH_ESP8266)
    serialPrintln("sendCustomMessage is only supported on ESP32 or ESP8266 platforms.");
    return false;
#endif

    if (message.length() == 0) {
        serialPrintln("Cannot send empty message.");
        return false;
    }

    // 获取互斥锁 / Acquire mutex lock
    acquireMutex();

    // 添加广播对等节点（如果还没添加） / Add broadcast peer node (if not already added)
    uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcastMac, 6);
    peerInfo.channel = 0;  // 0表示使用当前信道 / 0 means use current channel
    peerInfo.encrypt = false;

    // 检查是否已经添加了广播对等节点 / Check if broadcast peer node has already been added
    if (!esp_now_is_peer_exist(broadcastMac)) {
        esp_err_t result = esp_now_add_peer(&peerInfo);
        if (result != ESP_OK) {
            serialPrintln("Failed to add broadcast peer for sending. Error: " + String(result));
            releaseMutex();
            return false;
        }
        serialPrintln("Broadcast peer added successfully for sending.");
    }

    // 发送数据 / Send data
    esp_err_t result = esp_now_send(broadcastMac, (uint8_t*)message.c_str(), message.length());
    
    // 释放互斥锁 / Release mutex lock
    releaseMutex();

    if (result == ESP_OK) {
        serialPrintln("Message sent successfully: " + message);
        return true;
    } else {
        serialPrintln("Failed to send message: " + message + ". Error: " + String(result));
        return false;
    }
}

// 发送开关命令并等待响应 / Send switch command and wait for response
bool M5SwitchC6::sendSwitchCommandWithResponse(const String& deviceMac, bool switchOn, 
                                                     unsigned long timeoutMs, SwitchC6ParsedData_t* response) {
    // 添加响应等待 / Add response wait
    if (!addResponseWait(deviceMac, timeoutMs)) {
        serialPrintln("Failed to add response wait for device: " + deviceMac);
        return false;
    }

    // 发送命令（带响应） / Send command (with response)
    bool sendResult = sendSwitchCommand(deviceMac, switchOn, true);
    if (!sendResult) {
        removeResponseWait(deviceMac);
        return false;
    }

    // 等待响应 / Wait for response
    unsigned long startTime = millis();
    unsigned long lastSendTime = startTime;

    while (millis() - startTime < timeoutMs) {
        checkResponseTimeouts();  // 检查超时 / Check timeouts
        
        int waitIndex = findResponseWait(deviceMac);
        if (waitIndex >= 0 && responseWaits[waitIndex].hasResponse) {
            // 收到响应 / Response received
            if (response != nullptr) {
                *response = responseWaits[waitIndex].response;
            }
            removeResponseWait(deviceMac);
            serialPrintln("Received response from device: " + deviceMac);
            return true;
        }
        
        // 检查是否需要重发命令 / Check if command needs to be resent
        unsigned long currentTime = millis();
        if (currentTime - lastSendTime >= resendInterval) {
            serialPrintln("Resending command to device: " + deviceMac);
            sendSwitchCommand(deviceMac, switchOn, true);
            lastSendTime = currentTime;
        }
        delay(1);  // 避免过度占用CPU / Avoid excessive CPU usage
    }

    // 超时 / Timeout
    removeResponseWait(deviceMac);
    serialPrintln("Timeout waiting for response from device: " + deviceMac);
    return false;
}

// 发送状态查询并等待响应 / Send status query and wait for response
bool M5SwitchC6::sendStatusQueryWithResponse(const String& deviceMac,
                                                   unsigned long timeoutMs, SwitchC6ParsedData_t* response) {
    // 添加响应等待 / Add response wait
    if (!addResponseWait(deviceMac, timeoutMs)) {
        serialPrintln("Failed to add response wait for device: " + deviceMac);
        return false;
    }

    // 发送查询命令 / Send query command
    bool sendResult = sendStatusQuery(deviceMac);
    if (!sendResult) {
        removeResponseWait(deviceMac);
        return false;
    }

    // 等待响应 / Wait for response
    unsigned long startTime = millis();
    unsigned long lastSendTime = startTime;
    
    while (millis() - startTime < timeoutMs) {
        checkResponseTimeouts();  // 检查超时 / Check timeouts
        
        int waitIndex = findResponseWait(deviceMac);
        if (waitIndex >= 0 && responseWaits[waitIndex].hasResponse) {
            // 收到响应 / Response received
            if (response != nullptr) {
                *response = responseWaits[waitIndex].response;
            }
            removeResponseWait(deviceMac);
            serialPrintln("Received status response from device: " + deviceMac);
            return true;
        }
        
        // 检查是否需要重发命令 / Check if command needs to be resent
        unsigned long currentTime = millis();
        if (currentTime - lastSendTime >= resendInterval) {
            serialPrintln("Resending status query to device: " + deviceMac);
            sendStatusQuery(deviceMac);
            lastSendTime = currentTime;
        }
        
        delay(1);  // 避免过度占用CPU / Avoid excessive CPU usage
    }

    // 超时 / Timeout
    removeResponseWait(deviceMac);
    serialPrintln("Timeout waiting for status response from device: " + deviceMac);
    return false;
}

// 发送版本查询并等待响应 / Send version query and wait for response
bool M5SwitchC6::sendVersionQueryWithResponse(const String& deviceMac,
                                                    unsigned long timeoutMs, SwitchC6ParsedData_t* response) {
    // 添加响应等待 / Add response wait
    if (!addResponseWait(deviceMac, timeoutMs)) {
        serialPrintln("Failed to add response wait for device: " + deviceMac);
        return false;
    }

    // 发送版本查询命令 / Send version query command
    bool sendResult = sendVersionQuery(deviceMac);
    if (!sendResult) {
        removeResponseWait(deviceMac);
        return false;
    }

    // 等待响应 / Wait for response
    unsigned long startTime = millis();
    unsigned long lastSendTime = startTime;
    
    while (millis() - startTime < timeoutMs) {
        checkResponseTimeouts();  // 检查超时 / Check timeouts
        
        int waitIndex = findResponseWait(deviceMac);
        if (waitIndex >= 0 && responseWaits[waitIndex].hasResponse) {
            // 收到响应 / Response received
            if (response != nullptr) {
                *response = responseWaits[waitIndex].response;
            }
            removeResponseWait(deviceMac);
            serialPrintln("Received version response from device: " + deviceMac);
            return true;
        }
        
        // 检查是否需要重发命令 / Check if command needs to be resent
        unsigned long currentTime = millis();
        if (currentTime - lastSendTime >= resendInterval) {
            serialPrintln("Resending version query to device: " + deviceMac);
            sendVersionQuery(deviceMac);
            lastSendTime = currentTime;
        }

        delay(1);  // 避免过度占用CPU / Avoid excessive CPU usage
    }

    // 超时 / Timeout
    removeResponseWait(deviceMac);
    serialPrintln("Timeout waiting for version response from device: " + deviceMac);
    return false;
}

// 检查是否有等待中的响应 / Check if there are pending responses
bool M5SwitchC6::isWaitingForResponse() {
    checkResponseTimeouts();  // 先检查超时 / Check timeouts first
    return responseWaitCount > 0;
}

// 获取响应等待信息 / Get response wait information
bool M5SwitchC6::getResponseWaitInfo(const String& deviceMac, ResponseWaitInfo_t* waitInfo) {
    if (waitInfo == nullptr) {
        return false;
    }

    int index = findResponseWait(deviceMac);
    if (index >= 0) {
        *waitInfo = responseWaits[index];
        return true;
    }
    return false;
}

// 添加响应等待 / Add response wait
bool M5SwitchC6::addResponseWait(const String& deviceMac, unsigned long timeoutMs) {
    // 检查是否已经在等待这个设备的响应 / Check if already waiting for response from this device
    if (findResponseWait(deviceMac) >= 0) {
        serialPrintln("Already waiting for response from device: " + deviceMac);
        return false;
    }

    // 查找空闲的等待槽 / Find available wait slot
    for (uint16_t i = 0; i < MAX_RESPONSE_WAITS; i++) {
        if (!responseWaits[i].isWaiting) {
            responseWaits[i].deviceMac = normalizeMacAddress(deviceMac);
            responseWaits[i].requestTime = millis();
            responseWaits[i].timeoutMs = timeoutMs;
            responseWaits[i].isWaiting = true;
            responseWaits[i].hasResponse = false;
            responseWaitCount++;
            serialPrintln("Added response wait for device: " + deviceMac + ", timeout: " + String(timeoutMs) + "ms");
            return true;
        }
    }

    serialPrintln("No available response wait slots for device: " + deviceMac);
    return false;
}

// 移除响应等待 / Remove response wait
void M5SwitchC6::removeResponseWait(const String& deviceMac) {
    int index = findResponseWait(deviceMac);
    if (index >= 0) {
        responseWaits[index].isWaiting = false;
        responseWaits[index].hasResponse = false;
        responseWaits[index].deviceMac = "";
        responseWaits[index].requestTime = 0;
        responseWaits[index].timeoutMs = 0;
        responseWaitCount--;
        serialPrintln("Removed response wait for device: " + deviceMac);
    }
}

// 查找响应等待 / Find response wait
int M5SwitchC6::findResponseWait(const String& deviceMac) {
    String normalizedMac = normalizeMacAddress(deviceMac);
    // serialPrintln("findResponseWait: Looking for device: " + normalizedMac);
    
    for (uint16_t i = 0; i < MAX_RESPONSE_WAITS; i++) {
        if (responseWaits[i].isWaiting) {
            // serialPrintln("findResponseWait: Slot " + String(i) + " waiting for: " + responseWaits[i].deviceMac);
            if (responseWaits[i].deviceMac == normalizedMac) {
                serialPrintln("findResponseWait: Found match at index " + String(i));
                return i;
            }
        }
    }
    serialPrintln("findResponseWait: No match found for: " + normalizedMac);
    return -1;
}

// 检查响应超时 / Check response timeouts
void M5SwitchC6::checkResponseTimeouts() {
    unsigned long currentTime = millis();
    for (uint16_t i = 0; i < MAX_RESPONSE_WAITS; i++) {
        if (responseWaits[i].isWaiting && !responseWaits[i].hasResponse) {
            if (currentTime - responseWaits[i].requestTime >= responseWaits[i].timeoutMs) {
                serialPrintln("Response timeout for device: " + responseWaits[i].deviceMac);
                responseWaits[i].isWaiting = false;
                responseWaits[i].deviceMac = "";
                responseWaits[i].requestTime = 0;
                responseWaits[i].timeoutMs = 0;
                responseWaitCount--;
            }
        }
    }
}

// 处理收到的响应（带ESP-NOW发送方MAC地址） / Handle received response (with ESP-NOW sender MAC address)
void M5SwitchC6::handleReceivedResponse(const SwitchC6ParsedData_t& parsedData, const String& senderMac) {
    if (!parsedData.isValid) {
        serialPrintln("handleReceivedResponse: Invalid parsed data");
        return;
    }

    serialPrintln("handleReceivedResponse: DataMAC=" + parsedData.macAddress + 
                  ", SenderMAC=" + senderMac +
                  ", CommandType=" + String(parsedData.commandType) + 
                  ", State=" + String(parsedData.switchState ? "ON" : "OFF") +
                  ", Version=" + parsedData.version);

    // 处理状态返回、广播和版本响应 / Handle status return, broadcast and version response
    if (parsedData.commandType != COMMAND_STATUS_RETURN && 
        parsedData.commandType != COMMAND_BROADCAST && 
        parsedData.commandType != COMMAND_VERSION_RETURN) {
        serialPrintln("handleReceivedResponse: Ignoring unhandled response type");
        return;
    }

    // 根据ESP-NOW发送方MAC地址查找等待的响应 / Find waiting response based on ESP-NOW sender MAC address
    int index = findResponseWait(senderMac);
    if (index >= 0) {
        responseWaits[index].hasResponse = true;
        responseWaits[index].response = parsedData;
        serialPrintln("Matched response from sender: " + senderMac + " for waiting device: " + responseWaits[index].deviceMac);
        return;
    }
    
    serialPrintln("handleReceivedResponse: No waiting device found for sender MAC: " + senderMac);
}

// 处理收到的响应（兼容旧版本，自动查找第一个等待的设备） / Handle received response (compatible with old version, automatically find first waiting device)
void M5SwitchC6::handleReceivedResponse(const SwitchC6ParsedData_t& parsedData) {
    if (!parsedData.isValid) {
        serialPrintln("handleReceivedResponse: Invalid parsed data");
        return;
    }

    serialPrintln("handleReceivedResponse: MAC=" + parsedData.macAddress + 
                  ", CommandType=" + String(parsedData.commandType) + 
                  ", State=" + String(parsedData.switchState ? "ON" : "OFF") +
                  ", Version=" + parsedData.version);

    // 处理状态返回、广播和版本响应 / Handle status return, broadcast and version response
    if (parsedData.commandType != COMMAND_STATUS_RETURN && 
        parsedData.commandType != COMMAND_BROADCAST && 
        parsedData.commandType != COMMAND_VERSION_RETURN) {
        serialPrintln("handleReceivedResponse: Ignoring unhandled response type");
        return;
    }

    // 查找第一个正在等待响应的设备（简化版本） / Find first device waiting for response (simplified version)
    serialPrintln("handleReceivedResponse: Looking for any waiting device");
    for (uint16_t i = 0; i < MAX_RESPONSE_WAITS; i++) {
        if (responseWaits[i].isWaiting && !responseWaits[i].hasResponse) {
            responseWaits[i].hasResponse = true;
            responseWaits[i].response = parsedData;
            serialPrintln("Matched response for waiting device: " + responseWaits[i].deviceMac + 
                         " (received from device ID: " + parsedData.macAddress + ")");
            return; // 只匹配第一个等待的设备 / Only match first waiting device
        }
    }
    serialPrintln("handleReceivedResponse: No waiting device found for any response");
}

/**
 * @brief Set the resend interval for ESP-NOW communication.
 */
bool M5SwitchC6::setResendInterval(unsigned long intervalMs) {
    // 验证间隔范围：1ms到10000ms (10秒) / Validate interval range: 1ms to 10000ms (10 seconds)
    if (intervalMs < 1 || intervalMs > 10000) {
        serialPrintln("setResendInterval: Invalid interval " + String(intervalMs) + "ms (range: 1-10000ms)");
        return false;
    }
    
    acquireMutex();
    resendInterval = intervalMs;
    releaseMutex();
    
    serialPrintln("setResendInterval: Set to " + String(intervalMs) + "ms");
    return true;
}

/**
 * @brief Get the current resend interval.
 */
unsigned long M5SwitchC6::getResendInterval() {
    acquireMutex();
    unsigned long interval = resendInterval;
    releaseMutex();
    
    return interval;
}

/**
 * @brief Clear all discovered peer devices data.
 */
bool M5SwitchC6::clearPeersData() {
    if (peers == nullptr) {
        serialPrintln("Peers array is not initialized.");
        return false;
    }
    
    acquireMutex();
    
    // 清空所有设备数据 / Clear all device data
    for (uint16_t i = 0; i < max_broadcast_peers; i++) {
        memset(peers[i].senderMac, 0, sizeof(peers[i].senderMac));
        peers[i].lastData = "";
        peers[i].channel = 0;
        peers[i].rssi = 0;
        peers[i].lastReceiveTime = 0;
        peers[i].status = false;
    }
    
    // 重置设备计数 / Reset device count
    peerCount = 0;
    
    releaseMutex();
    
    serialPrintln("All peers data cleared. Device count reset to 0.");
    return true;
}

/**
 * @brief Clear all pending response wait requests.
 */
bool M5SwitchC6::clearResponseWaits() {
    acquireMutex();
    
    // 清空所有响应等待 / Clear all response waits
    for (uint16_t i = 0; i < MAX_RESPONSE_WAITS; i++) {
        responseWaits[i].isWaiting = false;
        responseWaits[i].hasResponse = false;
        responseWaits[i].deviceMac = "";
        responseWaits[i].requestTime = 0;
        responseWaits[i].timeoutMs = 0;
        // 清空响应数据 / Clear response data
        responseWaits[i].response.isValid = false;
        responseWaits[i].response.commandType = COMMAND_UNKNOWN;
        responseWaits[i].response.macAddress = "";
        responseWaits[i].response.switchState = false;
        responseWaits[i].response.voltage = 0.0f;
        responseWaits[i].response.version = "";
        responseWaits[i].response.errorMessage = "";
    }
    
    // 重置等待计数 / Reset wait count
    responseWaitCount = 0;
    
    releaseMutex();
    
    serialPrintln("All response waits cleared. Wait count reset to 0.");
    return true;
}

/**
 * @brief Clear all historical data including peers and response waits.
 */
bool M5SwitchC6::clearAllData() {
    serialPrintln("Clearing all historical data...");
    
    bool peersCleared = clearPeersData();
    bool waitsCleared = clearResponseWaits();
    
    if (peersCleared && waitsCleared) {
        serialPrintln("All historical data cleared successfully.");
        return true;
    } else {
        serialPrintln("Failed to clear some historical data.");
        return false;
    }
}

/**
 * @brief Clear specific device data by MAC address.
 */
bool M5SwitchC6::clearDeviceData(const String& deviceMac) {
    if (peers == nullptr) {
        serialPrintln("Peers array is not initialized.");
        return false;
    }
    
    if (deviceMac.isEmpty()) {
        serialPrintln("Device MAC address is empty.");
        return false;
    }
    
    // 标准化MAC地址格式 / Normalize MAC address format
    String normalizedMac = normalizeMacAddress(deviceMac);
    bool deviceFound = false;
    
    acquireMutex();
    
    // 查找并移除指定设备 / Find and remove specified device
    for (uint16_t i = 0; i < peerCount; i++) {
        String currentMac = macToString(peers[i].senderMac);
        String normalizedCurrentMac = normalizeMacAddress(currentMac);
        
        if (normalizedCurrentMac.equals(normalizedMac)) {
            serialPrintf("Found device %s at index %d, removing...\n", currentMac.c_str(), i);
            
            // 将后面的设备前移 / Move subsequent devices forward
            for (uint16_t j = i; j < peerCount - 1; j++) {
                memcpy(&peers[j], &peers[j + 1], sizeof(broadcast_packet_with_mac_t));
            }
            
            // 清空最后一个位置 / Clear last position
            memset(&peers[peerCount - 1], 0, sizeof(broadcast_packet_with_mac_t));
            peers[peerCount - 1].lastData = "";
            
            // 减少设备计数 / Decrease device count
            peerCount--;
            deviceFound = true;
            break;
        }
    }
    
    releaseMutex();
    
    // 同时清除该设备的等待响应 / Also clear waiting response for this device
    if (deviceFound) {
        removeResponseWait(normalizedMac);
        serialPrintf("Device %s data cleared successfully.\n", deviceMac.c_str());
        return true;
    } else {
        serialPrintf("Device %s not found in peer list.\n", deviceMac.c_str());
        return false;
    }
}

/**
 * @brief Clear devices that haven't been seen for a specified time.
 */
uint16_t M5SwitchC6::clearExpiredDevices(unsigned long timeoutMs) {
    if (peers == nullptr) {
        serialPrintln("Peers array is not initialized.");
        return 0;
    }
    
    unsigned long currentTime = millis();
    uint16_t removedCount = 0;
    
    acquireMutex();
    
    // 从后向前遍历，避免移动元素时的索引问题 / Traverse from back to front to avoid index issues when moving elements
    for (int i = peerCount - 1; i >= 0; i--) {
        unsigned long timeSinceLastReceive = currentTime - peers[i].lastReceiveTime;
        
        // 检查是否超时（考虑millis()溢出情况） / Check if timeout (considering millis() overflow)
        bool isExpired = false;
        if (currentTime >= peers[i].lastReceiveTime) {
            isExpired = (timeSinceLastReceive > timeoutMs);
        } else {
            // millis()溢出的情况 / millis() overflow case
            isExpired = ((ULONG_MAX - peers[i].lastReceiveTime + currentTime) > timeoutMs);
        }
        
        if (isExpired) {
            String deviceMac = macToString(peers[i].senderMac);
            serialPrintf("Removing expired device %s (last seen %lu ms ago)\n", 
                        deviceMac.c_str(), timeSinceLastReceive);
            
            // 将后面的设备前移 / Move subsequent devices forward
            for (uint16_t j = i; j < peerCount - 1; j++) {
                memcpy(&peers[j], &peers[j + 1], sizeof(broadcast_packet_with_mac_t));
            }
            
            // 清空最后一个位置 / Clear last position
            memset(&peers[peerCount - 1], 0, sizeof(broadcast_packet_with_mac_t));
            peers[peerCount - 1].lastData = "";
            
            // 减少设备计数 / Decrease device count
            peerCount--;
            removedCount++;
            
            // 同时清除该设备的等待响应 / Also clear waiting response for this device
            removeResponseWait(normalizeMacAddress(deviceMac));
        }
    }
    
    releaseMutex();
    
    if (removedCount > 0) {
        serialPrintf("Removed %d expired devices (timeout: %lu ms)\n", removedCount, timeoutMs);
    }
    
    return removedCount;
}

/**
 * @brief Set automatic cleanup interval for expired devices.
 */
bool M5SwitchC6::setAutoCleanup(unsigned long intervalMs, unsigned long deviceTimeoutMs) {
    if (deviceTimeoutMs == 0) {
        serialPrintln("Device timeout cannot be 0.");
        return false;
    }
    
    acquireMutex();
    
    autoCleanupInterval = intervalMs;
    deviceTimeout = deviceTimeoutMs;
    lastCleanupTime = millis();
    
    releaseMutex();
    
    if (intervalMs == 0) {
        serialPrintln("Automatic device cleanup disabled.");
    } else {
        serialPrintf("Automatic device cleanup enabled: interval=%lu ms, device_timeout=%lu ms\n", 
                    intervalMs, deviceTimeoutMs);
    }
    
    return true;
}

/**
 * @brief Check and perform automatic cleanup of expired devices.
 */
uint16_t M5SwitchC6::checkAutoCleanup() {
    // 如果自动清理被禁用，直接返回 / Return directly if auto cleanup is disabled
    if (autoCleanupInterval == 0) {
        return 0;
    }
    
    unsigned long currentTime = millis();
    unsigned long timeSinceLastCleanup;
    
    // 检查是否到了清理时间（考虑millis()溢出情况） / Check if it's cleanup time (considering millis() overflow)
    if (currentTime >= lastCleanupTime) {
        timeSinceLastCleanup = currentTime - lastCleanupTime;
    } else {
        // millis()溢出的情况 / millis() overflow case
        timeSinceLastCleanup = ULONG_MAX - lastCleanupTime + currentTime;
    }
    
    if (timeSinceLastCleanup >= autoCleanupInterval) {
        // 执行自动清理 / Execute auto cleanup
        uint16_t removedCount = clearExpiredDevices(deviceTimeout);
        
        // 更新上次清理时间 / Update last cleanup time
        acquireMutex();
        lastCleanupTime = currentTime;
        releaseMutex();
        
        if (removedCount > 0) {
            serialPrintf("Auto cleanup completed: %d devices removed\n", removedCount);
        }
        
        return removedCount;
    }
    
    return 0;
}

// 解析电压字符串的辅助函数 / Helper function to parse voltage string
float M5SwitchC6::parseVoltage(const String& voltageStr) {
    // 移除单位 'V' 并转换为浮点数 / Remove unit 'V' and convert to float
    String cleanStr = voltageStr;
    cleanStr.replace("V", "");
    cleanStr.replace("v", "");
    cleanStr.trim();
    
    if (cleanStr.length() == 0) {
        return -1.0f; // 表示解析失败 / Indicates parsing failure
    }
        
    float voltage = cleanStr.toFloat();
    
    // 检查toFloat是否成功解析（如果输入是"0"或"0.0"等有效的零值，这是正确的） / Check if toFloat successfully parsed (if input is "0" or "0.0" etc. valid zero values, this is correct)
    if (voltage == 0.0f && cleanStr != "0" && cleanStr != "0.0" && cleanStr != "0.00" && !cleanStr.startsWith("0.")) {
        return -1.0f; // toFloat失败，返回了默认值0.0 / toFloat failed, returned default value 0.0
    }
    
    // 验证电压范围（考虑SwitchC6的实际工作范围：通常在2.0V到4.2V之间，但允许更宽的范围以适应不同情况） / Validate voltage range (considering actual working range of SwitchC6: usually between 2.0V and 4.2V, but allow wider range for different situations)
    if (voltage < 0.0f || voltage > 6.0f) {
        return -1.0f; // 表示解析失败 / Indicates parsing failure
    }
    
    return voltage;
}