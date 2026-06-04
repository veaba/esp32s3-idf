/*
 * LVGL Demo on ESP-IDF (ESP32-S3 + XL9555 IO Expander Board)
 *
 * 硬件引脚参考 10_spi_lcd 工程:
 *   SPI2: MOSI=GPIO11, CLK=GPIO12, MISO=GPIO13
 *   LCD: DC=GPIO40, CS=GPIO21
 *   XL9555 (I2C): SDA=GPIO41, SCL=GPIO42
 *   LCD RST/PWR 通过 XL9555 IO 扩展器控制
 */

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iic.h"
#include "lvgl.h"
#include "xl9555.h"

#define TAG "LVGL_DEMO"

/* ==================== 硬件引脚配置（匹配实际开发板） ==================== */
#define LCD_HOST SPI2_HOST
#define LCD_DC_GPIO GPIO_NUM_40
#define LCD_CS_GPIO GPIO_NUM_21
#define LCD_CLK_GPIO GPIO_NUM_12
#define LCD_MOSI_GPIO GPIO_NUM_11

#define LCD_H_RES 240
#define LCD_V_RES 320
#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)

/* XL9555 IO 扩展器引脚（用于 LCD RST 和背光） */
#define LCD_RST_XL9555_PIN SLCD_RST_IO /* 0x0400 */
#define LCD_PWR_XL9555_PIN SLCD_PWR_IO /* 0x0800 */

/* ==================== LVGL 回调 ==================== */
static lv_display_t *g_disp = NULL;

static bool lcd_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata,
                                        void *user_ctx) {
  lvgl_port_flush_ready(g_disp);
  return false;
}

/* ==================== 通过 XL9555 控制 LCD 硬件复位 ==================== */
static void lcd_hard_reset(void) {
  xl9555_pin_write(LCD_RST_XL9555_PIN, 0);
  vTaskDelay(pdMS_TO_TICKS(100));
  xl9555_pin_write(LCD_RST_XL9555_PIN, 1);
  vTaskDelay(pdMS_TO_TICKS(100));
}

/* ==================== 显示初始化 ==================== */
static esp_err_t app_lcd_init(void) {
  ESP_LOGI(TAG, "Initialize SPI LCD (ST7789) with XL9555 IO expander");

  /* 1. 初始化 I2C 和 XL9555（必须先于 LCD 初始化） */
  i2c_obj_t i2c0_master = iic_init(I2C_NUM_0);
  xl9555_init(i2c0_master);

  /* 2. 通过 XL9555 开启 LCD 电源和背光 */
  xl9555_pin_write(LCD_PWR_XL9555_PIN, 1);
  ESP_LOGI(TAG, "LCD power on via XL9555");

  /* 3. 通过 XL9555 硬件复位 LCD */
  lcd_hard_reset();

  /* 4. 初始化 SPI 总线 */
  spi_bus_config_t buscfg = {
      .sclk_io_num = LCD_CLK_GPIO,
      .mosi_io_num = LCD_MOSI_GPIO,
      .miso_io_num = -1,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = LCD_H_RES * 80 * sizeof(lv_color_t),
  };
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

  /* 5. 创建 SPI Panel IO */
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_spi_config_t io_config = {
      .dc_gpio_num = LCD_DC_GPIO,
      .cs_gpio_num = LCD_CS_GPIO,
      .pclk_hz = LCD_PIXEL_CLOCK_HZ,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
      .spi_mode = 0,
      .trans_queue_depth = 10,
      .on_color_trans_done = lcd_notify_lvgl_flush_ready,
      .user_ctx = NULL,
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

  /* 6. 创建 LCD Panel (ST7789) — RST 由 XL9555 控制，此处不使用 GPIO RST */
  esp_lcd_panel_handle_t panel_handle = NULL;
  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = -1,
      .rgb_endian = LCD_RGB_ENDIAN_BGR,
      .bits_per_pixel = 16,
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  /* 7. 创建 LVGL 显示 */
  lvgl_port_display_cfg_t disp_cfg = {
      .io_handle = io_handle,
      .panel_handle = panel_handle,
      .buffer_size = LCD_H_RES * 40,
      .double_buffer = true,
      .hres = LCD_H_RES,
      .vres = LCD_V_RES,
      .rotation =
          {
              .swap_xy = false,
              .mirror_x = true,
              .mirror_y = true,
          },
#if LVGL_VERSION_MAJOR >= 9
      .color_format = LV_COLOR_FORMAT_RGB565,
#endif
      .flags =
          {
              .buff_dma = true,
#if LVGL_VERSION_MAJOR >= 9
              .swap_bytes = true,
#endif
          },
  };
  g_disp = lvgl_port_add_disp(&disp_cfg);

  return ESP_OK;
}

/* ==================== LVGL UI ==================== */
static void lvgl_create_ui(void) {
  lv_obj_t *scr = lv_screen_active();

  lv_obj_t *label = lv_label_create(scr);
  lv_label_set_text(label, "Hello LVGL!");
  lv_obj_set_style_text_color(label, lv_color_black(), 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, -40);

  lv_obj_t *btn = lv_button_create(scr);
  lv_obj_align(btn, LV_ALIGN_CENTER, 0, 40);
  lv_obj_set_size(btn, 120, 50);

  lv_obj_t *btn_label = lv_label_create(btn);
  lv_label_set_text(btn_label, "Button2");
  lv_obj_center(btn_label);

  static lv_style_t style_indic;
  lv_style_init(&style_indic);
  lv_style_set_bg_color(&style_indic, lv_palette_main(LV_PALETTE_BLUE));

  lv_obj_t *bar = lv_bar_create(scr);
  lv_obj_set_size(bar, 180, 15);
  lv_obj_align(bar, LV_ALIGN_CENTER, 0, 100);
  lv_bar_set_range(bar, 0, 100);
  lv_bar_set_value(bar, 70, LV_ANIM_ON);
}

/* ==================== 主入口 ==================== */
void app_main(void) {
  ESP_LOGI(TAG, "Starting LVGL demo");

  /* 初始化 LVGL */
  lvgl_port_cfg_t lvgl_cfg = {
      .task_priority = 4,
      .task_stack = 7168,
      .task_affinity = -1,
      .task_max_sleep_ms = 500,
      .task_stack_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_DEFAULT,
      .timer_period_ms = 5,
  };
  ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

  /* 初始化显示（包含 I2C + XL9555 + SPI LCD） */
  ESP_ERROR_CHECK(app_lcd_init());

  /* 创建 UI（在 LVGL 线程安全上下文中） */
  if (lvgl_port_lock(0)) {
    lvgl_create_ui();
    lvgl_port_unlock();
  }

  ESP_LOGI(TAG, "LVGL demo running");

  /* 主循环：每秒更新进度条 */
  int val = 0;
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    if (lvgl_port_lock(0)) {
      lv_obj_t *scr = lv_screen_active();
      lv_obj_t *bar = lv_obj_get_child(scr, 2);
      if (bar) {
        val = (val + 10) % 110;
        lv_bar_set_value(bar, val, LV_ANIM_ON);
      }
      lvgl_port_unlock();
    }
  }
}