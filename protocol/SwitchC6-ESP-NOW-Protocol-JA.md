# SwitchC6 ESP-NOW 制御プロトコル

## SwitchC6 オン（応答なし）

* コマンド

    ホストが送信
    ```C
    "XXXX-XXXX-XXXX=1;ch=n"
    ```
* 例

    ホストが送信
    ```C
    "AABB-CCDD-EEFF=1;ch=6" または "aabb-ccdd-eeff=1;ch=6" または "aaBB-CCdd-eeFF=1;ch=6"
    ```

## SwitchC6 オン（応答あり）

* コマンド

    ホストが送信
    ```C
    "XXXX-XXXX-XXXX=1;ch=n;"
    ```

    SwitchC6 が応答
    ```C
    "YYYY-YYYY-YYYY;1;z.zzV"
    ```
* 例

    ホストが送信
    ```C
    "AABB-CCDD-EEFF=1;ch=6;" または "aabb-ccdd-eeff=1;ch=6;" または "aaBB-CCdd-eeFF=1;ch=6;"
    ```

    SwitchC6 が応答
    ```C
    "1122-AABB-CCGG;1;3.30V" または "1122-aabb-ccgg;1;3.30V" または "1122-aaBB-ccgg;1;3.30V"
    ```

## SwitchC6 オフ（応答なし）

* コマンド

    ホストが送信
    ```C
    "YYYY-YYYY-YYYY=0;ch=n"
    ```
* 例

    ホストが送信
    ```C
    "AABB-CCDD-EEFF=0;ch=6" または "aabb-ccdd-eeff=0;ch=6" または "aaBB-CCdd-eeFF=0;ch=6"
    ```

## SwitchC6 オフ（応答あり）

* コマンド

    ホストが送信
    ```C
    "XXXX-XXXX-XXXX=0;ch=n;"
    ```

    SwitchC6 が応答
    ```C
    "YYYY-YYYY-YYYY;0;z.zzV"
    ```
* 例

    ホストが送信
    ```C
    "AABB-CCDD-EEFF=0;ch=6;" または "aabb-ccdd-eeff=0;ch=6;" または "aaBB-CCdd-eeFF=0;ch=6;"
    ```

    SwitchC6 が応答
    ```C
    "1122-AABB-CCGG;0;3.30V" または "1122-aabb-ccgg;0;3.30V" または "1122-aaBB-ccgg;0;3.30V"
    ```

## デバイス状態の読み取り

* コマンド

    ホストが送信
    ```C
    "XXXX-XXXX-XXXX=?;ch=n;"
    ```

    SwitchC6 が応答
    ```C
    "YYYY-YYYY-YYYY;0;z.zzV" または "YYYY-YYYY-YYYY;1;z.zzV"
    ```
* 例

    ホストが送信
    ```C
    "AABB-CCDD-EEFF=?;ch=6;" または "aabb-ccdd-eeff=?;ch=6;" または "aaBB-CCdd-eeFF=?;ch=6;"
    ```

    SwitchC6 が応答
    ```C
    "1122-AABB-CCGG;0;3.30V" または "1122-aabb-ccgg;0;3.30V" または "1122-aaBB-ccgg;0;3.30V" または 
    "1122-AABB-CCGG;1;3.30V" または "1122-aabb-ccgg;1;3.30V" または "1122-aaBB-ccgg;1;3.30V"
    ```

## バージョン照会

* コマンド

  ホストが送信

  ```C
  "XXXX-XXXX-XXXX=V;ch=n;"
  ```

  SwitchC6 が応答

  ```C
  "YYYY-YYYY-YYYY;ver"
  ```

* 例

  ホストが送信

  ```C
  "AABB-CCDD-EEFF=V;ch=6;" または "aabb-ccdd-eeff=V;ch=6;" または "aaBB-CCdd-eeFF=V;ch=6;"
  ```

  SwitchC6 が応答

  ```C
  "1122-AABB-CCGG;1.0.0" または "1122-aabb-ccgg;1.0.0" または "1122-aaBB-ccgg;1.0.0"
  ```

## ボタンのブロードキャスト

* 単押し

    SwitchC6 がブロードキャストし、スイッチ状態が 1 回トグルされます
    ```C
    "FFFF-FFFF-FFFF;0;z.zzV" または "FFFF-FFFF-FFFF;1;z.zzV"
    ```

* 5 秒長押し

    SwitchC6 がブロードキャスト
    ```C
    "FFFF-FFFF-FFFF;0;z.zzV" または "FFFF-FFFF-FFFF;1;z.zzV"
    ```

## 備考

* 送受信されるデータは文字列形式です。
* "XXXX-XXXX-XXXX" は SwitchC6 の MAC アドレスを表し、文字列形式で大文字小文字は区別しません。
* "YYYY-YYYY-YYYY" はホストの MAC アドレスを表し、文字列形式で大文字小文字は区別しません。
* 通信はブロードキャストで行います。ホストはブロードキャストアドレス（FF:FF:FF:FF:FF:FF）をピアに追加する必要があります。SwitchC6 もブロードキャストで応答を送信します。ホストは "YYYY-YYYY-YYYY" により受信したパケットが自分宛かどうかを判断し、SwitchC6 は "XXXX-XXXX-XXXX" により処理対象かどうかを判断します。
* "ch=n" の n はホストの Wi‑Fi チャネル（1〜14）です。ホストが送信するコマンドには正しいチャネルを記入する必要があります。
* ホストが送信するコマンドでは、MAC アドレス直後の最初の文字は "=" です。
* SwitchC6 が返すコマンドでは、MAC アドレス直後の最初の文字は ";" です。
* "z.zzV" はコンデンサ電圧を表します。
* 応答データでは、リレー状態とコンデンサ電圧は ";" で区切られます。
* SwitchC6 が返信するかどうかは、ホストが送信するパケット末尾の文字が ";" かどうかで判断します。末尾が ";" の場合は返信が必要です。
* 緑色 LED の状態はリレー状態に連動します。リレーが閉成（オン）のとき点灯し、開放（オフ）のとき消灯します。
* ボタンが押されると、デバイスは自身の状態をブロードキャストします。SwitchC6 はチャネル 1〜14 の各チャネルで 1 回ずつパケットを送信し、パケットの宛先アドレスは FFFF-FFFF-FFFF（ブロードキャストアドレス）です。

## ESP-IDF 使用例

* ESP-NOW 送信

    ```C
    // ブロードキャスト MAC アドレス（電波範囲内のすべての ESP-NOW デバイスに送信）
    const uint8_t broadcast_mac_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    // 対象デバイスの MAC アドレス（ここでは SwitchC6 の MAC）
    const uint8_t switchc6_mac[6] = {0xE4, 0xB3, 0x23, 0x86, 0x18, 0xC4};
    
    /**
    * @brief 送信する制御コマンド一覧（文字列形式）
    * 
    * 形式：MAC=操作;ch=チャネル;
    * - 操作：1 = オン、0 = オフ、? = 状態読み取り
    * - ch=6 は現在の Wi‑Fi 動作チャネルが 6 を意味します
    * - 末尾のセミコロン（;）はデバイス応答が必要かどうかを示します（存在すれば応答が必要）
    */
    const char *send_cmd[] = {
        "E4B3-2386-18C4=1;ch=6",    // リレーをオン（応答不要）
        "E4B3-2386-18C4=1;ch=6;",   // リレーをオン（応答必要）
        "E4B3-2386-18C4=0;ch=6",    // リレーをオフ（応答不要）
        "E4B3-2386-18C4=0;ch=6;",   // リレーをオフ（応答必要）
        "E4B3-2386-18C4=?;ch=6;",   // リレー状態を照会（応答必要）
    };
    
    // ESP-NOW を用いて上記の制御コマンドの 1 つを送信します。
    // ここでは 0 番目を送信：リレーをオン（応答待ちなし）。
    esp_now_send(broadcast_mac_addr, (const uint8_t *)send_cmd[0], strlen(send_cmd[0]));    
    ```
