# M5-SwitchC6 Library

## Overview

M5-SwitchC6 is an ESP-NOW based smart switch control library for controlling and managing Switch-C6 switch devices. It provides a complete device control API including switch control, status query, version query, response management, and more.

### Key Features

- Switch control: remotely turn Switch-C6 devices on/off
- Status query: get device status and voltage in real time
- Version query: retrieve firmware version
- Response management: async communications with timeout
- Channel management: set host ESP-NOW channel
- Device discovery: automatically discover and manage multiple devices
- Signal monitoring: obtain device RSSI
- Broadcast reception: automatically receive button broadcast data
- Multi-device support: manage multiple Switch-C6 devices concurrently
- Device cleanup: intelligently remove offline devices and stale data; optional auto cleanup
- Thread safety: FreeRTOS mutex-protected operations

### Supported Platforms

1. ESP32 family (recommended)
2. M5Stack devices

## Quick Start

### Basic usage

```cpp
#include <M5Switchc6.h>

M5SwitchC6 switchC6(10);
// Fill in your Switch-C6 device MAC address
const String deviceMac = "E4B3-2386-18C4";

void setup() {
    Serial.begin(115200);
  
    // Set WiFi to Station mode
    WiFi.mode(WIFI_STA);
    WiFi.begin();
  
    // Initialize Switch-C6 controller
    if (switchC6.begin()) {
        Serial.println("Switch-C6 initialized successfully");
    } else {
        Serial.println("Switch-C6 initialization failed");
    }
}

void loop() {
    // Run auto cleanup checks
    switchC6.checkAutoCleanup();
  
    // Turn device ON
    if (switchC6.sendSwitchCommand(deviceMac, true)) {
        Serial.println("ON command sent");
    }
    delay(5000);
  
    // Query device status
    SwitchC6ParsedData_t response;
    if (switchC6.sendStatusQueryWithResponse(deviceMac, 3000, &response)) {
        Serial.printf("Status: %s\n", response.switchState ? "ON" : "OFF");
        Serial.printf("Voltage: %.2fV\n", response.voltage);
        Serial.printf("MAC: %s\n", response.macAddress.c_str());
    } else {
        Serial.println("Status query failed or timed out");
    }
  
    delay(5000);
  
    // Turn device OFF
    if (switchC6.sendSwitchCommand(deviceMac, false)) {
        Serial.println("OFF command sent");
    }
    delay(5000);
}
```

### Enable debug logs

In debug builds, you can enable verbose logging by defining the macro below and initializing the chosen serial port in your sketch:

```cpp
#define M5_MODULE_DEBUG_SERIAL Serial0
```

### M5Stack example

```cpp
#include <M5Unified.h>
#include <WiFi.h>
#include <M5Switchc6.h>

M5SwitchC6 switchC6(10);
// Fill in your Switch-C6 device MAC address
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
        // Turn ON
        if (switchC6.sendSwitchCommandWithResponse(DEVICE_MAC, true, 10000)) {
            Serial.println("ON command sent");
            M5.Display.fillRect(10, 100, 300, 30, BLACK);
            M5.Display.setCursor(10, 100);
            M5.Display.println("Device ON sent");
        }
    }
  
    if (M5.BtnB.wasPressed()) {
        // Turn OFF
        if (switchC6.sendSwitchCommandWithResponse(DEVICE_MAC, false, 10000)) {
            Serial.println("OFF command sent");
            M5.Display.fillRect(10, 100, 300, 30, BLACK);
            M5.Display.setCursor(10, 100);
            M5.Display.println("Device OFF sent");
        }
    }
  
    if (M5.BtnC.wasPressed()) {
        // Query status
        SwitchC6ParsedData_t response;
        if (switchC6.sendStatusQueryWithResponse(DEVICE_MAC, 10000, &response)) {
            Serial.printf("Status: %s, Voltage: %.2fV\n", 
                         response.switchState ? "ON" : "OFF", response.voltage);
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

## API Reference

### Initialization and configuration

#### `M5SwitchC6(uint16_t max_broadcast_peers)`

Constructor, creates an M5SwitchC6 instance.

- Parameter:
  - `max_broadcast_peers` - Max number of broadcast devices to support

#### `bool begin()`

Initialize the module and set up ESP-NOW communications.

- Returns: `true` on success, `false` on failure

#### `bool setChannel(uint8_t channel)`

Set ESP-NOW communication channel (advanced).

- Parameter:
  - `channel` - WiFi channel (1-14)
- Returns: `true` success, `false` failure

> Note: The default channel is optimized; most users don’t need to change this.

#### `uint8_t getChannel()`

Get the current communication channel (for debugging).

- Returns: current WiFi channel

### Device control

#### `bool sendSwitchCommand(const String& deviceMac, bool switchOn, bool needResponse = false)`

Send an ON/OFF command.

- Parameters:
  - `deviceMac` - Device MAC (format: "XXXX-XXXX-XXXX")
  - `switchOn` - `true` ON, `false` OFF
  - `needResponse` - Whether a response is required
- Returns: `true` when sent, `false` on failure

#### `bool sendStatusQuery(const String& deviceMac)`

Send a status query command.

- Parameter: `deviceMac` - Device MAC address
- Returns: `true` when sent, `false` on failure

#### `bool sendVersionQuery(const String& deviceMac)`

Send a version query command.

- Parameter: `deviceMac` - Device MAC address
- Returns: `true` when sent, `false` on failure

#### `bool sendCustomMessage(const String& message)`

Send a custom message.

- Parameter: `message` - Custom message string
- Returns: `true` when sent, `false` on failure

### Response management

#### `bool sendSwitchCommandWithResponse(const String& deviceMac, bool switchOn, unsigned long timeoutMs = 5000, SwitchC6ParsedData_t* response = nullptr)`

Send an ON/OFF command and wait for a response.

- Parameters:
  - `deviceMac` - Device MAC address
  - `switchOn` - ON/OFF
  - `timeoutMs` - Timeout in milliseconds (default 5000ms)
  - `response` - Optional pointer to response struct
- Returns: `true` if response received, otherwise `false`

#### `bool sendStatusQueryWithResponse(const String& deviceMac, unsigned long timeoutMs = 5000, SwitchC6ParsedData_t* response = nullptr)`

Send a status query and wait for a response.

- Parameters:
  - `deviceMac` - Device MAC address
  - `timeoutMs` - Timeout in milliseconds (default 5000ms)
  - `response` - Optional pointer to response struct
- Returns: `true` if response received, otherwise `false`

#### `bool sendVersionQueryWithResponse(const String& deviceMac, unsigned long timeoutMs = 5000, SwitchC6ParsedData_t* response = nullptr)`

Send a version query and wait for a response.

- Parameters:
  - `deviceMac` - Device MAC address
  - `timeoutMs` - Timeout in milliseconds (default 5000ms)
  - `response` - Optional pointer to response struct
- Returns: `true` if response received, otherwise `false`

#### `bool isWaitingForResponse()`

Check if there are pending requests waiting for responses.

- Returns: `true` if waiting, `false` otherwise

#### `bool getResponseWaitInfo(const String& deviceMac, ResponseWaitInfo* waitInfo)`

Get waiting info for a specific device.

- Parameters:
  - `deviceMac` - Device MAC address
  - `waitInfo` - Pointer to a wait info struct
- Returns: `true` if the device is waiting for a response, otherwise `false`

### Device management

#### `uint16_t getPeerCount()`

Get the number of detected devices.

- Returns: device count

#### `bool getPeerInfo(uint16_t index, broadcast_packet_with_mac* peerInfo)`

Get device info by index.

- Parameters:
  - `index` - Device index (0 to getPeerCount()-1)
  - `peerInfo` - Pointer to a device info struct
- Returns: `true` on success, `false` if index is invalid

### Advanced configuration

#### `bool setResendInterval(unsigned long intervalMs)`

Set resend interval.

- Parameter:
  - `intervalMs` - Interval in milliseconds (1-10000ms)
- Returns: `true` on success, `false` if out of range

#### `unsigned long getResendInterval()`

Get the current resend interval.

- Returns: interval in milliseconds

### Data management

#### `bool clearPeersData()`

Clear all discovered device data.

- Behavior: clear all stored device info including MAC, last payload, RSSI and status
- Returns: `true` on success, `false` on failure

#### `bool clearResponseWaits()`

Clear all pending response requests.

- Behavior: cancel all pending requests and clear the queue
- Returns: `true` on success, `false` on failure

#### `bool clearAllData()`

Clear all historical data.

- Behavior: perform a full cleanup including discovered devices and response waits
- Returns: `true` if all cleared, `false` if partially failed

#### `bool clearDeviceData(const String& deviceMac)`

Clear data for a specific device.

- Parameter:
  - `deviceMac` - Device MAC address (format: "XX:XX:XX:XX:XX:XX" or "XXXX-XXXX-XXXX")
- Behavior: remove device from the list and clear associated waits
- Returns: `true` if found and removed, otherwise `false`

#### `uint16_t clearExpiredDevices(unsigned long timeoutMs = 300000)`

Remove expired devices.

- Parameter:
  - `timeoutMs` - Timeout in milliseconds, default 300000ms (5 minutes)
- Behavior: remove devices that have not sent any data within the specified time
- Returns: number of removed devices

#### `bool setAutoCleanup(unsigned long intervalMs, unsigned long deviceTimeoutMs = 300000)`

Configure automatic cleanup.

- Parameters:
  - `intervalMs` - Check interval in milliseconds (0 disables auto cleanup)
  - `deviceTimeoutMs` - Device timeout in milliseconds, default 300000ms (5 minutes)
- Behavior: enable periodic auto cleanup of expired devices
- Returns: `true` on success, `false` if parameters invalid

#### `uint16_t checkAutoCleanup()`

Perform auto cleanup check.

- Behavior: check and perform auto cleanup; call regularly in loop()
- Returns: number of devices removed in this run, or 0 if not time yet

### Data parsing

#### `SwitchC6ParsedData parseDataStructured(const uint8_t *data, int len)`

Parse received data and return a structured result.

- Parameters:
  - `data` - Pointer to the data buffer
  - `len` - Data length
- Returns: parsed data structure

## Data Structures

### SwitchC6ParsedData_t

Parsed device data structure

```cpp
struct SwitchC6ParsedData_t {
    SwitchC6CommandType_t commandType; // Command type
    String macAddress;               // Device MAC address string
    bool switchState;                // Switch state (true=ON, false=OFF)
    float voltage;                   // Voltage value (e.g., 3.30V)
    String version;                  // Version string
    bool isValid;                    // Whether parsing succeeded
    String errorMessage;             // Error message if any
};
```

### broadcast_packet_with_mac

Broadcast device info structure

```cpp
struct broadcast_packet_with_mac {
    uint8_t senderMac[6];            // Sender MAC address
    String lastData;                 // Last received payload
    uint8_t channel;                 // Channel data was received on
    int rssi;                        // Signal strength
    unsigned long lastReceiveTime;   // Last receive timestamp
    bool status;                     // Status flag (0:OFF 1:ON)
};
```

### SwitchC6CommandType_t

Command type enum

```cpp
enum SwitchC6CommandType_t {
    COMMAND_UNKNOWN = 0,         // Unknown command
    COMMAND_STATUS_RETURN = 1,   // Status response data
    COMMAND_BROADCAST = 2,       // Button broadcast data
    COMMAND_VERSION_RETURN = 3   // Version response data
};
```

### ResponseWaitInfo

Response waiting info structure

```cpp
struct ResponseWaitInfo {
    String deviceMac;                // Device MAC waiting for response
    unsigned long requestTime;       // Request sent time
    unsigned long timeoutMs;         // Timeout in milliseconds
    bool isWaiting;                  // Whether waiting
    bool hasResponse;                // Whether a response has been received
    SwitchC6ParsedData_t response;   // Response payload
};
```

## Advanced Usage

### Device cleanup

```cpp
M5SwitchC6 switchC6(20);

void setup() {
    WiFi.mode(WIFI_STA);
    WiFi.begin();

    switchC6.begin();
  
    // Enable auto cleanup: check every 30s, remove devices idle for 5 minutes
    switchC6.setAutoCleanup(30000, 300000);
}

void loop() {
    // Perform auto cleanup check (don’t miss this, or cleanup won’t run)
    uint16_t removed = switchC6.checkAutoCleanup();
    if (removed > 0) {
        Serial.printf("Auto cleanup removed %d device(s)\n", removed);
    }
  
    // Manually clear a specific device
    if (needToClearDevice) {
        if (switchC6.clearDeviceData("AA:BB:CC:DD:EE:FF")) {
            Serial.println("Specified device cleared");
        }
    }
  
    // Manually clear expired devices
    if (needToClearExpired) {
        uint16_t count = switchC6.clearExpiredDevices(60000); // 1-minute idle
        Serial.printf("Cleared %d expired device(s)\n", count);
    }
  
    delay(1000);
}
```

### Manage multiple devices

```cpp
M5SwitchC6 switchC6(20);  // Support 20 devices

void setup() {
    WiFi.mode(WIFI_STA);
    WiFi.begin();

    switchC6.begin();
}

void loop() {
    // Get all detected devices
    uint16_t deviceCount = switchC6.getPeerCount();
    Serial.printf("Detected %d device(s)\n", deviceCount);
  
    for (uint16_t i = 0; i < deviceCount; i++) {
        broadcast_packet_with_mac peerInfo;
        if (switchC6.getPeerInfo(i, &peerInfo)) {
            Serial.printf("Device %d: MAC=%02X:%02X:%02X:%02X:%02X:%02X, "
                         "Status=%s, RSSI=%d, Channel=%d\n",
                         i,
                         peerInfo.senderMac[0], peerInfo.senderMac[1],
                         peerInfo.senderMac[2], peerInfo.senderMac[3],
                         peerInfo.senderMac[4], peerInfo.senderMac[5],
                         peerInfo.status ? "ON" : "OFF",
                         peerInfo.rssi, peerInfo.channel);
        }
    }
    delay(5000);
}
```

### Error handling and retry

```cpp
bool controlDeviceWithRetry(const String& deviceMac, bool switchOn, int maxRetries = 3) {
    for (int i = 0; i < maxRetries; i++) {
        SwitchC6ParsedData_t response;
        if (switchC6.sendSwitchCommandWithResponse(deviceMac, switchOn, 3000, &response)) {
            if (response.isValid && response.switchState == switchOn) {
                Serial.printf("Success on attempt %d\n", i + 1);
                return true;
            }
        }
        Serial.printf("Attempt %d failed, retrying...\n", i + 1);
        delay(1000);  // Retry after 1 second
    }
    Serial.println("Control failed, reached max retries");
    return false;
}

void loop() {
    // Use retry mechanism to control the device
    if (controlDeviceWithRetry("E4B3-2386-18C4", true)) {
        Serial.println("Device turned ON");
    }
    delay(10000);
}
```

### Asynchronous operations

```cpp
void setup() {
    WiFi.mode(WIFI_STA);
    WiFi.begin();

    switchC6.begin();
}

void loop() {
    // Send command without waiting for response
    switchC6.sendSwitchCommand("E4B3-2386-18C4", true, true);
  
    // Continue with other tasks
    doOtherTasks();
  
    // Check if there are pending responses
    if (switchC6.isWaitingForResponse()) {
        Serial.println("Waiting for device response...");
    }
  
    delay(100);
}

void doOtherTasks() {
    // Perform other tasks that don't depend on Switch-C6 responses
    Serial.println("Doing other tasks...");
    delay(200);
}
```

### Data management example

```cpp
void setup() {
    WiFi.mode(WIFI_STA);
    WiFi.begin();

    switchC6.begin();
  
    // Auto cleanup: check every 2 minutes; remove devices idle for 10 minutes
    switchC6.setAutoCleanup(120000, 600000);
}

void loop() {
    // Auto cleanup is performed inside checkAutoCleanup()
    switchC6.checkAutoCleanup();
  
    // Show current device info
    uint16_t deviceCount = switchC6.getPeerCount();
    Serial.printf("Currently detected %d device(s)\n", deviceCount);
  
    // Monitor device status and clean long-idle devices
    for (uint16_t i = 0; i < deviceCount; i++) {
        broadcast_packet_with_mac peerInfo;
        if (switchC6.getPeerInfo(i, &peerInfo)) {
            unsigned long timeSinceLastReceive = millis() - peerInfo.lastReceiveTime;
  
            // If device is idle for more than 15 minutes, clear it
            if (timeSinceLastReceive > 900000) { // 15 minutes
                String deviceMac = "";
                for (int j = 0; j < 6; j++) {
                    if (j > 0) deviceMac += ":";
                    deviceMac += String(peerInfo.senderMac[j], HEX);
                }
  
                Serial.printf("Clearing long-idle device: %s\n", deviceMac.c_str());
                switchC6.clearDeviceData(deviceMac);
                break; // Clear one at a time to avoid affecting the loop
            }
        }
    }
  
    delay(5000);
}
```

### Smart device management

```cpp
void smartDeviceManagement() {
    // 1) Check device count to avoid excessive memory usage
    uint16_t deviceCount = switchC6.getPeerCount();
    if (deviceCount > 15) {
        Serial.println("Too many devices; clearing oldest/idle ones");
        switchC6.clearExpiredDevices(180000); // 3 minutes idle
    }
  
    // 2) Periodic full cleanup
    static unsigned long lastFullCleanup = 0;
    if (millis() - lastFullCleanup > 1800000) { // Every 30 minutes
        Serial.println("Performing periodic full cleanup");
        switchC6.clearAllData();
        lastFullCleanup = millis();
    }
  
    // 3) Cleanup weak-signal devices
    for (uint16_t i = 0; i < switchC6.getPeerCount(); i++) {
        broadcast_packet_with_mac peerInfo;
        if (switchC6.getPeerInfo(i, &peerInfo)) {
            if (peerInfo.rssi < -90) { // Very weak signal
                unsigned long timeSinceLastReceive = millis() - peerInfo.lastReceiveTime;
                if (timeSinceLastReceive > 300000) { // Also idle for 5 minutes
                    String deviceMac = "";
                    for (int j = 0; j < 6; j++) {
                        if (j > 0) deviceMac += ":";
                        deviceMac += String(peerInfo.senderMac[j], HEX);
                    }
                    Serial.printf("Clearing weak-signal device: %s (RSSI: %d)\n", deviceMac.c_str(), peerInfo.rssi);
                    switchC6.clearDeviceData(deviceMac);
                    break;
                }
            }
        }
    }
}
```

## Protocol Support

### ESP-NOW protocol

Fully supports the Switch-C6 ESP-NOW control protocol:

#### Control command formats

- Turn ON: `"1"` or `"1;"` (with response)
- Turn OFF: `"0"` or `"0;"` (with response)
- Status query: `"?"` or `"?;"` (with response)
- Version query: `"V"` or `"V;"` (with response)

#### Response formats

- Status response: `";status;voltage"` (e.g., ";1;3.30")
- Version response: `";version"` (e.g., ";1.0.0")
- Broadcast data: button broadcast information from the device

#### MAC address formats

- Standard: `"XXXX-XXXX-XXXX"` (e.g., "E4B3-2386-18C4")
- Broadcast: `"FFFF-FFFF-FFFF"` (receive broadcasts from all devices)

### Communication characteristics

- Channel range: 1-14 (6 or 11 recommended)
- Max devices: configurable; recommended ≤ 20
- Range: approx. 30-100 m indoors, 100-300 m outdoors
- Max payload: 250 bytes
- Retries: configurable resend interval and attempts

## Troubleshooting

### Common issues

#### 1) Initialization failed

```cpp
if (!switchC6.begin()) {
    Serial.println("Initialization failed, please check:");
    Serial.println("1. WiFi is set to STA mode");
    Serial.println("2. ESP-NOW is supported");
    Serial.println("3. Sufficient memory is available");
}
```

#### 2) Response timeout

```cpp
SwitchC6ParsedData_t response;
if (!switchC6.sendStatusQueryWithResponse(deviceMac, 5000, &response)) {
    Serial.println("Response timeout, possible reasons:");
    Serial.println("1. Device is offline or signal is weak");
    Serial.println("2. Wrong MAC address");
    Serial.println("3. Channel mismatch");
    Serial.println("4. Device is busy");
}
```

#### 3) MAC address format error

```cpp
String deviceMac = "E4B3-2386-18C4";  // Correct format
// String deviceMac = "E4:B3:23:86:18:C4";  // Incorrect format
```

#### 4) Channel configuration

```cpp
Serial.printf("Current channel: %d\n", switchC6.getChannel());
```

> Note: SwitchC6 uses broadcast packets and listens on all channels; channel setting is generally not critical. Change it only in high-interference environments.
