#ifndef _MODBUS_CLIENT_H
#define _MODBUS_CLIENT_H

#define MAX_POINT_COUNT 500
#define MAX_MODBUS 100


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



#endif /* _MODBUS_CLIENT_H */
