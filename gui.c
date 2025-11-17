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

// 主题色定义
#define DARK_BG_COLOR        lv_color_hex(0x1d2129)
#define GRAY_BG_COLOR        lv_color_hex(0x282C34)
#define BORDER_COLOR         lv_color_hex(0x373c45)
#define GRAY_BUTTON_COLOR    lv_color_hex(0x1858eb)
#define GRENN_FONT_COLOR     lv_color_hex(0x00b42a)
#define GRAY_FONT_COLOR      lv_color_hex(0x9097a2)
#define BLUE_FONT_COLOR      lv_color_hex(0x165dff)
#define SHADOW_COLOR         lv_color_hex(0x000000)
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
static void init_style(void);
static void add_new_item_event_handler(lv_event_t *e);
static void update_translater_event_handler(lv_event_t *e);
static void MQTT_setting_event_handler(lv_event_t *e);
static void del_item_event_handler(lv_event_t *e);
static lv_obj_t * colum_obj_create(lv_obj_t *parent);
static lv_obj_t * component_obj_create(lv_obj_t *parent);

/**********************
 *  STATIC VARIABLES
 **********************/
static disp_size_t disp_size;
static const lv_font_t *font_large;
static const lv_font_t *font_normal;

// 全局键盘对象
static lv_obj_t *g_kb;

// 背景框样式变量
static lv_style_t g_style_cont1;
static lv_style_t g_style_cont1_x;
static lv_style_t g_style_cont2;
static lv_style_t g_style_cont2_x;
static lv_style_t g_style_cont2_x_x;
static lv_style_t g_style_cont2_x_2_x;
static lv_style_t g_style_cont2_x_3_x;
static lv_style_t g_style_jump_bg_conf;
static lv_style_t g_style_jump_conf;

static lv_style_t g_style_cont_send_period;

static lv_style_t style_icon;
static lv_style_t button_style;
static lv_style_t text_style;


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
    lv_obj_set_flex_flow(cont2, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont2, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);


    // 添加顶栏按钮
    lv_obj_t *up_btn = NULL;
    lv_obj_t *up_label = NULL;

    up_label = lv_label_create(cont1);
    lv_label_set_text(up_label, "Industral Control");
    lv_obj_set_style_text_color(up_label, BLUE_FONT_COLOR, 0);
    lv_obj_set_flex_grow(up_label, 1);
    lv_obj_set_style_text_align(up_label, LV_TEXT_ALIGN_LEFT, 0);

    up_btn = lv_button_create(cont1);
    up_label = lv_label_create(up_btn);
    lv_obj_add_style(up_btn, &button_style, 0);
    lv_obj_set_style_bg_color(up_btn, GRAY_BUTTON_COLOR, 0);
    lv_obj_set_style_text_font(up_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(up_label, "Add Node");
    lv_obj_add_event_cb(up_btn, add_new_item_event_handler, LV_EVENT_CLICKED, cont2);

    up_btn = lv_button_create(cont1);
    up_label = lv_label_create(up_btn);
    lv_obj_add_style(up_btn, &button_style, 0);
    lv_obj_set_style_bg_color(up_btn, GRAY_BUTTON_COLOR, 0);
    lv_obj_set_style_text_font(up_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(up_label, "Update");
    lv_obj_add_event_cb(up_btn, update_translater_event_handler, LV_EVENT_CLICKED, NULL);

    up_btn  = lv_button_create(cont1);
    up_label = lv_label_create(up_btn);
    lv_obj_set_style_bg_color(up_btn, GRAY_BUTTON_COLOR, 0);
    lv_obj_set_style_text_font(up_label, &lv_font_montserrat_14, 0);
    lv_obj_add_style(up_btn, &button_style, 0);
    lv_label_set_text(up_label, "MQTT Setting");
    lv_obj_add_event_cb(up_btn, MQTT_setting_event_handler, LV_EVENT_CLICKED, NULL);


}

void init_style(void) {
    // 顶栏容器背景框样式
    lv_style_init(&g_style_cont1);

    lv_style_set_pad_all(&g_style_cont1, 12);
    lv_style_set_pad_column(&g_style_cont1, 15);
    // lv_style_set_pad_row(&g_style_cont2, 18);

    lv_style_set_bg_color(&g_style_cont1, GRAY_BG_COLOR); // 深色背景
    lv_style_set_bg_opa(&g_style_cont1, LV_OPA_COVER);

    lv_style_set_border_width(&g_style_cont1, 1);
    lv_style_set_border_color(&g_style_cont1, BORDER_COLOR); // 深灰色边框
    lv_style_set_border_opa(&g_style_cont1, LV_OPA_COVER);
    lv_style_set_border_side(&g_style_cont1, LV_BORDER_SIDE_BOTTOM);

    lv_style_set_outline_width(&g_style_cont1, 0);
    // lv_style_set_outline_opa(&g_style_cont1_x, LV_OPA_TRANSP);

    lv_style_set_shadow_width(&g_style_cont1, 0);

    // lv_style_set_radius(&g_style_cont2, 0);

    lv_style_set_text_font(&g_style_cont1, &lv_font_montserrat_30);




    // 操作栏容器背景框样式
    lv_style_init(&g_style_cont2);

    lv_style_set_pad_all(&g_style_cont2, 18);
    lv_style_set_pad_column(&g_style_cont2, 18);
    // lv_style_set_pad_row(&g_style_cont2, 18);

    lv_style_set_bg_color(&g_style_cont2, DARK_BG_COLOR);
    lv_style_set_bg_opa(&g_style_cont2, LV_OPA_COVER);

    lv_style_set_border_width(&g_style_cont2, 0);
    lv_style_set_outline_width(&g_style_cont2, 1);
    lv_style_set_outline_color(&g_style_cont2, DARK_BG_COLOR);

    // lv_style_set_shadow_width(&g_style_cont2, 0);

    lv_style_set_radius(&g_style_cont2, 0);


    // 操作框样式
    lv_style_init(&g_style_cont2_x);

    lv_style_set_pad_all(&g_style_cont2_x, 0);

    lv_style_set_bg_color(&g_style_cont2_x, GRAY_BG_COLOR);
    lv_style_set_bg_opa(&g_style_cont2_x, 95);

    lv_style_set_border_width(&g_style_cont2_x, 1);
    lv_style_set_border_color(&g_style_cont2_x, BORDER_COLOR);

    lv_style_set_radius(&g_style_cont2_x, 10);

    lv_style_set_shadow_width(&g_style_cont2_x, 20);
    lv_style_set_shadow_color(&g_style_cont2_x, SHADOW_COLOR);
    lv_style_set_shadow_opa(&g_style_cont2_x, 60);
    lv_style_set_shadow_offset_x(&g_style_cont2_x, 0);
    lv_style_set_shadow_offset_y(&g_style_cont2_x, 8);
    lv_style_set_shadow_spread(&g_style_cont2_x, 0);


    // 操作框内操作背景样式
    lv_style_init(&g_style_cont2_x_x);
    lv_style_set_pad_all(&g_style_cont2_x_x, 10);
    // lv_style_set_pad_left(&g_style_cont2_x_x, 0);
    lv_style_set_radius(&g_style_cont2_x_x, 0);
    lv_style_set_border_width(&g_style_cont2_x_x, 0);
    // lv_style_set_border_color(&g_style_cont2_x_x, BORDER_COLOR);
    lv_style_set_bg_opa(&g_style_cont2_x_x, LV_OPA_TRANSP);

    // 操作框内操作内容样式
    lv_style_init(&g_style_cont_send_period);
    lv_style_set_radius(&g_style_cont_send_period, 0);
    lv_style_set_pad_all(&g_style_cont_send_period, 3);


    // 通用弹出遮盖背景样式
    lv_style_init(&g_style_jump_bg_conf);
    lv_style_set_radius(&g_style_jump_bg_conf, 0);
    lv_style_set_pad_all(&g_style_jump_bg_conf, 0);
    lv_style_set_bg_color(&g_style_jump_bg_conf, lv_color_black());
    lv_style_set_bg_opa(&g_style_jump_bg_conf, LV_OPA_50);
    lv_style_set_border_width(&g_style_jump_bg_conf, 0);

    // 通用弹出框样式
    lv_style_init(&g_style_jump_conf);
    lv_style_set_radius(&g_style_jump_conf, 10);
    lv_style_set_pad_all(&g_style_jump_conf, 0);
    lv_style_set_bg_color(&g_style_jump_conf, GRAY_BG_COLOR);
    lv_style_set_bg_opa(&g_style_jump_conf, LV_OPA_COVER);
    lv_style_set_border_width(&g_style_jump_conf, 0);
    lv_style_set_shadow_width(&g_style_jump_conf, 20);
    lv_style_set_shadow_color(&g_style_jump_conf, SHADOW_COLOR);
    lv_style_set_shadow_opa(&g_style_jump_conf, 40);
    lv_style_set_shadow_offset_x(&g_style_jump_conf, 0);
    lv_style_set_shadow_offset_y(&g_style_jump_conf, 10);
    lv_style_set_shadow_spread(&g_style_jump_conf, 0);


    // 通用按钮样式
    lv_style_init(&button_style);

    // lv_style_set_bg_color(&button_style, GRAY_BUTTON_COLOR);
    lv_style_set_bg_opa(&button_style, LV_OPA_COVER);

    lv_style_set_border_width(&button_style, 0);
    lv_style_set_outline_width(&button_style, 0);
    lv_style_set_shadow_width(&button_style, 0);

    lv_style_set_radius(&button_style, 10);

    // lv_style_set_text_font(&button_style, &lv_font_montserrat_14);

    // 通用文本框背景样式
    lv_style_init(&text_style);
    lv_style_set_bg_color(&text_style, DARK_BG_COLOR); // 深色背景
    lv_style_set_bg_opa(&text_style, LV_OPA_COVER);

    lv_style_set_border_width(&text_style, 1);
    lv_style_set_border_color(&text_style, BORDER_COLOR); // 深灰色边框
    lv_style_set_border_opa(&text_style, LV_OPA_COVER);
    lv_style_set_outline_width(&text_style, 0);
    lv_style_set_shadow_width(&text_style, 0);

    lv_style_set_radius(&text_style, 10);
    lv_style_set_text_color(&text_style, GRENN_FONT_COLOR);
}


void add_new_item_event_handler(lv_event_t *e) {
    lv_obj_t *cont2 = lv_event_get_user_data(e);

    // 新建操作框
    lv_obj_t *cont2_x = lv_obj_create(cont2);
    lv_obj_set_size(cont2_x, LV_PCT(18), LV_PCT(100));
    lv_obj_add_style(cont2_x, &g_style_cont2_x, 0);
    lv_obj_clear_flag(cont2_x, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(cont2_x, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont2_x, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // // 利用LVGL的对象继承关系记录一些数据，这里只是记录 rpc_add_point 返回的唯一句柄
    // lv_obj_t *label_rpc_fd = lv_label_create(cont2_x); // [0]
    // lv_label_set_text_fmt(label_rpc_fd, "%d", rpc_fd);
    // lv_obj_set_style_opa(label_rpc_fd, LV_OPA_TRANSP, 0);
    // lv_obj_add_flag(label_rpc_fd, LV_OBJ_FLAG_HIDDEN);

    // 添加操作框内标题栏
    lv_obj_t *cont2_x_1 = lv_obj_create(cont2_x);
    lv_obj_set_size(cont2_x_1, LV_PCT(100), LV_PCT(4));
    lv_obj_add_style(cont2_x_1, &g_style_cont2_x_x, 0);
    lv_obj_set_flex_flow(cont2_x_1, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont2_x_1, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cont2_x_1, LV_OBJ_FLAG_SCROLLABLE);

    // 标题栏内容
    lv_obj_t *up_label = lv_label_create(cont2_x_1);
    lv_obj_set_flex_grow(up_label, 1);
    lv_label_set_text(up_label, "Node 1");
    lv_obj_set_style_text_color(up_label, GRENN_FONT_COLOR, 0);
    lv_obj_set_style_text_font(up_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(up_label, LV_TEXT_ALIGN_LEFT, 0);

    // 删除按钮
    lv_obj_t *btn_del_item = lv_button_create(cont2_x_1);
    lv_obj_set_height(btn_del_item, LV_PCT(50));
    lv_obj_update_layout(btn_del_item);
    lv_obj_set_width(btn_del_item, lv_obj_get_height(btn_del_item));
    lv_obj_set_style_radius(btn_del_item, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(btn_del_item, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_del_item, 0, 0);
    lv_obj_set_style_shadow_width(btn_del_item, 0, 0);
    up_label = lv_label_create(btn_del_item);
    lv_obj_center(up_label);
    lv_label_set_text(up_label, LV_SYMBOL_CLOSE);
    lv_obj_add_event_cb(btn_del_item, del_item_event_handler, LV_EVENT_CLICKED, NULL);

    // 添加操作框内操作内容
    lv_obj_t *cont2_x_2 = lv_obj_create(cont2_x);
    lv_obj_set_size(cont2_x_2, LV_PCT(100), LV_PCT(96));
    lv_obj_add_style(cont2_x_2, &g_style_cont2_x_x, 0);
    lv_obj_set_style_border_color(cont2_x_2, BORDER_COLOR, 0);
    lv_obj_set_style_border_width(cont2_x_2, 1, 0);
    lv_obj_set_style_border_side(cont2_x_2, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_flex_flow(cont2_x_2, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont2_x_2, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(cont2_x_2, LV_OBJ_FLAG_SCROLLABLE);

    /*********************************************************************************************************/
    // 在这里添加更多的控件到 cont2_x_2 中，作为节点的具体操作界面
    lv_obj_t *cont2_x_2_x;
    lv_obj_t *cont2_x_2_x_ta;
    lv_obj_t *cont2_x_2_x_label;
    lv_obj_t *btn;
    lv_obj_t *dd;
    lv_obj_t *ta;
    
    // 创建一行容器
    cont2_x_2_x = colum_obj_create(cont2_x_2);

    // 创建一个容器存放：modbus 通讯方式选择
    cont2_x_2_x_ta = component_obj_create(cont2_x_2_x);

    cont2_x_2_x_label = lv_label_create(cont2_x_2_x_ta);
    lv_label_set_text(cont2_x_2_x_label, "MODE");
    lv_obj_set_style_text_color(cont2_x_2_x_label, GRAY_FONT_COLOR, 0);

    dd = lv_dropdown_create(cont2_x_2_x_ta);
    lv_obj_set_style_bg_color(dd, DARK_BG_COLOR, 0);
    lv_obj_set_style_bg_opa(dd, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(dd, BORDER_COLOR, 0);
    lv_obj_set_style_border_opa(dd, LV_OPA_COVER, 0);
    lv_obj_set_width(dd, 70);
    lv_dropdown_set_options(dd, "RTU\n"
                                "TCP");
    lv_obj_set_style_text_color(dd, GRENN_FONT_COLOR, 0);
    lv_obj_set_style_text_font(dd, &lv_font_montserrat_14, 0);

    // 创建一个容器存放：COM/IP
    cont2_x_2_x_ta = component_obj_create(cont2_x_2_x);

    cont2_x_2_x_label = lv_label_create(cont2_x_2_x_ta);
    lv_label_set_text(cont2_x_2_x_label, "COM/IP");
    lv_obj_set_style_text_color(cont2_x_2_x_label, GRAY_FONT_COLOR, 0);

    btn = lv_button_create(cont2_x_2_x_ta);
    lv_obj_add_style(btn, &button_style, 0);
    lv_obj_set_style_bg_color(btn, BLUE_FONT_COLOR, 0);
    cont2_x_2_x_label = lv_label_create(btn);
    lv_label_set_text(cont2_x_2_x_label, "Setting");


    // 创建一行容器
    cont2_x_2_x = colum_obj_create(cont2_x_2);

    // 创建一个容器存放：设备地址标签和文本输入框
    cont2_x_2_x_ta = component_obj_create(cont2_x_2_x);

    cont2_x_2_x_label = lv_label_create(cont2_x_2_x_ta);
    lv_label_set_text(cont2_x_2_x_label, "Device Addr");
    lv_obj_set_style_text_color(cont2_x_2_x_label, GRAY_FONT_COLOR, 0);

    ta = lv_textarea_create(cont2_x_2_x_ta);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_accepted_chars(ta, "0123456789ABCDEFabcdef");
    lv_textarea_set_text(ta, "1");
    lv_obj_set_width(ta, 100);
    lv_obj_set_style_text_color(ta, GRENN_FONT_COLOR, 0);

    // 创建一个容器存放：寄存器地址标签和文本输入框
    cont2_x_2_x_ta = component_obj_create(cont2_x_2_x);

    cont2_x_2_x_label = lv_label_create(cont2_x_2_x_ta);
    lv_label_set_text(cont2_x_2_x_label, "Register Addr");
    lv_obj_set_style_text_color(cont2_x_2_x_label, GRAY_FONT_COLOR, 0);
    ta = lv_textarea_create(cont2_x_2_x_ta);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_accepted_chars(ta, "0123456789ABCDEFabcdef");
    lv_textarea_set_text(ta, "1");
    lv_obj_set_width(ta, 100);
    lv_obj_set_style_text_color(ta, GRENN_FONT_COLOR, 0);


    // 创建一行容器
    cont2_x_2_x = colum_obj_create(cont2_x_2);

    // 创建一个容器存放：功能码选择
    cont2_x_2_x_ta = component_obj_create(cont2_x_2_x);

    cont2_x_2_x_label = lv_label_create(cont2_x_2_x_ta);
    lv_label_set_text(cont2_x_2_x_label, "Function");
    lv_obj_set_style_text_color(cont2_x_2_x_label, GRAY_FONT_COLOR, 0);

    dd = lv_dropdown_create(cont2_x_2_x_ta);
    lv_obj_set_style_bg_color(dd, DARK_BG_COLOR, 0);
    lv_obj_set_style_bg_opa(dd, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(dd, BORDER_COLOR, 0);
    lv_obj_set_style_border_opa(dd, LV_OPA_COVER, 0);
    lv_obj_set_width(dd, 200);
    lv_dropdown_set_options(dd, "Coils (0x)\n"
                                "Discrete Inputs (1x)\n"
                                "Holding Registers (4x)\n"
                                "Input Registers (3x)");
    lv_obj_set_style_text_color(dd, GRENN_FONT_COLOR, 0);
    lv_obj_set_style_text_font(dd, &lv_font_montserrat_14, 0);


    // 创建一行容器
    cont2_x_2_x = colum_obj_create(cont2_x_2);

    // 创建一个容器存放：周期发送标签和文本输入框
    lv_obj_t *cb_send_period;
    lv_obj_t *ta_send_period;
    lv_obj_t *label_send_period;

    cb_send_period = lv_checkbox_create(cont2_x_2_x);
    lv_checkbox_set_text(cb_send_period, "Period");
    lv_obj_set_style_text_color(cb_send_period, GRAY_FONT_COLOR, 0);

    // 创建定时器
    // lv_timer_t *lv_timer_send_period = lv_timer_create(send_period_timer, period, NULL);
    // lv_timer_pause(lv_timer_send_period);
    // lv_obj_add_event_cb(cb_send_period, cb_send_period_event_handler, LV_EVENT_VALUE_CHANGED, lv_timer_send_period);

    ta_send_period = lv_textarea_create(cont2_x_2_x); // [1-3-1]
    lv_textarea_set_one_line(ta_send_period, true);
    lv_textarea_set_accepted_chars(ta_send_period, "0123456789");
    lv_textarea_set_text(ta_send_period, "1000");
    lv_obj_set_size(ta_send_period, 100, 36);
    lv_obj_remove_flag(ta_send_period, LV_OBJ_FLAG_SCROLLABLE);
    label_send_period = lv_label_create(cont2_x_2_x); // [1-3-2]
    lv_label_set_text(label_send_period, "ms/Tim");
    lv_obj_set_style_text_color(label_send_period, GRAY_FONT_COLOR, 0);


    // 创建一行容器
    cont2_x_2_x = colum_obj_create(cont2_x_2);

    // 创建一个容器存放：开始和停止按钮
    cont2_x_2_x_ta = component_obj_create(cont2_x_2_x);
    btn = lv_button_create(cont2_x_2_x_ta);
    lv_obj_add_style(btn, &button_style, 0);
    lv_obj_set_style_bg_color(btn, BLUE_FONT_COLOR, 0);
    cont2_x_2_x_label = lv_label_create(btn);
    lv_label_set_text(cont2_x_2_x_label, "Start");

    btn = lv_button_create(cont2_x_2_x_ta);
    lv_obj_add_style(btn, &button_style, 0);
    lv_obj_set_style_bg_color(btn, GRAY_BG_COLOR, 0);
    cont2_x_2_x_label = lv_label_create(btn);
    lv_label_set_text(cont2_x_2_x_label, "Stop");
    /*********************************************************************************************************/
}

lv_obj_t * colum_obj_create(lv_obj_t *parent) {
    lv_obj_t * cont2_x_2_x = lv_obj_create(parent);
    lv_obj_remove_style_all(cont2_x_2_x);
    lv_obj_set_size(cont2_x_2_x, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_add_style(cont2_x_2_x, &g_style_cont_send_period, 0);
    lv_obj_set_flex_flow(cont2_x_2_x, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont2_x_2_x, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(cont2_x_2_x, 6, 0);

    return cont2_x_2_x;
}

lv_obj_t * component_obj_create(lv_obj_t *parent) {
    lv_obj_t * cont2_x_2_x_ta = lv_obj_create(parent);
    lv_obj_remove_style_all(cont2_x_2_x_ta);
    lv_obj_set_flex_flow(cont2_x_2_x_ta, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont2_x_2_x_ta, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(cont2_x_2_x_ta, 6, 0);
    lv_obj_add_style(cont2_x_2_x_ta, &g_style_cont_send_period, 0);
    lv_obj_set_size(cont2_x_2_x_ta, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    return cont2_x_2_x_ta;
}

void update_translater_event_handler(lv_event_t *e) {
    // 创建半透明遮罩层
    lv_obj_t *mask = lv_obj_create(lv_layer_top());
    lv_obj_set_size(mask, LV_PCT(100), LV_PCT(100));
    lv_obj_center(mask);
    lv_obj_add_style(mask, &g_style_jump_bg_conf, 0);

    #if 1
    // 主弹出框容器
    lv_obj_t *panel = lv_obj_create(mask);
    lv_obj_set_size(panel, LV_PCT(40), LV_PCT(60));
    lv_obj_center(panel);
    lv_obj_add_style(panel, &g_style_jump_conf, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    // 标题栏
    lv_obj_t *header = lv_obj_create(panel);
    lv_obj_set_size(header, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(header, BLUE_FONT_COLOR, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(header, 12, 0);
    lv_obj_set_style_radius(header, 0, 12);
    lv_obj_set_style_pad_all(header, 15, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Firmware Update");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);

    // 内容区域
    lv_obj_t *content = lv_obj_create(panel);
    lv_obj_set_size(content, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 20, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 添加一些示例内容
    lv_obj_t *info_label = lv_label_create(content);
    lv_label_set_text(info_label, "Current Version: 1.0.0\nLatest Version: 1.1.0");
    lv_obj_set_style_text_color(info_label, lv_color_hex(0xc9cdd4), 0);
    lv_obj_set_style_text_font(info_label, &lv_font_montserrat_14, 0);

    // 按钮区域
    lv_obj_t *btn_cont = lv_obj_create(content);
    lv_obj_set_size(btn_cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(btn_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_cont, 0, 0);
    lv_obj_set_style_pad_all(btn_cont, 0, 0);
    lv_obj_clear_flag(btn_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 取消按钮
    lv_obj_t *cancel_btn = lv_button_create(btn_cont);
    lv_obj_set_size(cancel_btn, 100, 40);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0x4a4a4a), 0);
    lv_obj_set_style_radius(cancel_btn, 8, 0);
    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_set_style_text_color(cancel_label, lv_color_white(), 0);
    lv_obj_center(cancel_label);
    lv_obj_add_event_cb(cancel_btn, del_item_event_handler, LV_EVENT_CLICKED, NULL);

    // 确认按钮
    lv_obj_t *confirm_btn = lv_button_create(btn_cont);
    lv_obj_set_size(confirm_btn, 100, 40);
    lv_obj_set_style_bg_color(confirm_btn, lv_color_hex(0x165dff), 0);
    lv_obj_set_style_radius(confirm_btn, 8, 0);
    lv_obj_t *confirm_label = lv_label_create(confirm_btn);
    lv_label_set_text(confirm_label, "Update");
    lv_obj_set_style_text_color(confirm_label, lv_color_white(), 0);
    lv_obj_center(confirm_label);
    #endif
}

void MQTT_setting_event_handler(lv_event_t *e) {

}


void del_item_event_handler(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);

    lv_obj_t *cont2_x = lv_obj_get_parent(btn);
    cont2_x = lv_obj_get_parent(cont2_x);

    lv_obj_delete_async(cont2_x);
}

