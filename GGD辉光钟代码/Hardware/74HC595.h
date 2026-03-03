#ifndef __74HC595_H
#define __74HC595_H


#include "stm32f10x.h"

//// 定义引脚
//#define NIXIE_PORT      GPIOA
//#define NIXIE_PIN_DATA  GPIO_Pin_4  // DS
//#define NIXIE_PIN_SCK   GPIO_Pin_5  // SHCP (Shift Clock)
//#define NIXIE_PIN_RCK   GPIO_Pin_6  // STCP (Latch Clock)

//// 定义时钟
//#define NIXIE_RCC       RCC_APB2Periph_GPIOA

//// 函数声明
//void Nixie_Init(void);
//void Nixie_Display(uint8_t num1, uint8_t num2, uint8_t num3, uint8_t num4);
//void Nixie_AntiPoison_Random(uint16_t duration_sec);

// 硬件引脚定义 (保持你的原样)
#define NIXIE_PORT      GPIOA
#define NIXIE_RCC       RCC_APB2Periph_GPIOA
#define NIXIE_PIN_DATA  GPIO_Pin_4  // DS
#define NIXIE_PIN_SCK   GPIO_Pin_5  // SHCP (Shift Clock)
#define NIXIE_PIN_RCK   GPIO_Pin_6  // STCP (Latch Clock)

// 函数声明
void Nixie_Init(void);
void Nixie_Display_Time(uint8_t hour, uint8_t minute); // 核心显示函数
void Nixie_Clean(void); // 全灭
void Nixie_AntiPoison(void); // 防中毒




#endif

