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


/**********************
 *  STATIC VARIABLES
 **********************/
static disp_size_t disp_size;
static const lv_font_t *font_large;
static const lv_font_t *font_normal;

static lv_obj_t *g_kb;

// 背景框样式变量
static lv_style_t g_style_cont1;
static lv_style_t g_style_cont2;
static lv_style_t g_style_cont2_x;
static lv_style_t g_style_cont2_x_x;
static lv_style_t g_style_cont2_x_2_x;
static lv_style_t g_style_cont2_x_3_x;

// 自定义事件代码，给每个节点的 Send 按钮使用
static uint32_t MY_LV_EVENT_UPDATE_RPC;     // 更新节点信息事件
static uint32_t MY_LV_EVENT_READ_PERIOD;    // 周期发送事件

void gui_start(void)
{
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
    lv_obj_t *cont1 = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(cont1);
    lv_obj_set_size(cont1, LV_PCT(100), LV_PCT(6));
    lv_obj_set_align(cont1, LV_ALIGN_TOP_MID);
    lv_obj_add_style(cont1, &g_style_cont1, 0);
    lv_obj_set_flex_flow(cont1, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont1, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *btn = lv_button_create(cont1);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Add Node");
    // lv_obj_add_event_cb(btn, add_new_item_event_handler, LV_EVENT_CLICKED, NULL);
    
}

void init_style(void)
{
    // 顶栏容器背景框样式
    lv_style_init(&g_style_cont1);
    lv_style_set_pad_all(&g_style_cont1, 12);
    lv_style_set_bg_opa(&g_style_cont1, LV_OPA_COVER);
    lv_style_set_bg_color(&g_style_cont1, lv_color_hex(0xcdcdcd));
    // lv_style_set_radius(&g_style_cont2, 0);
    // lv_style_set_border_width(&g_style_cont2, 0);
    lv_style_set_pad_column(&g_style_cont1, 15);
    // lv_style_set_pad_row(&g_style_cont2, 18);


}