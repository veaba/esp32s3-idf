#include "blue_tooth.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "led.h"
#include "nvs_flash.h"
#include "probe.h"

/**
 * @brief       程序入口
 * @param       无
 * @retval      无
 */
void app_main(void) {
  esp_err_t ret;

  led_init(); /* LED初始化 */

  ret = nvs_flash_init(); /* 初始化NVS */
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  /* ── Hello World! ── */
  printf("\n");
  printf("  ╭─────────────────────────────────────────────────╮\n");
  printf("  │                                                 │\n");
  printf("  │       HELLO WORLD from ESP32-S3!                │\n");
  printf("  │                                                 │\n");
  printf("  │   WiFi-DensePose Capability Discovery v1.0      │\n");
  printf("  │                                                 │\n");
  printf("  ╰─────────────────────────────────────────────────╯\n");
  printf("\n");

  /* Run all probes */
  probe_chip_info();
  probe_memory();
  probe_flash();
  probe_temperature();
  probe_peripherals();
  probe_security();
  probe_power();
  probe_freertos();
  probe_wifi_capabilities();
  probe_bluetooth();
  probe_csi_details();

  print_separator("DONE — ALL CAPABILITIES REPORTED");
  printf("\n  This ESP32-S3 is ready for WiFi-DensePose!\n");
  printf("  Flash the full firmware (esp32-csi-node) to begin CSI sensing.\n\n");

  /* Keep alive — blink a status message every 10 seconds */
  int tick = 0;
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(10000));
    tick++;
    printf("[hello] Still running... uptime=%lld sec, free_heap=%" PRIu32 "\n", esp_timer_get_time() / 1000000LL, (uint32_t)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  }
}
