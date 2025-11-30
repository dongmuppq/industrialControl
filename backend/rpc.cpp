#include "rpc.h"
#include "cfg.h"
#include "mmodbus.h"

#include <iostream>

PointInfo g_PointInfos[MAX_POINT_NUM];

/* 获得点的数组
 * 返回值: PPointInfo(数组首地址)
 */
PPointInfo local_get_points(void)
{
    return g_PointInfos;
}

/**
 * @brief 添加点
 * 
 * @param root 
 * @param response 
 * @return true 
 */
bool server_add_point(const Json::Value& root, Json::Value& response)
{
    int number;
    Json::Value params = root["params"];

    response["jsonrpc"] = "2.0";
    response["id"] = root["id"];

    if (params == Json::Value::null) {
        response["result"] = -1;
    } else {
        while (g_PointInfos[number].dev_addr != 0) {
            number++;
            if (number >= MAX_POINT_NUM) {
                response["jsonrpc"] = "2.0";
                response["id"] = root["id"];
                response["result"] = -1;

                return true;
            }
        }

        strncpy_s(g_PointInfos[number].port_info, params["port_info"].asCString(), sizeof(g_PointInfos[number].port_info) - 1);
        if (g_PointInfos[number]->port_info[0] == 'C')
            g_PointInfos[number]->port_info[0] = 'c';
        if (g_PointInfos[number]->port_info[1] == 'O')
            g_PointInfos[number]->port_info[1] = 'o';
        if (g_PointInfos[number]->port_info[2] == 'M')
            g_PointInfos[number]->port_info[2] = 'm';

        strncpy_s(g_PointInfos[number].reg_type, params["reg_type"].asCString(), sizeof(g_PointInfos[number].reg_type) - 1);
        g_PointInfos[number].channel = params["channel"].asInt();
        g_PointInfos[number].dev_addr = params["dev_addr"].asInt();
        g_PointInfos[number].period = params["period"].asInt();
        g_PointInfos[number].reg_addr = params["reg_addr"].asInt();
        g_PointInfos[number].reg_addr_master = params["reg_addr_master"].asInt();

        response["result"] = number;

        // 每修改一次点就重新写入配置
        write_cfg();
        // create_point_maps();
        // modbus_write_point_maps();
    }
    
    return true;
}


/**
 * @brief 删除点
 * 
 * @param root 
 * @param response 
 * @return true 
 */
bool IndustrialControlRpc::server_remove_point(const Json::Value& root, Json::Value& response) {
    Json::Value params = root["params"];

    response["jsonrpc"] = "2.0";
    response["id"] = root["id"];

    if (params == Json::Value::null) {
        response["result"] = -1;
    } else {
        int number = params[0u].asInt();
        if (number < 0 || number >= MAX_POINT_NUM) {
            response["result"] = -1;
            return true;
        }

        for (int i = number; i < MAX_POINT_NUM - 1; i++) {
            g_PointInfos[i] = g_PointInfos[i + 1];
        }

        response["result"] = 0;

        // 每修改一次点就重新写入配置
        write_cfg();
        create_point_maps();
        modbus_write_point_maps();
    }

    return true;
}

/**
 * @brief 修改点
 * 
 * @param root 
 * @param response 
 * @return true 
 */
bool IndustrialControlRpc::server_modify_point(const Json::Value& root, Json::Value& response) {
    Json::Value params = root["params"];

    response["jsonrpc"] = "2.0";
    response["id"] = root["id"];

    if (params == Json::Value::null) {
        response["result"] = -1;
    } else {
        int number = params["number"].asInt();
        if (number < 0 || number >= MAX_POINT_NUM) {
            response["result"] = -1;
            return true;
        }

        strncpy_s(g_PointInfos[number].port_info, params["port_info"].asCString(), sizeof(g_PointInfos[number].port_info) - 1);
        if (g_PointInfos[number]->port_info[0] == 'C')
            g_PointInfos[number]->port_info[0] = 'c';
        if (g_PointInfos[number]->port_info[1] == 'O')
            g_PointInfos[number]->port_info[1] = 'o';
        if (g_PointInfos[number]->port_info[2] == 'M')
            g_PointInfos[number]->port_info[2] = 'm';

        strncpy_s(g_PointInfos[number].reg_type, params["reg_type"].asCString(), sizeof(g_PointInfos[number].reg_type) - 1);
        g_PointInfos[number].channel = params["channel"].asInt();
        g_PointInfos[number].dev_addr = params["dev_addr"].asInt();
        g_PointInfos[number].period = params["period"].asInt();
        g_PointInfos[number].reg_addr = params["reg_addr"].asInt();
        g_PointInfos[number].reg_addr_master = params["reg_addr_master"].asInt();

        response["result"] = 0;

        // 每修改一次点就重新写入配置
        write_cfg();
        create_point_maps();
        modbus_write_point_maps();
    }

    return true;
}

/**
 * @brief 获取点数量
 * 
 * @param root 
 * @param response 
 * @return true 
 */
bool IndustrialControlRpc::server_get_point_count(const Json::Value& root, Json::Value& response) {
    response["jsonrpc"] = "2.0";
    response["id"] = root["id"];

    int count = 0;
    for (int i = 0; i < MAX_POINT_NUM; i++) {
        if (g_PointInfos[i].dev_addr != 0) {
            count++;
        }
    }

    response["result"] = count;

    return true;
}

/**
 * @brief 获取下一个点
 * 
 * @param root 
 * @param response 
 * @return true 
 */
bool IndustrialControlRpc::server_get_next_point(const Json::Value& root, Json::Value& response) { 
    Json::Value params = root["params"];
    Json::Value result;

    PPointInfo pInfo;
    int pre_point, i;

    response["jsonrpc"] = "2.0";
    response["id"] = root["id"];
    if (params == Json::Value::null) { 
        response["result"] = -1;
    } else { 
        pre_point = params[0u].asInt();
    }

    for (i = pre_point + 1; i < MAX_POINT_NUM; i++) { 
        if (g_PointInfos[i].dev_addr != 0) { 
            pInfo = &g_PointInfos[i];
            break;
        }
    }

    if (pInfo != NULL) { 
        result["number"] = i;
        result["port_info"] = pInfo->port_info;
        result["channel"]   = pInfo->channel;
        result["dev_addr"]  = pInfo->dev_addr;
        result["reg_addr"]  = pInfo->reg_addr;
        result["reg_type"]  = pInfo->reg_type;
        result["period"]    = pInfo->period;
        result["reg_addr_master"] = pInfo->reg_addr_master;
    }

    response["result"] = result;
    return true;
}

