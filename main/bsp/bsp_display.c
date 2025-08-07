#include "bsp_display.h"

/* LCD IO and panel */

extern esp_lcd_panel_io_handle_t io_handle;
extern esp_lcd_panel_handle_t panel_handle;
/* LVGL display  */
static lv_display_t *lvgl_disp = NULL;
static lv_obj_t *label;
static lv_obj_t *emotion_label;
static lv_obj_t *scr;

// 定义一个结构体来存储表情信息
typedef struct
{
    const char *description; // 表情的英文描述
    const char *emoji_str;   // 表情的 UTF-8 字符串
} Emoji;

// 创建一个包含所有表情对象的结构体数组
const Emoji emoji_array[] = {
    {"neutral", "😶"},
    {"happy", "🙂"},
    {"laughing", "😆"},
    {"funny", "😂"},
    {"sad", "😔"},
    {"angry", "😠"},
    {"crying", "😭"},
    {"loving", "😍"},
    {"embarrassed", "😳"},
    {"surprised", "😯"},
    {"shocked", "😱"},
    {"thinking", "🤔"},
    {"winking", "😉"},
    {"cool", "😎"},
    {"relaxed", "😌"},
    {"delicious", "🤤"},
    {"kissy", "😘"},
    {"confident", "😏"},
    {"sleepy", "😴"},
    {"silly", "😜"},
    {"confused", "🙄"}};

// 获取数组的大小
const size_t emoji_array_size = sizeof(emoji_array) / sizeof(emoji_array[0]);

LV_FONT_DECLARE(font_puhui_14_1);
// LV_FONT_DECLARE(font_emoji_64);
// LVGL image declare
void bsp_lvgl_init()
{
    /* Initialize LVGL */
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,       /* LVGL task priority */
        .task_stack = 4096,       /* LVGL task stack size */
        .task_affinity = -1,      /* LVGL task pinned to core (-1 is no affinity) */
        .task_max_sleep_ms = 500, /* Maximum sleep in LVGL task */
        .timer_period_ms = 5      /* LVGL timer tick period in ms */
    };
    lvgl_port_init(&lvgl_cfg);

    /* Add LCD screen */
    MY_LOGD("Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = 2400,
        .double_buffer = true,
        .hres = LCD_H_RES, // 水平分辨率即宽度
        .vres = LCD_V_RES, // 垂直分辨率即高度
        .monochrome = false,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = true,
        },
        .flags = {
            .buff_dma = true,
            .swap_bytes = true,
        }};
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);
}
// 默认展示内容
void bsp_display_init()
{
    scr = lv_scr_act();
    lvgl_port_lock(0);

    /* Your LVGL objects code here .... */

    /* Label */
    label = lv_label_create(scr);
    emotion_label = lv_label_create(scr);
    lv_obj_set_width(label, LCD_H_RES);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    // 引入字模
    lv_obj_set_style_text_font(label, &font_puhui_14_1, 0);
    lv_label_set_recolor(label, true);
    lv_label_set_text(label, "hello master,我是小龙");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 36);
    // 显示表情
    lv_obj_set_width(emotion_label, LCD_H_RES);
    lv_obj_set_style_text_align(emotion_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(emotion_label, font_emoji_64_init(), 0);
    lv_obj_set_style_text_color(emotion_label, lv_color_hex(0xFFC200),LV_STATE_DEFAULT);

    const char *emoji = "🙂";
    lv_label_set_text(emotion_label, emoji);
    // lv_obj_set_style_text_color(emotion_label, lv_color_hex(0xF9D500), LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_label_set_text(emotion_label, emotion);
    lv_obj_align(emotion_label, LV_ALIGN_CENTER, 0, -20);

    /* Task unlock */
    lvgl_port_unlock();
}
void bsp_display_set_text(char *text)
{
    lvgl_port_lock(0);

    if (label)
    {
        lv_label_set_text(label, text);
    }
    /* Task unlock */
    lvgl_port_unlock();
}
void bsp_display_set_emotion(char *emotion)
{
    lvgl_port_lock(0);

    if (emotion_label)
    {
        for (uint8_t i = 0; i < emoji_array_size; i++)
        {
            if (strcmp(emotion, emoji_array[i].description) == 0)
            {
                lv_label_set_text(emotion_label, emoji_array[i].emoji_str);
            }
        }
    }
    // lv_label_set_text(emotion_label, emotion);

    /* Task unlock */
    lvgl_port_unlock();
}
