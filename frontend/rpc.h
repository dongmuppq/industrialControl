#ifndef _RPC_H
#define _RPC_H

#define PORT 1234

typedef struct PointInfo {
    char port_info[20];  // 中控与PC的连接接口  
    char reg_type[4];
    int channel;		  // 中控与传感器的连接接口
    int dev_addr;
    int reg_addr;
    int period;  /* ms */
}PointInfo, *PPointInfo;

typedef struct MQTTInfo {
    char broker[100];
    int  port;
    char client_id[100];
    char user[100];
    char password[100];
    char publish[100];
    char subcribe[100];
}MQTTInfo, *PMQTTInfo;

typedef struct UpdateInfo {
    char file[100];
    char port_info[100];
    int channel;
    int dev_addr;
} UpdateInfo, *PUpdateInfo;


int RPC_Client_Init(void);
int rpc_add_point(int iSocketClient, char *port_info, int channel, int dev_addr, int reg_addr, char *reg_type, int period);
int rpc_remove_point(int iSocketClient, int point);
int rpc_modify_point(int iSocketClient, int number, char *port_info, int channel, int dev_addr, int reg_addr, char *reg_type, int period);
int rpc_get_point_count(int iSocketClient);
int rpc_get_next_point(int iSocketClient, int *point, PPointInfo pInfo);
int rpc_write_point(int iSocketClient, int point, int val);
int rpc_read_point(int iSocketClient, int point, int *val);
int rpc_start_update(int iSocketClient, PUpdateInfo ptUpdateInfo);
int rpc_get_update_percent(int iSocketClient);

#endif  /* _RPC_H */
