#ifndef __BLUE_TOOTH_H
#define __BLUE_TOOTH_H

#include "esp_bt.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include <stdint.h>

#define GATTS_TABLE_TAG "【veaba_esp3-s3_ble】"

#define PROFILE_NUM 1
#define PROFILE_APP_IDX 0
#define ESP_APP_ID 0x55
#define SAMPLE_DEVICE_NAME "veaba_esp3-s3_ble" // 广播名称
#define SVC_INST_ID 0
#define GATTS_BLE_CHAR_VAL_LEN_MAX 500 // 长度
#define PREPARE_BUF_MAX_SIZE 1024
#define CHAR_DECLARATION_SIZE (sizeof(uint8_t))

#define ADV_CONFIG_FLAG (1 << 0)
#define SCAN_RSP_CONFIG_FLAG (1 << 1)

/***** Service *****/
static const uint16_t GATTS_SERVICE_UUID_TEST = 0x00ff;
static const uint16_t GATTS_CHAR_UUID_TEST_LED = 0xff01;
static const uint16_t GATTS_CHAR_UUID_TEST_TEMP = 0xff02;

static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

// static const uint8_t char_prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_read_write_notify = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t heart_measurement_ccc[2] = {0x00, 0x00};
static const uint8_t led_value[1] = {0x00};
static const uint8_t temp_value[1] = {0x00};

enum {
  IDX_SVC,
  IDX_CHAR_LED,
  IDX_CHAR_VAL_LED,

  IDX_CHAR_TEMP,
  IDX_CHAR_VAL_TEMP,
  IDX_CHAR_CFG_TEMP, // MOCK 温度控制
  HRS_IDX_NB,
};

typedef struct {
  uint8_t *prepare_buf;
  int prepare_len;
} prepare_type_env_t;

void blue_tooth_init();

#endif