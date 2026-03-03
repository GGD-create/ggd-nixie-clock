#ifndef __USART_H
#define __USART_H

#include "stm32f10x.h"
#include <stdio.h>




#define USART_DEBUG		USART1		//调试打印所使用的串口组


void Usart1_Init(unsigned int baud);

void Usart2_Init(unsigned int baud);

void Usart_SendString(USART_TypeDef *USARTx, unsigned char *str, unsigned short len);

void UsartPrintf(USART_TypeDef *USARTx, char *fmt,...);

void Usart_Init();


#endif

