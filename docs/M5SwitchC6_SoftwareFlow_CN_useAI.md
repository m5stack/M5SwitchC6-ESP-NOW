# M5SwitchC6 软件流程图（基于 src/M5Switchc6.cpp）

更新时间：2025-08-13

本文基于 `src/M5Switchc6.cpp` 与 `src/M5Switchc6.h` 提炼核心流程，给出总览流程图与收发应答时序图，帮助理解 ESP-NOW 初始化、数据接收/解析、命令发送与应答等待、设备清理等关键路径。

提示：VS Code Markdown 预览已原生支持 Mermaid。打开本文件后，使用“在侧边打开预览”即可查看图表。

---

## 一、总览流程图

### 1.1 初始化流程 (begin)

```mermaid
flowchart TB
    A[begin] --> B{Platform ESP32 or ESP8266}
    B -->|No| A0[Not supported return false]
    B -->|Yes| C{mutexLock == NULL}
    C -->|Yes| C1[Create mutex]
    C1 --> D{dataQueue == nullptr}
    C -->|No| D
    D -->|Yes| D1[Create queue 10]
    D1 --> E{parseTaskHandle == nullptr}
    D -->|No| E
    E -->|Yes| E1[Create task parseDataTask]
    E1 --> F{peers == nullptr}
    E -->|No| F
    F -->|Yes| F1[Alloc init peers max_broadcast_peers]
    F1 --> G{WiFi.getMode}
    F -->|No| G
    G -->|WIFI_OFF| A1[WiFi is off return false]
    G -->|WIFI_AP| A2[WiFi is AP mode return false]
    G -->|WIFI_STA or WIFI_AP_STA| H[Continue WiFi OK]
    H --> I[acquireMutex]
    I --> J[esp_now_init]
    J -->|ESP_OK| K[Register recv cb lambda]
    J -->|fail| J1[releaseMutex return false]
    K --> L[Register send cb lambda]
    L --> M[releaseMutex return true]
```

### 1.2 接收路径 (回调 -> 队列 -> 解析)

```mermaid
flowchart TB
    R0[recv_cb with esp_now_info data len] --> R1{esp_now_info and src_addr and instance}
    R1 -->|Yes| R2[acquireMutex]
    R1 -->|No| R0A[Log unknown MAC]
    R2 --> R3[Extract MAC channel rssi from rx_ctrl]
    R3 --> R4[Find or add peer in peers array]
    R4 --> R5[Update lastData status lastReceiveTime]
    R5 --> R6[releaseMutex]
    R6 --> R7[enqueueData with senderMac]
    R0A --> R7B[enqueueData without senderMac]
    R7 --> Q[(Queue size 10)]
    R7B --> Q
    Q -->|xQueueReceive 100ms| T[parseDataTask infinite loop]
    T --> T1{queuedData received}
    T1 -->|Yes| P0[parseData with data len senderMac]
    T1 -->|No| T2[vTaskDelay 1ms continue]
    P0 --> P1[parseDataStructured]
    P1 -->|isValid true| P2[handleReceivedResponse with senderMac]
    P1 -->|isValid false| P3[log Parse error]
    T2 --> T
```

### 1.3 发送路径 (构建命令 -> 发送广播)

```mermaid
flowchart TB
    S1[sendSwitchCommand / sendStatusQuery / sendVersionQuery / sendCustomMessage] --> S2[Validate channel 1-14]
    S2 -->|invalid| S2A[return false]
    S2 -->|valid| S3[Validate MAC format]
    S3 -->|invalid| S3A[return false]
    S3 -->|valid| S4[Build command string with channel]
    S4 --> S5[acquireMutex]
    S5 --> S6[Check broadcast peer exists]
    S6 -->|not exist| S6A[Add broadcast peer FF:FF:FF:FF:FF:FF]
    S6 -->|exist| S7
    S6A --> S7[esp_now_send broadcast]
    S7 --> S8[releaseMutex]
    S8 -->|ESP_OK| S9[Log success]
    S8 -->|fail| S10[Log fail]
    S7 --> S11[send_cb status log]
```

### 1.4 应答等待机制 (可选)

```mermaid
flowchart TB
    W0[sendSwitchCommandWithResponse] --> W1[addResponseWait deviceMac timeoutMs]
    W1 -->|success| W2[Send with needResponse true]
    W1 -->|fail| W1A[return false]
    W2 -->|success| W3[millis startTime lastSendTime]
    W2 -->|fail| W2A[removeResponseWait return false]
    W3 --> W4{millis - startTime < timeoutMs}
    W4 -->|Yes| W5[checkResponseTimeouts]
    W5 --> W6[findResponseWait index]
    W6 -->|hasResponse true| W7[Copy response removeResponseWait return true]
    W6 -->|hasResponse false| W8{millis - lastSendTime >= resendInterval}
    W8 -->|Yes| W9[Resend command update lastSendTime]
    W8 -->|No| W10[delay 1ms]
    W9 --> W4
    W10 --> W4
    W4 -->|No timeout| W11[removeResponseWait return false]
  
    %% 从接收路径的响应匹配
    P2[handleReceivedResponse from RX path] -->|match normalized senderMAC| W6
```

### 1.5 自动清理机制 (可选)

```mermaid
flowchart TB
    GC0[setAutoCleanup intervalMs deviceTimeoutMs] --> GC1[Save autoCleanupInterval deviceTimeout lastCleanupTime]
    GC2[checkAutoCleanup] --> GC3{autoCleanupInterval == 0}
    GC3 -->|Yes disabled| GC5[return 0]
    GC3 -->|No| GC4{currentTime - lastCleanupTime >= autoCleanupInterval}
    GC4 -->|No| GC5
    GC4 -->|Yes| GC6[clearExpiredDevices deviceTimeout]
    GC6 --> GC7[Remove expired devices and response waits]
    GC7 --> GC8[Update lastCleanupTime return removedCount]
```

说明：

- **初始化流程**：严格按照源码顺序，包含所有条件检查（mutex/queue/task/peers的nullptr检查，WiFi模式的具体枚举值检查）
- **接收路径**：体现了 ESP-NOW 回调的完整处理逻辑，包括 esp_now_info 检查、MAC提取、peers更新、队列入队、解析任务循环
- **发送路径**：包含参数验证（信道范围、MAC格式）、命令字符串构建、广播peer确保存在等完整步骤
- **WithResponse机制**：详细展示了超时循环、重发间隔检查、响应匹配的完整逻辑，以及各种失败情况的处理
- **自动清理**：体现了间隔检查、过期设备移除的完整流程，包括禁用状态的处理

---

## 二、收发与应答时序图

```mermaid
sequenceDiagram
    participant App
    participant M5 as M5SwitchC6
    participant ESP as ESP_NOW
    participant Q as dataQueue
    participant T as parseTask

    App->>M5: sendSwitchCommandWithResponse(deviceMac, switchOn, timeoutMs)
    M5->>M5: addResponseWait(deviceMac, timeoutMs)
    alt addResponseWait success
        M5->>M5: sendSwitchCommand(deviceMac, switchOn, needResponse=true)
        alt sendSwitchCommand success
            M5->>M5: normalizeMacAddress + build command string
            M5->>M5: acquireMutex
            M5->>ESP: esp_now_send(FF:FF:FF:FF:FF:FF, command)
            ESP-->>M5: send_cb(mac, status)
            M5->>M5: releaseMutex
          
            Note over M5: Start timeout loop with resend logic
          
            loop while millis() - startTime < timeoutMs
                M5->>M5: checkResponseTimeouts()
                M5->>M5: findResponseWait(deviceMac)
                alt hasResponse == true
                    M5->>App: return response data (success)
                else resend interval reached
                    M5->>ESP: esp_now_send (resend command)
                else
                    M5->>M5: delay(1)
                end
            end
          
            M5->>App: timeout - return false
        else
            M5->>M5: removeResponseWait(deviceMac)
            M5->>App: send failed - return false
        end
    else
        M5->>App: addResponseWait failed - return false
    end

    Note over ESP,M5: Remote device receives and replies

    ESP-->>M5: recv_cb(esp_now_info, data, len)
    M5->>M5: acquireMutex
    M5->>M5: extract senderMAC from esp_now_info.src_addr
    M5->>M5: find/add peer in peers array
    M5->>M5: update peers MAC/channel/RSSI/lastData/status/time
    M5->>M5: releaseMutex
    M5->>Q: enqueueData(data, len, senderMAC)
  
    T->>Q: xQueueReceive(dataQueue, queuedData, 100ms)
    Q-->>T: data, len, senderMAC
    T->>M5: parseData(data, len, senderMAC)
    M5->>M5: parseDataStructured(data, len) -> parsed
    alt parsed.isValid == true
        M5->>M5: handleReceivedResponse(parsed, senderMacStr)
        M5->>M5: findResponseWait(senderMacStr)
        alt waiting device found
            M5->>M5: set hasResponse = true, copy parsed data
            Note over M5: This breaks the timeout loop above
        end
    else
        M5->>M5: log parse error
    end
```

---

## 参考与扩展

- 关键接口：

  - 初始化与通道：`begin()`, `setChannel()`, `getChannel()`
  - 发送：`sendSwitchCommand()`, `sendStatusQuery()`, `sendVersionQuery()`, `sendCustomMessage()`
  - 带应答：`sendSwitchCommandWithResponse()`, `sendStatusQueryWithResponse()`, `sendVersionQueryWithResponse()`
  - 解析：`parseData()`, `parseDataStructured()`
  - 等待/匹配：`addResponseWait()`, `findResponseWait()`, `removeResponseWait()`, `checkResponseTimeouts()`, `handleReceivedResponse()`
  - 清理：`clearPeersData()`, `clearResponseWaits()`, `clearAllData()`, `clearDeviceData()`, `clearExpiredDevices()`, `setAutoCleanup()`, `checkAutoCleanup()`
- 预览方式：

  1) 在 VS Code 中打开本文件
  2) 右上角“在侧边打开预览”（或快捷键 Ctrl+K V）
