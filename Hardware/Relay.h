#ifndef __RELAY_H
#define __RELAY_H

// ESP01S复位引脚定义（PB10）
#define ESP_RST_PIN    GPIO_Pin_10
#define ESP_RST_PORT   GPIOB
#define ESP_RST_CLK    RCC_APB2Periph_GPIOB

// 复位低电平持续时间（可自定义，默认10ms，足够触发ESP01S复位）
#define ESP_RST_LOW_TIME_MS  10

// 函数声明
void ESP_RST_Init(void);       // 初始化ESP复位引脚并执行复位操作
void ESP_RST_Set_Floating(void);// 将RST引脚设为浮空输入（释放引脚）

void Relay_Init(void);
void Relay_ON(void);
void Relay_OFF(void);

#endif
