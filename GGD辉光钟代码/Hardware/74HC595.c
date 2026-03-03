#include "74HC595.h"
#include "stm32f10x.h"
#include <stdlib.h>
#include <string.h> 
#include "Delay.h"


/* 核心显存数组：
  Display_Buf[0][x] -> 第1个辉光管 (最左边)
  Display_Buf[1][x] -> 第2个辉光管
  Display_Buf[2][x] -> 第3个辉光管
  Display_Buf[3][x] -> 第4个辉光管
  数值为 1 点亮，0 熄灭
*/
static uint8_t Display_Buf[4][10] = {0};

// 引脚操作宏
#define DS_L()    GPIO_ResetBits(NIXIE_PORT, NIXIE_PIN_DATA)
#define DS_H()    GPIO_SetBits(NIXIE_PORT, NIXIE_PIN_DATA)
#define SCK_L()   GPIO_ResetBits(NIXIE_PORT, NIXIE_PIN_SCK)
#define SCK_H()   GPIO_SetBits(NIXIE_PORT, NIXIE_PIN_SCK)
#define RCK_L()   GPIO_ResetBits(NIXIE_PORT, NIXIE_PIN_RCK)
#define RCK_H()   GPIO_SetBits(NIXIE_PORT, NIXIE_PIN_RCK)

// 延时宏，如果显示不稳定，把这个值改大
#define NIXIE_DELAY_US  50 

/**
  * @brief  初始化GPIO
  */
void Nixie_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(NIXIE_RCC, ENABLE);

    GPIO_InitStructure.GPIO_Pin = NIXIE_PIN_DATA | NIXIE_PIN_SCK | NIXIE_PIN_RCK;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(NIXIE_PORT, &GPIO_InitStructure);

    SCK_L();
    RCK_L();
    Nixie_Clean(); // 上电全灭
}

/**
  * @brief  将显存数组推送到595芯片 (核心驱动)
  * @note   逻辑：遍历40个位，根据数组值决定发1还是0
  */
static void HC595_Update(void)
{
    int8_t tube, num;

    // 你的硬件是 U1->U2->U3->U4->U5 级联
    // U5 是链尾，对应第4个管子的后半部分
    // U1 是链头，对应第1个管子
    // 移位寄存器特性是：先发的数据被推到最远端
    // 所以发送顺序应该是：从第4个管子(最右)开始发，最后发第1个管子(最左)

    /* 注意：你的硬件连接非常特殊，是线性连接的
       Q1-0...Q1-9 接在 U1,U2
       ...
       Q4-0...Q4-9 接在 U4,U5
       这意味着我们其实可以把 4个管子*10个引脚 看作一个 40长度的数组
       发送时，先发 Tube 4 的 '9' (因为他在链条最末端)，最后发 Tube 1 的 '0'
    */

    for(tube = 3; tube >= 0; tube--) // 从第4个管子开始遍历
    {
        for(num = 9; num >= 0; num--) // 从数字9开始遍历 (高位先入，会被推到远处)
        {
            SCK_L();
            Delay_us(NIXIE_DELAY_US); // 延时稳定时序

            if(Display_Buf[tube][num] == 1)
            {
                DS_H();
            }
            else
            {
                DS_L();
            }
            Delay_us(NIXIE_DELAY_US);

            SCK_H(); // 上升沿移位
            Delay_us(NIXIE_DELAY_US); 
        }
    }

    // 全部40位发完后，锁存输出
    RCK_L();
    Delay_us(NIXIE_DELAY_US);
    RCK_H(); // 上升沿锁存
}

/**
  * @brief  显示时间 (主功能函数)
  * @param  hour: 小时
  * @param  minute: 分钟
  */
void Nixie_Display_Time(uint8_t hour, uint8_t minute)
{
    // 1. 安全检查，防止越界
    if(hour > 99) hour = 0;
    if(minute > 99) minute = 0;

    // 2. 清空显存 (防止重影的关键！)
    memset(Display_Buf, 0, sizeof(Display_Buf));

    // 3. 填充显存
    // Tube 0: 小时十位 (如果小于10，可以选择显示0或者熄灭，这里显示0)
    Display_Buf[0][hour / 10] = 1; 
    
    // Tube 1: 小时个位
    Display_Buf[1][hour % 10] = 1;

    // Tube 2: 分钟十位
    Display_Buf[2][minute / 10] = 1;

    // Tube 3: 分钟个位
    Display_Buf[3][minute % 10] = 1;

    // 4. 硬件刷新
    HC595_Update();
}

/**
  * @brief  熄灭所有辉光管
  */
void Nixie_Clean(void)
{
    memset(Display_Buf, 0, sizeof(Display_Buf));
    HC595_Update();
}


/**
  * @brief  防中毒程序 (随机乱跳)
  * @note   建议每隔一段时间调用一次，运行几秒钟
  */
void Nixie_AntiPoison(void)
{
    uint8_t i, t;
    
    // 快速变换 50 次 (约5秒)
    for(i = 0; i < 50; i++)
    {
        // 1. 清空显存
        memset(Display_Buf, 0, sizeof(Display_Buf));

        // 2. 为每个管子生成随机数
        for(t = 0; t < 4; t++)
        {
            uint8_t rnd = rand() % 10;
            Display_Buf[t][rnd] = 1;
        }

        // 3. 刷新并延时
        HC595_Update();
        Delay_ms(200); // 10Hz 刷新率
    }

    // 结束时恢复全灭，或者你可以调用 Nixie_Display_Time 恢复时间显示
    Nixie_Clean();
}



//// 内部使用的缓存，5个芯片 * 8位 = 40位
//static uint8_t send_buffer[5];

//// 宏定义操作引脚电平，提高代码可读性
//#define NIXIE_DATA_H()  GPIO_SetBits(NIXIE_PORT, NIXIE_PIN_DATA)
//#define NIXIE_DATA_L()  GPIO_ResetBits(NIXIE_PORT, NIXIE_PIN_DATA)

//#define NIXIE_SCK_H()   GPIO_SetBits(NIXIE_PORT, NIXIE_PIN_SCK)
//#define NIXIE_SCK_L()   GPIO_ResetBits(NIXIE_PORT, NIXIE_PIN_SCK)

//#define NIXIE_RCK_H()   GPIO_SetBits(NIXIE_PORT, NIXIE_PIN_RCK)
//#define NIXIE_RCK_L()   GPIO_ResetBits(NIXIE_PORT, NIXIE_PIN_RCK)

///**
//  * @brief  初始化GPIO
//  */
//void Nixie_Init(void)
//{
//    GPIO_InitTypeDef GPIO_InitStructure;

//    RCC_APB2PeriphClockCmd(NIXIE_RCC, ENABLE);

//    GPIO_InitStructure.GPIO_Pin = NIXIE_PIN_DATA | NIXIE_PIN_SCK | NIXIE_PIN_RCK;
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(NIXIE_PORT, &GPIO_InitStructure);

//    // 初始化状态：拉低所有时钟
//    NIXIE_SCK_L();
//    NIXIE_RCK_L();
//	
//	Nixie_Display(0xFF, 0xFF, 0xFF, 0xFF); //上电立即熄灭所有管子，防止OE接地导致的乱码
//}


///**
//  * @brief  发送一个字节到74HC595 (高位先出)
//  * @note   标准595逻辑：先送入Bit7，最终它会停留在Q7；最后送入Bit0，停在Q0
//  */
//static void HC595_SendByte(uint8_t byte)
//{
//    uint8_t i;
//    for(i = 0; i < 8; i++)
//    {
////        NIXIE_SCK_L(); // 拉低SCK准备数据
////        
////        // 准备数据位 (MSB First)
////        if(byte & 0x80) {
////            NIXIE_DATA_H();
////        } else {
////            NIXIE_DATA_L();
////        }
////        
////        byte <<= 1; // 移位，准备下一位
////        
////        NIXIE_SCK_H(); // 拉高SCK，数据在上升沿移入寄存器
//		if (byte & (0x80 >> i)) NIXIE_DATA_H();
//        else NIXIE_DATA_L();
//        
//        NIXIE_SCK_L();
//        for(volatile int j=0; j<100; j++); // 增加极短延时，确保595能识别电平
//        NIXIE_SCK_H();
//        for(volatile int j=0; j<100; j++);
//    }
//}


///**
//  * @brief  设置某个管子显示特定数字
//  * @param  tube_index: 管子序号 0-3 (对应Q1-Q4)
//  * @param  number: 要显示的数字 0-9 (如果传入255则熄灭该管)
//  */
//static void Set_Bit_In_Buffer(uint8_t tube_index, uint8_t number)
//{
//    if (number > 9) return; // 如果不是0-9，则不设置任何位（即熄灭）

//    /* 核心映射逻辑：
//       所有管子加起来共40个引脚，线性排布。
//       Tube 1 占用全局位: 0 - 9
//       Tube 2 占用全局位: 10 - 19
//       Tube 3 占用全局位: 20 - 29
//       Tube 4 占用全局位: 30 - 39
//    */
//    
//    // 1. 计算该数字对应的“全局位索引” (0 - 39)
//    uint8_t global_bit_index = (tube_index * 10) + number;

//    // 2. 计算该位在 buffer 的哪个字节 (0 - 4)
//    // U1是Byte0, U2是Byte1...
//    uint8_t byte_index = global_bit_index / 8;

//    // 3. 计算该位在字节中的偏移 (0 - 7)
//    uint8_t bit_offset = global_bit_index % 8;

//    // 4. 将对应位置1
//    send_buffer[byte_index] |= (1 << bit_offset);
//}

///**
//  * @brief  控制4个辉光管显示
//  * @param  num1: 第1个管子的数字 (0-9)
//  * @param  num2: 第2个管子的数字 (0-9)
//  * @param  num3: 第3个管子的数字 (0-9)
//  * @param  num4: 第4个管子的数字 (0-9)
//  * @note   传入 0xFF 可以熄灭对应的管子
//  */
//void Nixie_Display(uint8_t num1, uint8_t num2, uint8_t num3, uint8_t num4)
//{
//    int8_t i;

//    // 1. 清空缓存 (所有位清0，熄灭所有段)
//    for(i = 0; i < 5; i++) {
//        send_buffer[i] = 0x00;
//    }

//    // 2. 根据数字设置对应的位
//    Set_Bit_In_Buffer(0, num1);
//    Set_Bit_In_Buffer(1, num2);
//    Set_Bit_In_Buffer(2, num3);
//    Set_Bit_In_Buffer(3, num4);

//    // 3. 将5个字节的数据刷入移位寄存器
//    // 注意：硬件连接是 U1 -> U2 -> U3 -> U4 -> U5
//    // 移位特性意味着：最后发出的数据留在 U1，最先发出的数据被推到 U5
//    // 所以我们需要倒序发送：先发 send_buffer[4] (给U5)，最后发 send_buffer[0] (给U1)
//    
//    for(i = 4; i >= 0; i--)
//    {
//        HC595_SendByte(send_buffer[i]);
//    }

//    // 4. 产生锁存信号 (上升沿)，更新输出
//    NIXIE_RCK_L();
//    // 稍微延时（对于72MHz的STM32，GPIO翻转很快，加几个NOP保险，虽然595通常能跟上）
//    __NOP(); __NOP(); 
//    NIXIE_RCK_H();
//	
//	
//}


///**
//  * @brief  辉光管防中毒程序 (随机乱跳模式)
//  * @param  duration_sec: 程序持续运行的总时间(秒)
//  * @note   频率约为 10Hz (100ms跳动一次)
//  */
//void Nixie_AntiPoison_Random(uint16_t duration_sec)
//{
//    
//    uint32_t total_frames;
//    uint32_t i;
//    uint8_t n1, n2, n3, n4;

//    // 计算总共要跳变多少次 (1秒10次)
//    total_frames = duration_sec * 10;

//    for(i = 0; i < total_frames; i++)
//    {
//        // 生成 0-9 的随机数
//        n1 = rand() % 10;
//        n2 = rand() % 10;
//        n3 = rand() % 10;
//        n4 = rand() % 10;

//        // 显示随机数字
//        Nixie_Display(n1, n2, n3, n4);

//        // 延时 100ms，即 10Hz 的刷新率
//        Delay_ms(100);
//    }
//    
//    // 运行结束后，建议熄灭或恢复显示（这里选择熄灭，防止停留在最后的一组随机数上）
//    Nixie_Display(0xFF, 0xFF, 0xFF, 0xFF);
//}


