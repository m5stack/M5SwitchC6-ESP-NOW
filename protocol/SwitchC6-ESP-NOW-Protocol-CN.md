# SwitchC6 ESP-NOW 控制协议

## SwitchC6开启(不返回)

* 命令

    主机发送
    ```C
    "XXXX-XXXX-XXXX=1;ch=n"
    ```
* 例子

    主机发送
    ```C
    "AABB-CCDD-EEFF=1;ch=6" 或 "aabb-ccdd-eeff=1;ch=6" 或 "aaBB-CCdd-eeFF=1;ch=6"
    ```

## SwitchC6开启(带返回)

* 命令

    主机发送
    ```C
    "XXXX-XXXX-XXXX=1;ch=n;"
    ```

    SwitchC6返回
    ```C
    "YYYY-YYYY-YYYY;1;z.zzV"
    ```
* 例子

    主机发送
    ```C
    "AABB-CCDD-EEFF=1;ch=6;" 或 "aabb-ccdd-eeff=1;ch=6;" 或 "aaBB-CCdd-eeFF=1;ch=6;"
    ```

    SwitchC6返回
    ```C
    "1122-AABB-CCGG;1;3.30V" 或 "1122-aabb-ccgg;1;3.30V" 或 "1122-aaBB-ccgg;1;3.30V"
    ```

## SwitchC6关闭(不返回)

* 命令

    主机发送
    ```C
    "YYYY-YYYY-YYYY=0;ch=n"
    ```
* 例子

    主机发送
    ```C
    "AABB-CCDD-EEFF=0;ch=6" 或 "aabb-ccdd-eeff=0;ch=6" 或 "aaBB-CCdd-eeFF=0;ch=6"
    ```

## SwitchC6关闭(带返回)

* 命令

    主机发送
    ```C
    "XXXX-XXXX-XXXX=0;ch=n;"
    ```

    SwitchC6返回
    ```C
    "YYYY-YYYY-YYYY;0;z.zzV"
    ```
* 例子

    主机发送
    ```C
    "AABB-CCDD-EEFF=0;ch=6;" 或 "aabb-ccdd-eeff=0;ch=6;" 或 "aaBB-CCdd-eeFF=0;ch=6;"
    ```

    SwitchC6返回
    ```C
    "1122-AABB-CCGG;0;3.30V" 或 "1122-aabb-ccgg;0;3.30V" 或 "1122-aaBB-ccgg;0;3.30V"
    ```

## 读取设备状态

* 命令

    主机发送
    ```C
    "XXXX-XXXX-XXXX=?;ch=n;"
    ```

    SwitchC6返回
    ```C
    "YYYY-YYYY-YYYY;0;z.zzV" 或 "YYYY-YYYY-YYYY;1;z.zzV"
    ```
* 例子

    主机发送
    ```C
    "AABB-CCDD-EEFF=?;ch=6;" 或 "aabb-ccdd-eeff=?;ch=6;" 或 "aaBB-CCdd-eeFF=?;ch=6;"
    ```

    SwitchC6返回
    ```C
    "1122-AABB-CCGG;0;3.30V" 或 "1122-aabb-ccgg;0;3.30V" 或 "1122-aaBB-ccgg;0;3.30V" 或 
    "1122-AABB-CCGG;1;3.30V" 或 "1122-aabb-ccgg;1;3.30V" 或 "1122-aaBB-ccgg;1;3.30V"
    ```

## 版本查询

* 命令

  主机发送

  ```C
  "XXXX-XXXX-XXXX=V;ch=n;"
  ```

  SwitchC6返回

  ```C
  "YYYY-YYYY-YYYY;ver"
  ```

* 例子

  主机发送

  ```C
  "AABB-CCDD-EEFF=V;ch=6;" 或 "aabb-ccdd-eeff=V;ch=6;" 或 "aaBB-CCdd-eeFF=V;ch=6;"
  ```

  SwitchC6返回

  ```C
  "1122-AABB-CCGG;1.0.0" 或 "1122-aabb-ccgg;1.0.0" 或 "1122-aaBB-ccgg;1.0.0"
  ```

## Button广播

* 单击一次

    SwitchC6广播并且开关状态切换一次
    ```C
    "FFFF-FFFF-FFFF;0;z.zzV" 或 "FFFF-FFFF-FFFF;1;z.zzV"
    ```

* 长按5s

    SwitchC6广播
    ```C
    "FFFF-FFFF-FFFF;0;z.zzV" 或 "FFFF-FFFF-FFFF;1;z.zzV"
    ```

## 说明：

* 数据发送和接收均采用字符串格式。
* "XXXX-XXXX-XXXX" 表示单火开关 SwitchC6 的 MAC 地址，使用字符形式，不区分大小写。
* "YYYY-YYYY-YYYY" 表示主机的 MAC 地址，使用字符形式，不区分大小写。
* 设备的通信采用广播方式，主机需要将广播地址（FF:FF:FF:FF:FF:FF）添加为对等节点。SwitchC6 也通过广播发送返回数据。主机可以通过 "YYYY-YYYY-YYYY" 判断接收到的数据包是否属于本设备，而 SwitchC6 则通过 "XXXX-XXXX-XXXX" 判断是否为需要自己处理的数据包。
* "ch=n" 中的 n 表示主机的 Wi-Fi 通道，取值范围为 1~14，主机发送的命令中必须填入正确的通道。
* 主机发送的命令中，MAC 地址后紧跟的第一个字符为 "="。
* SwitchC6 返回的命令中，MAC 地址后紧跟的第一个字符为 ";"。
* "z.zzV" 表示电容电压。
* 返回数据中的继电器状态与电容电压使用 ";" 分隔。
* SwitchC6 是否需要回复由主机发送数据包的最后一位是否为 ";" 来判断：如果末尾是 ";"，则需要返回数据。
* 绿色 LED 灯状态与继电器状态绑定：继电器闭合时，绿色 LED 灯点亮；继电器断开时，绿色 LED 灯熄灭。
* 按下按钮时，设备会广播自身状态。SwitchC6 会在 1~14 通道各发送一次数据包，且数据包中的地址为 FFFF-FFFF-FFFF（广播地址）。

## ESP-IDF使用示例

* ESP-NOW 发送

    ```C
    // 广播 MAC 地址（表示将数据发送给所有接收范围内的 ESP-NOW 设备）
    const uint8_t broadcast_mac_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    // 目标设备的 MAC 地址（此处为 SwitchC6 的 MAC 地址）
    const uint8_t switchc6_mac[6] = {0xE4, 0xB3, 0x23, 0x86, 0x18, 0xC4};
    
    /**
    * @brief 需要发送的控制指令列表（字符串格式）
    * 
    * 格式：MAC=操作;ch=信道;
    * - 操作 1 表示开，0 表示关，? 表示读取状态
    * - ch=6 表示当前 Wi-Fi 工作信道为 6
    * - 最后的分号（;）代表是否需要设备响应（有则需要响应）
    */
    const char *send_cmd[] = {
        "E4B3-2386-18C4=1;ch=6",    // 开启继电器，不需要设备响应
        "E4B3-2386-18C4=1;ch=6;",   // 开启继电器，需要设备响应
        "E4B3-2386-18C4=0;ch=6",    // 关闭继电器，不需要设备响应
        "E4B3-2386-18C4=0;ch=6;",   // 关闭继电器，需要设备响应
        "E4B3-2386-18C4=?;ch=6;",   // 查询继电器状态，需要设备响应
    };
    
    // 使用 ESP-NOW 发送其中一条控制指令
    // 此处发送第 0 条指令，即：开启继电器，且不等待响应
    esp_now_send(broadcast_mac_addr, (const uint8_t *)send_cmd[0], strlen(send_cmd[0]));    
    ```
