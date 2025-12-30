#ifndef __RPC_H
#define __RPC_H

#include <json/json.h>

#define MAX_POINT_NUM 100

typedef struct PointInfo {
    char port_info[100];
    int channel;
    int dev_addr;
    int reg_addr;
    char reg_type[4];
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
}UpdateInfo, *PUpdateInfo;


class IndustrialControlRpc
{
  public:

    bool server_start_update(const Json::Value& root, Json::Value& response);
    bool server_get_update_pecent(const Json::Value& root, Json::Value& response);
    bool server_get_mqttinfo(const Json::Value& root, Json::Value& response);
    bool server_set_mqttinfo(const Json::Value& root, Json::Value& response);
    bool server_get_point_count(const Json::Value& root, Json::Value& response);
    bool server_get_next_point(const Json::Value& root, Json::Value& response);
    bool server_add_point(const Json::Value& root, Json::Value& response);
    bool server_remove_point(const Json::Value& root, Json::Value& response);
    bool server_modify_point(const Json::Value& root, Json::Value& response);
    bool server_read_point(const Json::Value& root, Json::Value& response);
    bool server_write_point(const Json::Value& root, Json::Value& response);


    /**
     * \brief Reply with success.
     * \param root JSON-RPC request
     * \param response JSON-RPC response
     * \return true if correctly processed, false otherwise
     */
    bool Print(const Json::Value& root, Json::Value& response);

    /**
     * \brief Notification.
     * \param root JSON-RPC request
     * \param response JSON-RPC response
     * \return true if correctly processed, false otherwise
     */
    bool Notify(const Json::Value& root, Json::Value& response);

    /**
     * \brief Get the description in JSON format.
     * \return JSON description
     */
    Json::Value GetDescription();

};

/* 添加点
 * pNewInfo : 新的点信息
 * 返回值: 添加的点索引
*/
int local_add_point(PPointInfo pNewInfo);

/* 删除点
 * point : 索引
 */
void local_remove_point(int point);

/* 获得点的数组
 * 返回值: PPointInfo(数组首地址)
 */
PPointInfo local_get_points(void);

/* 获得某个点
 * point : 索引
 * 返回值: PPointInfo
 */
PPointInfo local_get_point(int point);

/* 获得本地记录的MQTT信息
 *
 */
void local_get_mqttinfo(PMQTTInfo pInfo);


/* 设置更新百分比
 */
void local_set_update_percent(int percent);


#endif  // __RPC_H