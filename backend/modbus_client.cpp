#include "rpc.h"
#include "cfg.h"
#include "modbus_client.h"
//#include "update.h"
#include "system.h"
#include <libmodbus/modbus.h>

#include <iostream>


typedef struct modbus_info {
    char info[200];
    modbus_t* ctx;
    system_util::Mutex m_mutex;
}modbus_info, * pmodbus_info;

static modbus_info g_atModbusInfos[MAX_MODBUS];
static HostPointMap g_HostPointMaps[MAX_MODBUS];

static modbus_t* get_modbus_ctx(char* port_info);
static void put_modbus_ctx(modbus_t* ctx);
static modbus_t* create_modbus_rtu_ctx(char* port_info);
static modbus_t* create_modbus_tcp_ctx(char* port_info);
static PPointMap get_point_map(PPointInfo ptPointInfo);

// TODO: 修改为只有一个中控（未来扩展为多个中控）
// 创建点映射表
void create_point_maps(void) {
    PHostPointMap ptHostPointMap = NULL;
    PPointInfo Points;
    int reg_addr_master = 0;
    int map_index = 0, point_index = 0;
    int i = 0;

    memset(g_HostPointMaps, 0, sizeof(g_HostPointMaps));

    Points = local_get_points();

    while (Points[i].dev_addr != 0) {
        // 查找同端口映射表，若无则创建
        for (map_index = 0; map_index < MAX_MODBUS; map_index++) {
            // 找到相同端口
            if (strcmp(g_HostPointMaps[map_index].port_info, Points[i].port_info) == 0) {
                ptHostPointMap = &g_HostPointMaps[map_index];
                break;
            }
            // 找到空位置（创建新的）
            else if (g_HostPointMaps[map_index].port_info[0] == '\0') {
                strncpy_s(g_HostPointMaps[map_index].port_info,
                    Points[i].port_info,
                    sizeof(g_HostPointMaps[map_index].port_info) - 1);

                // 初始化点映射数组
                memset(g_HostPointMaps[map_index].tPointMaps, 0,
                    sizeof(PointMap) * MAX_POINT_COUNT);

                ptHostPointMap = &g_HostPointMaps[map_index];
                break;
            }
        }

        for (point_index = 0;
            point_index < MAX_POINT_COUNT && ptHostPointMap->tPointMaps[point_index].reg_type[0];
            point_index++) {
            if (strcmp(ptHostPointMap->tPointMaps[point_index].reg_type, Points[i].reg_type) == 0) {
                if (reg_addr_master <= ptHostPointMap->tPointMaps[point_index].reg_addr_master) {
                    reg_addr_master = ptHostPointMap->tPointMaps[point_index].reg_addr_master + 1;
                }
            }
        }

        // 4. 找到空闲位置（point_index 现在指向第一个空位）
        if (point_index >= MAX_POINT_COUNT) {
            printf("警告：端口 %s 的点数已满\n", Points[i].port_info);
            i++;
            continue;
        }

        // 5. 添加点映射
        strncpy_s(ptHostPointMap->tPointMaps[point_index].reg_type,
            Points[i].reg_type,
            sizeof(ptHostPointMap->tPointMaps[point_index].reg_type) - 1);
        ptHostPointMap->tPointMaps[point_index].reg_addr_master = reg_addr_master;
        ptHostPointMap->tPointMaps[point_index].channel = Points[i].channel;
        ptHostPointMap->tPointMaps[point_index].dev_addr = Points[i].dev_addr;
        ptHostPointMap->tPointMaps[point_index].reg_addr_salve = Points[i].reg_addr;

        i++;
    }
}


/* 获得"点"创建映射关系
 * ptPointInfo-用户设置的"点"的信息
 * 返回值: 返回一个PPointMap,它指向"点"的映射信息
 */
static PPointMap get_point_map(PPointInfo ptPointInfo) {
    PHostPointMap ptHostPointMap;
    int i, j;

    for (i = 0; i < MAX_MODBUS; i++)
    {
        if (!g_HostPointMaps[i].port_info[0])
            break;

        ptHostPointMap = &g_HostPointMaps[i];

        /* 遍历映射表 */
        for (j = 0; j < MAX_POINT_COUNT && ptHostPointMap->tPointMaps[j].reg_type[0]; j++)
        {
            if (!strcmp(ptHostPointMap->tPointMaps[j].reg_type, ptPointInfo->reg_type) && \
                (ptHostPointMap->tPointMaps[j].channel == ptPointInfo->channel) && \
                (ptHostPointMap->tPointMaps[j].dev_addr == ptPointInfo->dev_addr) && \
                (ptHostPointMap->tPointMaps[j].reg_addr_salve == ptPointInfo->reg_addr))
            {
                return &ptHostPointMap->tPointMaps[j];
            }
        }

    }

    return NULL;
}


// TODO: 添加多中控通道处理
/* 把"点"的映射表发送给中控
 * 返回值: 0-成功,(-1)-失败
 */
int modbus_write_point_maps(void) {
    PHostPointMap ptHostPointMap;
    int file_size = 0;
    modbus_t* ctx = NULL;
    int rc = 0;
    int i, j;
    int cnt = 0;

    PPointMap pNewMap;

    for (i = 0; i < MAX_MODBUS; i++)
    {
        if (!g_HostPointMaps[i].port_info[0])
            break;

        ptHostPointMap = &g_HostPointMaps[i];

        /* 计算映射表大小 */
        for (j = 0; j < MAX_POINT_COUNT && ptHostPointMap->tPointMaps[j].reg_type[0]; j++);

        file_size += j * sizeof(PointMap);
    }

    if (file_size == 0)
        return 0;

    /* 分配一个总的映射表 */
    pNewMap = (PPointMap)malloc(file_size);

    for (i = 0; i < MAX_MODBUS; i++)
    {
        if (!g_HostPointMaps[i].port_info[0])
            break;

        ptHostPointMap = &g_HostPointMaps[i];

        /* 构造总的映射表 */
        for (j = 0; j < MAX_POINT_COUNT && ptHostPointMap->tPointMaps[j].reg_type[0]; j++)
        {
            pNewMap[cnt++] = ptHostPointMap->tPointMaps[j];
        }
    }

    /* 为了保险,使用每一个ctx发送映射表   */
    for (i = 0; i < MAX_MODBUS; i++)
    {
        if (!g_HostPointMaps[i].port_info[0])
            break;

        ptHostPointMap = &g_HostPointMaps[i];

        /* 得到modbus上下文 */
        ctx = get_modbus_ctx(ptHostPointMap->port_info);
        if (!ctx)
            continue;

        /* 中控地址永远是1 */
        modbus_set_slave(ctx, 1);
        rc = modbus_write_file(ctx, 0, (uint8_t*)"reg_map", (uint8_t*)pNewMap, file_size);
        put_modbus_ctx(ctx);
    }

    free(pNewMap);

    if (rc == 1)
        return 0;
    else
        return -1;
}


/*
* 获取modbus上下文
*/
static modbus_t* get_modbus_ctx(char* port_info) {
    int i;
    modbus_t* ctx = NULL;

    // 有则返回，无则创建
    for (i = 0; i < MAX_MODBUS; i++)
    {
        if (!strcmp(port_info, g_atModbusInfos[i].info) && g_atModbusInfos[i].ctx)
        {
            g_atModbusInfos[i].m_mutex.Lock();
            return g_atModbusInfos[i].ctx;
        }

        if (!g_atModbusInfos[i].ctx)
            break;
    }

    // 在空g_atModbusInfos中创建modbus上下文
    if (!strncmp(port_info, "/dev", 4) || strstr(port_info, "com") || strstr(port_info, "COM"))
        ctx = create_modbus_rtu_ctx(port_info);
    else
        ctx = create_modbus_tcp_ctx(port_info);

    if (ctx)
    {
        strncpy_s(g_atModbusInfos[i].info, port_info, sizeof(g_atModbusInfos[i].info) - 1);
        g_atModbusInfos[i].m_mutex.Lock();
        g_atModbusInfos[i].ctx = ctx;
    }

    return ctx;
}

/**
* 释放modbus上下文
*/
static void put_modbus_ctx(modbus_t* ctx)
{
    for (int i = 0; i < MAX_MODBUS; i++) {
        if (ctx == g_atModbusInfos[i].ctx) {
            g_atModbusInfos[i].m_mutex.Unlock();
            return;
        }
    }
}

/**
* 创建modbus上下文
*/
static modbus_t* create_modbus_rtu_ctx(char* port_info)
{
    char tmp[200];
    char dev[50];
    int baud;
    char parity;
    int data;
    int stop;
    modbus_t* ctx = NULL;

    char* str0;
    char* str;


    /* /dev/ttyUSB0,115200,8n1 */
    strncpy_s(tmp, port_info, sizeof(tmp) - 1);

    str = strstr(tmp, ",");
    if (str)
    {
        *str = '\0';
        if (strstr(tmp, "dev")) /* linux */
            strncpy_s(dev, tmp, sizeof(dev) - 1);
        else /* windows */
        {
            /* \\\\.\\COM1 */
            snprintf(dev, 50, "\\\\.\\%s", tmp);
            printf("dev name: %s\r\n", dev);
        }
    }
    else
    {
        return NULL;
    }

    str0 = str + 1;
    str = strstr(str0, ",");
    if (str)
    {
        *str = '\0';
        baud = (int)strtoul(str0, NULL, 0);
    }

    str0 = str + 1;
    data = *str0 - '0';

    str0++;
    if (*str0 == 'n' || *str0 == 'N')
        parity = 'N';
    if (*str0 == 'e' || *str0 == 'E')
        parity = 'E';
    if (*str0 == 'o' || *str0 == 'O')
        parity = 'O';

    str0++;
    stop = *str0 - '0';

    // 创建modbus上下文
    ctx = modbus_new_rtu(dev, baud, parity, data, stop);
    if (ctx == NULL) {
        fprintf(stderr, "Unable to allocate libmodbus context\n");
        return NULL;
    }

    // 连接modbus
    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return NULL;
    }

    return ctx;
}


static modbus_t* create_modbus_tcp_ctx(char* port_info)
{
    modbus_t* ctx = NULL;
    char tmp[200];

    char* str;

    /* 192.168.1.123:1502 */
    strncpy_s(tmp, port_info, sizeof(tmp) - 1);
    str = strstr(tmp, ":");
    if (str)
    {
        *str = '\0';
        str++;
        int port = (int)strtoul(str, NULL, 0);

        ctx = modbus_new_tcp(tmp, port);
        if (ctx == NULL) {
            fprintf(stderr, "Unable to allocate libmodbus context\n");
            return NULL;
        }


        if (modbus_connect(ctx) == -1) {
            fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
            modbus_free(ctx);
            return NULL;
        }

        printf("******* create modbus ctx ok for %s\n", port_info);
        return ctx;
    }
    return NULL;
}


int modbus_clear_status(int cmd_status_point) {
    int err;
    int val;
    for (int i = 0; i < 5; i++)
    {
        err = modbus_write_point(cmd_status_point, 0xbb);
        err = modbus_read_point(cmd_status_point, &val);

        if (val != 0xbb)
            return 0;
    }

    return -1;
}

int modbus_write_point(int point, int val) {
    PPointInfo ptInfo = local_get_points();
    ptInfo = &ptInfo[point];
    int rc;

    modbus_t* ctx = NULL;

    if (ptInfo) {
        ctx = get_modbus_ctx(ptInfo->port_info);
        if (!ctx) {
            printf("******* can not get modbus ctx for %s\n", ptInfo->port_info);
            return -1;
        }

        /* 中控地址永远是1 */
        modbus_set_slave(ctx, 1);

        PPointMap ptPointMap = get_point_map(ptInfo);
        if (!ptPointMap) {
            put_modbus_ctx(ctx);
            return -1;
        }

        // 写bit
        if (!strcmp(ptInfo->reg_type, "0x")) {
            printf("******* modbus_write_bit for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s, val = %d\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type, val);
            rc = modbus_write_bit(ctx, ptPointMap->reg_addr_master, val);
            put_modbus_ctx(ctx);
            if (rc == 1)
                return 0;
            else
            {
                printf("******* modbus_write_bit err for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s, val = %d\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type, val);
                return -1;
            }
        }
        // 写coil
        else if (!strcmp(ptInfo->reg_type, "4x")) {
            printf("******* modbus_write_register for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s, val = %d\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type, val);
            rc = modbus_write_register(ctx, ptPointMap->reg_addr_master, val);
            put_modbus_ctx(ctx);
            if (rc == 1)
                return 0;
            else {
                put_modbus_ctx(ctx);
                printf("******* modbus_write_register err for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s, val = %d\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type, val);
                return -1;
            }
        }
        else {
            //printf("******* can not write for %s\n", ptInfo->reg_type);
            put_modbus_ctx(ctx);
            return -1;
        }

    }
    else {
        put_modbus_ctx(ctx);
        printf("******* can not get point for %d\n", point);
        return -1;
    }

}


int modbus_read_point(int point, int* pVal) {
    PPointInfo ptInfo = local_get_points();
    ptInfo = &ptInfo[point];
    int rc;

    modbus_t* ctx = NULL;

    if (ptInfo) {
        ctx = get_modbus_ctx(ptInfo->port_info);
        if (!ctx) {
            printf("******* can not get modbus ctx for %s\n", ptInfo->port_info);
            return -1;
        }

        *pVal = 0;
        /* 中控地址永远是1 */
        modbus_set_slave(ctx, 1);

        PPointMap ptPointMap = get_point_map(ptInfo);
        if (!ptPointMap) {
            printf("get_point_map err for %s, dev_addr = %d, reg_addr = %d, reg_ype = %s\n", ptInfo->port_info, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type);
            put_modbus_ctx(ctx);
            return -1;
        }

        if (!strcmp(ptInfo->reg_type, "0x")) {
            printf("******* modbus_read_bits for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type);
            rc = modbus_read_bits(ctx, ptPointMap->reg_addr_master, 1, (uint8_t*)pVal);
            put_modbus_ctx(ctx);
            if (rc == 1)
                return 0;
            else
            {
                printf("******* modbus_read_bits err for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type);
                return -1;
            }
        }
        else if (!strcmp(ptInfo->reg_type, "1x")) {
            printf("******* modbus_read_input_bits for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type);
            rc = modbus_read_input_bits(ctx, ptPointMap->reg_addr_master, 1, (uint8_t*)pVal);
            put_modbus_ctx(ctx);
            if (rc == 1)
                return 0;
            else
            {
                printf("******* modbus_read_input_bits err for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type);
                return -1;
            }
        }
        else if (!strcmp(ptInfo->reg_type, "4x")) {
            printf("******* modbus_read_registers for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type);
            rc = modbus_read_registers(ctx, ptPointMap->reg_addr_master, 1, (uint16_t*)pVal);
            put_modbus_ctx(ctx);
            if (rc == 1)
                return 0;
            else
            {
                printf("******* modbus_read_registers err for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type);
                return -1;
            }
        }
        else if (!strcmp(ptInfo->reg_type, "3x")) {
            printf("******* modbus_read_input_registers for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type);
            rc = modbus_read_input_registers(ctx, ptPointMap->reg_addr_master, 1, (uint16_t*)pVal);
            put_modbus_ctx(ctx);
            if (rc == 1)
                return 0;
            else
            {
                printf("******* modbus_read_input_registers err for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type);
                return -1;
            }
        }
        else {
            put_modbus_ctx(ctx);
            printf("******* modbus_read_point err for %s, not supported reg_type %s\n", ptInfo->port_info, ptInfo->reg_type);
            return -1;
        }
    }

    put_modbus_ctx(ctx);
    return -1;
}

int modbus_update(char* file, char* port_info, int channel, int dev_addr, void(*set_percent_func)(int)) {}

