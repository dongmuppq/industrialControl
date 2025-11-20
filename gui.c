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

#include "rpc.h"
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
#define GREEN_FONT_COLOR     lv_color_hex(0x00b42a)
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
// 主界面创建相关函数
static void init_style(void);
static void add_new_item_event_handler(lv_event_t *e);
static void add_new_item(lv_obj_t *parent, int point, char *port_info, int dev_addr, int reg_addr, char *reg_type, int period, int channel);
static lv_obj_t * colum_obj_create(lv_obj_t *parent);
static lv_obj_t * component_obj_create(lv_obj_t *parent);
static void del_item_event_handler(lv_event_t *e);
// 周期发送相关函数
static void send_period_timer(lv_timer_t *timer);
static void cb_send_period_event_handler(lv_event_t *e);
// COM/IP 配置页面相关函数
static lv_obj_t *com_or_ip_conf_page_init(lv_obj_t *user_data, char *port_info, uint16_t port_type, int channel);
static void show_conf_event_handler(lv_event_t *e);
static void com_conf_opt_btn_event_handler(lv_event_t *e);
// 升级页面相关函数
static void update_translater_event_handler(lv_event_t *e);
static void upgrade_bar_event_cb(lv_event_t *e);
static void file_explorer_event_handler(lv_event_t *e);
static void file_explorer_upgrade_btn_event_handler(lv_event_t *e);
static void update_progress_timer(lv_timer_t *timer);
// MQTT 配置页面相关函数
static void MQTT_setting_event_handler(lv_event_t *e);
// 事件处理相关函数
static void dd_reg_type_event_handler(lv_event_t *e);
static void ta_event_event_handler(lv_event_t *e);
static void btn_send_event_handler(lv_event_t *e);
static void update_rpc_data(lv_obj_t *cont1, lv_obj_t *cont2);

/**********************
 *  STATIC VARIABLES
 **********************/
// RPC 客户端 ID
static int g_socket_client_id;
// 显示尺寸
static disp_size_t disp_size;
static const lv_font_t *font_large;
static const lv_font_t *font_normal;

// 全局键盘对象
static lv_obj_t *g_kb;
// 全局标签对象，显示所选择的升级文件路径
static lv_obj_t *g_label_selected_upgrade_file;

// 背景框样式变量
static lv_style_t g_style_cont1;
static lv_style_t g_style_cont2;
static lv_style_t g_style_cont2_x;
static lv_style_t g_style_cont2_x_x;
static lv_style_t g_style_jump_bg_conf;
static lv_style_t g_style_jump_conf;
static lv_style_t g_style_cont_send_period;
static lv_style_t button_style;
static lv_style_t text_style;


// 自定义事件代码，给每个节点的 Send 按钮使用
static uint32_t MY_LV_EVENT_UPDATE_RPC;     // 更新节点信息事件
static uint32_t MY_LV_EVENT_READ_PERIOD;    // 周期发送事件


static int g_bUpdatingStatus = 0;

static int isUpdating(void)
{
    return g_bUpdatingStatus;
}

static void SetUpdatingStatus(int on)
{
    g_bUpdatingStatus = on;
}

void gui_start(void) {
    lv_obj_t *cont1;
    lv_obj_t *cont2;

    // g_socket_client_id = RPC_Client_Init();
    // if (g_socket_client_id == -1)
    // {
    //     LV_LOG_ERROR("RCP Clinet Init Error %d!", g_socket_client_id);
    //     //exit(1);
    // }

    if (LV_HOR_RES <= 320)
        disp_size = DISP_SMALL;
    else if (LV_HOR_RES < 720)
        disp_size = DISP_MEDIUM;
    else
        disp_size = DISP_LARGE;

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

#ifdef RPC_ENABLE
    int cnt = rpc_get_point_count(g_socket_client_id);
    LV_LOG_USER("point count = %d", cnt);
    if (cnt > 0) {
        int err;
        int pre_point = -1;
        PointInfo tInfo;
        for (int i = 0; i < cnt; i++) {
            err = rpc_get_next_point(g_socket_client_id, pre_point, &tInfo);
            if (!err)
            {
                printf("Point %d:\n", tInfo.point);
                printf("port_info: %s\n", tInfo.port_info);
                printf("dev_addr: %d\n", tInfo.dev_addr);
                printf("reg_addr: %d\n", tInfo.reg_addr);
                printf("reg_type: %s\n", tInfo.reg_type);
                printf("period: %d\n", tInfo.period);
                printf("channel: %d\n", tInfo.channel);

                // 添加新节点
                add_new_item(cont2, tInfo.point, tInfo.port_info, tInfo.dev_addr, tInfo.reg_addr, tInfo.reg_type, tInfo.period, tInfo.channel);
            }
            pre_point = tInfo.point;
        }
    } else {
#endif
        // 添加新节点
        add_new_item(cont2, -1, "COM1,115200,8n1", 4, 0, "1x", 300, DEFAULT_CHAEENL_INT);
    // }
}


/*
 * @brief 初始化样式
 * @param 无
 * @return 无
 */
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
    lv_style_set_pad_all(&g_style_cont_send_period, 6);
    // lv_style_set_pad_column(&g_style_cont_send_period, 30);

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
    lv_style_set_border_width(&g_style_jump_conf, 1);
    lv_style_set_border_color(&g_style_jump_conf, BORDER_COLOR);
    lv_style_set_border_opa(&g_style_jump_conf, LV_OPA_COVER);

    lv_style_set_shadow_width(&g_style_jump_conf, 20);
    lv_style_set_shadow_color(&g_style_jump_conf, SHADOW_COLOR);
    lv_style_set_shadow_opa(&g_style_jump_conf, 60);
    lv_style_set_shadow_offset_x(&g_style_jump_conf, 0);
    lv_style_set_shadow_offset_y(&g_style_jump_conf, 8);
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
    lv_style_set_text_color(&text_style, GREEN_FONT_COLOR);
}

// 添加新节点按钮事件处理函数
static void add_new_item_event_handler(lv_event_t *e) {
    // lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t *cont2 = lv_event_get_user_data(e);

    add_new_item(cont2, -1, "COM2,115200,8n1", 2, 2, "1x", 1000, DEFAULT_CHAEENL_INT);   
}

/*
 * @brief 添加新节点
 * @param parent       lvgl界面容器
 * @param point        节点索引
 * @param port_info    节点信息，如："/dev/ttyUSB0,115200,8n1" or "192.168.0.123:234"
 * @param dev_addr     modbus设备地址
 * @param reg_addr     modebus寄存器地址（modbus register address）
 * @param reg_type     modebus功能码："0x" - Coils, "1x" - Discrete Inputs, "4x" - Holding Registers, "3x" - Input Registers
 * @param period       访问周期(ms)
 * @param channel      H5上哪个通道
 */
static void add_new_item(lv_obj_t *parent, int point, char *port_info, int dev_addr, int reg_addr, char *reg_type, int period, int channel) {
    // 新建操作框
    lv_obj_t *cont2_x = lv_obj_create(parent);
    lv_obj_set_size(cont2_x, LV_PCT(18), LV_PCT(100));
    lv_obj_add_style(cont2_x, &g_style_cont2_x, 0);
    lv_obj_clear_flag(cont2_x, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(cont2_x, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont2_x, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    int rpc_fd;
    uint16_t port_type = 0; // 0:RTU, 1:TCP
    char tmp_str[32];

    if (point == -1) {
#ifdef RPC_ENABLE
        rpc_fd = rpc_add_point(g_socket_client_id, port_info, channel, dev_addr, reg_addr, reg_type, period);
        if (rpc_fd == -1) {
            LV_LOG_ERROR("RPC add point error! %d", rpc_fd);
            return;
        }
#else
        rpc_fd = rand() % 1000 + 1; // 模拟返回一个唯一句柄
#endif
    } else
        rpc_fd = point;

    // 利用LVGL的对象继承关系记录一些数据，这里只是记录 rpc_add_point 返回的唯一句柄
    lv_obj_t *label_rpc_fd = lv_label_create(cont2_x); // [0]
    lv_label_set_text_fmt(label_rpc_fd, "%d", rpc_fd);
    lv_obj_set_style_opa(label_rpc_fd, LV_OPA_TRANSP, 0);
    lv_obj_add_flag(label_rpc_fd, LV_OBJ_FLAG_HIDDEN);

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
    lv_obj_set_style_text_color(up_label, GREEN_FONT_COLOR, 0);
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

#if 1
    // 创建一行容器[0]
    cont2_x_2_x = colum_obj_create(cont2_x_2);

    // 创建一个容器存放：modbus 通讯方式选择
    cont2_x_2_x_ta = component_obj_create(cont2_x_2_x);

    cont2_x_2_x_label = lv_label_create(cont2_x_2_x_ta);
    lv_label_set_text(cont2_x_2_x_label, "MODE");
    lv_obj_set_style_text_color(cont2_x_2_x_label, GRAY_FONT_COLOR, 0);

    dd = lv_dropdown_create(cont2_x_2_x_ta);
    lv_obj_add_style(dd, &text_style, 0);
    lv_obj_set_style_bg_color(dd, DARK_BG_COLOR, 0);
    lv_obj_set_style_bg_opa(dd, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(dd, BORDER_COLOR, 0);
    lv_obj_set_style_border_opa(dd, LV_OPA_COVER, 0);
    lv_obj_set_width(dd, 100);
    lv_dropdown_set_options(dd, "RTU\n"
                                "TCP");
    // lv_obj_set_style_text_color(dd, GREEN_FONT_COLOR, 0);
    lv_obj_set_style_text_font(dd, &lv_font_montserrat_14, 0);
    if (strchr(port_info, ':')) {
        lv_dropdown_set_selected(dd, 1);    // TCP
        port_type = 1;
    } else {
        lv_dropdown_set_selected(dd, 0);    // RTU
        port_type = 0;
    }

    // 创建一个容器存放：COM/IP
    cont2_x_2_x_ta = component_obj_create(cont2_x_2_x);

    cont2_x_2_x_label = lv_label_create(cont2_x_2_x_ta);
    lv_label_set_text(cont2_x_2_x_label, "COM/IP");
    lv_obj_set_style_text_color(cont2_x_2_x_label, GRAY_FONT_COLOR, 0);

    btn = lv_button_create(cont2_x_2_x_ta);
    lv_obj_add_style(btn, &button_style, 0);
    lv_obj_set_style_bg_color(btn, BLUE_FONT_COLOR, 0);
    lv_obj_set_width(btn, 100);
    cont2_x_2_x_label = lv_label_create(btn);
    lv_label_set_text(cont2_x_2_x_label, "Setting");

    lv_obj_t *com_conf_page = com_or_ip_conf_page_init(cont2_x, port_info, port_type, channel);
    lv_obj_set_user_data(cont2_x, com_conf_page); // set user data
    lv_obj_set_user_data(com_conf_page, cont2_x); // set user data
    lv_obj_add_event_cb(btn, show_conf_event_handler, LV_EVENT_CLICKED, com_conf_page);

    // 创建一行容器[1]
    cont2_x_2_x = colum_obj_create(cont2_x_2);

    // 创建一个容器存放：设备地址标签和文本输入框
    cont2_x_2_x_ta = component_obj_create(cont2_x_2_x);

    cont2_x_2_x_label = lv_label_create(cont2_x_2_x_ta);
    lv_label_set_text(cont2_x_2_x_label, "Device Addr");
    lv_obj_set_style_text_color(cont2_x_2_x_label, GRAY_FONT_COLOR, 0);

    memset(tmp_str, 0, sizeof(tmp_str));
    lv_snprintf(tmp_str, sizeof(tmp_str), "%d", dev_addr);
    ta = lv_textarea_create(cont2_x_2_x_ta);
    lv_obj_add_style(ta, &text_style, 0);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_accepted_chars(ta, "0123456789ABCDEF");
    lv_textarea_set_text(ta, tmp_str);
    lv_obj_set_width(ta, 100);
    lv_obj_set_user_data(ta, cont2_x); // set user data
    lv_obj_add_event_cb(ta, ta_event_event_handler, LV_EVENT_ALL, NULL);


    // 创建一个容器存放：寄存器地址标签和文本输入框
    cont2_x_2_x_ta = component_obj_create(cont2_x_2_x);

    cont2_x_2_x_label = lv_label_create(cont2_x_2_x_ta);
    lv_label_set_text(cont2_x_2_x_label, "Register Addr");
    lv_obj_set_style_text_color(cont2_x_2_x_label, GRAY_FONT_COLOR, 0);
    ta = lv_textarea_create(cont2_x_2_x_ta);

    memset(tmp_str, 0, sizeof(tmp_str));
    lv_snprintf(tmp_str, sizeof(tmp_str), "%d", reg_addr);
    lv_obj_add_style(ta, &text_style, 0);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_accepted_chars(ta, "0123456789ABCDEF");
    lv_textarea_set_text(ta, tmp_str);
    lv_obj_set_width(ta, 100);
    lv_obj_set_user_data(ta, cont2_x); // set user data
    lv_obj_add_event_cb(ta, ta_event_event_handler, LV_EVENT_ALL, NULL);



    // 创建一行容器[2]
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
    lv_obj_set_width(dd, 250);
    lv_dropdown_set_options(dd, "Coils (0x)\n"
                                "Discrete Inputs (1x)\n"
                                "Holding Registers (4x)\n"
                                "Input Registers (3x)");
    lv_obj_set_style_text_color(dd, GREEN_FONT_COLOR, 0);
    lv_obj_set_style_text_font(dd, &lv_font_montserrat_14, 0);

    if (strstr(reg_type, "0x"))
        lv_dropdown_set_selected(dd, 0);
    else if (strstr(reg_type, "1x"))
        lv_dropdown_set_selected(dd, 1);
    else if (strstr(reg_type, "4x"))
        lv_dropdown_set_selected(dd, 2);
    else if (strstr(reg_type, "3x"))
        lv_dropdown_set_selected(dd, 3);

    // 创建一行容器[3]
    cont2_x_2_x = colum_obj_create(cont2_x_2);

    // 创建周期发送标签和文本输入框
    lv_obj_t *cb_send_period;
    lv_obj_t *ta_send_period;
    lv_obj_t *label_send_period;
    lv_timer_t *lv_timer_send_period;

    // 创建勾选框
    cb_send_period = lv_checkbox_create(cont2_x_2_x);
    lv_obj_add_style(cb_send_period, &text_style, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(cb_send_period, GRAY_FONT_COLOR, LV_PART_INDICATOR);
    lv_checkbox_set_text(cb_send_period, "Period");
    lv_obj_set_style_text_color(cb_send_period, GRAY_FONT_COLOR, 0);
    // 创建定时器
    lv_timer_send_period = lv_timer_create(send_period_timer, period, NULL);
    lv_timer_pause(lv_timer_send_period);
    lv_obj_add_event_cb(cb_send_period, cb_send_period_event_handler, LV_EVENT_VALUE_CHANGED, lv_timer_send_period);
    // 创建文本输入框，用于输入周期时间（ms）
    memset(tmp_str, 0, sizeof(tmp_str));
    lv_snprintf(tmp_str, sizeof(tmp_str), "%d", period);
    ta_send_period = lv_textarea_create(cont2_x_2_x);
    lv_obj_add_style(ta_send_period, &text_style, 0);
    lv_textarea_set_one_line(ta_send_period, true);
    lv_textarea_set_accepted_chars(ta_send_period, "0123456789");
    lv_textarea_set_text(ta_send_period, tmp_str);
    lv_obj_set_size(ta_send_period, 80, 36);
    lv_obj_remove_flag(ta_send_period, LV_OBJ_FLAG_SCROLLABLE);
    label_send_period = lv_label_create(cont2_x_2_x);
    lv_label_set_text(label_send_period, "ms/Tim");
    lv_obj_set_style_text_color(label_send_period, GRAY_FONT_COLOR, 0);
    lv_obj_set_user_data(ta_send_period, cont2_x); // set user data
    lv_obj_add_event_cb(ta_send_period, ta_event_event_handler, LV_EVENT_ALL, lv_timer_send_period);
    lv_obj_add_event_cb(btn_del_item, del_item_event_handler, LV_EVENT_CLICKED, lv_timer_send_period);

    // 创建一行容器[4]
    cont2_x_2_x = colum_obj_create(cont2_x_2);

    btn = lv_button_create(cont2_x_2_x);
    lv_obj_add_style(btn, &button_style, 0);
    lv_obj_set_style_bg_color(btn, BLUE_FONT_COLOR, 0);
    lv_obj_set_width(btn, 100);
    cont2_x_2_x_label = lv_label_create(btn);
    lv_label_set_text(cont2_x_2_x_label, "Start");

    lv_obj_add_event_cb(btn, btn_send_event_handler, LV_EVENT_ALL, cont2_x);
    lv_obj_add_event_cb(dd, dd_reg_type_event_handler, LV_EVENT_VALUE_CHANGED, btn);
    lv_obj_send_event(dd, LV_EVENT_VALUE_CHANGED, btn);
    lv_timer_set_user_data(lv_timer_send_period, btn);

    btn = lv_button_create(cont2_x_2_x);
    lv_obj_add_style(btn, &button_style, 0);
    lv_obj_set_style_bg_color(btn, GRAY_BG_COLOR, 0);
    lv_obj_set_width(btn, 100);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, BORDER_COLOR, 0);
    cont2_x_2_x_label = lv_label_create(btn);
    lv_label_set_text(cont2_x_2_x_label, "Stop");
    /*********************************************************************************************************/
    // 添加数据显示
    // 创建一行容器[5]
    cont2_x_2_x = colum_obj_create(cont2_x_2);

    // 创建一个容器存放：Write value(0x)标签和文本输入框
    cont2_x_2_x_ta = component_obj_create(cont2_x_2_x);

    cont2_x_2_x_label = lv_label_create(cont2_x_2_x_ta);
    lv_label_set_text(cont2_x_2_x_label, "Write value(0x)");
    lv_obj_set_style_text_color(cont2_x_2_x_label, GRAY_FONT_COLOR, 0);

    ta = lv_textarea_create(cont2_x_2_x_ta);
    lv_obj_add_style(ta, &text_style, 0);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_accepted_chars(ta, "0123456789");
    lv_textarea_set_text(ta, "1");
    lv_obj_set_width(ta, 100);
    lv_obj_set_user_data(ta, cont2_x); // set user data
    lv_obj_add_event_cb(ta, ta_event_event_handler, LV_EVENT_ALL, NULL);

    // 创建一个容器存放：Read value(0x)标签和文本输入框
    cont2_x_2_x_ta = component_obj_create(cont2_x_2_x);

    cont2_x_2_x_label = lv_label_create(cont2_x_2_x_ta); // [2-5-0]
    lv_label_set_text(cont2_x_2_x_label, "Read value(0x)");
    lv_obj_set_style_text_color(cont2_x_2_x_label, GRAY_FONT_COLOR, 0);

    ta = lv_textarea_create(cont2_x_2_x_ta);
    lv_obj_add_style(ta, &text_style, LV_STATE_DISABLED);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_accepted_chars(ta, "0123456789");
    lv_textarea_set_text(ta, "FF");
    lv_obj_set_width(ta, 100);
    lv_obj_add_state(ta, LV_STATE_DISABLED);
    // lv_obj_add_event_cb(ta, ta_event_event_handler, LV_EVENT_ALL, cont2_x);
#endif
#if 0
    // 添加图表
    cont2_x_2_x = colum_obj_create(cont2_x_2);
    lv_obj_t *chart = lv_chart_create(cont2_x_2_x);
    lv_obj_set_size(chart, LV_PCT(90), 100);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
#endif
}

/*
 * @brief 创建一个con2_x_2_x行容器
 * @param cont2_x_2 父容器
 * @return 新创建的行容器
 */
lv_obj_t * colum_obj_create(lv_obj_t *parent) {
    lv_obj_t * cont2_x_2_x = lv_obj_create(parent);
    lv_obj_remove_style_all(cont2_x_2_x);
    lv_obj_set_size(cont2_x_2_x, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_add_style(cont2_x_2_x, &g_style_cont_send_period, 0);
    lv_obj_set_flex_flow(cont2_x_2_x, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont2_x_2_x, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    return cont2_x_2_x;
}

/*
 * @brief 创建一个con2_x_2_x_ta组件容器
 * @param cont2_x_2_x 父容器
 * @return 新创建的组件容器
 */
lv_obj_t * component_obj_create(lv_obj_t *parent) {
    lv_obj_t * cont2_x_2_x_ta = lv_obj_create(parent);
    lv_obj_remove_style_all(cont2_x_2_x_ta);
    lv_obj_set_flex_flow(cont2_x_2_x_ta, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont2_x_2_x_ta, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    // lv_obj_add_style(cont2_x_2_x_ta, &g_style_cont_send_period, 0);
    lv_obj_set_style_pad_row(cont2_x_2_x_ta, 5, 0);
    lv_obj_set_size(cont2_x_2_x_ta, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    return cont2_x_2_x_ta;
}

/*
 * @brief 删除节点按钮事件处理函数
 */
static void del_item_event_handler(lv_event_t *e) {
    int point;
    lv_obj_t *label;

    lv_obj_t *btn = lv_event_get_target(e);
    lv_timer_t *timer = lv_event_get_user_data(e);

    lv_obj_t *cont2_x = lv_obj_get_parent(btn);
    cont2_x = lv_obj_get_parent(cont2_x);

    // 删除对应的定时器
    if (timer != NULL) {
        lv_timer_delete(timer);
    }

    lv_obj_delete_async(cont2_x);

#ifdef RPC_ENABLE
    // 删除对应的RPC节点
    label = lv_obj_get_child(cont2_x, 0);
    point = atoi(lv_label_get_text(label));
    rpc_remove_point(g_socket_client_id, point);
#endif
}

/*
 * @brief 周期发送定时器回调函数
 */
static void send_period_timer(lv_timer_t *timer)
{
    char str_tmp[64];
    int val_read;
    int val_write;
    int point;
    lv_obj_t *btn_send;

    btn_send = lv_timer_get_user_data(timer);

    if (!isUpdating()) /* wei */
    {
        lv_obj_send_event(btn_send, MY_LV_EVENT_READ_PERIOD, NULL);
    }
}


/*
 * @brief 周期发送勾选框事件处理函数
 */
static void cb_send_period_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *cb = lv_event_get_target(e);
    lv_timer_t *timer = lv_event_get_user_data(e);
    lv_obj_t *ta = lv_obj_get_parent(cb);
    ta = lv_obj_get_child(ta, 1);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        const char *ta_text = lv_textarea_get_text(ta);
        lv_timer_set_period(timer, atoi(ta_text));

        if (lv_obj_get_state(cb) & LV_STATE_CHECKED)
        {
            lv_timer_resume(timer);
        }
        else
        {
            lv_timer_pause(timer);
        }
    }
}


/*
 * @brief Function选择事件处理函数
 */
static void dd_reg_type_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *dd = lv_event_get_target(e);
    lv_obj_t *btn_send = lv_event_get_user_data(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        char buf[32];
        lv_dropdown_get_selected_str(dd, buf, sizeof(buf));
        if ((strstr(buf, "1x")) || (strstr(buf, "3x")))
        {
            lv_obj_add_state(btn_send, LV_STATE_DISABLED);
        }
        else
        {
            lv_obj_clear_state(btn_send, LV_STATE_DISABLED);
        }

        lv_obj_send_event(btn_send, MY_LV_EVENT_UPDATE_RPC, btn_send);
    }
}


/*
 * @brief 显示COM/IP配置页面按钮事件处理函数
 */
static void show_conf_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *conf_page = lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED)
    {
        lv_obj_clear_flag(conf_page, LV_OBJ_FLAG_HIDDEN);
    }
}

/*
 * @brief 创建COM/IP配置页面
 * @param user_data    父容器对象
 * @param port_info    端口信息字符串
 * @param port_type    端口类型，0:RTU, 1:TCP
 * @param channel      H5上哪个通道
 * @return 创建的配置页面对象
 */
static lv_obj_t *com_or_ip_conf_page_init(lv_obj_t *user_data, char *port_info, uint16_t port_type, int channel) {
    lv_obj_t *ta;
    lv_obj_t *dd;

    char tmp_str[128];

    char *ipaddr_str = "192.168.1.74";
    char *ip_port = "1502";

    char dev[64] = "COM1";
    char baud[32] = "115200";
    char parity[2] = "N";
    char data[3] = "8";
    char stop[2] = "1";
    char *str0;
    char *str;

    // port_type: 0:RTU, 1:TCP
    if (port_type == 1) {
        // TCP
        strcpy(tmp_str, port_info);
        str = strstr(tmp_str, ":");
        if (str)
        {
            *str = '\0';
            ipaddr_str = tmp_str;
            str++;
            ip_port = str;
        }
    } else if (port_type == 0) {
        // RTU
        /* /dev/ttyUSB0,115200,8n1 */
        strcpy(tmp_str, port_info);

        str = strstr(tmp_str, ",");
        if (str) {
            *str = '\0';
            strcpy(dev, tmp_str);
        } else {
            return NULL;
        }

        str0 = str + 1;
        str = strstr(str0, ",");
        if (str) {
            *str = '\0';
            lv_snprintf(baud, sizeof(baud), "%s", str0);
        }

        str0 = str + 1;
        data[0] = *str0;

        str0++;
        if (*str0 == 'n' || *str0 == 'N')
            parity[0] = 'N';
        if (*str0 == 'e' || *str0 == 'E')
            parity[0] = 'E';
        if (*str0 == 'o' || *str0 == 'O')
            parity[0] = 'O';

        str0++;
        stop[0] = *str0;
    } else {}

    
    // 创建半透明遮罩层
    lv_obj_t *mask = lv_obj_create(lv_layer_top());
    lv_obj_set_user_data(mask, user_data);
    lv_obj_set_size(mask, LV_PCT(100), LV_PCT(100));
    lv_obj_center(mask);
    lv_obj_add_style(mask, &g_style_jump_bg_conf, 0);

    // 主弹出框容器
    lv_obj_t *panel = lv_obj_create(mask);
    if (port_type == 2) {   // upgrade config
        lv_obj_set_size(panel, LV_PCT(25), LV_PCT(46));
    } else {                // com/ip config
        lv_obj_set_size(panel, LV_PCT(25), LV_PCT(40));
    }
    lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, 150);
    lv_obj_add_style(panel, &g_style_jump_conf, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label;

    /* title */
    lv_obj_t *panel_label = lv_obj_create(panel); // [0-0]
    lv_obj_remove_style_all(panel_label);
    lv_obj_set_size(panel_label, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(panel_label, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(panel_label, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    label = lv_label_create(panel_label);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_label_set_text(label, "Advanced Settings");
    lv_obj_set_style_text_color(label, BLUE_FONT_COLOR, 0);

    /* com port */
    lv_obj_t *panel_com_port = lv_obj_create(panel); // [0-1]
    lv_obj_remove_style_all(panel_com_port);
    lv_obj_set_size(panel_com_port, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(panel_com_port, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(panel_com_port, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    label = lv_label_create(panel_com_port);
    lv_label_set_text(label, "COM PORT  ");
    lv_obj_set_style_text_color(label, GRAY_FONT_COLOR, 0);

    ta = lv_textarea_create(panel_com_port);
    lv_obj_add_style(ta, &text_style, 0);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_text(ta, dev);
    // lv_obj_set_width(ta, LV_PCT(40));
    lv_obj_add_event_cb(ta, ta_event_event_handler, LV_EVENT_ALL, NULL);

    /* com baudrate */
    lv_obj_t *panel_com_baudrate = lv_obj_create(panel); // [0-2]
    lv_obj_remove_style_all(panel_com_baudrate);
    lv_obj_set_size(panel_com_baudrate, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(panel_com_baudrate, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(panel_com_baudrate, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    label = lv_label_create(panel_com_baudrate);
    lv_label_set_text(label, "Baudrate  ");
    lv_obj_set_style_text_color(label, GRAY_FONT_COLOR, 0);
    
    ta = lv_textarea_create(panel_com_baudrate);
    lv_obj_add_style(ta, &text_style, 0);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_accepted_chars(ta, "0123456789");
    lv_textarea_set_max_length(ta, 10);
    lv_textarea_set_text(ta, baud);
    // lv_obj_set_width(ta, LV_PCT(40));
    lv_obj_add_event_cb(ta, ta_event_event_handler, LV_EVENT_ALL, NULL);

    int32_t dd_index_option = 0;
    /* data bits */
    lv_obj_t *panel_data_bits = lv_obj_create(panel); // [0-3]
    lv_obj_remove_style_all(panel_data_bits);
    lv_obj_set_size(panel_data_bits, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(panel_data_bits, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(panel_data_bits, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    label = lv_label_create(panel_data_bits);
    lv_label_set_text(label, "Data bits ");
    lv_obj_set_style_text_color(label, GRAY_FONT_COLOR, 0);

    dd = lv_dropdown_create(panel_data_bits);
    lv_obj_add_style(dd, &text_style, 0);
    lv_dropdown_set_options(dd,
                            "5\n6\n7\n8");
    dd_index_option = lv_dropdown_get_option_index(dd, data);
    lv_dropdown_set_selected(dd, dd_index_option);

    /* stop bits */
    lv_obj_t *panel_stop_bits = lv_obj_create(panel); // [0-4]
    lv_obj_remove_style_all(panel_stop_bits);
    lv_obj_set_size(panel_stop_bits, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(panel_stop_bits, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(panel_stop_bits, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    label = lv_label_create(panel_stop_bits);
    lv_label_set_text(label, "Stop bits ");
    lv_obj_set_style_text_color(label, GRAY_FONT_COLOR, 0);

    dd = lv_dropdown_create(panel_stop_bits);
    lv_obj_add_style(dd, &text_style, 0);
    lv_dropdown_set_options(dd,
                            "1\n2");
    dd_index_option = lv_dropdown_get_option_index(dd, stop);
    lv_dropdown_set_selected(dd, dd_index_option);

    /* Parity */
    lv_obj_t *panel_parity = lv_obj_create(panel); // [0-5]
    lv_obj_remove_style_all(panel_parity);
    lv_obj_set_size(panel_parity, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(panel_parity, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(panel_parity, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    label = lv_label_create(panel_parity);
    lv_label_set_text(label, "Parity ");
    lv_obj_set_style_text_color(label, GRAY_FONT_COLOR, 0);

    dd = lv_dropdown_create(panel_parity);
    lv_obj_add_style(dd, &text_style, 0);
    lv_dropdown_set_options(dd,
                            "N\nO\nE");
    // lv_dropdown_set_options(dd, "None\nOdd\nEven\nMark\nSpace");
    dd_index_option = lv_dropdown_get_option_index(dd, parity);
    lv_dropdown_set_selected(dd, dd_index_option);

    /* ip addr setting */
    lv_obj_t *panel_ip_addr;

    /* 2. Remote IP */
    panel_ip_addr = lv_obj_create(panel); // [0-6]
    lv_obj_remove_style_all(panel_ip_addr);
    lv_obj_set_size(panel_ip_addr, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(panel_ip_addr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(panel_ip_addr, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    label = lv_label_create(panel_ip_addr);
    lv_label_set_text(label, "Remote IP ");
    lv_obj_set_style_text_color(label, GRAY_FONT_COLOR, 0);

    ta = lv_textarea_create(panel_ip_addr);
    lv_obj_add_style(ta, &text_style, 0);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_text(ta, ipaddr_str);
    lv_obj_add_event_cb(ta, ta_event_event_handler, LV_EVENT_ALL, NULL);

    panel_ip_addr = lv_obj_create(panel); // [0-7]
    lv_obj_remove_style_all(panel_ip_addr);
    lv_obj_set_size(panel_ip_addr, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(panel_ip_addr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(panel_ip_addr, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    label = lv_label_create(panel_ip_addr);
    lv_label_set_text(label, "Remote Port ");
    lv_obj_set_style_text_color(label, GRAY_FONT_COLOR, 0);

    ta = lv_textarea_create(panel_ip_addr);
    lv_obj_add_style(ta, &text_style, 0);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_accepted_chars(ta, "0123456789");
    lv_textarea_set_text(ta, ip_port);
    lv_obj_add_event_cb(ta, ta_event_event_handler, LV_EVENT_ALL, NULL);

    /* Channel */
    lv_memzero(tmp_str, sizeof(tmp_str));
    lv_snprintf(tmp_str, sizeof(tmp_str), "%d", channel);
    lv_obj_t *panel_dev_channel;
    panel_dev_channel = lv_obj_create(panel); // [0-8]
    lv_obj_remove_style_all(panel_dev_channel);
    lv_obj_set_size(panel_dev_channel, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(panel_dev_channel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(panel_dev_channel, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    label = lv_label_create(panel_dev_channel);
    lv_label_set_text(label, "Channel       ");
    lv_obj_set_style_text_color(label, GRAY_FONT_COLOR, 0);


    ta = lv_textarea_create(panel_dev_channel);
    lv_obj_add_style(ta, &text_style, 0);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_accepted_chars(ta, "0123456789");
    lv_textarea_set_text(ta, tmp_str);
    lv_obj_add_event_cb(ta, ta_event_event_handler, LV_EVENT_ALL, NULL);


    /* Upgrade mode */
    if (port_type == 2) {
        lv_obj_t *panel_upgrade_mode = lv_obj_create(panel); // [0-3]
        lv_obj_remove_style_all(panel_upgrade_mode);
        lv_obj_set_size(panel_upgrade_mode, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(panel_upgrade_mode, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(panel_upgrade_mode, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        label = lv_label_create(panel_upgrade_mode); // [0-3-0]
        lv_label_set_text(label, "Upgrade mode");
         lv_obj_set_style_text_color(label, GRAY_FONT_COLOR, 0);

        dd = lv_dropdown_create(panel_upgrade_mode); // [0-3-1]
        lv_obj_add_style(dd, &text_style, 0);
        lv_dropdown_set_options(dd,
                                "UART\nNETWORK");
        lv_dropdown_set_selected(dd, 0);
    }


    /* Opt Btn */
    lv_obj_t *panel_opt = lv_obj_create(panel); // [0-8]
    lv_obj_remove_style_all(panel_opt);
    lv_obj_set_size(panel_opt, LV_PCT(100), LV_PCT(20));
    lv_obj_set_flex_flow(panel_opt, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(panel_opt, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *btn_ok = lv_btn_create(panel_opt);
    lv_obj_add_style(btn_ok, &button_style, 0);
    label = lv_label_create(btn_ok);
    lv_obj_set_style_text_color(label, lv_color_hex(0x00ff00), LV_PART_SELECTED);
    lv_obj_set_style_bg_color(label, lv_palette_main(LV_PALETTE_BLUE), LV_PART_SELECTED);
    lv_label_set_text_selection_start(label, 0);
    lv_label_set_text_selection_end(label, 1);

    lv_label_set_text(label, LV_SYMBOL_OK " OK    ");

    lv_obj_t *btn_Cancel = lv_btn_create(panel_opt);
    lv_obj_add_style(btn_Cancel, &button_style, 0);
    label = lv_label_create(btn_Cancel);
    lv_obj_set_style_text_color(label, lv_color_hex(0xff0000), LV_PART_SELECTED);
    lv_obj_set_style_bg_color(label, lv_palette_main(LV_PALETTE_BLUE), LV_PART_SELECTED);
    lv_label_set_text_selection_start(label, 0);
    lv_label_set_text_selection_end(label, 1);
    lv_label_set_text(label, LV_SYMBOL_CLOSE " Cancel");

    lv_obj_add_event_cb(btn_ok, com_conf_opt_btn_event_handler, LV_EVENT_CLICKED, mask);
    lv_obj_add_event_cb(btn_Cancel, com_conf_opt_btn_event_handler, LV_EVENT_CLICKED, mask);

    lv_obj_add_flag(mask, LV_OBJ_FLAG_HIDDEN);

    return mask;
}


/*
 * @brief COM/IP配置页面确定/取消按钮事件处理函数
 */
static void com_conf_opt_btn_event_handler(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *mask = lv_event_get_user_data(e);

    lv_obj_t *label = lv_obj_get_child(btn, 0);
    lv_obj_t *cont2_x = lv_obj_get_user_data(mask);

    char *label_str = lv_label_get_text(label);

    if (strstr(label_str, "OK")) {
        lv_obj_t *btn_send;
        if (cont2_x) {
            btn_send = lv_obj_get_child(cont2_x, 2); // cont2_x_2
            if (btn_send)
                btn_send = lv_obj_get_child(btn_send, 4);
            if (btn_send)
                btn_send = lv_obj_get_child(btn_send, 0);
            if (btn_send)
                lv_obj_send_event(btn_send, MY_LV_EVENT_UPDATE_RPC, NULL);
        }
    } else if (strstr(label_str, "Cancel")) {
        // TODO

    }
    // 关闭配置页面
    lv_obj_add_flag(mask, LV_OBJ_FLAG_HIDDEN);
}


/*
 * @brief 固件升级页面事件处理函数
 */
static void update_translater_event_handler(lv_event_t *e) {
    // 创建半透明遮罩层
    lv_obj_t *mask = lv_obj_create(lv_layer_top());
    lv_obj_set_size(mask, LV_PCT(100), LV_PCT(100));
    lv_obj_center(mask);
    lv_obj_add_style(mask, &g_style_jump_bg_conf, 0);


    // 主弹出框容器
    lv_obj_t *panel = lv_obj_create(mask);
    lv_obj_set_size(panel, LV_PCT(50), LV_PCT(50));
    lv_obj_center(panel);
    lv_obj_add_style(panel, &g_style_jump_conf, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    // [1] 标题栏
    lv_obj_t *header = lv_obj_create(panel);
    lv_obj_set_size(header, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(header, BLUE_FONT_COLOR, 0);
    // lv_obj_set_style_radius(header, 12, 0);
    // lv_obj_set_style_radius(header, 0, 12);
    // lv_obj_set_style_pad_all(header, 15, 0);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Firmware Update");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);

    // [2] 文件选择
    lv_obj_t *file_explorer = lv_file_explorer_create(panel); // [0-1]
    lv_file_explorer_set_sort(file_explorer, LV_EXPLORER_SORT_KIND);
    lv_obj_set_size(file_explorer, LV_PCT(100), LV_PCT(70));
    lv_file_explorer_open_dir(file_explorer, "./");
    // lv_obj_set_style_bg_color(file_explorer, DARK_BG_COLOR, LV_PART_MAIN | LV_PART_ITEMS);
    // lv_obj_set_style_text_color(file_explorer, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);

    // [3] 内容区域
    lv_obj_t *cont = lv_obj_create(panel); // [0-2]
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *label_show_sel_file = lv_label_create(cont);
    lv_obj_set_style_text_font(label_show_sel_file, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(label_show_sel_file, lv_color_white(), 0);
    lv_label_set_text(label_show_sel_file, "Please select a file");

    g_label_selected_upgrade_file = lv_label_create(cont);
    lv_obj_set_style_text_font(g_label_selected_upgrade_file, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(g_label_selected_upgrade_file, lv_color_white(), 0);
    lv_label_set_text(g_label_selected_upgrade_file, "test.bin");
    lv_obj_add_flag(g_label_selected_upgrade_file, LV_OBJ_FLAG_HIDDEN);


    // [4] 进度条
    cont = lv_obj_create(panel); // [0-3]
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *bar = lv_bar_create(cont);
    lv_bar_set_range(bar, 0, 100);
    lv_obj_set_size(bar, LV_PCT(100), 16);
    lv_obj_set_style_radius(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 0, LV_PART_INDICATOR);
    lv_obj_add_event_cb(bar, upgrade_bar_event_cb, LV_EVENT_DRAW_MAIN_END, NULL);
    
    #if 1
    // [5] 按钮区域
    lv_obj_t *panel_opt = lv_obj_create(panel); // [0-4]
    lv_obj_remove_style_all(panel_opt);
    lv_obj_set_size(panel_opt, LV_PCT(100), LV_PCT(20));
    lv_obj_set_flex_flow(panel_opt, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(panel_opt, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(panel_opt, 30, 0);


    lv_obj_t *btn_setting = lv_btn_create(panel_opt);
    lv_obj_add_style(btn_setting, &button_style, 0);
    lv_obj_t *label = lv_label_create(btn_setting);
    lv_obj_set_style_text_color(label, lv_color_hex(0x00ff00), LV_PART_SELECTED);
    lv_obj_set_style_bg_color(label, lv_palette_main(LV_PALETTE_BLUE), LV_PART_SELECTED);
    lv_label_set_text_selection_start(label, 0);
    lv_label_set_text_selection_end(label, 1);
    lv_label_set_text(label, LV_SYMBOL_SETTINGS " Settings");

    lv_obj_t *setting_page = com_or_ip_conf_page_init(NULL, NULL, 2, 0);
    lv_obj_add_event_cb(btn_setting, show_conf_event_handler, LV_EVENT_CLICKED, setting_page);

    lv_obj_t *btn_ok = lv_btn_create(panel_opt);
    lv_obj_add_style(btn_ok, &button_style, 0);
    label = lv_label_create(btn_ok);
    lv_obj_set_style_text_color(label, lv_color_hex(0x00ff00), LV_PART_SELECTED);
    lv_obj_set_style_bg_color(label, lv_palette_main(LV_PALETTE_BLUE), LV_PART_SELECTED);
    lv_label_set_text_selection_start(label, 0);
    lv_label_set_text_selection_end(label, 1);
    lv_label_set_text(label, LV_SYMBOL_OK " Upgrade");

    lv_obj_t *btn_Cancel = lv_btn_create(panel_opt);
    lv_obj_add_style(btn_Cancel, &button_style, 0);
    label = lv_label_create(btn_Cancel);
    lv_obj_set_style_text_color(label, lv_color_hex(0xff0000), LV_PART_SELECTED);
    lv_obj_set_style_bg_color(label, lv_palette_main(LV_PALETTE_BLUE), LV_PART_SELECTED);
    lv_label_set_text_selection_start(label, 0);
    lv_label_set_text_selection_end(label, 1);
    lv_label_set_text(label, LV_SYMBOL_CLOSE " Exit   ");

    lv_obj_set_user_data(btn_ok, setting_page); // 将设置界面传递给升级按钮用于升级参数获取
    lv_obj_add_event_cb(file_explorer, file_explorer_event_handler, LV_EVENT_ALL, label_show_sel_file);
    lv_obj_add_event_cb(btn_ok, file_explorer_upgrade_btn_event_handler, LV_EVENT_CLICKED, bar);
    lv_obj_add_event_cb(btn_Cancel, file_explorer_upgrade_btn_event_handler, LV_EVENT_CLICKED, bar);

    #endif
}

/*
 * @brief 升级进度条进度数值显示事件回调函数
 */
static void upgrade_bar_event_cb(lv_event_t *e) {
    lv_obj_t *obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_DRAW_MAIN_END)
    {
        lv_draw_label_dsc_t label_dsc;
        lv_draw_label_dsc_init(&label_dsc);
        label_dsc.font = LV_FONT_DEFAULT;

        char buf[8];
        lv_snprintf(buf, sizeof(buf), "%d", (int)lv_bar_get_value(obj));

        lv_point_t txt_size;
        lv_text_get_size(&txt_size, buf, label_dsc.font, label_dsc.letter_space, label_dsc.line_space, LV_COORD_MAX,
                         label_dsc.flag);

        lv_area_t txt_area;
        txt_area.x1 = 0;
        txt_area.x2 = txt_size.x - 1;
        txt_area.y1 = 0;
        txt_area.y2 = txt_size.y - 1;

        lv_bar_t *bar = (lv_bar_t *)obj;
        const lv_area_t *indic_area = &bar->indic_area;

        /*If the indicator is long enough put the text inside on the right*/
        if (lv_area_get_width(indic_area) > txt_size.x + 20)
        {
            lv_area_align(indic_area, &txt_area, LV_ALIGN_RIGHT_MID, -10, 0);
            label_dsc.color = lv_color_white();
        }
        /*If the indicator is still short put the text out of it on the right*/
        else
        {
            lv_area_align(indic_area, &txt_area, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
            label_dsc.color = lv_color_black();
        }
        label_dsc.text = buf;
        label_dsc.text_local = true;
        lv_layer_t *layer = lv_event_get_layer(e);
        lv_draw_label(layer, &label_dsc, &txt_area);
    }
}

/*
 * @brief 文件浏览器事件处理函数
 */
static void file_explorer_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *file_explorer = lv_event_get_target(e);
    lv_obj_t *label = lv_event_get_user_data(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        const char *cur_path = lv_file_explorer_get_current_path(file_explorer);
        const char *sel_fn = lv_file_explorer_get_selected_file_name(file_explorer);
        LV_LOG_USER("%s%s", cur_path, sel_fn);
        lv_label_set_text_fmt(label, "Selected file: %s%s", cur_path, sel_fn);
        lv_label_set_text_fmt(g_label_selected_upgrade_file, "%s%s", cur_path, sel_fn);
    }
}


static lv_timer_t *bar_timer = NULL;
/*
 * @brief 固件升级页面升级/退出按钮事件处理函数
 */
static void file_explorer_upgrade_btn_event_handler(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *bar = lv_event_get_user_data(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);

    lv_obj_t *panel = lv_obj_get_parent(btn);
    panel = lv_obj_get_parent(panel);
    panel = lv_obj_get_parent(panel);

    char *label_str = lv_label_get_text(label);

    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    if (strstr(label_str, "Upgrade")) {
        lv_obj_t *setting_page_panel_item;
        lv_obj_t *ta_com_port;
        lv_obj_t *ta_baudrate;
        lv_obj_t *dd_data_bits;
        lv_obj_t *dd_parity;
        lv_obj_t *dd_stop_bits;
        lv_obj_t *ta_ip_addr;
        lv_obj_t *ta_ip_port;
        lv_obj_t *ta_dev_addr;
        lv_obj_t *ta_dev_channel;
        lv_obj_t *dd_upgrade_mode;

        char str_data_bit[8];
        char str_stop_bit[8];
        char str_parity[8];
        char str_upgrade_mode[8];
        UpdateInfo tUpdateInfo;

        setting_page_panel_item = lv_obj_get_user_data(btn);
        setting_page_panel_item = lv_obj_get_child(setting_page_panel_item, 0);

        ta_com_port = lv_obj_get_child(setting_page_panel_item, 1);
        ta_com_port = lv_obj_get_child(ta_com_port, 1);

        ta_baudrate = lv_obj_get_child(setting_page_panel_item, 2);
        ta_baudrate = lv_obj_get_child(ta_baudrate, 1);

        dd_data_bits = lv_obj_get_child(setting_page_panel_item, 3);
        dd_data_bits = lv_obj_get_child(dd_data_bits, 1);
        lv_dropdown_get_selected_str(dd_data_bits, str_data_bit, sizeof(str_data_bit));

        dd_parity = lv_obj_get_child(setting_page_panel_item, 4);
        dd_parity = lv_obj_get_child(dd_parity, 1);
        lv_dropdown_get_selected_str(dd_parity, str_parity, sizeof(str_parity));

        dd_stop_bits = lv_obj_get_child(setting_page_panel_item, 5);
        dd_stop_bits = lv_obj_get_child(dd_stop_bits, 1);
        lv_dropdown_get_selected_str(dd_stop_bits, str_stop_bit, sizeof(str_stop_bit));

        ta_ip_addr = lv_obj_get_child(setting_page_panel_item, 6);
        ta_ip_addr = lv_obj_get_child(ta_ip_addr, 1);

        ta_ip_port = lv_obj_get_child(setting_page_panel_item, 7);
        ta_ip_port = lv_obj_get_child(ta_ip_port, 1);

        ta_dev_addr = lv_obj_get_child(setting_page_panel_item, 8);
        ta_dev_addr = lv_obj_get_child(ta_dev_addr, 1);

        ta_dev_channel = lv_obj_get_child(setting_page_panel_item, 9);
        ta_dev_channel = lv_obj_get_child(ta_dev_channel, 1);

        dd_upgrade_mode = lv_obj_get_child(setting_page_panel_item, 10);
        dd_upgrade_mode = lv_obj_get_child(dd_upgrade_mode, 1);
        lv_dropdown_get_selected_str(dd_upgrade_mode, str_upgrade_mode, sizeof(str_upgrade_mode));

        if (strstr(str_upgrade_mode, "UART")) {
            lv_snprintf(tUpdateInfo.port_info, sizeof(tUpdateInfo.port_info), "%s,%s,%s%s%s",
                        lv_textarea_get_text(ta_com_port),
                        lv_textarea_get_text(ta_baudrate),
                        str_data_bit,
                        str_parity,
                        str_stop_bit);

        }
		else /* "NETWORK" */ {
            printf("Upgrade mode: %s, ip add: %s, ip port:%s\n", str_upgrade_mode, lv_textarea_get_text(ta_ip_addr), lv_textarea_get_text(ta_ip_port));
            lv_snprintf(tUpdateInfo.port_info, sizeof(tUpdateInfo.port_info), "%s:%s",
                        lv_textarea_get_text(ta_ip_addr), lv_textarea_get_text(ta_ip_port));
		}

        lv_snprintf(tUpdateInfo.file, sizeof(tUpdateInfo.file), "%s", lv_label_get_text(g_label_selected_upgrade_file));
        tUpdateInfo.channel = atoi(lv_textarea_get_text(ta_dev_channel));
        tUpdateInfo.dev_addr = atoi(lv_textarea_get_text(ta_dev_addr));

        SetUpdatingStatus(1); /* wei */
#if RPC_ENABLE
        rpc_start_update(g_socket_client_id, &tUpdateInfo);
#endif
        lv_obj_add_state(btn, LV_STATE_DISABLED);
        lv_obj_remove_local_style_prop(bar, LV_STYLE_BG_COLOR, LV_PART_INDICATOR);
        bar_timer = lv_timer_create(update_progress_timer, 10, bar);
    }
    else if (strstr(label_str, "Exit")) {
        if (!isUpdating()) {
            // TODO
            if (bar_timer != NULL)
            {
                lv_timer_delete(bar_timer);
                bar_timer = NULL;
            }

            // btn = lv_obj_get_parent(btn);
            // btn = lv_obj_get_child(btn, 1);
            // lv_obj_clear_state(btn, LV_STATE_DISABLED);
            // SetUpdatingStatus(0); /* wei */
            lv_obj_delete_async(panel);
        } else {
            LV_LOG_USER("Updating in progress, cannot exit!");
        }
    }
}

/*
 * @brief 固件升级进度条定时器回调函数
 */
static void update_progress_timer(lv_timer_t *timer)
{
    lv_obj_t *bar = lv_timer_get_user_data(timer);
    int32_t bar_curren_value = lv_bar_get_value(bar);
    int32_t bar_max_value = lv_bar_get_max_value(bar);

#ifdef RPC_ENABLE
    int32_t update_pecent = rpc_get_update_percent(g_socket_client_id);

    // lv_bar_set_value(bar, (bar_curren_value + lv_rand(0, 10)), LV_ANIM_ON);
    lv_bar_set_value(bar, update_pecent == -1 ? 0 : update_pecent , LV_ANIM_ON);
#else
    int32_t update_pecent = bar_curren_value + lv_rand(0, 2);
    lv_bar_set_value(bar, update_pecent, LV_ANIM_ON);
#endif

    // lv_timer_set_period(timer, lv_rand(0, 400));
    if (update_pecent >= bar_max_value || update_pecent == -1)
    {
        lv_obj_set_style_bg_color(bar, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
        lv_obj_t *btn = lv_obj_get_parent(bar);
        btn = lv_obj_get_parent(btn);
        btn = lv_obj_get_child(btn, 4);
        btn = lv_obj_get_child(btn, 1);

        lv_obj_clear_state(btn, LV_STATE_DISABLED);
        SetUpdatingStatus(0); /* wei */
        lv_timer_delete(timer);
        bar_timer = NULL;
    }
}


/*
 * @brief MQTT设置页面事件处理函数
 */
static void MQTT_setting_event_handler(lv_event_t *e) {

}



/*
 * @brief 文本输入框事件处理函数
 */
static void ta_event_event_handler(lv_event_t *e) {
    lv_obj_t *kb = g_kb;
    lv_obj_t *cont;
    lv_obj_t *btn_send;
    lv_obj_t *cont2_x = NULL;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    lv_timer_t *timer = lv_event_get_user_data(e);
    cont2_x = lv_obj_get_user_data(ta);
    if (cont2_x)
    {
        btn_send = lv_obj_get_child(cont2_x, 2); // cont2_x_2
        if (btn_send)
            btn_send = lv_obj_get_child(btn_send, 4);
        if (btn_send)
            btn_send = lv_obj_get_child(btn_send, 0);
    }

    // if(code == LV_EVENT_FOCUSED) {
    if (code == LV_EVENT_CLICKED)
    {
        if (lv_indev_get_type(lv_indev_active()) != LV_INDEV_TYPE_KEYPAD)
        {
            lv_keyboard_set_textarea(kb, ta);
            // lv_obj_set_style_max_height(kb, LV_HOR_RES * 2 / 3, 0);
            lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
            lv_indev_wait_release(lv_event_get_param(e));
        }
    }
    else if (code == LV_EVENT_DEFOCUSED)
    {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_indev_reset(NULL, ta);
    }
    else if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL)
    {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_indev_reset(NULL, ta); /*To forget the last clicked object to make it focusable again*/
    }

    // 在文本输入框内容改变后，更新定时器周期或者发送RPC更新事件
    if ((code == LV_EVENT_DEFOCUSED) || (code == LV_EVENT_READY || code == LV_EVENT_CANCEL))
    {

        if (timer)
        {
            const char *ta_text = lv_textarea_get_text(ta);
            lv_timer_set_period(timer, atoi(ta_text));
        }
        if (cont2_x)
            lv_obj_send_event(btn_send, MY_LV_EVENT_UPDATE_RPC, NULL);
    }
}


/*
 * @brief send按钮事件处理函数
 */
static void btn_send_event_handler(lv_event_t *e)
{
#if 1
    char str_tmp[64];
    int val_read;
    int val_write;
    int point;
    lv_obj_t *label;
    lv_obj_t *dd;
    lv_obj_t *chart;
    lv_obj_t *ta_read;
    lv_obj_t *ta_write;

    lv_obj_t *cont1;
    lv_obj_t *cont2;

    lv_event_code_t code = lv_event_get_code(e);

    cont1 = lv_event_get_user_data(e);   // cont2_x
    cont2 = lv_obj_get_user_data(cont1); // com_or_ip_conf_page

    /* 按钮被点击时会进入处理
     * 或者如果勾选了周期发送框，那么会通过定时器周期给按钮发送 MY_LV_EVENT_READ_PERIOD 事件
     */
    if (code == LV_EVENT_CLICKED || code == MY_LV_EVENT_READ_PERIOD)
    {
        // chart = lv_obj_get_child(cont1, 3);
        label = lv_obj_get_child(cont1, 0);
        point = atoi(lv_label_get_text(label));

        dd = lv_obj_get_child(cont1, 2); // fuction code dropdown
        dd = lv_obj_get_child(dd, 2);
        dd = lv_obj_get_child(dd, 0);

        ta_write = lv_obj_get_child(cont1, 2);
        ta_write = lv_obj_get_child(ta_write, 5);
        ta_write = lv_obj_get_child(ta_write, 0);
        ta_write = lv_obj_get_child(ta_write, 1);
        val_write = atoi(lv_textarea_get_text(ta_write));

        ta_read = lv_obj_get_child(cont1, 2);
        ta_read = lv_obj_get_child(ta_read, 5);
        ta_read = lv_obj_get_child(ta_read, 1);
        ta_read = lv_obj_get_child(ta_read, 1);

#ifdef RPC_ENABLE
        lv_dropdown_get_selected_str(dd, str_tmp, sizeof(str_tmp));
        if (!(strstr(str_tmp, "1x")) || !(strstr(str_tmp, "3x")))
        {
            //printf("rpc_write_point: %d\n", val_write);
            if (code == LV_EVENT_CLICKED)
                rpc_write_point(g_socket_client_id, point, val_write);
        }
        rpc_read_point(g_socket_client_id, point, &val_read);
#else 
        // 模拟读数据,随机值
        val_read = rand() % 100;
#endif
        memset(str_tmp, 0, sizeof(str_tmp));
        lv_snprintf(str_tmp, sizeof(str_tmp), "%d", val_read);
        lv_textarea_set_text(ta_read, str_tmp);
        // lv_chart_set_next_value(chart, lv_chart_get_series_next(chart, NULL), val_read);
    }
    // 一些配置修改会给按钮发送 MY_LV_EVENT_UPDATE_RPC 事件
    else if (code == MY_LV_EVENT_UPDATE_RPC)
    {
        update_rpc_data(cont1, cont2);
    }
#endif
}

#if 1
/*
 * @brief 更新RPC配置信息函数
 */
static void update_rpc_data(lv_obj_t *cont1, lv_obj_t *cont2)
{
    uint16_t port_type = 0; // 0:RTU, 1:TCP

    lv_obj_t *label;
    lv_obj_t *cont2_x_2;
    lv_obj_t *cont2_x_2_x;
    lv_obj_t *cont2_x_2_x_ta;
    lv_obj_t *cont_com_or_ip_conf_page;

    lv_obj_t *cb_rtu_or_tcp;
    lv_obj_t *ta_send_period;

    lv_obj_t *ta_dev_addr;
    lv_obj_t *ta_reg_addr;
    lv_obj_t *dd_reg_type; // functon_code

    lv_obj_t *ta_com_port;
    lv_obj_t *ta_baudrate;
    lv_obj_t *dd_data_bit;
    lv_obj_t *dd_stop_bit;
    lv_obj_t *dd_parity;
    lv_obj_t *ta_remote_ip;
    lv_obj_t *ta_remote_port;
    lv_obj_t *ta_dev_channel;

    int rpc_fd;

    char tmp_str[64];
    char port_info[128];
    char str_data_bit[8];
    char str_stop_bit[8];
    char str_parity[8];
    int dev_addr;
    int reg_addr;
    char reg_type[8];
    int period;
    int channel = 0;

    ////////////////////////////////////////////
    label = lv_obj_get_child(cont1, 0);
    rpc_fd = atoi(lv_label_get_text(label));

    cont2_x_2 = lv_obj_get_child(cont1, 2);

    cont2_x_2_x = lv_obj_get_child(cont2_x_2, 0);
    cont2_x_2_x_ta = lv_obj_get_child(cont2_x_2_x, 0);
    cb_rtu_or_tcp = lv_obj_get_child(cont2_x_2_x_ta, 1);
    lv_memzero(tmp_str, sizeof(tmp_str));
    lv_dropdown_get_selected_str(cb_rtu_or_tcp, tmp_str, sizeof(tmp_str));

    if (strcmp(tmp_str, "RTU")) {
        port_type = 0;
    } else if (strcmp(tmp_str, "TCP")) {
        port_type = 1;
    }

    cont2_x_2_x = lv_obj_get_child(cont2_x_2, 3);
    ta_send_period = lv_obj_get_child(cont2_x_2_x, 1);
    period = atoi(lv_textarea_get_text(ta_send_period)),

    cont2_x_2_x = lv_obj_get_child(cont2_x_2, 1);
    cont2_x_2_x_ta = lv_obj_get_child(cont2_x_2_x, 0);
    ta_dev_addr = lv_obj_get_child(cont2_x_2_x_ta, 1);
    dev_addr = atoi(lv_textarea_get_text(ta_dev_addr)),

    cont2_x_2_x_ta = lv_obj_get_child(cont2_x_2_x, 1);
    ta_reg_addr = lv_obj_get_child(cont2_x_2_x_ta, 1);
    reg_addr = atoi(lv_textarea_get_text(ta_reg_addr)),

    cont2_x_2_x = lv_obj_get_child(cont2_x_2, 2);
    cont2_x_2_x_ta = lv_obj_get_child(cont2_x_2_x, 0);
    dd_reg_type = lv_obj_get_child(cont2_x_2_x_ta, 1);
    lv_memzero(tmp_str, sizeof(tmp_str));
    lv_dropdown_get_selected_str(dd_reg_type, tmp_str, sizeof(tmp_str));
    if (strstr(tmp_str, "0x"))
        lv_snprintf(reg_type, sizeof(reg_type), "0x");
    else if (strstr(tmp_str, "1x"))
        lv_snprintf(reg_type, sizeof(reg_type), "1x");
    else if (strstr(tmp_str, "3x"))
        lv_snprintf(reg_type, sizeof(reg_type), "3x");
    else if (strstr(tmp_str, "4x"))
        lv_snprintf(reg_type, sizeof(reg_type), "4x");

    ////////////////////////////////////////////////
    cont_com_or_ip_conf_page = lv_obj_get_child(cont2, 0);

    ta_com_port = lv_obj_get_child(cont_com_or_ip_conf_page, 1);
    ta_com_port = lv_obj_get_child(ta_com_port, 1);

    ta_baudrate = lv_obj_get_child(cont_com_or_ip_conf_page, 2);
    ta_baudrate = lv_obj_get_child(ta_baudrate, 1);

    dd_data_bit = lv_obj_get_child(cont_com_or_ip_conf_page, 3);
    dd_data_bit = lv_obj_get_child(dd_data_bit, 1);
    lv_dropdown_get_selected_str(dd_data_bit, str_data_bit, sizeof(str_data_bit));

    dd_stop_bit = lv_obj_get_child(cont_com_or_ip_conf_page, 4);
    dd_stop_bit = lv_obj_get_child(dd_stop_bit, 1);
    lv_dropdown_get_selected_str(dd_stop_bit, str_stop_bit, sizeof(str_stop_bit));

    dd_parity = lv_obj_get_child(cont_com_or_ip_conf_page, 5);
    dd_parity = lv_obj_get_child(dd_parity, 1);
    lv_dropdown_get_selected_str(dd_parity, str_parity, sizeof(str_parity));

    ta_remote_ip = lv_obj_get_child(cont_com_or_ip_conf_page, 6);
    ta_remote_ip = lv_obj_get_child(ta_remote_ip, 1);

    ta_remote_port = lv_obj_get_child(cont_com_or_ip_conf_page, 7);
    ta_remote_port = lv_obj_get_child(ta_remote_port, 1);

    ta_dev_channel = lv_obj_get_child(cont_com_or_ip_conf_page, 8);
    ta_dev_channel = lv_obj_get_child(ta_dev_channel, 1);

    if (port_type == 1)
        lv_snprintf(port_info, sizeof(port_info), "%s:%s",
                    lv_textarea_get_text(ta_remote_ip),
                    lv_textarea_get_text(ta_remote_port));
    else
        lv_snprintf(port_info, sizeof(port_info), "%s,%s,%s%s%s",
                    lv_textarea_get_text(ta_com_port),
                    lv_textarea_get_text(ta_baudrate),
                    str_data_bit,
                    str_parity,
                    str_stop_bit);

    channel = atoi(lv_textarea_get_text(ta_dev_channel));

#ifdef RPC_ENABLE
    // LV_LOG_USER("point: %d, port_info: %s", rpc_fd, port_info);
    rpc_modify_point(g_socket_client_id, rpc_fd, port_info, channel, dev_addr, reg_addr, reg_type, period);
#endif
}
#endif
