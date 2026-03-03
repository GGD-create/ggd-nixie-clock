#ifndef _ESP8266_H_
#define _ESP8266_H_


#define REV_OK		0	//接收完成标志
#define REV_WAIT	1	//接收未完成标志



// 新增：WiFi连接状态枚举
typedef enum {
    WIFI_DISCONNECTED = 0,  // 未连接
    WIFI_CONNECTED = 1      // 已连接
} WiFi_Status;

// 新增：全局WiFi状态变量声明
extern WiFi_Status g_wifi_status;



void ESP8266_Init(void);

void ESP8266_Clear(void);

_Bool ESP8266_SendCmd(char *cmd, char *res);

void ESP8266_SendData(unsigned char *data, unsigned short len);

//unsigned char *ESP8266_GetIPD(unsigned short timeOut);

_Bool ESP8266_ParseTime(void);

_Bool ESP8266_Sync_To_DS1302(void);

#endif
