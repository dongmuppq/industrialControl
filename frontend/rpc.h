#ifndef _RPC_H
#define _RPC_H

#define PORT 1234

typedef struct PointInfo {
    char port_info[20];  // 中控与PC的连接接口  
    char reg_type[4];
    uint16_t channel;		  // 中控与传感器的连接接口
    uint16_t dev_addr;
    uint16_t reg_addr;
    uint16_t period;  /* ms */
    uint16_t reg_addr_master; /* 主控的寄存器地址 */
}PointInfo, *PPointInfo;

typedef struct UpdateInfo {
    char file[100];
    char port_info[100];
    int channel;
    int dev_addr;
} UpdateInfo, *PUpdateInfo;


#endif  /* _RPC_H */