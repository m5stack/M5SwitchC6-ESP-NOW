# SwitchC6 ESP-NOW Control Protocol

## SwitchC6 ON (no response)

* Command

    Host sends
    ```C
    "XXXX-XXXX-XXXX=1;ch=n"
    ```
* Example

    Host sends
    ```C
    "AABB-CCDD-EEFF=1;ch=6" or "aabb-ccdd-eeff=1;ch=6" or "aaBB-CCdd-eeFF=1;ch=6"
    ```

## SwitchC6 ON (with response)

* Command

    Host sends
    ```C
    "XXXX-XXXX-XXXX=1;ch=n;"
    ```

    SwitchC6 responds
    ```C
    "YYYY-YYYY-YYYY;1;z.zzV"
    ```
* Example

    Host sends
    ```C
    "AABB-CCDD-EEFF=1;ch=6;" or "aabb-ccdd-eeff=1;ch=6;" or "aaBB-CCdd-eeFF=1;ch=6;"
    ```

    SwitchC6 responds
    ```C
    "1122-AABB-CCGG;1;3.30V" or "1122-aabb-ccgg;1;3.30V" or "1122-aaBB-ccgg;1;3.30V"
    ```

## SwitchC6 OFF (no response)

* Command

    Host sends
    ```C
    "YYYY-YYYY-YYYY=0;ch=n"
    ```
* Example

    Host sends
    ```C
    "AABB-CCDD-EEFF=0;ch=6" or "aabb-ccdd-eeff=0;ch=6" or "aaBB-CCdd-eeFF=0;ch=6"
    ```

## SwitchC6 OFF (with response)

* Command

    Host sends
    ```C
    "XXXX-XXXX-XXXX=0;ch=n;"
    ```

    SwitchC6 responds
    ```C
    "YYYY-YYYY-YYYY;0;z.zzV"
    ```
* Example

    Host sends
    ```C
    "AABB-CCDD-EEFF=0;ch=6;" or "aabb-ccdd-eeff=0;ch=6;" or "aaBB-CCdd-eeFF=0;ch=6;"
    ```

    SwitchC6 responds
    ```C
    "1122-AABB-CCGG;0;3.30V" or "1122-aabb-ccgg;0;3.30V" or "1122-aaBB-ccgg;0;3.30V"
    ```

## Read device status

* Command

    Host sends
    ```C
    "XXXX-XXXX-XXXX=?;ch=n;"
    ```

    SwitchC6 responds
    ```C
    "YYYY-YYYY-YYYY;0;z.zzV" or "YYYY-YYYY-YYYY;1;z.zzV"
    ```
* Example

    Host sends
    ```C
    "AABB-CCDD-EEFF=?;ch=6;" or "aabb-ccdd-eeff=?;ch=6;" or "aaBB-CCdd-eeFF=?;ch=6;"
    ```

    SwitchC6 responds
    ```C
    "1122-AABB-CCGG;0;3.30V" or "1122-aabb-ccgg;0;3.30V" or "1122-aaBB-ccgg;0;3.30V" or 
    "1122-AABB-CCGG;1;3.30V" or "1122-aabb-ccgg;1;3.30V" or "1122-aaBB-ccgg;1;3.30V"
    ```

## Version query

* Command

  Host sends

  ```C
  "XXXX-XXXX-XXXX=V;ch=n;"
  ```

  SwitchC6 responds

  ```C
  "YYYY-YYYY-YYYY;ver"
  ```

* Example

  Host sends

  ```C
  "AABB-CCDD-EEFF=V;ch=6;" or "aabb-ccdd-eeff=V;ch=6;" or "aaBB-CCdd-eeFF=V;ch=6;"
  ```

  SwitchC6 responds

  ```C
  "1122-AABB-CCGG;1.0.0" or "1122-aabb-ccgg;1.0.0" or "1122-aaBB-ccgg;1.0.0"
  ```

## Button broadcast

* Single press

    SwitchC6 broadcasts and toggles the switch state once
    ```C
    "FFFF-FFFF-FFFF;0;z.zzV" or "FFFF-FFFF-FFFF;1;z.zzV"
    ```

* Long press for 5 s

    SwitchC6 broadcasts
    ```C
    "FFFF-FFFF-FFFF;0;z.zzV" or "FFFF-FFFF-FFFF;1;z.zzV"
    ```

## Notes

* Both transmitted and received data are in string format.
* "XXXX-XXXX-XXXX" denotes the MAC address of the single-live-wire switch (SwitchC6), in character form, case-insensitive.
* "YYYY-YYYY-YYYY" denotes the host MAC address, in character form, case-insensitive.
* Communication uses broadcast. The host must add the broadcast address (FF:FF:FF:FF:FF:FF) as a peer. SwitchC6 also sends return data via broadcast. The host can determine whether a received packet is intended for itself by checking "YYYY-YYYY-YYYY", while SwitchC6 determines whether it should process a command by checking "XXXX-XXXX-XXXX".
* In "ch=n", n is the host's Wi‑Fi channel, in the range 1–14. The host must include the correct channel in the command it sends.
* In commands sent by the host, the first character immediately following the MAC address is "=".
* In commands returned by SwitchC6, the first character immediately following the MAC address is ";".
* "z.zzV" represents the capacitor voltage.
* In the return data, the relay state and capacitor voltage are separated by ";".
* Whether SwitchC6 needs to reply is determined by whether the last character of the packet sent by the host is ";": if it ends with ";", a reply is required.
* The green LED state is tied to the relay state: when the relay is closed, the green LED is on; when the relay is open, the green LED is off.
* When the button is pressed, the device broadcasts its own state. SwitchC6 sends one packet on each of channels 1–14, and the destination address in the packet is FFFF-FFFF-FFFF (broadcast address).

## ESP-IDF usage example

* ESP-NOW sending

    ```C
    // Broadcast MAC address (send data to all ESP-NOW devices within range)
    const uint8_t broadcast_mac_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    // Target device MAC address (here, the MAC of SwitchC6)
    const uint8_t switchc6_mac[6] = {0xE4, 0xB3, 0x23, 0x86, 0x18, 0xC4};
    
    /**
    * @brief List of control commands to send (string format)
    * 
    * Format: MAC=operation;ch=channel;
    * - operation: 1 = ON, 0 = OFF, ? = read status
    * - ch=6 means the current Wi‑Fi operating channel is 6
    * - The trailing semicolon (;) indicates whether a device response is required (if present, a response is required)
    */
    const char *send_cmd[] = {
        "E4B3-2386-18C4=1;ch=6",    // Turn on the relay; no device response required
        "E4B3-2386-18C4=1;ch=6;",   // Turn on the relay; device response required
        "E4B3-2386-18C4=0;ch=6",    // Turn off the relay; no device response required
        "E4B3-2386-18C4=0;ch=6;",   // Turn off the relay; device response required
        "E4B3-2386-18C4=?;ch=6;",   // Query relay status; device response required
    };
    
    // Use ESP-NOW to send one of the control commands.
    // Here we send index 0: turn on the relay without waiting for a response.
    esp_now_send(broadcast_mac_addr, (const uint8_t *)send_cmd[0], strlen(send_cmd[0]));    
    ```
