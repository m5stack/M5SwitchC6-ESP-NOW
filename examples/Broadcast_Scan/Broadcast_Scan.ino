// Switch-C6 ESP-NOW 广播扫描器 / Switch-C6 ESP-NOW Broadcast Scanner
// 使用 M5SwitchC6 库监听和解析广播数据 / Listen and parse broadcast frames using M5SwitchC6
// 串口输出可见: MAC地址 / RSSI / 信道 / 状态 / 电压 / Serial output: MAC / RSSI / Channel / State / Voltage

#include <Arduino.h>
#include <WiFi.h>
#include <M5Switchc6.h>

// ===== 可调参数 / Configurable parameters =====
static const uint8_t FIXED_CHANNEL = 6;  // 目标信道，需与对端一致；设为 0 则不改 / Target channel; must match the peer; set to 0 to keep unchanged
static const uint16_t MAX_DEVICES = 20;  // 最大设备数量 / Maximum number of devices
// ===================

// 创建 M5SwitchC6 实例 / Create M5SwitchC6 instance
M5SwitchC6 switchC6(MAX_DEVICES);

// 上次显示设备列表的时间 / Timestamp of last device list print
unsigned long lastDisplayTime = 0;
const unsigned long DISPLAY_INTERVAL = 5000; // 每5秒显示一次设备列表 / Print the device list every 5 seconds

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println("========================================");
  Serial.println("Switch-C6 Broadcast Scanner Starting");
  Serial.println("========================================");

  // 1) 启动 Wi-Fi：STA（Station，工作站）模式 / Start Wi-Fi in STA (station) mode
  WiFi.mode(WIFI_STA);
  WiFi.begin();                  // 仅初始化协议栈，不必真的连 AP / Initialize stack only; no real AP connection
  WiFi.disconnect(false, true);  // 不关闭无线，仅清理历史热点 / Keep radio on; clear saved hotspots

  // 2) 初始化 M5SwitchC6 库 / Initialize M5SwitchC6 library
  if (!switchC6.begin()) {
    Serial.println("ERROR: M5SwitchC6 initialization failed!");
    while (true) delay(1000);
  }

  // 3) 设置信道 / Set Wi-Fi channel
  if (FIXED_CHANNEL >= 1 && FIXED_CHANNEL <= 14) {
    if (switchC6.setChannel(FIXED_CHANNEL)) {
      Serial.printf("Channel set successfully: %u\n", FIXED_CHANNEL);
    } else {
      Serial.printf("Channel set failed: %u\n", FIXED_CHANNEL);
    }
  }

  uint8_t currentChannel = switchC6.getChannel();
  Serial.printf("Current channel: %u\n", currentChannel);
  
  // 4) 设置自动清理过期设备（每60秒检查一次，设备超5分钟无响应则清理） / Enable auto cleanup of expired devices (check every 60 s; remove if silent > 5 min)
  switchC6.setAutoCleanup(60000, 300000);
  
  Serial.println("ESP-NOW broadcast scanner ready");
  Serial.println("========================================");
}

void loop() {
  // 检查是否有自动清理过期设备 / Check whether expired devices were auto-cleaned
  uint16_t removedCount = switchC6.checkAutoCleanup();
  if (removedCount > 0) {
    Serial.printf("Auto cleaned %u expired devices\n", removedCount);
  }

  // 定期显示设备列表 / Periodically display the device list
  unsigned long currentTime = millis();
  if (currentTime - lastDisplayTime >= DISPLAY_INTERVAL) {
    lastDisplayTime = currentTime;
    
    uint16_t peerCount = switchC6.getPeerCount();
    
    if (peerCount > 0) {
      Serial.println("========================================");
      Serial.printf("Discovered devices count: %u\n", peerCount);
      Serial.println("----------------------------------------");
      
      for (uint16_t i = 0; i < peerCount; i++) {
        broadcast_packet_with_mac_t peerInfo;
        if (switchC6.getPeerInfo(i, &peerInfo)) {
          // 转换MAC地址为字符串格式 / Convert MAC address to string
          char macStr[18];
          snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                   peerInfo.senderMac[0], peerInfo.senderMac[1], peerInfo.senderMac[2],
                   peerInfo.senderMac[3], peerInfo.senderMac[4], peerInfo.senderMac[5]);
          
          // 计算设备静默时间 / Calculate device silent time
          unsigned long silentTime = (currentTime - peerInfo.lastReceiveTime) / 1000;
          
          Serial.printf("Device %d:\n", i + 1);
          Serial.printf("  MAC: %s\n", macStr);
          Serial.printf("  Channel: %u\n", peerInfo.channel);
          Serial.printf("  RSSI: %d dBm\n", peerInfo.rssi);
          Serial.printf("  Status: %s\n", peerInfo.status ? "ON" : "OFF");
          Serial.printf("  Last data: %s\n", peerInfo.lastData.c_str());
          Serial.printf("  Silent time: %lu seconds\n", silentTime);
          Serial.println();
        }
      }
      Serial.println("========================================");
    } else {
      Serial.println("No devices discovered yet...");
    }
  }

  // 主循环延迟 / Main loop delay
  delay(100);
}