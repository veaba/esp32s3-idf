# esp32s3-idf 学习项目

| project       | description           |
|---------------|-----------------------|
| `01_led`      | `宇宙为你闪烁`        |
| `02_key`      | `你先别闪，按一下再闪` |
| `03_exit`     | `我电拔了，你再闪`     |
| `04_uart`     | `三体舰队给我发私信！` |
| `05_esptimer` | `定时给我闪`          |
| `06_gptimer`  | `为你闪烁，定时拉闸` |
| `07_ledc`     | `宇宙为你呼吸闪烁`    |

## clion 配置

<!-- ![clion-tools-chain](docs/public/clion-tools-chain.png)

![clion-cmake](docs/public/clion-cmake.png) -->

## debug 技巧

### 10ms

```shell
 vTaskDelay(pdMS_TO_TICKS(10));
```
