#include "rpc.h"
#include "cfg.h"
#include "modbus_client.h"
#include "update.h"
#include "system.h"
#include <libmodbus/modbus.h>

#include <iostream>

typedef struct modbus_info
{
    char info[200];
    modbus_t *ctx;
    system_util::Mutex m_mutex;
} modbus_info, *pmodbus_info;

static modbus_info g_atModbusInfos[MAX_MODBUS];
static HostPointMap g_HostPointMaps[MAX_MODBUS];

static modbus_t *get_modbus_ctx(char *port_info);
static void put_modbus_ctx(modbus_t *ctx);
static modbus_t *create_modbus_rtu_ctx(char *port_info);
static modbus_t *create_modbus_tcp_ctx(char *port_info);
static PPointMap get_point_map(PPointInfo ptPointInfo);
static int modbus_wait_for_bootup(int cmd_status_point);
static int modbus_wait_for_idle(int cmd_status_point);

// TODO: 修改为只有一个中控（未来扩展为多个中控）
// 创建点映射表
void create_point_maps(void)
{
    PHostPointMap ptHostPointMap = NULL;
    PPointInfo Points;
    int reg_addr_master = 0;
    int map_index = 0, point_index = 0;
    int i = 0;

    memset(g_HostPointMaps, 0, sizeof(g_HostPointMaps));

    Points = local_get_points();

    while (Points[i].dev_addr != 0)
    {
        // 查找同端口映射表，若无则创建
        for (map_index = 0; map_index < MAX_MODBUS; map_index++)
        {
            // 找到相同端口
            if (strcmp(g_HostPointMaps[map_index].port_info, Points[i].port_info) == 0)
            {
                ptHostPointMap = &g_HostPointMaps[map_index];
                break;
            }
            // 找到空位置（创建新的）
            else if (g_HostPointMaps[map_index].port_info[0] == '\0')
            {
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

        // 找到已经映射的相同reg_type的最大reg_addr_master
        for (point_index = 0;
             point_index < MAX_POINT_COUNT && ptHostPointMap->tPointMaps[point_index].reg_type[0];
             point_index++)
        {
            if (strcmp(ptHostPointMap->tPointMaps[point_index].reg_type, Points[i].reg_type) == 0)
            {
                if (reg_addr_master <= ptHostPointMap->tPointMaps[point_index].reg_addr_master)
                {
                    reg_addr_master = ptHostPointMap->tPointMaps[point_index].reg_addr_master + 1;
                }
            }
        }

        // 找到空闲位置（point_index 现在指向第一个空位）
        if (point_index >= MAX_POINT_COUNT)
        {
            printf("警告：端口 %s 的点数已满\n", Points[i].port_info);
            i++;
            continue;
        }

        // 添加点映射
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
static PPointMap get_point_map(PPointInfo ptPointInfo)
{
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
            if (!strcmp(ptHostPointMap->tPointMaps[j].reg_type, ptPointInfo->reg_type) &&
                (ptHostPointMap->tPointMaps[j].channel == ptPointInfo->channel) &&
                (ptHostPointMap->tPointMaps[j].dev_addr == ptPointInfo->dev_addr) &&
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
int modbus_write_point_maps(void)
{
    PHostPointMap ptHostPointMap;
    int file_size = 0;
    modbus_t *ctx = NULL;
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
        for (j = 0; j < MAX_POINT_COUNT && ptHostPointMap->tPointMaps[j].reg_type[0]; j++)
            ;

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
        rc = modbus_write_file(ctx, 0, (uint8_t *)"reg_map", (uint8_t *)pNewMap, file_size);
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
static modbus_t *get_modbus_ctx(char *port_info)
{
    int i;
    modbus_t *ctx = NULL;

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
static void put_modbus_ctx(modbus_t *ctx)
{
    for (int i = 0; i < MAX_MODBUS; i++)
    {
        if (ctx == g_atModbusInfos[i].ctx)
        {
            g_atModbusInfos[i].m_mutex.Unlock();
            return;
        }
    }
}

/**
 * 创建modbus上下文
 */
static modbus_t *create_modbus_rtu_ctx(char *port_info)
{
    char tmp[200];
    char dev[50];
    int baud;
    char parity;
    int data;
    int stop;
    modbus_t *ctx = NULL;

    char *str0;
    char *str;

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
    if (ctx == NULL)
    {
        fprintf(stderr, "Unable to allocate libmodbus context\n");
        return NULL;
    }

    // 连接modbus
    if (modbus_connect(ctx) == -1)
    {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return NULL;
    }

    return ctx;
}

static modbus_t *create_modbus_tcp_ctx(char *port_info)
{
    modbus_t *ctx = NULL;
    char tmp[200];

    char *str;

    /* 192.168.1.123:1502 */
    strncpy_s(tmp, port_info, sizeof(tmp) - 1);
    str = strstr(tmp, ":");
    if (str)
    {
        *str = '\0';
        str++;
        int port = (int)strtoul(str, NULL, 0);

        ctx = modbus_new_tcp(tmp, port);
        if (ctx == NULL)
        {
            fprintf(stderr, "Unable to allocate libmodbus context\n");
            return NULL;
        }

        if (modbus_connect(ctx) == -1)
        {
            fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
            modbus_free(ctx);
            return NULL;
        }

        printf("******* create modbus ctx ok for %s\n", port_info);
        return ctx;
    }
    return NULL;
}

int modbus_clear_status(int cmd_status_point)
{
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

int modbus_write_point(int point, int val)
{
    PPointInfo ptInfo = local_get_points();
    ptInfo = &ptInfo[point];
    int rc;

    modbus_t *ctx = NULL;

    if (ptInfo)
    {
        ctx = get_modbus_ctx(ptInfo->port_info);
        if (!ctx)
        {
            printf("******* can not get modbus ctx for %s\n", ptInfo->port_info);
            return -1;
        }

        /* 中控地址永远是1 */
        modbus_set_slave(ctx, 1);

        PPointMap ptPointMap = get_point_map(ptInfo);
        if (!ptPointMap)
        {
            put_modbus_ctx(ctx);
            return -1;
        }

        // 写bit
        if (!strcmp(ptInfo->reg_type, "0x"))
        {
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
        else if (!strcmp(ptInfo->reg_type, "4x"))
        {
            printf("******* modbus_write_register for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s, val = %d\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type, val);
            rc = modbus_write_register(ctx, ptPointMap->reg_addr_master, val);
            put_modbus_ctx(ctx);
            if (rc == 1)
                return 0;
            else
            {
                put_modbus_ctx(ctx);
                printf("******* modbus_write_register err for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s, val = %d\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type, val);
                return -1;
            }
        }
        else
        {
            // printf("******* can not write for %s\n", ptInfo->reg_type);
            put_modbus_ctx(ctx);
            return -1;
        }
    }
    else
    {
        put_modbus_ctx(ctx);
        printf("******* can not get point for %d\n", point);
        return -1;
    }
}

int modbus_read_point(int point, int *pVal)
{
    PPointInfo ptInfo = local_get_points();
    ptInfo = &ptInfo[point];
    int rc;

    modbus_t *ctx = NULL;

    if (ptInfo)
    {
        ctx = get_modbus_ctx(ptInfo->port_info);
        if (!ctx)
        {
            printf("******* can not get modbus ctx for %s\n", ptInfo->port_info);
            return -1;
        }

        *pVal = 0;
        /* 中控地址永远是1 */
        modbus_set_slave(ctx, 1);

        PPointMap ptPointMap = get_point_map(ptInfo);
        if (!ptPointMap)
        {
            printf("get_point_map err for %s, dev_addr = %d, reg_addr = %d, reg_ype = %s\n", ptInfo->port_info, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type);
            put_modbus_ctx(ctx);
            return -1;
        }

        if (!strcmp(ptInfo->reg_type, "0x"))
        {
            printf("******* modbus_read_bits for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type);
            rc = modbus_read_bits(ctx, ptPointMap->reg_addr_master, 1, (uint8_t *)pVal);
            put_modbus_ctx(ctx);
            if (rc == 1)
                return 0;
            else
            {
                printf("******* modbus_read_bits err for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type);
                return -1;
            }
        }
        else if (!strcmp(ptInfo->reg_type, "1x"))
        {
            printf("******* modbus_read_input_bits for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type);
            rc = modbus_read_input_bits(ctx, ptPointMap->reg_addr_master, 1, (uint8_t *)pVal);
            put_modbus_ctx(ctx);
            if (rc == 1)
                return 0;
            else
            {
                printf("******* modbus_read_input_bits err for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type);
                return -1;
            }
        }
        else if (!strcmp(ptInfo->reg_type, "4x"))
        {
            printf("******* modbus_read_registers for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type);
            rc = modbus_read_registers(ctx, ptPointMap->reg_addr_master, 1, (uint16_t *)pVal);
            put_modbus_ctx(ctx);
            if (rc == 1)
                return 0;
            else
            {
                printf("******* modbus_read_registers err for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type);
                return -1;
            }
        }
        else if (!strcmp(ptInfo->reg_type, "3x"))
        {
            printf("******* modbus_read_input_registers for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type);
            rc = modbus_read_input_registers(ctx, ptPointMap->reg_addr_master, 1, (uint16_t *)pVal);
            put_modbus_ctx(ctx);
            if (rc == 1)
                return 0;
            else
            {
                printf("******* modbus_read_input_registers err for %s, channel = %d, dev_addr = %d, reg_addr = %d, reg_type = %s\n", ptInfo->port_info, ptInfo->channel, ptInfo->dev_addr, ptInfo->reg_addr, ptInfo->reg_type);
                return -1;
            }
        }
        else
        {
            put_modbus_ctx(ctx);
            printf("******* modbus_read_point err for %s, not supported reg_type %s\n", ptInfo->port_info, ptInfo->reg_type);
            return -1;
        }
    }

    put_modbus_ctx(ctx);
    return -1;
}

int modbus_update(char *file, char *port_info, int channel, int dev_addr, void (*set_percent_func)(int))
{
    int cmd_status_point;
    PointInfo tCmdStatusPointInfo;
    uint8_t *file_buf;
    int file_len;
    int err;

    FileInfo tFileInfo;
    int rc;
    int file_no = 0;
    uint16_t record_no = 0;
    int pos = 0;
    int send_len = 0;

    int percent;

    modbus_t *ctx;

    /* 读取文件 */
    printf("******* read firmware ...\n");
    file_buf = read_file(file, &file_len);
    if (!file_buf)
    {
        printf("******* read_file for update err\n");
        return -1;
    }

    /* 映射升级时用到的1个寄存器
     * AO[MODBUS_UPDATE_REG_ADDR] : 写入命令, 读出状态
     *  写: 1-升级, 2-启动
     *  读: 0-传感器无法接收文件, 1-传感器可以接收文件, 2-文件接收完毕
     */
    {
        if (port_info[0] == 'C')
            port_info[0] = 'c';
        if (port_info[1] == 'O')
            port_info[1] = 'o';
        if (port_info[2] == 'M')
            port_info[2] = 'm';

        printf("******* map cmd_status register for port %s, channel %d, dev_addr %d, reg %d ...\n", port_info, channel, dev_addr, MODBUS_UPDATE_REG_ADDR);
        memset(&tCmdStatusPointInfo, 0, sizeof(tCmdStatusPointInfo));

        strncpy_s(tCmdStatusPointInfo.port_info, port_info, sizeof(tCmdStatusPointInfo.port_info) - 1);
        tCmdStatusPointInfo.channel = channel;
        tCmdStatusPointInfo.dev_addr = dev_addr;
        tCmdStatusPointInfo.reg_addr = MODBUS_UPDATE_REG_ADDR;
        strncpy_s(tCmdStatusPointInfo.reg_type, "4x", sizeof(tCmdStatusPointInfo.reg_type) - 1);

        cmd_status_point = local_add_point(&tCmdStatusPointInfo);
        if (cmd_status_point < 0)
        {
            printf("******* local_add_point for cmd_status reg err\n");
            free(file_buf);
            if (set_percent_func)
                set_percent_func(-1);
            return -1;
        }

        create_point_maps();
        err = modbus_write_point_maps();
        if (err)
        {
            printf("******* modbus_write_point_maps for cmd_status reg err\n");
            free(file_buf);
            if (set_percent_func)
                set_percent_func(-1);
            return -1;
        }
    }

    /* 发送升级命令:这会导致目标板重启 */
    printf("******* send update command to enter bootloader ...\n");
    err = modbus_write_point(cmd_status_point, MODBUS_PRIVATE_CMD_UPDATE);
    if (err)
    {
        printf("******* modbus_write_point for cmd_status reg to reboot target err\n");
        free(file_buf);

        local_remove_point(cmd_status_point);
        create_point_maps();
        err = modbus_write_point_maps();

        if (set_percent_func)
            set_percent_func(-1);
        return -1;
    }
    /* 等待目标板重启进入bootloader */
    Sleep(2000);
    printf("******* wait target to enter bootloader ...\n");
    if (0 != modbus_wait_for_bootup(cmd_status_point))
    {
        printf("******* modbus_wait_for_bootup for bootloader err\n");

        local_remove_point(cmd_status_point);
        create_point_maps();
        err = modbus_write_point_maps();

        free(file_buf);
        if (set_percent_func)
            set_percent_func(-1);
        return -1;
    }

    /* 发送文件头 */
    {
        memset(&tFileInfo, 0, sizeof(tFileInfo));

        tFileInfo.file_len = file_len;
        tFileInfo.file_len = LE32toBE32((uint8_t *)&tFileInfo.file_len);
        if (file)
        {
            strncpy_s(tFileInfo.file_name, sizeof(tFileInfo.file_name), (char *)file, sizeof(tFileInfo.file_name) - 1);
        }

        /* 得到modbus上下文 */
        printf("******* write firmware head ...\n");
        ctx = get_modbus_ctx(port_info);
        if (!ctx)
        {
            free(file_buf);

            local_remove_point(cmd_status_point);
            create_point_maps();
            err = modbus_write_point_maps();

            printf("******* get_modbus_ctx for update err\n");
            if (set_percent_func)
                set_percent_func(-1);
            return -1;
        }

        /* file_no是16位的数值
         * 约定:
         * bit[15:12]: channel
         * bit[11:8] : 真正的file_no, 0-map信息,1-固件
         * bit[7:0]  : dev_addr
         */
        file_no = (channel << 12) | (FILE_NUMBER_FIRMWARE << 8) | dev_addr;
        record_no = 0; /* 文件信息 */
        rc = modbus_write_file_record(ctx, file_no, record_no, (uint8_t *)&tFileInfo, sizeof(tFileInfo));
        if (rc < 0)
        {
            put_modbus_ctx(ctx);
            free(file_buf);

            local_remove_point(cmd_status_point);
            create_point_maps();
            err = modbus_write_point_maps();

            if (set_percent_func)
                set_percent_func(-1);
            printf("******* modbus_write_file_record for update head err\n");
            return rc;
        }

        put_modbus_ctx(ctx);
        record_no++;
    }

    /* 发送文件块 */
    {
        /* 循环:
         * a. 发送"File Record"
         * b. 读取对方的状态, 等待直到对方变为"IDLE"(表示已经处理完接收到的文件块)
         */

        while (pos < file_len)
        {
            /* 选取一次发送240字节是为了便于烧录(烧录时一次烧写16字节)
             * 240是16的整数倍
             */
            send_len = file_len - pos;
            if (send_len > MAX_FILE_RECORD_SIZE)
                send_len = MAX_FILE_RECORD_SIZE;

            /* 得到modbus上下文 */
            printf("******* write firmware record  %d, len = %d, ...\n", record_no, send_len);
            ctx = get_modbus_ctx(port_info);
            if (!ctx)
            {
                free(file_buf);

                local_remove_point(cmd_status_point);
                create_point_maps();
                err = modbus_write_point_maps();

                if (set_percent_func)
                    set_percent_func(-1);
                printf("******* get_modbus_ctx for send file record err\n");
                return -1;
            }

            rc = modbus_write_file_record(ctx, file_no, record_no, file_buf + pos, send_len);
            if (rc < 0)
            {
                put_modbus_ctx(ctx);
                free(file_buf);

                local_remove_point(cmd_status_point);
                create_point_maps();
                err = modbus_write_point_maps();

                if (set_percent_func)
                    set_percent_func(-1);
                printf("******* modbus_write_file_record for update err\n");
                return -1;
            }

            put_modbus_ctx(ctx);

            record_no++;
            pos += send_len;

            if (set_percent_func)
            {
                percent = pos * 100 / file_len;
                if (percent == 100)
                    percent = 99; /* 启动APP后再设置为100 */
                set_percent_func(percent);
            }
        }
    }
    free(file_buf);

    /* 发送启动命令 */
    printf("******* send update command to enter app ...\n");
    modbus_write_point(cmd_status_point, MODBUS_PRIVATE_CMD_START);

    /* 循环读取状态:等待目标板重启进入APP */
    Sleep(2000);
    printf("******* wait target to enter app ...\n");
    if (0 != modbus_wait_for_bootup(cmd_status_point))
    {
        printf("******* modbus_wait_for_bootup to app err\n");

        local_remove_point(cmd_status_point);
        create_point_maps();
        err = modbus_write_point_maps();

        if (set_percent_func)
            set_percent_func(-1);
        return -1;
    }

    /* 删除点 */
    printf("******* del map cmd_status register ...\n");
    local_remove_point(cmd_status_point);
    create_point_maps();
    err = modbus_write_point_maps();
    if (err)
    {
        printf("******* modbus_write_point_maps for endup update err\n");

        local_remove_point(cmd_status_point);
        create_point_maps();
        err = modbus_write_point_maps();

        if (set_percent_func)
            set_percent_func(-1);
        return -1;
    }

    if (set_percent_func)
    {
        set_percent_func(100);
    }

    printf("******* modbus_update OK\n");
    return 0;
}

/**********************************************************************
 * 函数名称： modbus_wait_for_idle
 * 功能描述： 等待目标板启动完毕(进入bootloader或者app)
 * 输入参数： cmd_status_point      - CMD_STATUS寄存器对应点
 * 输出参数： 无
 * 返 回 值： 0-成功, (-1)-失败
 * 修改日期：      版本号     修改人       修改内容
 * -----------------------------------------------
 * 2024/06/24        V1.0     韦东山       创建
 ***********************************************************************/
static int modbus_wait_for_bootup(int cmd_status_point)
{
    int err;
    int status;
    int i;

    for (i = 0; i < 5; i++)
    {
        err = modbus_write_point_maps();
        if (err)
        {
            Sleep(1000);
        }
        else
        {
            return 0;
        }
    }

    return -1;
}

/**********************************************************************
 * 函数名称： modbus_wait_for_idle
 * 功能描述： 等待目标板能再次接收数据
 * 输入参数： cmd_status_point      - CMD_STATUS寄存器对应点
 * 输出参数： 无
 * 返 回 值： 0-成功, (-1)-失败
 * 修改日期：      版本号     修改人       修改内容
 * -----------------------------------------------
 * 2024/06/24        V1.0     韦东山       创建
 ***********************************************************************/
static int modbus_wait_for_idle(int cmd_status_point)
{
    int err;
    int status = 0;
    int i;

    for (i = 0; i < 5; i++)
    {
        err = modbus_read_point(cmd_status_point, &status);
        if (err || (status != MODBUS_PRIVATE_STATUS_IDLE))
        {
            Sleep(1000);
        }
        else
        {
            break;
        }
    }

    if (i == 5)
        return -1;
    else
        return 0;
}