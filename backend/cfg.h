#ifndef _CFG_H
#define _CFG_H

#define ENABLE_CFG_WRITE       1


/* 初始化: 监测配置文件
 * 内部创建一个inotify文件句柄, 以后可以使用poll函数查询这个句柄
 * 返回值: 0-成功, (-1)-失败
 */
int cfg_inotify_init(void);

/* 监测配置文件
 * timeout_ms - 超时时间(ms)
 * 返回值: 0-文件被创建或被修改了, (-1)-文件没有被创建或修改
 */
int cfg_inotify_poll(int timeout_ms);


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

/* 设置更新百分比
 */
void local_set_update_percent(int percent);


#endif /* _CFG_H */

