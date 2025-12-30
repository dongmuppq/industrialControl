#ifndef _MODBUS_CLIENT_H
#define _MODBUS_CLIENT_H

#define MAX_POINT_COUNT 500
#define MAX_MODBUS 100

/* 选取一次发送240字节是为了便于烧录(烧录时一次烧写16字节)
 * 240是16的整数倍
 */
#define MAX_FILE_RECORD_SIZE 240

/* 映射升级时用到的1个寄存器
 * AO[MODBUS_UPDATE_REG_ADDR] : 写入命令, 读出状态
 *  写: 1-升级, 2-启动
 *  读: 0-传感器无法接收文件, 1-传感器可以接收文件, 2-文件接收完毕
 */

#define MODBUS_UPDATE_REG_ADDR 0

#define MODBUS_PRIVATE_CMD_UPDATE 0x55
#define MODBUS_PRIVATE_CMD_START  0xAA

#define MODBUS_PRIVATE_STATUS_BUSY 0x12
#define MODBUS_PRIVATE_STATUS_IDLE 0x34
#define MODBUS_PRIVATE_STATUS_DONE 0x56

#define FILE_NUMBER_POINT_MAP  0
#define FILE_NUMBER_FIRMWARE   1


typedef struct PointMap {
    char reg_type[4];
    uint16_t reg_addr_master; /* 主控的寄存器地址 */    
    uint16_t channel;         /* 0-主控本身, 1-通过CH1访问, 2-通过CH2访问 */
    uint16_t dev_addr;        /* 传感器的设备地址 */
    uint16_t reg_addr_salve;  /* 传感器的寄存器地址 */
}PointMap, *PPointMap;

typedef struct HostPointMap {
    char port_info[100]; /* COM1, COM2, /dev/ttyUSB0, /dev/ttyUSB1 */
    PointMap tPointMaps[MAX_POINT_COUNT];
}HostPointMap, *PHostPointMap;

void create_point_maps(void);
int modbus_write_point_maps(void);
int modbus_write_point(int point, int val);
int modbus_read_point(int point, int *pVal);

int modbus_clear_status(int cmd_status_point);
int modbus_update(char *file, char *port_info, int channel, int dev_addr, void(*set_percent_func)(int));


#endif /* _MODBUS_CLIENT_H */
