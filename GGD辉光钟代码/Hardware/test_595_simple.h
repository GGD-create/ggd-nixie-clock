#ifndef __TEST_595_SIMPLE_H
#define __TEST_595_SIMPLE_H

#include "stm32f10x.h"

// 74HC595引脚定义 - 只需要3个控制引脚
#define HC595_DS_PORT          GPIOA
#define HC595_DS_PIN           GPIO_Pin_4

#define HC595_SHCP_PORT        GPIOA
#define HC595_SHCP_PIN         GPIO_Pin_6

#define HC595_STCP_PORT        GPIOA
#define HC595_STCP_PIN         GPIO_Pin_5

#define HC595_OE_PORT        GPIOA
#define HC595_OE_PIN         GPIO_Pin_7


// 总共控制16个LED（2个595，每个8位）
#define HC595_CHIP_COUNT       5
#define HC595_TOTAL_LEDS       40


// 辉光管相关定义
#define NIXIE_TUBE_COUNT       4      // 4个辉光管
#define DIGITS_PER_TUBE        10     // 每个辉光管10个数字（0-9）
#define NIXIE_TOTAL_DIGITS     40     // 总共40个数字引脚（4×10）


// 函数声明
void HC595_Init(void);
void HC595_WriteData(uint8_t chip1_data, uint8_t chip2_data, uint8_t chip3_data, 
                     uint8_t chip4_data, uint8_t chip5_data); 

void HC595_SetLED(uint8_t led_num, uint8_t state);
void HC595_ToggleLED(uint8_t led_num);
void HC595_ClearAll(void);

void HC595_ShiftLeft(void);
void HC595_ShiftRight(void);
void HC595_RotateLeft(void);
void HC595_RotateRight(void);
void HC595_SetPattern40(uint64_t pattern); // 新增：40位图案设置（使用64位存储）
void HC595_SetPattern32(uint32_t pattern); // 新增：32位图案设置
void HC595_SetPattern24(uint32_t pattern); // 新增：24位图案设置（兼容前三个芯片）


// 辉光管专用函数
void HC595_SetNixieTube(uint8_t tube1_num, uint8_t tube2_num, uint8_t tube3_num, uint8_t tube4_num);
void HC595_SetNixieTubeByIndex(uint8_t tube_index, uint8_t number);
void HC595_ClearNixieTube(uint8_t tube_index);
void HC595_ClearAllNixieTubes(void);
void HC595_ShowNumber(uint16_t number);  // 显示4位数字


void HC595_0_9_In_Order(uint32_t ms);
void HC595_RandomNixieShow(uint32_t ms);
void HC595_RandomNixieShowTimes(uint32_t ms, uint16_t times);

// 新辉光管时间显示功能
void HC595_ShowTimeOnNixie(void);
void HC595_StartTimeDisplay(void);
void HC595_StopTimeDisplay(void);
void HC595_UpdateTimeFromDS1302(void);
void HC595_NeonControl(uint32_t elapsed_ms);
uint8_t HC595_GetNeonState(void);

// 辉光管防中毒功能
void HC595_AntiPoisonStart(void);
void HC595_AntiPoisonStop(void);
uint8_t HC595_AntiPoisonIsRunning(void);
void HC595_AntiPoisonUpdate(void);


extern struct TIMEData TimeData;

#endif 