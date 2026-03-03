//单片机头文件
#include "stm32f10x.h"
#include <time.h>


//网络设备驱动
#include "esp8266.h"

//硬件驱动
#include "bsp_delay.h"
#include "usart.h"
#include "oled.h"
#include "ds1302.h"

//C库
#include <string.h>
#include <stdio.h>
#include <stdlib.h> 


#define ESP8266_WIFI_INFO		"AT+CWJAP=\"name\",\"password\"\r\n"//wifi的账号和密码

// 拼多多时间API TCP连接配置
#define ESP8266_PDD_INFO      "AT+CIPSTART=\"TCP\",\"api.pinduoduo.com\",80\r\n"

// 定义透传结束符 (用于从透传模式退出)
#define EXIT_TRANSPARENT "+++"


unsigned char esp8266_buf[1024];
unsigned short esp8266_cnt = 0, esp8266_cntPre = 0;

// 时间存储缓冲区
char time_buf[32] = {0};

// 全局WiFi状态变量定义
WiFi_Status g_wifi_status = WIFI_DISCONNECTED;


//==========================================================
//	函数名称：	ESP8266_Clear
//
//	函数功能：	清空缓存
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP8266_Clear(void)
{

	memset(esp8266_buf, 0, sizeof(esp8266_buf));
	esp8266_cnt = 0;

}

//==========================================================
//	函数名称：	ESP8266_WaitRecive
//
//	函数功能：	等待接收完成
//
//	入口参数：	无
//
//	返回参数：	REV_OK-接收完成		REV_WAIT-接收超时未完成
//
//	说明：		循环调用检测是否接收完成
//==========================================================
_Bool ESP8266_WaitRecive(void)
{

	if(esp8266_cnt == 0) 							//如果接收计数为0 则说明没有处于接收数据中，所以直接跳出，结束函数
		return REV_WAIT;
		
	if(esp8266_cnt == esp8266_cntPre)				//如果上一次的值和这次相同，则说明接收完毕
	{
		esp8266_cnt = 0;							//清0接收计数
			
		return REV_OK;								//返回接收完成标志
	}
		
	esp8266_cntPre = esp8266_cnt;					//置为相同
	
	return REV_WAIT;								//返回接收未完成标志

}

//==========================================================
//	函数名称：	ESP8266_SendCmd
//
//	函数功能：	发送命令
//
//	入口参数：	cmd：命令
//				res：需要检查的返回指令
//
//	返回参数：	0-成功	1-失败
//
//	说明：		
//==========================================================
_Bool ESP8266_SendCmd(char *cmd, char *res)
{
	unsigned char timeOut = 200;

	Usart_SendString(USART2, (unsigned char *)cmd, strlen((const char *)cmd));
	
	while(timeOut--)
	{
		if(ESP8266_WaitRecive() == REV_OK)							//如果收到数据
		{
			if(strstr((const char *)esp8266_buf, res) != NULL)		//如果找到关键字
			{
				ESP8266_Clear();									//清空缓存
				return 0;											//返回成功
			}
		}
		
		DelayXms(10);
	}
	
	return 1;														//超时失败
}



//==========================================================
//	函数名称：	ESP8266_SendData
//
//	函数功能：	发送数据
//
//	入口参数：	data：数据
//				len：长度
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP8266_SendData(unsigned char *data, unsigned short len)
{

	char cmdBuf[32];
	
	ESP8266_Clear();								//清空接收缓存
	sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);		//发送命令
	if(!ESP8266_SendCmd(cmdBuf, ">"))				//收到‘>’时可以发送数据
	{
		Usart_SendString(USART2, data, len);		//发送设备连接请求数据
	}

}



_Bool ESP8266_Sync_To_DS1302(void)
{
    UsartPrintf(USART_DEBUG, "[Sync] Start sync time to DS1302...\r\n");
    
    // 1. 确保TCP连接已建立
    if(ESP8266_SendCmd("AT+CIPSTATUS\r\n", "CONNECTED") != 0) {
        // TCP未连接，尝试重新连接
        UsartPrintf(USART_DEBUG, "[Sync] TCP not connected, reconnecting...\r\n");
        if(ESP8266_SendCmd(ESP8266_PDD_INFO, "CONNECT") != 0) {
            UsartPrintf(USART_DEBUG, "[Sync] TCP connect failed\r\n");
            return 1;
        }
        DelayXms(500);
    }
    
    // 2. 发送HTTP请求（使用更精确的API）
    char *http_req = "GET /api/server/_stm HTTP/1.1\r\n"
                     "Host: api.pinduoduo.com\r\n"
                     "User-Agent: STM32/ESP8266\r\n"
                     "Accept: */*\r\n"
                     "Connection: close\r\n\r\n";
    
    char send_cmd[32];
    sprintf(send_cmd, "AT+CIPSEND=%d\r\n", strlen(http_req));
    
    ESP8266_Clear();
    if(ESP8266_SendCmd(send_cmd, ">") == 0) {
        Usart_SendString(USART2, (unsigned char *)http_req, strlen(http_req));
        UsartPrintf(USART_DEBUG, "[Sync] HTTP request sent, waiting for response...\r\n");
        
        // 3. 等待响应（增加等待时间）
        int retry = 0;
        while(retry < 50) { // 5秒超时
            DelayXms(100);
            if(strstr((char *)esp8266_buf, "server_time") != NULL) {
                break; // 找到了时间戳
            }
            retry++;
        }
        
        if(retry >= 50) {
            UsartPrintf(USART_DEBUG, "[Sync] Timeout waiting for response\r\n");
            return 1;
        }
        
        // 4. 解析时间戳并写入DS1302
        char *ptr = strstr((char *)esp8266_buf, "server_time");
        if(ptr) {
            ptr = strchr(ptr, ':');
            if(ptr) {
                ptr++; // 跳过冒号
                // 跳过空格和引号
                while(*ptr == ' ' || *ptr == '\"' || *ptr == '\'') ptr++;
                
                // 提取13位时间戳（毫秒）
                long long ts_ms = atoll(ptr);
                UsartPrintf(USART_DEBUG, "[Sync] Timestamp ms: %lld\r\n", ts_ms);
                
                if(ts_ms > 0) {
                    // 毫秒转秒，加8小时（北京时间UTC+8）
                    time_t rawtime = (time_t)(ts_ms / 1000) + 28800;
                    struct tm *timeinfo;
                    timeinfo = localtime(&rawtime);
                    
                    // 填充时间结构体
                    TimeData.year   = timeinfo->tm_year + 1900;
                    TimeData.month  = timeinfo->tm_mon + 1;
                    TimeData.day    = timeinfo->tm_mday;
                    TimeData.hour   = timeinfo->tm_hour;
                    TimeData.minute = timeinfo->tm_min;
                    TimeData.second = timeinfo->tm_sec;
                    TimeData.week   = (timeinfo->tm_wday == 0) ? 7 : timeinfo->tm_wday;
                    
                    UsartPrintf(USART_DEBUG, "[Sync] Parsed time: %04d-%02d-%02d %02d:%02d:%02d\r\n",
                                TimeData.year, TimeData.month, TimeData.day,
                                TimeData.hour, TimeData.minute, TimeData.second);
                    
                    // 写入DS1302
                    DS1302_SetTime();
                    
                    // 验证写入
                    DS1302_read_realTime();
                    UsartPrintf(USART_DEBUG, "[Sync] DS1302 time after write: %02d:%02d:%02d\r\n",
                                TimeData.hour, TimeData.minute, TimeData.second);
                    
                    return 0; // 成功
                }
            }
        }
    }
    
    UsartPrintf(USART_DEBUG, "[Sync] Sync failed\r\n");
    return 1; // 失败
}


//==========================================================
// 函数名: ESP8266_ParseTime
// 功能: 从HTTP响应中解析时间字符串
// 返回值: 0-解析成功 1-解析失败
//==========================================================
_Bool ESP8266_ParseTime(void)
{
    char *ptr = strstr((char *)esp8266_buf, "\"server_time\":");
    if(ptr == NULL) return 1;
    
    ptr = strchr(ptr, ':');
    if(ptr == NULL) return 1;
    ptr++; 
    
    while(*ptr == ' ' || *ptr == '\"') ptr++; // 跳过空格和引号
    
    // atoll 需要 #include <stdlib.h>
    long long timestamp_ms = atoll(ptr); 
    if(timestamp_ms == 0) return 1;

    // 毫秒转秒，并加 8 小时 (28800秒)
    unsigned int seconds = (unsigned int)(timestamp_ms / 1000) + 28800;
    
    unsigned int h = (seconds / 3600) % 24;
    unsigned int m = (seconds % 3600) / 60;
    unsigned int s = seconds % 60;
    
    sprintf(time_buf, "%02d:%02d:%02d", h, m, s);
    return 0;
}


void ESP8266_Init(void)
{
    UsartPrintf(USART_DEBUG, "\r\n[System] WiFi Sync Start...\r\n");
    
    // 1. 关闭回显，防止响应数据混入指令字符串
    ESP8266_SendCmd("ATE0\r\n", "OK");
    
    // 2. 基础配置
    ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK");
    ESP8266_SendCmd("AT+CIPMUX=0\r\n", "OK");

    // 3. 连接 WiFi (增加超时判断)
    UsartPrintf(USART_DEBUG, "Connecting to WiFi...");
    uint32_t wifi_connect_timeout = 0;  // WiFi连接超时计数器（单位10ms）
    while(ESP8266_SendCmd(ESP8266_WIFI_INFO, "GOT IP")) {
        DelayXms(500);
        wifi_connect_timeout++;
        
        if(wifi_connect_timeout >= 20) {  // 500ms * 20 = 10秒超时
            UsartPrintf(USART_DEBUG, " [TIMEOUT]\r\n");
            g_wifi_status = WIFI_DISCONNECTED; // 标记WiFi未连接
            return; // 退出初始化，进入本地时间显示逻辑
        }
    }
    UsartPrintf(USART_DEBUG, " [OK]\r\n");
    g_wifi_status = WIFI_CONNECTED; // 标记WiFi已连接

    // 4. 建立TCP连接
    UsartPrintf(USART_DEBUG, "Connecting to PDD API...");
    while(ESP8266_SendCmd("AT+CIPSTART=\"TCP\",\"api.pinduoduo.com\",80\r\n", "CONNECT")) {
        DelayXms(1000);
    }
    UsartPrintf(USART_DEBUG, " [OK]\r\n");

    // 5. 使用普通发送模式
    char *http_req = "GET /api/server/_stm HTTP/1.1\r\nHost: api.pinduoduo.com\r\nConnection: close\r\n\r\n";
    char send_cmd[32];
    sprintf(send_cmd, "AT+CIPSEND=%d\r\n", strlen(http_req));
    
    ESP8266_Clear();
    if(ESP8266_SendCmd(send_cmd, ">") == 0) {
        Usart_SendString(USART2, (unsigned char *)http_req, strlen(http_req));
        UsartPrintf(USART_DEBUG, "HTTP Request Sent.\r\n");
    } else {
        UsartPrintf(USART_DEBUG, "CIPSEND Failed.\r\n");
        g_wifi_status = WIFI_DISCONNECTED;
        return;
    }

    // 6. 等待并解析
    DelayXms(1200);
    if(ESP8266_ParseTime() == 0) {
        UsartPrintf(USART_DEBUG, "[SUCCESS] Time: %s\r\n", time_buf);
//        OLED_Clear();
//        OLED_ShowString(0, 0, (uint8_t *)"WIFI Connected", 8, 1);
//        OLED_ShowString(10, 8, (uint8_t *)time_buf, 16, 1);
//        OLED_Refresh();
        g_wifi_status = WIFI_CONNECTED;
    } else {
        UsartPrintf(USART_DEBUG, "[FAIL] No time data found.\r\n");
        g_wifi_status = WIFI_DISCONNECTED;
    }
}




//==========================================================
//	函数名称：	USART2_IRQHandler
//
//	函数功能：	串口2收发中断
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void USART2_IRQHandler(void)
{
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        uint8_t res = USART_ReceiveData(USART2);
        // 只有缓冲区没满时才接收，防止指针越界，也不要自动清零
        if(esp8266_cnt < 1023) 
        {
            esp8266_buf[esp8266_cnt++] = res;
        }
        USART_ClearFlag(USART2, USART_FLAG_RXNE);
    }
}
