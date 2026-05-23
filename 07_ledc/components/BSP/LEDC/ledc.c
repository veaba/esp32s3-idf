#include "ledc.h"

// uint32_t ledc_duty_pow(uint32_t duty, uint8_t m, uint8_t n)
// {
//   uint32_t result = 1;

//   while (n--)
//   {
//     result *= m;
//   }
//   return (result * duty) / 100;
// }
/**
 * 当 duty=100 时，计算出 16384，超过了 LEDC 的最大值 16383，溢出导致行为异常。
 * 同时 14 位分辨率下每个 step 太大（16384/100 ≈ 164），导致亮度变化不平滑
 * @param duty 0-100 百分比
 * @param duty_resolution PWM 分辨率，单位 bit
 * 示例：duty = 50（50%），duty_resolution = 8（最大 255），计算结果为 127，符合预期。duty = 100（100%），duty_resolution = 8，计算结果为 255，符合预期。
 * 计算整数，作为寄存器所需
 */
uint32_t ledc_duty_pow(uint32_t duty, uint8_t duty_resolution)
{
  printf("duty: %lu, pwm: %lu\n", duty, duty_resolution);
  return (duty * ((1 << duty_resolution) - 1)) / 100;
}

void ledc_init(ledc_config_t *ledc_config)
{
  ledc_config->duty = ledc_duty_pow(ledc_config->duty, ledc_config->duty_resolution);

  // timer config
  ledc_timer_config_t ledc_timer = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = ledc_config->duty_resolution,
      .clk_cfg = ledc_config->clk_cfg,
      .freq_hz = ledc_config->freq_hz,
      .timer_num = ledc_config->timer_num,
  };

  ledc_timer_config(&ledc_timer);

  // channel config
  ledc_channel_config_t ledc_channel = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .intr_type = LEDC_INTR_DISABLE,
      .channel = LEDC_CHANNEL_0,
      .duty = ledc_config->duty,
      .gpio_num = ledc_config->gpio_num,
      .hpoint = 0,
      .timer_sel = ledc_config->timer_num,
  };

  ledc_channel_config(&ledc_channel);
}

void ledc_pwn_set_duty(ledc_config_t *ledc_config, uint32_t duty)
{
  ledc_config->duty = ledc_duty_pow(duty, ledc_config->duty_resolution);
  ledc_set_duty(LEDC_LOW_SPEED_MODE, ledc_config->channel, ledc_config->duty);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, ledc_config->channel);
}