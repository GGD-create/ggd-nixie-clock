#include "stm32f10x.h"                  // Device header
#include "test_595_simple.h"
#include "LED.h"



void Timer3_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);  // 使能TIM3时钟
    
    TIM_InternalClockConfig(TIM3);  // 使用内部时钟
    
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;  // 时钟分频
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;  // 向上计数模式
    TIM_TimeBaseInitStructure.TIM_Period = 1000 - 1;  // 自动重装载值
    TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;  // 预分频器
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;  // 重复计数器（高级定时器用）
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);  // 初始化TIM3
    
    TIM_ClearFlag(TIM3, TIM_FLAG_Update);  // 清除更新标志
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);  // 使能更新中断
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  // 中断优先级分组
    
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  // TIM3中断通道
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;  // 使能中断通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;  // 抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;  // 子优先级
    NVIC_Init(&NVIC_InitStructure);  // 初始化中断配置
    
    TIM_Cmd(TIM3, ENABLE);  // 使能TIM3
}

/*
// TIM3中断服务函数模板
void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET)
    {
        // 中断处理代码
        
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  // 清除中断标志
    }
}
*/

void Timer4_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);  // 使能TIM4时钟
    
    TIM_InternalClockConfig(TIM4);  // 使用内部时钟
    
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;  // 时钟分频
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;  // 向上计数模式
    TIM_TimeBaseInitStructure.TIM_Period = 1000 - 1;  // 自动重装载值，1ms定时
    TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;  // 预分频器，72分频
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;  // 重复计数器（高级定时器用）
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);  // 初始化TIM4
    
    TIM_ClearFlag(TIM4, TIM_FLAG_Update);  // 清除更新标志
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);  // 使能更新中断
    
    // 注意：NVIC优先级分组只需配置一次，建议在主初始化函数中统一配置
    // NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  // 已在其他定时器初始化中配置
    
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;  // TIM4中断通道
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;  // 使能中断通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;  // 抢占优先级，可根据需要调整
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;  // 子优先级，与其他定时器区分
    NVIC_Init(&NVIC_InitStructure);  // 初始化中断配置
    
    TIM_Cmd(TIM4, ENABLE);  // 使能TIM4
}




// TIM4中断服务函数模板
void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET)
    {
        static uint32_t neon_update_timer = 0;
        static uint32_t nixie_update_timer = 0;
        
        // 更新时间计数器（每次中断1ms）
        neon_update_timer += 1;    // 用于氖管控制
        nixie_update_timer += 1;   // 用于辉光管更新
        
        // 氖管控制：每1秒切换一次
        if(neon_update_timer >= 1000)  // 1000ms = 1秒
        {
            HC595_NeonControl(1000);  // 传递1000ms作为时间增量
            neon_update_timer = 0;
        }
        
        // 辉光管时间更新：每100ms更新一次
        if(nixie_update_timer >= 100)  // 100ms
        {
            // 更新辉光管时间显示
            HC595_UpdateTimeFromDS1302();
            HC595_ShowTimeOnNixie();
            nixie_update_timer = 0;
        }
		
		DOT_WaterfallHandler();     // 每1ms调用一次，内部计时决定步进
		
        
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);  // 清除中断标志
    }
}

