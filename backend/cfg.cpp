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
int write__cfg(void) {
    FILE *fp = fopen(CFG_FILE, "w");
    int i = 0;
    char *line = (char *)malloc(1024);
    PPointInfo point = local_get_points();
    
    if (fp != NULL) {
        while (point[i]->dev_addr != 0) {
            snprintf(line, 1024, "{\"port_info\": \"%s\", \"reg_type\": \"%s\", \"channel\": %d, \"dev_addr\": %d, \"reg_addr\": \"%s\", \"period\": %d, \"reg_addr_master\": %d}\n", \
                point[i]->port_info, point[i]->channel, point[i]->dev_addr, point[i]->reg_addr, point[i]->reg_type, point[i]->period);
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
    FILE *fp = fopen(CFG_FILE, "r");
    Json::Value params;
    Json::Reader m_reader;
    PPointInfo tPointInfo;
    bool parsing = false;
    int i = 0;

    tPointInfo = local_get_points();

    if (fp != NULL) { 
        while (fgets(line, 400, fp) != NULL) { 
            parsing = m_reader.parse(line, params);

            if (!parsing) { 
                continue; 
            }

            Json::Value port_info = params["port_info"];
            if (port_info != Json::Value::null) {
                // point information
                strcpy(tPointInfo[i].port_info, params["port_info"].asCString());
                strcpy(tPointInfo[i].reg_type, params["reg_type"].asCString());
                tPointInfo[i].reg_addr_master = params["reg_addr_master"].asInt();
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