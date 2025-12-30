#include "rpc.h"
#include "cfg.h"
#include "modbus_client.h"
#include "update.h"
#include <libmodbus/modbus.h>
#include <iostream>

static MQTTInfo g_MQTTInfo;
static PointInfo g_PointInfos[MAX_POINT_NUM];
static UpdateThread g_UpdateThread;



/* 本地增加一个点
 * 比如读取配置文件增加点时就调用此函数
 * 返回值: >0-成功,返回点的索引, (-1)失败
 */
int local_add_point(PPointInfo pNewInfo) {
    int i = 0;
    PPointInfo pInfo;
    int point = -1;
    
    while (g_PointInfos[i].dev_addr != 0) {
        i++;
        if (i >= MAX_POINT_NUM) {
            return -1;
        }
    }

    pInfo = &g_PointInfos[i];
    
    strncpy_s(pInfo->port_info, pNewInfo->port_info, sizeof(pInfo->port_info)-1);
    if (pInfo->port_info[0] == 'C')
        pInfo->port_info[0] = 'c';
    if (pInfo->port_info[1] == 'O')
        pInfo->port_info[1] = 'o';
    if (pInfo->port_info[2] == 'M')
        pInfo->port_info[2] = 'm';

    pInfo->channel = pNewInfo->channel;
    
    pInfo->dev_addr = pNewInfo->dev_addr;

    pInfo->reg_addr = pNewInfo->reg_addr;

    strncpy_s(pInfo->reg_type, pNewInfo->reg_type, sizeof(pInfo->reg_type)-1);

    pInfo->period = pNewInfo->period;
    
    return i;
}


/* 本地删除一个点
 * 返回值: 无
 */
void local_remove_point(int point) {
    for (int i = point; i < MAX_POINT_NUM - 1; i++) {
        g_PointInfos[i] = g_PointInfos[i + 1];
    }
}


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
bool IndustrialControlRpc::server_add_point(const Json::Value& root, Json::Value& response)
{
    std::cout << "Start server_add_point" << std::endl;
    int number = 0;
    PointInfo tInfo;
    Json::Value params = root["params"];

    response["jsonrpc"] = "2.0";
    response["id"] = root["id"];

    if (params == Json::Value::null) {
        response["result"] = -1;
    }
    else {
        strncpy_s(tInfo.port_info, params["port_info"].asCString(), sizeof(tInfo.port_info)-1); 

        tInfo.channel  = params["channel"].asInt();
        tInfo.dev_addr = params["dev_addr"].asInt();
        tInfo.reg_addr = params["reg_addr"].asInt();
        strncpy_s(tInfo.reg_type, params["reg_type"].asCString(), sizeof(tInfo.reg_type)-1);
        tInfo.period = params["period"].asInt();

        number = local_add_point(&tInfo);
        printf("add point %d: %s\r\n", number, tInfo.port_info);

        response["jsonrpc"] = "2.0";
        response["id"] = root["id"];
        response["result"] = number;

        // 每修改一次点就重新写入配置
        write_cfg();
        create_point_maps();
        modbus_write_point_maps();
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
    std::cout << "Start server_remove_point" << std::endl;
    Json::Value params = root["params"];

    response["jsonrpc"] = "2.0";
    response["id"] = root["id"];

    if (params == Json::Value::null) {
        response["result"] = -1;
    }
    else {
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
    std::cout << "Start server_modify_point" << std::endl;

    Json::Value params = root["params"];

    response["jsonrpc"] = "2.0";
    response["id"] = root["id"];

    if (params == Json::Value::null) {
        response["result"] = -1;
    }
    else {
        int number = params["number"].asInt();
        if (number < 0 || number >= MAX_POINT_NUM) {
            response["result"] = -1;
            return true;
        }

        strncpy_s(g_PointInfos[number].port_info, params["port_info"].asCString(), sizeof(g_PointInfos[number].port_info) - 1);
        if (g_PointInfos[number].port_info[0] == 'C')
            g_PointInfos[number].port_info[0] = 'c';
        if (g_PointInfos[number].port_info[1] == 'O')
            g_PointInfos[number].port_info[1] = 'o';
        if (g_PointInfos[number].port_info[2] == 'M')
            g_PointInfos[number].port_info[2] = 'm';

        strncpy_s(g_PointInfos[number].reg_type, params["reg_type"].asCString(), sizeof(g_PointInfos[number].reg_type) - 1);
        g_PointInfos[number].channel = params["channel"].asInt();
        g_PointInfos[number].dev_addr = params["dev_addr"].asInt();
        g_PointInfos[number].period = params["period"].asInt();
        g_PointInfos[number].reg_addr = params["reg_addr"].asInt();


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
    std::cout << "Start server_get_point_count" << std::endl;

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
    std::cout << "Start server_get_next_point" << std::endl;

    Json::Value params = root["params"];
    Json::Value result;

    PPointInfo pInfo = NULL;
    int pre_point, i;

    response["jsonrpc"] = "2.0";
    response["id"] = root["id"];
    if (params == Json::Value::null) {
        response["result"] = -1;
    }
    else {
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
        result["channel"] = pInfo->channel;
        result["dev_addr"] = pInfo->dev_addr;
        result["reg_addr"] = pInfo->reg_addr;
        result["reg_type"] = pInfo->reg_type;
        result["period"] = pInfo->period;
    }

    response["result"] = result;
    return true;
}


bool IndustrialControlRpc::server_read_point(const Json::Value& root, Json::Value& response) {
    std::cout << "Start server_read_point" << std::endl;

    Json::Value params = root["params"];
    int point = params[0u].asInt();
    int val = 0;

    response["jsonrpc"] = "2.0";
    response["id"] = root["id"];

    // TODO: 更新时禁止读写
    if (g_UpdateThread.isRunning())
        return false;

    // 中控断电重连后重新写入
    if (modbus_is_reconnect()) {
        create_point_maps();
        if (!modbus_write_point_maps())
            modbus_clear_reconnect_status();
    }

    if (0 == modbus_read_point(point, &val)) {
        response["result"] = val;
    }

    return true;
}


bool IndustrialControlRpc::server_write_point(const Json::Value& root, Json::Value& response) {
    std::cout << "Start server_write_point" << std::endl;
    Json::Value params = root["params"];
    int point = params[0u].asInt();
    int val = params[1u].asInt();

    response["jsonrpc"] = "2.0";
    response["id"] = root["id"];

    // TODO: 更新时禁止读写
    if (g_UpdateThread.isRunning())
        return false;

    // 中控断电重连后重新写入
    if (modbus_is_reconnect()) {
        create_point_maps();
        if (!modbus_write_point_maps())
            modbus_clear_reconnect_status();
    }

    if (0 == modbus_write_point(point, val)) {
        response["result"] = 0;
    }

    return true;
}


/* 获得本地记录的MQTT信息
 *
 */
void local_get_mqttinfo(PMQTTInfo pInfo)
{
    *pInfo = g_MQTTInfo;
}


bool IndustrialControlRpc::server_start_update(const Json::Value& root, Json::Value& response)
{
    std::cout << "Receive query: " << root << std::endl;
    Json::Value params = root["params"];

    response["jsonrpc"] = "2.0";
    response["id"] = root["id"];

    if (params == Json::Value::null) {
        std::cout << "no params" << std::endl;
        response["result"] = -1;
    }
    else {
        Json::Value file = params["file"];
        Json::Value port_info = params["port_info"];
        Json::Value channel = params["channel"];
        Json::Value dev_addr = params["dev_addr"];

        g_UpdateThread.StartUpdate(file.asCString(), port_info.asCString(), channel.asInt(), dev_addr.asInt());

        response["result"] = 0;
    }

    return true;
}

bool IndustrialControlRpc::server_get_update_pecent(const Json::Value& root, Json::Value& response)
{
    int percent = g_UpdateThread.GetUpdatePercent();

    response["jsonrpc"] = "2.0";
    response["id"] = root["id"];
    response["result"] = percent;

    return true;
}

/* 设置更新百分比
 */
void local_set_update_percent(int percent)
{
    g_UpdateThread.SetUpdatePercent(percent);
}

bool IndustrialControlRpc::server_get_mqttinfo(const Json::Value& root, Json::Value& response)
{

}

bool IndustrialControlRpc::server_set_mqttinfo(const Json::Value& root, Json::Value& response)
{
}


Json::Value IndustrialControlRpc::GetDescription()
{
    Json::FastWriter writer;
    Json::Value root;
    Json::Value parameters;
    Json::Value param1;

    root["description"] = "Print";

    /* type of parameter named arg1 */
    param1["type"] = "integer";
    param1["description"] = "argument 1";

    /* push it into the parameters list */
    parameters["arg1"] = param1;
    root["parameters"] = parameters;

    /* no value returned */
    root["returns"] = Json::Value::null;

    return root;
}


bool IndustrialControlRpc::Print(const Json::Value& root, Json::Value& response)
{
    std::cout << "Receive query: " << root << std::endl;
    response["jsonrpc"] = "2.0";
    response["id"] = root["id"];
    response["result"] = "success";
    return true;
}

bool IndustrialControlRpc::Notify(const Json::Value& root, Json::Value& response)
{
    std::cout << "Notification: " << root << std::endl;
    response = Json::Value::null;
    return true;
}