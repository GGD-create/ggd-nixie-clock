#ifndef __LED_H
#define __LED_H

#include "stm32f10x.h"

void DOT_Init(void);
void DOT1_ON(void);
void DOT2_ON(void);
void DOT3_ON(void);
void DOT4_ON(void);
void DOTs_ON(void);
void DOTs_ON_In_Order(uint32_t ms);

void DOT1_OFF(void);
void DOT2_OFF(void);
void DOT3_OFF(void);
void DOT4_OFF(void);
void DOTs_OFF(void);

void NE_ON(void);
void NE_OFF(void);

void DOT_StartWaterfall(void);      // 启动流水灯（重置状态机）
void DOT_StopWaterfall(void);       // 停止流水灯，全部熄灭
void DOT_WaterfallHandler(void);    // 流水灯状态机（在定时器中断中调用）
uint8_t DOT_IsWaterfallRunning(void); // 查询流水灯是否运行中


#endif
