#include "stm32f10x.h"                  // Device header
#include "LED.h"
#include "Delay.h"


static uint8_t waterfall_running = 0;   // 0: 停止, 1: 运行
static uint8_t waterfall_step = 0;      // 当前步骤 0~7
static uint16_t waterfall_timer = 0;    // 步骤计时器（毫秒）

#define WATERFALL_INTERVAL_MS  200      // 每步持续时间 200ms


/**
  * 函    数：LED初始化
  * 参    数：无
  * 返 回 值：无
  */
void DOT_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15| GPIO_Pin_6 | GPIO_Pin_7 ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);						
	
	/*设置GPIO初始化后的默认电平*/
	GPIO_ResetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15 
	                                        | GPIO_Pin_6 | GPIO_Pin_7);			//low电平
}


//ON
void DOT1_ON(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_12);		
}

void DOT2_ON(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_13);		
}

void DOT3_ON(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_14);		
}

void DOT4_ON(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_15);		
}

void DOTs_ON(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_12);	
	GPIO_SetBits(GPIOB, GPIO_Pin_13);	
	GPIO_SetBits(GPIOB, GPIO_Pin_14);
	GPIO_SetBits(GPIOB, GPIO_Pin_15);	
}

void DOTs_ON_In_Order(uint32_t ms)
{	
	DOT4_ON();
	Delay_ms(ms);
	DOT4_OFF();
	
	DOT3_ON();
	Delay_ms(ms);
	DOT3_OFF();
	
	DOT2_ON();
	Delay_ms(ms);
	DOT2_OFF();
	
	DOT1_ON();
	Delay_ms(ms);
	DOT1_OFF();
}



//OFF
void DOT1_OFF(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_12);		
}

void DOT2_OFF(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_13);		
}

void DOT3_OFF(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_14);		
}

void DOT4_OFF(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_15);		
}


void DOTs_OFF(void)
{
	DOT1_OFF();	
	DOT2_OFF();	
	DOT3_OFF();	
	DOT4_OFF();	
}

void NE_ON(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_6);	
	GPIO_SetBits(GPIOB, GPIO_Pin_7);		
}

void NE_OFF(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_6);	
	GPIO_ResetBits(GPIOB, GPIO_Pin_7);		
}




/**
  * 函    数：启动流水灯（重置状态机）
  */
void DOT_StartWaterfall(void)
{
    waterfall_running = 1;
    waterfall_step = 0;
    waterfall_timer = 0;
    DOTs_OFF();                         // 初始全灭
}

/**
  * 函    数：停止流水灯，全部熄灭
  */
void DOT_StopWaterfall(void)
{
    waterfall_running = 0;
    DOTs_OFF();
}

/**
  * 函    数：查询流水灯是否运行中
  */
uint8_t DOT_IsWaterfallRunning(void)
{
    return waterfall_running;
}


/**
  * 函    数：流水灯状态机（应在 1ms 定时器中断中调用）
  * 说    明：每 WATERFALL_INTERVAL_MS 切换一次状态
  */
void DOT_WaterfallHandler(void)
{
    if (!waterfall_running) return;
    
    waterfall_timer++;
    if (waterfall_timer < WATERFALL_INTERVAL_MS) return;
    
    waterfall_timer = 0;
    
    /* 状态机：0~3 依次点亮，4~7 依次熄灭 */
    switch (waterfall_step)
    {
        case 0: DOT4_ON(); break;   // 第一个灯亮
        case 1: DOT3_ON(); break;   // 第二个灯亮
        case 2: DOT2_ON(); break;   // 第三个灯亮
        case 3: DOT1_ON(); break;   // 第四个灯亮
        case 4: DOT4_OFF(); break;  // 第一个灯灭
        case 5: DOT3_OFF(); break;  // 第二个灯灭
        case 6: DOT2_OFF(); break;  // 第三个灯灭
        case 7: DOT1_OFF(); break;  // 第四个灯灭
        default: break;
    }
    
    waterfall_step++;
    if (waterfall_step >= 8) waterfall_step = 0;
}


