#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Relay.h"

void Relay_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						
	
//	/*设置GPIO初始化后的默认电平*/
//	GPIO_SetBits(GPIOA, GPIO_Pin_9);
	GPIO_ResetBits(GPIOA, GPIO_Pin_9);
}

void Relay_OFF(void)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_9);		
}

void Relay_ON(void)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_9);		
}

/**
 * @brief  初始化ESP01S复位引脚，执行复位操作
 * @note   单片机复位后调用此函数：先输出低电平（触发ESP复位），再设为浮空
 * @param  无
 * @retval 无
 */
void ESP_RST_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(ESP_RST_CLK, ENABLE);

    //配置PB10为推挽输出模式（先输出低电平）
    GPIO_InitStructure.GPIO_Pin = ESP_RST_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 输出速率
    GPIO_Init(ESP_RST_PORT, &GPIO_InitStructure);

    //输出低电平（触发ESP01S复位）
    GPIO_ResetBits(ESP_RST_PORT, ESP_RST_PIN);

    //保持低电平（时间可通过宏定义调整，默认10ms）
    Delay_ms(ESP_RST_LOW_TIME_MS);

    //复位完成，将引脚设为浮空输入（释放引脚，不影响ESP01S）
    ESP_RST_Set_Floating();
}

/**
 * @brief  将ESP_RST引脚设为浮空输入模式
 * @note   释放引脚，避免STM32持续驱动影响ESP01S正常工作
 * @param  无
 * @retval 无
 */
void ESP_RST_Set_Floating(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    //配置PB10为浮空输入模式
    GPIO_InitStructure.GPIO_Pin = ESP_RST_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //浮空输入
    GPIO_Init(ESP_RST_PORT, &GPIO_InitStructure);
}
