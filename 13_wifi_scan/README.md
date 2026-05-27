# 13_wifi_scan

> 扫描宇宙家的 wifi

## wifi 数据模型

```shell
I (3996) scan: SSID             XiaoMi
I (3996) scan: RSSI             -79
I (3996) scan: Authmode         WIFI_AUTH_WPA_WPA2_PSK
I (4006) scan: Pairwise Cipher  WIFI_CIPHER_TYPE_TKIP_CCMP
I (4006) scan: Group Cipher     WIFI_CIPHER_TYPE_TKIP
I (4016) scan: Channel          10
```

## stdio.h

- `printf`

## esp_wifi.h

- `esp_netif_init`
- `esp_wifi_start`
- `esp_wifi_scan_start`
- `esp_wifi_scan_get_ap_records`
- `esp_wifi_scan_get_ap_num`

## esp_event.h

- `esp_event_loop_create_default`

## <string.h>

- `memset`

## esp_log.h

- `ESP_LOGI`
