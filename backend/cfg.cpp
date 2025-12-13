#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <direct.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>  
#include "rpc.h"
#include "cfg.h"
#include <json/json.h>


#define CFG_FILE "./control.cfg"

/**
 * @brief 写入配置文件
 * 
 * @return int 
 */
int write_cfg(void) {
    FILE* fp;
    int i = 0;
    char *line = (char *)malloc(1024);
    PPointInfo point = local_get_points();
    
    errno_t err = fopen_s(&fp, CFG_FILE, "w");

    if (!err) {
        while (point[i].dev_addr != 0) {
            snprintf(line, 1024, "{\"port_info\": \"%s\", \"reg_type\": \"%s\", \"channel\": %d, \"dev_addr\": %d, \"reg_addr\": %d, \"period\": %d}\n", \
                point[i].port_info, point[i].reg_type, point[i].channel, point[i].dev_addr, point[i].reg_addr, point[i].period);
            fwrite(line, strlen(line), 1, fp);
            i++;
        }

        fclose(fp);
        free(line);
        return 0;
    }

    return -1;
}

/**
 * @brief 读取配置文件
 * 
 * @return int 
 */
int read_cfg(void) { 
    FILE* fp;
    char line[400];
    Json::Value params;
    Json::Reader m_reader;
    PPointInfo tPointInfo;
    bool parsing = false;
    int i = 0;
    errno_t err;

    tPointInfo = local_get_points();

    err = fopen_s(&fp, CFG_FILE, "r");

    if (!err) {
        while (fgets(line, 400, fp) != NULL) { 
            parsing = m_reader.parse(line, params);

            if (!parsing) { 
                continue; 
            }

            Json::Value port_info = params["port_info"];
            if (port_info != Json::Value::null) {
                // point information
                strncpy_s(tPointInfo[i].port_info, params["port_info"].asCString(), sizeof(tPointInfo[i].port_info) - 1);
                strncpy_s(tPointInfo[i].reg_type, params["reg_type"].asCString(), sizeof(tPointInfo[i].reg_type) - 1);
                tPointInfo[i].reg_addr = params["reg_addr"].asInt();
                tPointInfo[i].channel = params["channel"].asInt();
                tPointInfo[i].dev_addr = params["dev_addr"].asInt();
                tPointInfo[i].period = params["period"].asInt();
            
                i++;
            } else { 
                // MQTT information
            
            }

        }

        fclose(fp);
        return 0;
    }
    
    return -1;
}

uint8_t * read_file(char *file_name, int *file_len) {
    FILE* fp;
    int size;
    uint8_t *buf;

    errno_t err = fopen_s(&fp, file_name, "rb");

    if (err) {
        printf("can't open file %s\n", file_name);
        perror("Error opening file");
        return NULL;
    }

    // 将指针移至文件末尾，计算文件大小
    if (fseek(fp, 0, SEEK_END) != 0) {
        printf("can't fseek file %s\n", file_name);
        fclose(fp);
        return NULL;
    }

    size = ftell(fp);
    *file_len = size;
    printf("file size = %d\n", size);

    buf = (uint8_t *)malloc(size);

    if (!buf) {
        printf("can't malloc buf\n");
        fclose(fp);
        return NULL;
    }

    // 移回文件开头，二进制读取文件
    fseek(fp, 0, SEEK_SET);
    fread_s(buf, size, 1, size, fp);
        
    fclose(fp);
    return buf;
}
