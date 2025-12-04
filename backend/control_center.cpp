#include <cstdio>
#include <cstdlib>
#include <csignal>

#include <libmodbus/modbus.h>
#include "jsonrpc.h"
#include "rpc.h"
#include "cfg.h"
#include "modbus_client.h"

static volatile sig_atomic_t g_run = 0;


/**
 * \brief Signal management.
 * \param code signal code
 */
static void signal_handler(int code)
{
    switch (code)
    {
    case SIGINT:
    case SIGTERM:
        g_run = 0;
        break;
    default:
        break;
    }
}

/**
 * @brief 主函数
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char* argv[]) {
    // 初始化 jsonrpc 服务
    IndustrialControlRpc a;
    //Json::Rpc::TcpServer server(std::string("192.168.0.131"), 1234);
    Json::Rpc::TcpServer server(std::string("127.0.0.1"), 1234);

    (void)argc;
    (void)argv;

    // cfg 文件读取，建立映射表并写入中控
    read_cfg();
    create_point_maps();
    modbus_write_point_maps();
    
    if (!networking::init())
    {
        std::cerr << "Networking initialization failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (signal(SIGTERM, signal_handler) == SIG_ERR)
    {
        std::cout << "Error signal SIGTERM will not be handled" << std::endl;
    }

    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        std::cout << "Error signal SIGINT will not be handled" << std::endl;
    }

    // 注册 rpc 函数
    server.AddMethod(new Json::Rpc::RpcMethod<IndustrialControlRpc>(a, &IndustrialControlRpc::server_get_point_count,
        std::string("get_point_count")));
    server.AddMethod(new Json::Rpc::RpcMethod<IndustrialControlRpc>(a, &IndustrialControlRpc::server_get_next_point,
        std::string("get_next_point")));
    server.AddMethod(new Json::Rpc::RpcMethod<IndustrialControlRpc>(a, &IndustrialControlRpc::server_add_point,
        std::string("add_point")));
    server.AddMethod(new Json::Rpc::RpcMethod<IndustrialControlRpc>(a, &IndustrialControlRpc::server_remove_point,
        std::string("remove_point")));
    server.AddMethod(new Json::Rpc::RpcMethod<IndustrialControlRpc>(a, &IndustrialControlRpc::server_modify_point,
        std::string("modify_point")));



    // 启动 jsonrpc 服务
    if (!server.Bind())
    {
        std::cout << "Bind failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (!server.Listen())
    {
        std::cout << "Listen failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    g_run = 1;

    std::cout << "Start JSON-RPC TCP server" << std::endl;

    while (g_run)
    {
        server.WaitMessage(1000);
    }

    std::cout << "Stop JSON-RPC TCP server" << std::endl;
    server.Close();
    networking::cleanup();

    return EXIT_SUCCESS;
}
