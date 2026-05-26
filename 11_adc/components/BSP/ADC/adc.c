
#include "adc.h"

#define LOST_VAL 1

adc_oneshot_unit_handle_t adc_handle = NULL; // ADC 处理函数

void adc_init(void)
{
  adc_oneshot_unit_init_cfg_t adc_config = {
      .unit_id = ADC_UNIT_1,           // adc单元： ADC1/ADC2
      .ulp_mode = ADC_ULP_MODE_DISABLE // 主 cpu 下，不需要 BLE 模式
  };

  ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_config, &adc_handle));

  adc_oneshot_chan_cfg_t chan_config = {
      .atten = ADC_ATTEN_DB_0,     // ADC 衰减
      .bitwidth = ADC_BITWIDTH_12, // ADC 分辨率
  };

  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHAN, &chan_config));
}

// 滤波算法
uint32_t adc_get_result_average(adc_channel_t ch, uint32_t times)
{
  uint32_t sum = 0;

  int *raw_data = heap_caps_malloc(times * sizeof(int), MALLOC_CAP_INTERNAL);

  if (NULL == raw_data)
  {

    ESP_LOGE("adc", "Memory for adc is not enough!");
  }

  for (uint32_t t = 0; t < times; t++)
  {

    adc_oneshot_read(adc_handle, ch, &raw_data[t]); // 读取原始数据
    vTaskDelay(pdMS_TO_TICKS(5));
  }

  // 冒泡排序

  // for (uint16_t i = 0; i < times - 1; i++)
  // {
  //   for (uint16_t j = i + 1; j < times; j++)
  //   {
  //     if (raw_data[i] > raw_data[j])
  //     {
  //       temp_val = raw_data[i];
  //       raw_data[i] = raw_data[j];
  //       raw_data[j] = temp_val;
  //     }
  //   }
  // }

  // 优化算法版本：插入排序+稳定性
  for (uint16_t i = 1; i < times; i++)
  {
    uint16_t temp_val = raw_data[i];
    uint16_t j = i;
    while (j > 0 && raw_data[j - 1] > temp_val)
    {
      raw_data[j] = raw_data[j - 1];
      j--;
    }
    raw_data[j] = temp_val;
  }

  // 舍弃 min + max 值，再求平均
  for (uint32_t i = LOST_VAL; i < times - LOST_VAL; i++)
  {
    sum += raw_data[i];
  }

  return sum / (times - 2 * LOST_VAL);
}
