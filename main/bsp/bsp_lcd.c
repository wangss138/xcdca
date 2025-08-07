#include "bsp_lcd.h"

esp_lcd_panel_io_handle_t io_handle = NULL;
esp_lcd_panel_handle_t panel_handle = NULL;
#define LCD_HOST SPI2_HOST

void bsp_lcd_init()
{

    MY_LOGI("Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_BK_LIGHT}; // 选择背光引脚，掩码模式
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    // 设置背光高电平
    // ESP_ERROR_CHECK(gpio_set_level(PIN_NUM_BK_LIGHT, 1));

    MY_LOGI("Initialize SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    MY_LOGI("Install panel IO");
    // 这个结构体用于配置LCD面板的I/O（输入/输出）接口，也就是SPI总线和LCD面板控制器之间的逻辑连接和协议
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_LCD_DC,
        .cs_gpio_num = PIN_NUM_LCD_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));
    // 配置lcd设备级参数
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    // 是否控制颜色反转
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, false));
    // 水平屏幕翻转
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, true));
    // 用于控制 LCD 屏幕的显示开关
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
}

void bsp_lcd_set_close()
{
    ESP_ERROR_CHECK(gpio_set_level(PIN_NUM_BK_LIGHT, 0));
}