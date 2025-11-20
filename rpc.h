#ifndef _RPC_H
#define _RPC_H

#define PORT 1234

typedef struct PointInfo {
    int point;
    char port_info[100];
    int channel;
    int dev_addr;
    int reg_addr;
    char reg_type[4];
    int period;  /* ms */
} PointInfo, *PPointInfo;


typedef struct UpdateInfo {
    char file[100];
    char port_info[100];
    int channel;
    int dev_addr;
} UpdateInfo, *PUpdateInfo;


#endif  /* _RPC_H */