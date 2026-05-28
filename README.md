# esp32s3-idf 学习项目

| project                                           | description                  |
|---------------------------------------------------|------------------------------|
| [`01_led`](01_led/)                               | `宇宙为你闪烁`               |
| [`02_key`](02_key/)                               | `你先别闪，按一下再闪`        |
| [`03_exit`](03_exit/)                             | `我电拔了，你再闪`            |
| [`04_uart`](04_uart/)                             | `三体舰队给我发私信！`        |
| [`05_esptimer`](05_esptimer/)                     | `定时给我闪`                 |
| [`06_gptimer`](06_gptimer/)                       | `为你闪烁，定时拉闸`          |
| [`07_ledc`](07_ledc/)                             | `宇宙为你呼吸闪烁`           |
| [`08_iic`](08_iic/)                               | `宇宙为你发声，宇宙为你闪烁`  |
| [`09_iic_eeprom`](09_iic_eeprom/)                 | `宇宙为你写下存档`           |
| [`10_spi_lcd`](10_spi_lcd/)                       | `宇宙为你炫彩显示`           |
| [`11_adc`](11_adc/)                               | `我不是ADC`                  |
| [`12_infrared`](12_infrared/)                     | `红外线：你看不到的光`        |
| [`13_wifi_scan`](13_wifi_scan/)                   | `WiFi 扫描`                  |
| [`14_chinese_all`](14_chinese_all/)               | `宇宙语：两万完全中文`        |
| [`15_chinese_3k`](15_chinese_3k/)                 | `宇宙通用语：三千中文`        |
| [`16_wifi_scan_chinese`](16_wifi_scan_chinese/)   | `WiFi扫描中文显示`           |
| [`17_wifi_station`](17_wifi_station/)             | `网管，你家 WIFI 密码是多少`  |
| [`18_wifi_ap`](18_wifi_ap/)                       | `宇宙家的 wifi，你要不要试试` |
| [`19_wifi_auto_esptouch`](19_wifi_auto_esptouch/) | `esptouch 你嗅啥？`           |
| [`20_wifi_auto_qrcode`](20_wifi_auto_qrcode/)     | `wifi 密码在二维码上`        |
| [`21_ble_led`](21_ble_led/)                       | `蓝色的灯，简称蓝灯`          |
| [`22_qrcode`](22_qrcode/)                         | `出动二维码（二向箔）武器`     |

## helper

| project                                                              | description                            |
|----------------------------------------------------------------------|----------------------------------------|
| [`code_diff`](code_diff/)                                            | `项目对比工具`                         |
| [`EsptouchForHarmony`](https://github.com/veaba/EsptouchForHarmony/) | `Harmony 版本的 EspTouch 嗅探网络 APP` |

## clion 配置

![clion-tools-chain](docs/public/clion-tools-chain.png)
![clion-cmake](docs/public/clion-cmake.png)

## debug 技巧

### 10ms

```shell
 vTaskDelay(pdMS_TO_TICKS(10));
```
