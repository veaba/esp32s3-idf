#include "rmt_nec_rx.h"

uint16_t s_nec_code_address = 0x0000;
uint16_t s_nec_code_command = 0x0000;

QueueHandle_t receive_queue = NULL;
rmt_channel_handle_t rx_channel = NULL;
rmt_symbol_word_t raw_symbols[64]; // NEC
rmt_receive_config_t receive_config;

/**
 * RMT 数据接收完成回调函数
 */
bool rmt_nec_rx_done_callback(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data)
{
  BaseType_t high_task_wakeup = pdFALSE;
  QueueHandle_t receive_queue = (QueueHandle_t)user_data;
  xQueueSendFromISR(receive_queue, edata, &high_task_wakeup);
  return high_task_wakeup == pdTRUE;
}

esp_err_t rmt_nec_rx_init(void)
{
  ESP_ERROR_CHECK(gpio_reset_pin(RMT_IN_GPIO_PIN));

  rmt_rx_channel_config_t rx_channel_cfg = {
      .gpio_num = RMT_IN_GPIO_PIN,        // 设置红外线接收通道引脚
      .clk_src = RMT_CLK_SRC_DEFAULT,     // RMT 时钟源
      .resolution_hz = RMT_RESOLUTION_HZ, // 始时钟分辨率
      .mem_block_symbols = 64,            // 通道一次可以存储 RMT 符号梳理
  };

  ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_channel_cfg, &rx_channel));

  receive_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));

  assert(receive_queue);

  rmt_rx_event_callbacks_t cbs = {
      .on_recv_done = rmt_nec_rx_done_callback, // RMT 接收完成回调
  };

  ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(rx_channel, &cbs, receive_queue));

  // NEC 时序要求
  receive_config.signal_range_min_ns = 2000;
  receive_config.signal_range_max_ns = 12000000;

  // 开启RMT通道
  ESP_ERROR_CHECK(rmt_enable(rx_channel));
  ESP_ERROR_CHECK(rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols), &receive_config));

  return ESP_OK;
}

inline bool rmt_nec_check_range(uint32_t signal_duration, uint32_t spec_duration)
{
  return (signal_duration < (spec_duration + RMT_NEC_DECODE_MARGIN)) && (signal_duration > (spec_duration - RMT_NEC_DECODE_MARGIN));
}

/**
 * 判断逻辑 0
 */
bool rmt_nec_logic0(rmt_symbol_word_t *rmt_nec_symbols)
{

  return rmt_nec_check_range(rmt_nec_symbols->duration0, NEC_PAYLOAD_ZERO_DURATION_0) && rmt_nec_check_range(rmt_nec_symbols->duration1, NEC_PAYLOAD_ZERO_DURATION_1);
}

/**
 * 判断逻辑 1
 */
bool rmt_nec_logic1(rmt_symbol_word_t *rmt_nec_symbols)
{

  return rmt_nec_check_range(rmt_nec_symbols->duration0, NEC_PAYLOAD_ONE_DURATION_0) && rmt_nec_check_range(rmt_nec_symbols->duration1, NEC_PAYLOAD_ONE_DURATION_1);
}

bool rmt_nec_parse_frame(rmt_symbol_word_t *rmt_nec_symbols)
{

  rmt_symbol_word_t *cur = rmt_nec_symbols;

  uint16_t address = 0;
  uint16_t command = 0;

  bool valid_leading_code = rmt_nec_check_range(cur->duration0, NEC_LEADING_CODE_DURATION_0) && rmt_nec_check_range(cur->duration1, NEC_LEADING_CODE_DURATION_1);

  // 校验特征
  if (!valid_leading_code)
  {
    return false;
  }

  cur++;

  for (int i = 0; i < 16; i++)
  {
    if (rmt_nec_logic1(cur))
    {
      address |= 1 << i;
    }
    else if (rmt_nec_logic0(cur))
    {
      address &= ~(1 << i);
    }
    else
    {
      return false;
    }
    cur++;
  }

  for (int i = 0; i < 16; i++)
  {
    if (rmt_nec_logic1(cur))
    {
      command |= 1 << i;
    }
    else if (rmt_nec_logic0(cur))
    {
      command &= ~(1 << i);
    }
    else
    {
      return false;
    }
    cur++;
  }

  // 保存 地址+命令，用于判断重复

  s_nec_code_address = address;
  s_nec_code_command = command;

  return true;
}

// 检查重复按键
bool rmt_nec_parse_frame_repeat(rmt_symbol_word_t *rmt_nec_symbols)
{
  return rmt_nec_check_range(rmt_nec_symbols->duration0, NEC_REPEAT_CODE_DURATION_0) && rmt_nec_check_range(rmt_nec_symbols->duration1, NEC_REPEAT_CODE_DURATION_1);
}