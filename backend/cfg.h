#ifndef _CFG_H
#define _CFG_H

#define ENABLE_CFG_WRITE       1


/* 读取配置文件 
 * 配置文件里有如下多行信息:
 * {"port_info": "/dev/ttyUSB0,115200,8n1", "dev_addr": 3, "reg_addr": 0, "reg_type": "0x", "period": 100}
 */
int read_cfg(void);

/* {"port_info": "/dev/ttyUSB0,115200,8n1", "dev_addr": 3, "reg_addr": 0, "reg_type": "0x", "period": 100} */
/* 写配置文件 
 */
int write_cfg(void);

/* 读取文件 
 */
uint8_t * read_file(char *file_name, int *file_len);


#endif /* _CFG_H */

