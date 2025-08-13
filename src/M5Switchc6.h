/*
 *SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 *SPDX-License-Identifier: MIT
 */

#ifndef __M5SWITCHC6_H__
#define __M5SWITCHC6_H__

#include "Arduino.h"
#include <WiFi.h>

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_idf_version.h>
#endif

#define M5_DEVICE_NAME "M5SwitchC6"

// #define M5_MODULE_DEBUG_SERIAL Serial // The corresponding serial port must be initialized before use
// This macro definition can be annotated without sending and receiving data prints
// Define the serial port you want to use, e.g., Serial1 or Serial2
#define FORMAT_TIMESTAMP() ([]() -> String { \
    unsigned long ms = millis(); \
    unsigned long total_seconds = ms / 1000; \
    unsigned long hours = total_seconds / 3600; \
    unsigned long minutes = (total_seconds % 3600) / 60; \
    unsigned long seconds = total_seconds % 60; \
    float sec_frac = seconds + (ms % 1000) / 1000.0f; \
    char buf[20]; \
    snprintf(buf, sizeof(buf), "%02luh:%02lum:%.3lfs\0", hours, minutes, sec_frac); \
    return String(buf); \
})()

#if defined M5_MODULE_DEBUG_SERIAL
#define serialPrint(...)   do { \
    char _msg_buf[256]; \
    String _timestamp = FORMAT_TIMESTAMP(); \
    snprintf(_msg_buf, sizeof(_msg_buf), "[at %s] %s: ", _timestamp.c_str(), M5_DEVICE_NAME); \
    M5_MODULE_DEBUG_SERIAL.print(_msg_buf); \
    M5_MODULE_DEBUG_SERIAL.print(__VA_ARGS__); \
    M5_MODULE_DEBUG_SERIAL.flush(); \
} while(0)

#define serialPrintln(...) do { \
    char _msg_buf[256]; \
    String _timestamp = FORMAT_TIMESTAMP(); \
    snprintf(_msg_buf, sizeof(_msg_buf), "[at %s] %s: ", _timestamp.c_str(), M5_DEVICE_NAME); \
    M5_MODULE_DEBUG_SERIAL.print(_msg_buf); \
    M5_MODULE_DEBUG_SERIAL.println(__VA_ARGS__); \
    M5_MODULE_DEBUG_SERIAL.flush(); \
} while(0)

#define serialPrintf(...)  do { \
    char _msg_buf[256]; \
    String _timestamp = FORMAT_TIMESTAMP(); \
    snprintf(_msg_buf, sizeof(_msg_buf), "[at %s] %s: ", _timestamp.c_str(), M5_DEVICE_NAME); \
    M5_MODULE_DEBUG_SERIAL.print(_msg_buf); \
    M5_MODULE_DEBUG_SERIAL.printf(__VA_ARGS__); \
    M5_MODULE_DEBUG_SERIAL.flush(); \
} while(0)

#define serialFlush()      M5_MODULE_DEBUG_SERIAL.flush()
#else
#define serialPrint(...)
#define serialPrintln(...)
#define serialPrintf(...)
#define serialFlush()
#endif

/**
 *  @brief Structure to hold broadcast packet information.
 *  用于保存广播包发送者和其最新数据
 */
struct broadcast_packet_with_mac_t {
  uint8_t senderMac[6];
  String lastData;
  uint8_t channel;
  int rssi;              // 信号强度
  unsigned long lastReceiveTime;  // 最后接收时间
  bool status;            // 状态标志 0:关断 1:开启
};

/**
 * @brief Enumeration for Switch-C6 command types
 * 作为主机，主要接收和解析从机的状态返回和广播数据
 */
enum SwitchC6CommandType_t {
    COMMAND_UNKNOWN = 0,
    COMMAND_STATUS_RETURN = 1, // ;状态;电压 从机状态返回数据
    COMMAND_BROADCAST = 2,     // 从机按钮广播数据
    COMMAND_VERSION_RETURN = 3  // ;版本号 从机版本返回数据
};

/**
 * @brief Structure to hold parsed Switch-C6 device data (Host side)
 * 主机端解析从机设备数据的结构体
 */
struct SwitchC6ParsedData_t {
    SwitchC6CommandType_t commandType; // 命令类型（状态返回或广播）
    String macAddress;               // 设备MAC地址字符串 (格式: XXXX-XXXX-XXXX，广播时为FFFF-FFFF-FFFF)
    bool switchState;                // 开关状态 (true=开, false=关)
    float voltage;                   // 电压值 (例如: 3.30)
    String version;                  // 版本号字符串 (例如: "1.0.0")
    bool isValid;                    // 解析是否成功
    String errorMessage;             // 错误信息
};

/**
 * @brief Structure to hold response waiting information
 * 响应等待信息结构体
 */
struct ResponseWaitInfo_t {
    String deviceMac;                // 等待响应的设备MAC地址
    unsigned long requestTime;       // 请求发送时间
    unsigned long timeoutMs;         // 超时时间（毫秒）
    bool isWaiting;                  // 是否正在等待响应
    bool hasResponse;                // 是否已收到响应
    SwitchC6ParsedData_t response;     // 收到的响应数据
};

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
/**
 * @brief Structure to hold queued data for processing
 */
struct QueuedData_t {
    uint8_t* data;
    int len;
    uint8_t senderMac[6];  // ESP-NOW发送方MAC地址
};
#endif

class M5SwitchC6 {
public:
    /** 构造和析构函数 */
    /**
     * @brief Constructs a new M5SwitchC6 object.
     *
     * This constructor initializes the M5SwitchC6 instance and creates a mutex lock
     * for thread-safe operations on ESP32 or ESP8266 platforms.
     */
    M5SwitchC6(uint16_t max_broadcast_peers);
    ~M5SwitchC6();

    /**
     * @brief Initializes the module.
     *
     * This function initializes the M5SwitchC6 module, setting up necessary configurations
     * and preparing it for use. It should be called before any other operations on the module.
     * 初始化模块，设置必要的配置并准备使用
     */
    bool begin();

    /**
     * @brief Sets the WiFi channel for ESP-NOW communication.
     *
     * This function sets the WiFi channel for ESP-NOW communication. The channel must be
     * between 1 and 14. Note that both devices must be on the same channel to communicate.
     * 设置ESP-NOW通信的WiFi信道
     */
    bool setChannel(uint8_t channel);

    /**
     * @brief Gets the current WiFi channel.
     *
     * This function returns the current WiFi channel being used for communication.
     * 获取当前使用的WiFi信道
     */
    uint8_t getChannel();

    /**
     * @brief Gets the number of detected peers.
     *
     * @return The number of peers that have been detected.
     * 获取检测到的设备数量
     */
    uint16_t getPeerCount();

    /**
     * @brief Gets peer information by index.
     *
     * @param index The index of the peer (0 to getPeerCount()-1).
     * @param peerInfo Pointer to structure to store peer information.
     * @return true if peer exists and information was retrieved, false otherwise.
     * 根据索引获取设备信息
     */
    bool getPeerInfo(uint16_t index, broadcast_packet_with_mac_t* peerInfo);

    /**
     * @brief Parses received device data with sender MAC address.
     * 解析接收的数据，并包含发送者的MAC地址
     *
     * @param data Pointer to the received data buffer
     * @param len Length of the received data
     * @param senderMac ESP-NOW sender MAC address
     */
    void parseData(const uint8_t *data, int len, const uint8_t *senderMac);

    /**
     * @brief Parses received device data and returns structured result (Host side).
     * 解析接收到的数据并返回结构化结果
     *
     * @param data Pointer to the received data buffer
     * @param len Length of the received data
     * @return SwitchC6ParsedData_t structure containing parsed device information
     */
    SwitchC6ParsedData_t parseDataStructured(const uint8_t *data, int len);

    /**
     * @brief Send control command to Switch-C6 device.
     * 发送控制命令到Switch-C6设备
     *
     * 发送控制命令到Switch-C6设备，使用当前信道
     * 
     * @param deviceMac Target device MAC address in format "XXXX-XXXX-XXXX"
     * @param switchOn true to turn on, false to turn off
     * @param needResponse true if response is needed (adds ';' at the end)
     * @return true if command was sent successfully, false otherwise
     */
    bool sendSwitchCommand(const String& deviceMac, bool switchOn, bool needResponse = false);

    /**
     * @brief Send status query to Switch-C6 device.
     * 发送状态查询命令到Switch-C6设备
     *
     * 发送状态查询命令到Switch-C6设备，使用当前信道
     * 
     * @param deviceMac Target device MAC address in format "XXXX-XXXX-XXXX"
     * @return true if query was sent successfully, false otherwise
     */
    bool sendStatusQuery(const String& deviceMac);

    /**
     * @brief Send version query to Switch-C6 device.
     * 发送版本查询命令到Switch-C6设备
     *
     * 发送版本查询命令到Switch-C6设备，使用当前信道
     * 
     * @param deviceMac Target device MAC address in format "XXXX-XXXX-XXXX"
     * @return true if query was sent successfully, false otherwise
     */
    bool sendVersionQuery(const String& deviceMac);

    /**
     * @brief Send custom message via ESP-NOW.
     * 通过ESP-NOW发送自定义消息
     *
     * 发送自定义消息（通用发送函数）
     * 
     * @param message Custom message string to send
     * @return true if message was sent successfully, false otherwise
     */
    bool sendCustomMessage(const String& message);

    /**
     * @brief Send switch command and wait for response.
     * 发送开关命令并等待响应
     *
     * 发送开关命令并等待响应，使用当前信道
     * 
     * @param deviceMac Target device MAC address in format "XXXX-XXXX-XXXX"
     * @param switchOn true to turn on, false to turn off
     * @param timeoutMs Timeout in milliseconds (default 5000ms)
     * @param response Pointer to store the response data (optional)
     * @return true if response received within timeout, false otherwise
     */
    bool sendSwitchCommandWithResponse(const String& deviceMac, bool switchOn, 
                                      unsigned long timeoutMs = 5000, SwitchC6ParsedData_t* response = nullptr);

    /**
     * @brief Send status query and wait for response.
     * 发送状态查询并等待响应
     *
     * 发送状态查询并等待响应，使用当前信道
     * 
     * @param deviceMac Target device MAC address in format "XXXX-XXXX-XXXX"
     * @param timeoutMs Timeout in milliseconds (default 5000ms)
     * @param response Pointer to store the response data (optional)
     * @return true if response received within timeout, false otherwise
     */
    bool sendStatusQueryWithResponse(const String& deviceMac,
                                    unsigned long timeoutMs = 5000, SwitchC6ParsedData_t* response = nullptr);

    /**
     * @brief Send version query and wait for response.
     * 发送版本查询并等待响应
     *
     * 发送版本查询并等待响应，使用当前信道
     * 
     * @param deviceMac Target device MAC address in format "XXXX-XXXX-XXXX"
     * @param timeoutMs Timeout in milliseconds (default 5000ms)
     * @param response Pointer to store the response data (optional)
     * @return true if response received within timeout, false otherwise
     */
    bool sendVersionQueryWithResponse(const String& deviceMac,
                                     unsigned long timeoutMs = 5000, SwitchC6ParsedData_t* response = nullptr);

    /**
     * @brief Check if there are any pending response waits.
     * 检查是否有等待中的响应请求
     *
     * @return true if waiting for any response, false otherwise
     */
    bool isWaitingForResponse();

    /**
     * @brief Get response waiting information for a specific device.
     * 获取指定设备的响应等待信息
     *
     * @param deviceMac Device MAC address
     * @param waitInfo Pointer to store wait information
     * @return true if device is being waited for, false otherwise
     */
    bool getResponseWaitInfo(const String& deviceMac, ResponseWaitInfo_t* waitInfo);

    /**
     * @brief Set the resend interval for ESP-NOW communication.
     * 设置ESP-NOW通信的重发间隔
     *
     * @param intervalMs Resend interval in milliseconds (1-10000ms)
     * @return true if interval was set successfully, false if out of range
     */
    bool setResendInterval(unsigned long intervalMs);

    /**
     * @brief Get the current resend interval.
     * 获取当前的重发间隔
     *
     * @return Current resend interval in milliseconds
     */
    unsigned long getResendInterval();

    /**
     * @brief Clear all discovered peer devices data.
     * 清空所有已发现的设备数据
     *
     * This function clears all stored peer device information including MAC addresses,
     * last received data, signal strength (RSSI), and status information.
     * 此函数清空所有存储的设备信息，包括MAC地址、最后接收的数据、信号强度和状态信息。
     *
     * @return true if peers data was cleared successfully, false otherwise
     */
    bool clearPeersData();

    /**
     * @brief Clear all pending response wait requests.
     * 清空所有等待中的响应请求
     *
     * This function cancels all pending response wait requests and clears the wait queue.
     * 此函数取消所有等待中的响应请求并清空等待队列。
     *
     * @return true if response waits were cleared successfully, false otherwise
     */
    bool clearResponseWaits();

    /**
     * @brief Clear all historical data including peers and response waits.
     * 清空所有历史数据，包括设备信息和响应等待
     *
     * This function performs a complete cleanup of all stored data, including
     * discovered devices and pending response requests.
     * 此函数执行完整的数据清理，包括已发现的设备和等待中的响应请求。
     *
     * @return true if all data was cleared successfully, false otherwise
     */
    bool clearAllData();

    /**
     * @brief Clear specific device data by MAC address.
     * 根据MAC地址清除指定设备数据
     *
     * This function removes a specific device from the peer list by its MAC address.
     * Both peer data and any pending response waits for this device will be cleared.
     * 此函数根据MAC地址从设备列表中移除指定设备。
     * 设备数据和该设备的等待响应都会被清除。
     *
     * @param deviceMac Device MAC address in format "XX:XX:XX:XX:XX:XX" or "XXXX-XXXX-XXXX"
     * @return true if device was found and cleared successfully, false otherwise
     */
    bool clearDeviceData(const String& deviceMac);

    /**
     * @brief Clear devices that haven't been seen for a specified time.
     * 清除指定时间内未收到消息的设备
     *
     * This function removes devices from the peer list if they haven't sent any
     * data for longer than the specified timeout period.
     * 此函数移除在指定超时时间内未发送任何数据的设备。
     *
     * @param timeoutMs Timeout in milliseconds (devices not seen for this long will be removed)
     * @return Number of devices that were removed
     */
    uint16_t clearExpiredDevices(unsigned long timeoutMs = 300000); // 默认5分钟

    /**
     * @brief Set automatic cleanup interval for expired devices.
     * 设置过期设备的自动清理间隔
     *
     * This function enables automatic cleanup of devices that haven't been seen
     * for a specified period. Set to 0 to disable automatic cleanup.
     * 此函数启用对指定时间内未见到的设备的自动清理。设置为0禁用自动清理。
     *
     * @param intervalMs Cleanup interval in milliseconds (0 to disable)
     * @param deviceTimeoutMs Device timeout in milliseconds (devices not seen for this long will be removed)
     * @return true if settings were applied successfully, false otherwise
     */
    bool setAutoCleanup(unsigned long intervalMs, unsigned long deviceTimeoutMs = 300000);

    /**
     * @brief Check and perform automatic cleanup of expired devices.
     * 检查并执行过期设备的自动清理
     *
     * This function should be called periodically (e.g., in loop()) to perform
     * automatic cleanup of expired devices based on the settings from setAutoCleanup().
     * 此函数应定期调用（例如在loop()中）以基于setAutoCleanup()的设置执行过期设备的自动清理。
     *
     * @return Number of devices that were removed, or 0 if no cleanup was needed
     */
    uint16_t checkAutoCleanup();


private:

    uint16_t max_broadcast_peers = 16;  // 最大广播节点数量
    broadcast_packet_with_mac_t* peers = nullptr;  // 广播节点数组
    uint16_t peerCount = 0;  // 当前节点数量
    
    // 响应管理相关变量
    static const uint16_t MAX_RESPONSE_WAITS = 16;  // 最大同时等待响应的数量
    ResponseWaitInfo_t responseWaits[MAX_RESPONSE_WAITS];  // 响应等待数组
    uint16_t responseWaitCount = 0;  // 当前等待响应的数量
    unsigned long resendInterval = 0; // 重发间隔（毫秒）
    
    // 自动清理相关变量
    unsigned long autoCleanupInterval = 0;  // 自动清理间隔（毫秒，0表示禁用）
    unsigned long deviceTimeout = 300000;   // 设备超时时间（毫秒，默认5分钟）
    unsigned long lastCleanupTime = 0;      // 上次清理时间
    
    // 静态实例指针，用于在回调函数中访问类成员
    static M5SwitchC6* instance;
    
    // MAC地址转字符串的辅助函数
    static String macToString(const uint8_t *mac) {
        char buf[18];
        snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return String(buf);
    }

    /**
     * @brief Acquires a mutex lock.
     *
     * This function attempts to acquire a mutex lock to ensure thread-safe access
     * to shared resources. It should be paired with a corresponding call to
     * releaseMutex() to prevent deadlocks.
     */
    #if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    /** FreeRTOS 互斥锁句柄 */
    static SemaphoreHandle_t mutexLock;
    
    /** 队列相关静态变量 */
    static QueueHandle_t dataQueue;
    static TaskHandle_t parseTaskHandle;
    
    /** 队列相关方法 */
    static void parseDataTask(void* parameter);
    bool enqueueData(const uint8_t* data, int len);
    bool enqueueData(const uint8_t* data, int len, const uint8_t* senderMac);
    #endif
    /** 获取互斥锁 */
    void acquireMutex();

    /**
     * @brief Releases a mutex lock.
     *
     * This function releases a previously acquired mutex lock, allowing other
     * threads to access shared resources. It should only be called after
     * successfully acquiring the mutex with acquireMutex().
     */
    /** 释放互斥锁 */
    void releaseMutex();

    /**
     * @brief Helper functions for data parsing
     */
    bool isBroadcastAddress(const String& macStr);
    bool isValidMacFormat(const String& macStr);
    String normalizeMacAddress(const String& macStr);
    float parseVoltage(const String& voltageStr);

    /**
     * @brief Helper functions for response management
     */
    bool addResponseWait(const String& deviceMac, unsigned long timeoutMs);
    void removeResponseWait(const String& deviceMac);
    int findResponseWait(const String& deviceMac);
    void checkResponseTimeouts();
    void handleReceivedResponse(const SwitchC6ParsedData_t& parsedData);
    void handleReceivedResponse(const SwitchC6ParsedData_t& parsedData, const String& senderMac);


};

#endif // __M5SWITCHC6_H__