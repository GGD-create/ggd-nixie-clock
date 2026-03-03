#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "test_595_simple.h"
#include "LED.h"

/* ---------- 按键引脚定义（与硬件一致） ---------- */
#define KEY1_PIN    GPIO_Pin_1
#define KEY2_PIN    GPIO_Pin_0
#define KEY_PORT    GPIOB

/* ---------- 按键状态机相关变量（静态全局） ---------- */
static uint8_t key1_state = 0;      // 0: 空闲, 1: 消抖中, 2: 按下确认
static uint8_t key2_state = 0;
static uint8_t key1_pressed = 0;    // 按下事件标志（仅持续一个扫描周期）
static uint8_t key2_pressed = 0;
static uint16_t key1_debounce_cnt = 0;   // 消抖计数器
static uint16_t key2_debounce_cnt = 0;

#define DEBOUNCE_TIME   2           // 消抖时间 = 2 * 定时器周期（若TIM3为10ms，则20ms）

/**
  * 函    数：按键GPIO初始化（只需配置一次）
  */
void Key_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = KEY1_PIN | KEY2_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;   // 上拉输入，按键接GND
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(KEY_PORT, &GPIO_InitStructure);
}

/**
  * 函    数：按键扫描状态机（在TIM3中断中每10ms调用一次）
  * 说    明：完全非阻塞，通过全局按键事件标志传递按下信息
  */
void Key_Scan(void)
{
    uint8_t key1_level = GPIO_ReadInputDataBit(KEY_PORT, KEY1_PIN);
    uint8_t key2_level = GPIO_ReadInputDataBit(KEY_PORT, KEY2_PIN);
    
    /* ---------- 按键1状态机 ---------- */
    switch (key1_state)
    {
        case 0:                     // 空闲，等待按下
            if (key1_level == 0)    // 检测到低电平（按下）
            {
                key1_state = 1;
                key1_debounce_cnt = 0;
            }
            break;
            
        case 1:                     // 消抖中
            if (key1_level == 0)
            {
                key1_debounce_cnt++;
                if (key1_debounce_cnt >= DEBOUNCE_TIME)
                {
                    key1_state = 2;
                    key1_pressed = 1;       // 产生按下事件
                }
            }
            else                    // 抖动或干扰，回到空闲
            {
                key1_state = 0;
            }
            break;
            
        case 2:                     // 已确认按下，等待释放
            if (key1_level == 1)    // 检测到高电平（释放）
            {
                key1_state = 0;     // 回到空闲
            }
            break;
    }
    
    /* ---------- 按键2状态机（相同逻辑） ---------- */
    switch (key2_state)
    {
        case 0:
            if (key2_level == 0)
            {
                key2_state = 1;
                key2_debounce_cnt = 0;
            }
            break;
            
        case 1:
            if (key2_level == 0)
            {
                key2_debounce_cnt++;
                if (key2_debounce_cnt >= DEBOUNCE_TIME)
                {
                    key2_state = 2;
                    key2_pressed = 1;       // 按键2按下事件
                    
                    /* ===== 直接在此处触发防中毒 ===== */
                    if (!HC595_AntiPoisonIsRunning())
                    {
                        HC595_AntiPoisonStart();
                        
                    }
                    /* ============================= */
                }
            }
            else
            {
                key2_state = 0;
            }
            break;
            
        case 2:
            if (key2_level == 1)
            {
                key2_state = 0;
            }
            break;
    }
}

/**
  * 函    数：主循环获取按键码（非阻塞）
  * 返 回 值：1-按键1按下，2-按键2按下，0-无按下
  * 说    明：每次调用仅返回一次，需在主循环中周期调用
  */
uint8_t Key_GetNum(void)
{
    uint8_t ret = 0;
    
    if (key1_pressed)
    {
        key1_pressed = 0;    // 清除事件标志
        ret = 1;
    }
    else if (key2_pressed)
    {
        key2_pressed = 0;
        ret = 2;
    }
    
    return ret;
}



