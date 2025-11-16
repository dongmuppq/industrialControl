/*
 *****************************************************************************
 * @file lv_modbus_tool.c
 *
 * @brief  Modbus工具主界面实现文件
 *
 * @author  dongmu
 * @date    2025-11-10
 * @version V1.0
 *
 *****************************************************************************
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "gui.h"

/*********************
 *      DEFINES
 *********************/
#define DEFAULT_CHAEENL_STR     "0"
#define DEFAULT_CHAEENL_INT     0

/**********************
 *      TYPEDEFS
 **********************/
typedef enum
{
    DISP_SMALL,
    DISP_MEDIUM,
    DISP_LARGE,
} disp_size_t;

 /**********************
 *  STATIC PROTOTYPES
 **********************/
static void add_new_item_event_handler(lv_event_t *e);


/**********************
 *  STATIC VARIABLES
 **********************/
static disp_size_t disp_size;
static const lv_font_t *font_large;
static const lv_font_t *font_normal;

static lv_obj_t *g_kb;

// 背景框样式变量
static lv_style_t g_style_cont1;
static lv_style_t g_style_cont1_x;
static lv_style_t g_style_cont2;
static lv_style_t g_style_cont2_x;
static lv_style_t g_style_cont2_x_x;
static lv_style_t g_style_cont2_x_2_x;
static lv_style_t g_style_cont2_x_3_x;

// 自定义事件代码，给每个节点的 Send 按钮使用
static uint32_t MY_LV_EVENT_UPDATE_RPC;     // 更新节点信息事件
static uint32_t MY_LV_EVENT_READ_PERIOD;    // 周期发送事件

void gui_start(void) {
    lv_obj_t *cont1;
    lv_obj_t *cont2;

    font_large = LV_FONT_DEFAULT;
    font_normal = LV_FONT_DEFAULT;

    // 自定义两个事件代码
    MY_LV_EVENT_UPDATE_RPC = lv_event_register_id();
    MY_LV_EVENT_READ_PERIOD = lv_event_register_id();

    // 初始化样式
    init_style();

    // 创建全局键盘
    g_kb = lv_keyboard_create(lv_layer_sys());
    lv_obj_set_size(g_kb, LV_PCT(100), LV_PCT(40));
    lv_obj_add_flag(g_kb, LV_OBJ_FLAG_HIDDEN);

    // 创建顶栏容器
    cont1 = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(cont1);
    lv_obj_set_size(cont1, LV_PCT(100), LV_PCT(6));
    lv_obj_set_align(cont1, LV_ALIGN_TOP_MID);
    lv_obj_add_style(cont1, &g_style_cont1, 0);
    lv_obj_set_flex_flow(cont1, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont1, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 创建操作栏容器
    cont2 = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(cont2);
    lv_obj_set_size(cont2, LV_PCT(100), LV_PCT(94));
    lv_obj_set_align(cont2, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_scrollbar_mode(cont2, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_style(cont2, &g_style_cont2, 0);
    lv_obj_set_flex_flow(cont2, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont2, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);


    // 添加顶栏按钮
    lv_obj_t *up_btn = NULL;
    lv_obj_t *up_label = NULL;

    up_label = lv_label_create(cont1);
    lv_label_set_text(up_label, "Industral Control");
    lv_obj_set_style_text_color(up_label, lv_color_hex(0x165dff), 0); // 蓝色字体
    lv_obj_set_flex_grow(up_label, 1);
    lv_obj_set_style_text_align(up_label, LV_TEXT_ALIGN_LEFT, 0);


    up_btn = lv_button_create(cont1);
    up_label = lv_label_create(up_btn);
    lv_obj_add_style(up_btn, &g_style_cont1_x, 0);
    lv_label_set_text(up_label, "Add Node");
    lv_obj_add_event_cb(up_btn, add_new_item_event_handler, LV_EVENT_CLICKED, cont2);

    up_btn = lv_button_create(cont1);
    up_label = lv_label_create(up_btn);
    lv_obj_add_style(up_btn, &g_style_cont1_x, 0);
    lv_label_set_text(up_label, "Update");
    // lv_obj_add_event_cb(up_btn, update_translater_event_handler, LV_EVENT_CLICKED, NULL);

    up_btn  = lv_button_create(cont1);
    up_label = lv_label_create(up_btn);
    lv_obj_add_style(up_btn, &g_style_cont1_x, 0);
    lv_label_set_text(up_label, "MQTT Setting");
    // lv_obj_add_event_cb(up_btn, MQTT_setting_event_handler, LV_EVENT_CLICKED, NULL);





}

void init_style(void) {
    // 顶栏容器背景框样式
    lv_style_init(&g_style_cont1);

    lv_style_set_pad_all(&g_style_cont1, 12);
    lv_style_set_pad_column(&g_style_cont1, 15);
    // lv_style_set_pad_row(&g_style_cont2, 18);

    lv_style_set_bg_color(&g_style_cont1, lv_color_hex(0x282C34)); // 深色背景
    lv_style_set_bg_opa(&g_style_cont1, LV_OPA_COVER);

    lv_style_set_border_width(&g_style_cont1, 1);
    lv_style_set_border_color(&g_style_cont1, lv_color_hex(0x373c45)); // 深灰色边框
    lv_style_set_border_opa(&g_style_cont1, LV_OPA_COVER);
    lv_style_set_border_side(&g_style_cont1, LV_BORDER_SIDE_BOTTOM);

    lv_style_set_outline_width(&g_style_cont1, 0);
    // lv_style_set_outline_opa(&g_style_cont1_x, LV_OPA_TRANSP);

    lv_style_set_shadow_width(&g_style_cont1, 0);

    // lv_style_set_radius(&g_style_cont2, 0);

    lv_style_set_text_font(&g_style_cont1, &lv_font_montserrat_30);


    // 顶栏容器内按钮样式
    lv_style_init(&g_style_cont1_x);

    lv_style_set_bg_color(&g_style_cont1_x, lv_color_hex(0x1858eb));
    lv_style_set_bg_opa(&g_style_cont1_x, LV_OPA_COVER);

    lv_style_set_border_width(&g_style_cont1_x, 0);
    lv_style_set_outline_width(&g_style_cont1_x, 0);
    lv_style_set_shadow_width(&g_style_cont1_x, 0);

    lv_style_set_radius(&g_style_cont1_x, 8);

    lv_style_set_text_font(&g_style_cont1_x, &lv_font_montserrat_14);


    // 操作栏容器背景框样式
    lv_style_init(&g_style_cont2);

    lv_style_set_pad_all(&g_style_cont2, 18);
    // lv_style_set_pad_column(&g_style_cont2, 15);
    lv_style_set_pad_row(&g_style_cont2, 18);

    lv_style_set_bg_color(&g_style_cont2, lv_color_hex(0x1d2129));
    lv_style_set_bg_opa(&g_style_cont2, LV_OPA_COVER);

    lv_style_set_border_width(&g_style_cont2, 0);
    lv_style_set_outline_width(&g_style_cont2, 1);
    lv_style_set_outline_color(&g_style_cont2, lv_color_hex(0x1d2129));

    // lv_style_set_shadow_width(&g_style_cont2, 0);

    lv_style_set_radius(&g_style_cont2, 0);


    // 操作框样式
    lv_style_init(&g_style_cont2_x);

    lv_style_set_pad_all(&g_style_cont2_x, 0);

    lv_style_set_bg_color(&g_style_cont2_x, lv_color_hex(0x282c34));
    lv_style_set_bg_opa(&g_style_cont2_x, 95);

    lv_style_set_border_width(&g_style_cont2_x, 1);
    lv_style_set_border_color(&g_style_cont2_x, lv_color_hex(0x363b44));

    lv_style_set_radius(&g_style_cont2_x, 10);

    lv_style_set_shadow_width(&g_style_cont2_x, 20);
    lv_style_set_shadow_color(&g_style_cont2_x, lv_color_hex(0x000000));
    lv_style_set_shadow_opa(&g_style_cont2_x, 60);
    lv_style_set_shadow_offset_x(&g_style_cont2_x, 0);
    lv_style_set_shadow_offset_y(&g_style_cont2_x, 8);
    lv_style_set_shadow_spread(&g_style_cont2_x, 0);
}


void add_new_item_event_handler(lv_event_t *e) {
    lv_obj_t *cont2 = lv_event_get_user_data(e);

    lv_obj_t *cont2_x = lv_obj_create(cont2);
    lv_obj_set_size(cont2_x, LV_PCT(100), LV_PCT(14));
    lv_obj_add_style(cont2_x, &g_style_cont2_x, 0);

}
