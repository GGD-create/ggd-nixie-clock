#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "bsp_delay.h"
#include "oled.h"
#include "DHT11.h"
#include "usart.h"
#include "Relay.h"
#include "ds1302.h"
#include "esp8266.h"
#include "Timer.h"
#include <stdio.h>
#include "test_595_simple.h"
#include "LED.h"
#include "Key.h"

#define ANTI_POISON_INTERVAL_MS 6000000  //防中毒间隔

uint32_t g_sync_timer = 0;   // 计时变量
uint8_t  g_need_sync = 0;    // 对时标志位，1表示需要对时

uint32_t g_wifi_reconnect_cnt = 0;  // WiFi重连计数器
 
//volatile 修饰，防止编译器优化
volatile uint32_t g_wifi_reconnect_timer = 0;


int main(void)
{
	
	/*模块初始化*/
	OLED_Init();		//OLED初始化

	Delay_Init();
	Usart_Init();
	Timer3_Init();
	Timer4_Init();

	Relay_Init();
	Relay_OFF();
	
	char temp_str[10], humi_str[10];  //存储温湿度字符串
	
	DHT11_GPIO_Init();    //DHT11引脚初始化
	DS1302_GPIO_Init();
	DS1302_Init();
	ESP_RST_Init();
	Key_Init();
	
	
	DOT_Init();
	HC595_Init();
		 
	Delay_ms(1500);
    Relay_ON();
	
	DS1302_read_realTime();   //读取实时数据

	OLED_ShowString(0, 0, (uint8_t *)"Connecting WiFi", 8, 1);
	OLED_ShowString(32, 8, (uint8_t *)"--:--:--", 16, 1);
	OLED_Refresh();
	
	HC595_ClearAll();
	Delay_ms(500);
	
	HC595_0_9_In_Order(200);

	DOT_StartWaterfall();      // 启动流水灯
	
	ESP8266_Init(); 
	
	DOT_StopWaterfall();       // 无论成功/失败，都停止流水灯
	
	HC595_StartTimeDisplay();
	
	// 如果初始化时WiFi已连接，立即同步时间到DS1302
	if(g_wifi_status == WIFI_CONNECTED) {
		// 增加重试机制
		int retry_count = 0;
		while(retry_count < 3) {
			UsartPrintf(USART_DEBUG, "[Main] Attempt %d to sync time...\r\n", retry_count + 1);
			if(ESP8266_Sync_To_DS1302() == 0) {
				UsartPrintf(USART_DEBUG, "[Main] Time sync successful\r\n");
				break;
			}
			retry_count++;
			Delay_ms(1000);
		}
		
		if(retry_count >= 3) {
			UsartPrintf(USART_DEBUG, "[Main] Time sync failed after 3 attempts\r\n");
		}
	}

	// 读取并显示DS1302时间
	DS1302_read_realTime();
	UsartPrintf(USART_DEBUG, "[Main] Current DS1302 time: %02d:%02d:%02d\r\n",
				TimeData.hour, TimeData.minute, TimeData.second);
	
	
	OLED_Clear();


	while (1) 
    {
		/* 1. 始终读取并刷新显示 */
        DS1302_read_realTime(); // 读取 DS1302 内部时间
		
 	
        char time_str[20];
        sprintf(time_str, "%02d:%02d:%02d", TimeData.hour, TimeData.minute, TimeData.second);
        
        // 更新温湿度和时间显示
        DHT11_Read_Data();
		Float2Str(Get_temperature(), temp_str);
        OLED_ShowString(0+16, 25, (uint8_t *)"T:", 8, 1);
        OLED_ShowString(16+16, 25,  (uint8_t *)temp_str, 8, 1);
      
        Float2Str(Get_humidity(), humi_str);
		OLED_ShowString(48+16, 25, (uint8_t *)"H:", 8, 1);
        OLED_ShowString(64+16, 25,  (uint8_t *)humi_str, 8, 1);
		
      
        if(g_wifi_status == WIFI_DISCONNECTED) {
            OLED_ShowString(0, 0, (uint8_t *)"WiFi:Not connected", 8, 1);
        } else {
            OLED_ShowString(0, 0, (uint8_t *)"WiFi:connected    ", 8, 1);
        }
        OLED_ShowString(32, 8, (uint8_t *)time_str, 16, 0);
        OLED_Refresh();

        /* 2. 处理重连逻辑 (每10分钟秒尝试一次) */
        if(g_wifi_status == WIFI_DISCONNECTED && g_wifi_reconnect_timer >= 600000) 
        {
            g_wifi_reconnect_timer = 0; // 重置计时器
            // 提示用户正在尝试后台重连
            OLED_ShowString(0, 0, (uint8_t *)"Reconnecting...", 8, 1);
            OLED_Refresh();
            
            ESP8266_Init(); 
            
            if(g_wifi_status == WIFI_CONNECTED) {
                ESP8266_Sync_To_DS1302();
            }
        }

        /* 3. 处理定时同步 (WiFi 连通时每 10 分钟一次) */
        if (g_need_sync == 1 && g_wifi_status == WIFI_CONNECTED)
        {
            ESP8266_Sync_To_DS1302(); //
            g_need_sync = 0;
        }

        Delay_ms(100); //主循环延时，给系统呼吸空间
    }
	
}

//74HC595测试程序
//int main(void)
//{
//	DOT_Init();
//	uint8_t i;
//	
//	
//	Relay_Init();
//	Relay_OFF();
//	Delay_ms(3000);
//	Relay_ON();
//	
//    
//    
//    HC595_Init();
//    
//    // 测试1：逐个辉光管测试数字0-9
//    for(i = 0; i < 4; i++)
//    {
//        uint8_t j;
//        for(j = 0; j < 10; j++)  // 0-9
//        {
//            HC595_SetNixieTubeByIndex(i, j);
//            Delay_ms(100);
//        }
//        HC595_ClearNixieTube(i);

//    }
//    
//	
//	
//    while(1)
//    {
//		HC595_ClearAll();
//        
//		HC595_SetNixieTube(1, 2, 3, 4);
//		Delay_ms(5000);
//		HC595_SetNixieTube(5, 6, 7, 8);
//      Delay_ms(5000);
//      HC595_SetNixieTube(0, 0, 0, 0);
//		Delay_ms(5000);
//		
//		HC595_ClearAll();
//		Delay_ms(1000);
//		HC595_0_9_In_Order(100);
//		Delay_ms(1000);
//		HC595_RandomNixieShowTimes(80, 100);
//        
//    }

//	
//}


// 必须实现中断服务函数
void TIM3_IRQHandler(void)
{
	static uint32_t anti_poison_interval_counter = 0;  // 防中毒间隔计数器
	
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
    {
        g_sync_timer++;            // 用于 10 分钟同步
        g_wifi_reconnect_timer++;  // 用于断网重连计数
        
        if(g_sync_timer >= 600000) // 10 分钟 = 600,000ms
        {
            g_sync_timer = 0;
            g_need_sync = 1;
        }
		
		 Key_Scan();
	
		//防中毒模式
        HC595_AntiPoisonUpdate();
        
        //检查是否到达防中毒间隔时间
        anti_poison_interval_counter += 10;  // 每次中断增加10ms
        
        //防中毒间隔时间应大于防中毒模式持续时间
        if(anti_poison_interval_counter >= ANTI_POISON_INTERVAL_MS && 
           !HC595_AntiPoisonIsRunning())
        {
            //启动防中毒模式
            HC595_AntiPoisonStart();
            anti_poison_interval_counter = 0;  // 重置间隔计数器
            
        }
		
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}

