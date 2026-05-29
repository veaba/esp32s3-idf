# ESP32 C 端：Esptouch V2 嗅探（待实现）

## 原理

Harmony App 的 **Manual 标签页** 通过 UDP 广播（端口 7001/7002）发送 Esptouch V2 编码数据。
数据编码在 **UDP 包长度** 中（非包内容），ESP32 需用 **混杂模式** 捕获 802.11 帧并解码长度。

## 切换逻辑

```
KEY1 → esp_wifi_set_mode(WIFI_MODE_NULL)     # 关闭 AP
     → esp_wifi_set_promiscuous(true)         # 开启混杂模式
     → 开始嗅探 (60s 超时)

嗅探成功 → 保存 NVS → 切 STA
超时/KEY0 → esp_wifi_set_promiscuous(false)
          → esp_wifi_set_mode(WIFI_MODE_AP)  # 重启 AP
```

## 数据包格式（与 Harmony TouchPacketUtils 一致）

| 包类型 | 格式 | 说明 |
|--------|------|------|
| Sync | 1048 字节 | 同步包 |
| Data | `(index << 7) \| (1 << 6) \| data` | 1 字节 |
| Sequence | `128 + seq` 字节 | 序号码 |
| Sequence Size | `1072 + size - 1` 字节 | 总数码 |

## 解码流程

```c
// 1. 注册混杂模式回调
esp_wifi_set_promiscuous_rx_cb(esptouch_rx_callback);

// 2. 回调中提取每个 802.11 帧的 UDP payload 长度
//    长度值即 Esptouch 编码的数据位

// 3. 累积长度序列，还原数据：
//    - 先收 Sync（长度 1048）
//    - 再收 Data 包（1 字节: index + data bit）
//    - 再收 Sequence 包（标记顺序）

// 4. 6 字节一组，7-bit → 8-bit 还原 SSID / password

// 5. CRC 校验（TouchCRC 算法，多项式 0x8c）

// 6. 校验通过 → 保存 NVS → 切 STA
```

参考 Harmony 端编码逻辑：[TouchPacketUtils.ets](entry/src/main/ets/esptouch/provision/TouchPacketUtils.ets)

## 参考

- [Harmony 端 Esptouch V2 实现](entry/src/main/ets/esptouch/provision/EspProvisionerImpl.ets)
- [Esptouch V2 Android 参考](../EsptouchForAndroid/esptouch-v2/)
- [TouchPacketUtils.ets](entry/src/main/ets/esptouch/provision/TouchPacketUtils.ets)
- [EspProvisioningParams.ets](entry/src/main/ets/esptouch/provision/EspProvisioningParams.ets)
