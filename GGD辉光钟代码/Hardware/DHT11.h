#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"

 /**************引脚修改此处****************/
#define RCU_DHT11   RCC_APB2Periph_GPIOA
#define PORT_DHT11  GPIOA
#define GPIO_DHT11  GPIO_Pin_8

//设置DHT11输出高或低电平
#define DATA_GPIO_OUT(x)    GPIO_WriteBit(PORT_DHT11, GPIO_DHT11, x ? Bit_SET : Bit_RESET)
//获取DHT11数据引脚高低电平状态
#define DATA_GPIO_IN        GPIO_ReadInputDataBit(PORT_DHT11, GPIO_DHT11)

extern float temperature;
extern float humidity;

void DHT11_GPIO_Init(void);//引脚初始化
unsigned int DHT11_Read_Data(void);//读取模块数据
float Get_temperature(void);//返回读取模块后的温度数据
float Get_humidity(void);//返回读取模块后的湿度数据

void Float2Str(float num, char *str);

#endif