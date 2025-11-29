#include "rpc.h"

PointInfo g_PointInfos[MAX_POINT_NUM];

bool server_add_point(const Json::Value& root, Json::Value& response)
{
    int number;
    PointInfo point;
    Json::Value params = root["params"];

    if (params == Json::Value::null) {
        response["jsonrpc"] = "2.0";
        response["id"] = root["id"];
        response["result"] = -1;
    } else {
        while (g_PointInfos[number].dev_addr != 0) {
            number++;
            if (number >= MAX_POINT_NUM) {
                response["jsonrpc"] = "2.0";
                response["id"] = root["id"];
                response["result"] = -1;
            }
        }

        strncpy_s(point.port_info, params["port_info"].asCString(), sizeof(point.port_info) - 1);
        strncpy_s(point.reg_type, params["reg_type"].asCString(), sizeof(point.reg_type) - 1);
        point.channel = params["channel"].asInt();
        point.dev_addr = params["dev_addr"].asInt();
        point.period = params["period"].asInt();
        point.reg_addr = params["reg_addr"].asInt();
        point.reg_addr_master = params["reg_addr_master"].asInt();

        response["jsonrpc"] = "2.0";
        response["id"] = root["id"];
        response["result"] = number;


        // 没加一个点就重新写入配置
        write_cfg();
        create_point_maps();
        modbus_write_point_maps();
    }
    
    return true;
}