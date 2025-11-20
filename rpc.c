/*
 *****************************************************************************
 * @file rpc.c
 *
 * @brief  Modbus工具RPC接口实现
 *
 * @author  dongmu
 * @date    2025-11-20
 * @version V1.0
 *
 *****************************************************************************
 */
#define WINDOWS_SOCKET 1

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if WINDOWS_SOCKET != 1

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#else if WINDOWS_SOCKET == 1

#include <winsock.h>

// Winsock library
#pragma comment(lib, "ws2_32.lib")

static void close_socket(SOCKET so)
{
    closesocket(so);
    WSACleanup();
    return;
}

#endif
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "rpc.h"
#include "cJSON.h"

#define ENABLE_RPC_CLIENT       1
#define ENABLE_RPC_SERVER       0

/*
 * @brief RPC客户端初始化
 * @param 无
 * @return socket句柄
 */
int rpc_client_init() {
    int iSocketClient;
    struct sockaddr_in tSocketServerAddr;
    int iRet;
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        DEBUG_PRINTF("Failed WSAStartup.\n");
        return -1;
    }

    DEBUG_PRINTF("Winsock Initialised.\n");

    if ((iSocketClient = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        DEBUG_PRINTF("Error on created the socket.");
        return -1;
    }

    DEBUG_PRINTF("Socket created.\n");

    memset(&tSocketServerAddr, 0, sizeof(tSocketServerAddr));
    tSocketServerAddr.sin_family = AF_INET;
    tSocketServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    tSocketServerAddr.sin_port = htons(PORT);

    iRet = connect(iSocketClient, (const struct sockaddr *)&tSocketServerAddr, sizeof(struct sockaddr));
    if (-1 == iRet)
    {
        DEBUG_PRINTF("connect error!\n");
        return -1;
    }

    return iSocketClient;
}

/*
 * @brief RPC添加点，添加到后台程序
 * @param int iSocketClient  客户端socket句柄
 * @param char *port_info    端口信息 
 * @param int channel        通道号
 * @param int dev_addr      设备地址
 * @param int reg_addr      寄存器地址
 * @param char *reg_type    寄存器类型
 * @param int period        访问周期
 * @return -1失败, 其他值:唯一的句柄
 */
int rpc_add_point(int iSocketClient, char *port_info, int channel, int dev_addr, int reg_addr, char *reg_type, int period)
{
    char buf[300];
    int iLen;
    sprintf(buf, "{\"method\": \"add_point\","
                     "\"params\": {"
                         "\"port_info\": \"%s\","
                         "\"channel\": %d,"
                         "\"dev_addr\": %d,"
                         "\"reg_addr\": %d,"
                         "\"reg_type\": \"%s\","
                         "\"period\": %d"
                         "},"
                     "\"id\": \"2\""
                 "}", port_info, channel, dev_addr, reg_addr, reg_type, period);
    iLen = send(iSocketClient, buf, strlen(buf), 0);
    if (iLen ==  strlen(buf))
    {
        while (1)
        {
            iLen = recv(iSocketClient, buf, sizeof(buf), 0);
            buf[iLen] = 0;
            if (iLen == 1 && (buf[0] == '\r' || buf[0] == '\n'))
                continue;
            else
                break;
        }

        if (iLen > 0)
        {
            cJSON *root = cJSON_Parse(buf);
            cJSON *result = cJSON_GetObjectItem(root, "result");
            if (result)
            {
                cJSON_Delete(root);
                return result->valueint;
            }
            else
            {
                cJSON_Delete(root);
                return -1;
            }
        }
        else
        {
            DEBUG_PRINTF("read rpc reply err : %d\n", iLen);
            return -1;
        }
    }
    else
    {
        DEBUG_PRINTF("send rpc request err : %d, %s\n", iLen, strerror(errno));
        return -1;
    }
}


/*
 * @brief RPC删除点
 * @param int iSocketClient  客户端socket句柄
 * @param int point          点索引
 * @return -1失败, 其他成功
 */
int rpc_remove_point(int iSocketClient, int point) {
    char buf[300];
    int iLen;
    sprintf(buf, "{\"method\": \"remove_point\","
                   "\"params\": [%d], \"id\": \"2\" }", point);
    iLen = send(iSocketClient, buf, strlen(buf), 0);
    if (iLen ==  strlen(buf))
    {
        while (1)
        {
            iLen = recv(iSocketClient, buf, sizeof(buf), 0);
            buf[iLen] = 0;
            if (iLen == 1 && (buf[0] == '\r' || buf[0] == '\n'))
                continue;
            else
                break;
        }

        if (iLen > 0)
        {
            cJSON *root = cJSON_Parse(buf);
            cJSON *result = cJSON_GetObjectItem(root, "result");
            if (result)
            {
                cJSON_Delete(root);
                return result->valueint;
            }
            else
            {
                cJSON_Delete(root);
                return -1;
            }
        }
        else
        {
            DEBUG_PRINTF("read rpc reply err : %d\n", iLen);
            return -1;
        }
    }
    else
    {
        DEBUG_PRINTF("send rpc request err : %d, %s\n", iLen, strerror(errno));
        return -1;
    }
}

/*
 * @brief RPC修改点
 * @param 无
 * @return 无
 */
rpc_modify_point()

rpc_get_point_count()

rpc_get_point()

rpc_read_point()

rpc_write_point()

rpc_start_update()

rpc_get_update_pecent()

