#ifndef __RPC_H__
#define __RPC_H__ 

#include <json/json.h>

#define MAX_POINT_NUM 100

typedef struct PointInfo {
    char port_info[20];  // 中控与PC的连接接口  
    char reg_type[4];
    uint16_t channel;		  // 中控与传感器的连接接口
    uint16_t dev_addr;
    uint16_t reg_addr;
    uint16_t period;  /* ms */
    uint16_t reg_addr_master; /* 主控的寄存器地址 */
}PointInfo, *PPointInfo;

PPointInfo local_get_points(void);

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

#endif  // __RPC_H__