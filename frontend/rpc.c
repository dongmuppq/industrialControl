/**
 ******************************************************************************
 * @file    rpc.c
 * @author  dongmu
 * @version V1.0
 * @date    2025-11-20
 * @brief	rpc API
 ******************************************************************************
 */

#define WINDOWS_SOCKET 1

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if WINDOWS_SOCKET != 1
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "rpc.h"
#include "cJSON.h"

#if WINDOWS_SOCKET == 1
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

#define ENABLE_RPC_CLIENT       1

#define MY_DEBUG 1
#ifdef MY_DEBUG
#define DEBUG_PRINTF(fmt, arg...)                           \
    do {                                                    \
        printf("%s()%d - " fmt, __func__, __LINE__, ##arg); \
    } while (0)
#else
#define DEBUG_PRINTF(x)
#endif

#if ENABLE_RPC_CLIENT

/**
 * @brief  RPC Client Init
 * @param  None
 * @retval iSocketClient
 */
int RPC_Client_Init(void)
{
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
    //tSocketServerAddr.sin_addr.s_addr = inet_addr("192.168.1.67");
    //tSocketServerAddr.sin_port = htons(PORT);

    iRet = connect(iSocketClient, (const struct sockaddr *)&tSocketServerAddr, sizeof(struct sockaddr));
    if (-1 == iRet)
    {
        DEBUG_PRINTF("connect error!\n");
        return -1;
    }

    return iSocketClient;
}

/**
 * @brief  add point
 * @param  iSocketClient
 * @param  port_info
 * @param  channel
 * @param  dev_addr
 * @param  reg_addr
 * @param  reg_type
 * @param  period
 * @retval point
 */
int rpc_add_point(int iSocketClient, char *port_info, int channel, int dev_addr, int reg_addr, char *reg_type, int period)
{
    char buf[300];
    int iLen;
    sprintf(buf, "{\"method\": \"add_point\"," \
                  "\"params\":"                \
                  "{\"port_info\": \"%s\","    \
                   "\"channel\": %d,"         \
                   "\"dev_addr\": %d,"         \
                   "\"reg_addr\": %d,"         \
                   "\"reg_type\": \"%s\"," \
                   "\"period\": %d}, \"id\": \"2\" }", port_info, channel, dev_addr, reg_addr, reg_type, period);
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

/**
 * @brief  remove point
 * @param  iSocketClient
 * @param  point
 * @retval 0:success, -1:fail
 */
int rpc_remove_point(int iSocketClient, int point)
{
    char buf[300];
    int iLen;
    sprintf(buf, "{\"method\": \"remove_point\"," \
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


/**
 * @brief  modify point
 * @param  iSocketClient
 * @param  point
 * @param  port_info
 * @param  channel
 * @param  dev_addr
 * @param  reg_addr
 * @param  reg_type
 * @param  period
 * @retval 0:success, -1:fail
 */
int rpc_modify_point(int iSocketClient, int point, char *port_info, int channel, int dev_addr, int reg_addr, char *reg_type, int period)
{
    char buf[400];
    int iLen;
    sprintf(buf, "{\"method\": \"modify_point\"," \
                 "\"params\":"                \
                 "{\"number\": %d,"            \
                  "\"port_info\": \"%s\","    \
                  "\"channel\": %d,"         \
                  "\"dev_addr\": %d,"         \
                  "\"reg_addr\": %d,"         \
                  "\"reg_type\": \"%s\"," \
                  "\"period\": %d}, \"id\": \"2\" }", point, port_info, channel, dev_addr, reg_addr, reg_type, period);
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

/**
 * @brief  get point count
 * @param  iSocketClient
 * @retval point count
 */
int rpc_get_point_count(int iSocketClient)
{
    char buf[200];
    int iLen;
    sprintf(buf, "{\"method\": \"get_point_count\", \"params\": [0], \"id\": \"2\" }");
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
                DEBUG_PRINTF("read rpc reply iLen : %d, but failed\n", iLen);
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


/**
 * @brief  get next point
 * @param  iSocketClient
 * @param  pre_point
 * @param  pInfo
 * @retval 0:success, -1:fail
 */
int rpc_get_next_point(int iSocketClient, int *point, PPointInfo pInfo)
{
    char buf[300];
    int iLen;
    sprintf(buf, "{\"method\": \"get_next_point\"," \
                   "\"params\": [%d], \"id\": \"2\" }", *point);
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
                cJSON *number = cJSON_GetObjectItem(result, "number");
                *point = number->valueint;

                cJSON *port_info = cJSON_GetObjectItem(result, "port_info");
                strcpy(pInfo->port_info, port_info->valuestring);

                cJSON *channel = cJSON_GetObjectItem(result, "channel");
                pInfo->channel = channel->valueint;

                cJSON *dev_addr = cJSON_GetObjectItem(result, "dev_addr");
                pInfo->dev_addr = dev_addr->valueint;

                cJSON *reg_addr = cJSON_GetObjectItem(result, "reg_addr");
                pInfo->reg_addr = reg_addr->valueint;

                cJSON *reg_type = cJSON_GetObjectItem(result, "reg_type");
                strcpy(pInfo->reg_type, reg_type->valuestring);

                cJSON *period = cJSON_GetObjectItem(result, "period");
                pInfo->period = period->valueint;
                cJSON_Delete(root);
                return 0;
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


/**
 * @brief  write point
 * @param  iSocketClient
 * @param  point
 * @param  val
 * @retval 0:success, -1:fail
 */
int rpc_write_point(int iSocketClient, int point, int val)
{
    char buf[100];
    int iLen;
    sprintf(buf, "{\"method\": \"write_point\"," \
                   "\"params\": [%d, %d], \"id\": \"2\" }", point, val);

    //DEBUG_PRINTF("rpc_write_point : %s\n", buf);

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
                DEBUG_PRINTF("set ok\n");
                return result->valueint;
            }
            else
            {
                cJSON_Delete(root);
                DEBUG_PRINTF("set error\n");
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


/**
 * @brief  read point
 * @param  iSocketClient
 * @param  point
 * @param  val
 * @retval 0:success, -1:fail
 */
int rpc_read_point(int iSocketClient, int point, int *val)
{
    char buf[100];
    int iLen;
    sprintf(buf, "{\"method\": \"read_point\"," \
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

        //DEBUG_PRINTF("iLen = %d, buf = %s\n", iLen, buf);

        if (iLen > 0)
        {
            cJSON *root = cJSON_Parse(buf);
            cJSON *result = cJSON_GetObjectItem(root, "result");
            if (result)
            {
                cJSON_Delete(root);
                *val = result->valueint;
                return 0;
            }
            else
            {
                cJSON *error = cJSON_GetObjectItem(root, "error");
                if (error)
                {
                    DEBUG_PRINTF("get error\n");
                    cJSON *message = cJSON_GetObjectItem(error, "message");
                    if (message)
                        DEBUG_PRINTF("%s\n", message->valuestring);
                }
                DEBUG_PRINTF("error\n");
				DEBUG_PRINTF("iLen = %d, buf = %s\n", iLen, buf);
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

/**
 * @brief  start update
 * @param  iSocketClient
 * @param  ptUpdateInfo
 * @retval 0:success, -1:fail
 */
int rpc_start_update(int iSocketClient, PUpdateInfo ptUpdateInfo)
{
    char buf[512];
    int iLen;

    char file[100];
    char port_info[100];
    int dev_addr;

    /* {"broker": "192.168.5.10", "port": 1833, "client_id": "device_01", "user": "100ask", "password": "www.100ask.net", "publish":"/iot/up", "subcribe":"/iot/down"} */
    sprintf(buf, "{\"method\": \"start_update\"," \
                  "\"params\":"                \
                  "{\"file\": \"%s\","    \
                   "\"port_info\": \"%s\","         \
                   "\"channel\": %d },"         \
                   "\"dev_addr\": %d },"         \
                   "\"id\": \"2\" }", ptUpdateInfo->file, ptUpdateInfo->port_info, ptUpdateInfo->channel, ptUpdateInfo->dev_addr);
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
            {
                break;
            }

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



/**
 * @brief  get update percent
 * @param  iSocketClient
 * @retval 0:success, -1:fail
 */
int rpc_get_update_percent(int iSocketClient)
{
    char buf[200];
    int iLen;
    sprintf(buf, "{\"method\": \"get_update_pecent\", \"params\": [0], \"id\": \"2\" }");
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
                DEBUG_PRINTF("read rpc reply iLen : %d, but failed\n", iLen);
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

#endif /* ENABLE_RPC_CLIENT */

