#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

/* 按键初始化（仅配置GPIO） */
void Key_Init(void);

/* 定时器中断中调用的按键扫描函数（由TIM3_IRQHandler调用） */
void Key_Scan(void);

/* 主循环获取按键值（非阻塞，读取全局标志） */
uint8_t Key_GetNum(void);

#endif