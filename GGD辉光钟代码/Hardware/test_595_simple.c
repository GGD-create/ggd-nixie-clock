#include "test_595_simple.h"
#include "delay.h" 
#include "LED.h"
#include "ds1302.h"


// 存储两个595的数据
static uint8_t hc595_data[HC595_CHIP_COUNT] = {0x00, 0x00, 0x00, 0x00, 0x00};


//辉光管当前显示的数字（0-9，255表示无显示）
static uint8_t nixie_current_digits[NIXIE_TUBE_COUNT] = {255, 255, 255, 255};


static uint32_t random_seed = 12345;  //随机初始种子

extern struct TIMEData TimeData;  //声明外部变量，已在ds1302.c中定义


//时间显示相关变量
static uint8_t nixie_show_time = 0;          // 是否显示时间标志
static uint8_t nixie_hour_tens, nixie_hour_ones;   // 小时的十位和个位
static uint8_t nixie_min_tens, nixie_min_ones;     // 分钟的十位和个位

//氖管控制变量
static uint8_t neon_state = 0;              // 氖管状态：0=灭，1=亮
static uint32_t neon_timer_ms = 0;          // 氖管计时器（毫秒）

//辉光管防中毒相关变量
static uint8_t anti_poison_running = 0;       // 防中毒模式是否运行
static uint32_t anti_poison_timer = 0;        // 防中毒计时器
static uint16_t anti_poison_count = 0;        // 防中毒显示计数
static uint8_t anti_poison_digits[4];         // 存储随机数字




//内联函数优化
static inline void HC595_DS_HIGH(void) { HC595_DS_PORT->BSRR = HC595_DS_PIN; }
static inline void HC595_DS_LOW(void)  { HC595_DS_PORT->BRR  = HC595_DS_PIN; }
static inline void HC595_SHCP_HIGH(void) { HC595_SHCP_PORT->BSRR = HC595_SHCP_PIN; }
static inline void HC595_SHCP_LOW(void)  { HC595_SHCP_PORT->BRR  = HC595_SHCP_PIN; }
static inline void HC595_STCP_HIGH(void) { HC595_STCP_PORT->BSRR = HC595_STCP_PIN; }
static inline void HC595_STCP_LOW(void)  { HC595_STCP_PORT->BRR  = HC595_STCP_PIN; }

static inline void HC595_OE_LOW(void)  { HC595_OE_PORT->BRR  = HC595_OE_PIN; }

//初始化GPIO
void HC595_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能GPIO时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    // 配置DS引脚（数据引脚）
    GPIO_InitStructure.GPIO_Pin = HC595_DS_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(HC595_DS_PORT, &GPIO_InitStructure);
    
    // 配置SHCP引脚（移位时钟）
    GPIO_InitStructure.GPIO_Pin = HC595_SHCP_PIN;
    GPIO_Init(HC595_SHCP_PORT, &GPIO_InitStructure);
    
    // 配置STCP引脚（锁存时钟）
    GPIO_InitStructure.GPIO_Pin = HC595_STCP_PIN;
    GPIO_Init(HC595_STCP_PORT, &GPIO_InitStructure);
	
	// 配置OE引脚
    GPIO_InitStructure.GPIO_Pin = HC595_OE_PIN;
    GPIO_Init(HC595_OE_PORT, &GPIO_InitStructure);
	
    
    // 初始状态：所有引脚低电平
    HC595_DS_LOW();
    HC595_SHCP_LOW();
    HC595_STCP_LOW();
	
	HC595_OE_LOW();//OE引脚如果接地就不用配置了

    // 清空595数据
    HC595_ClearAll();
}

// 发送一个位到595
static void HC595_SendBit(uint8_t bit)
{
    // 设置数据位
    if(bit) HC595_DS_HIGH();
    else HC595_DS_LOW();
    
    // 产生移位时钟上升沿
    HC595_SHCP_HIGH();
    __NOP(); __NOP(); __NOP(); __NOP();
    HC595_SHCP_LOW();
    __NOP(); __NOP(); __NOP(); __NOP();
}

// 发送一个字节到595（从最高位开始）
static void HC595_SendByte(uint8_t data)
{
    uint8_t i;
    
    // 发送8位数据
    for(i = 0; i < 8; i++)
    {
        // 发送最高位
        HC595_SendBit((data & 0x80) >> 7);
        data <<= 1;  // 左移，准备下一位
    }
}


void HC595_WriteData(uint8_t chip1_data, uint8_t chip2_data, uint8_t chip3_data,
                     uint8_t chip4_data, uint8_t chip5_data)
{
    // 更新数据缓冲区
    hc595_data[0] = chip1_data;  // 第一个595（控制LED0-7）
    hc595_data[1] = chip2_data;  // 第二个595（控制LED8-15）
    hc595_data[2] = chip3_data;  // 第三个595（控制LED16-23）
    hc595_data[3] = chip4_data;  // 第四个595（控制LED24-31）
    hc595_data[4] = chip5_data;  // 第五个595（控制LED32-39）
    
    // 注意：595是串联的，数据先进入第一个595，然后依次传递
    // 发送顺序：从离单片机最远的开始（第五个），最后发送第一个
    
    // 发送第五个595的数据（LED32-39）
    HC595_SendByte(chip5_data);
    
    // 发送第四个595的数据（LED24-31）
    HC595_SendByte(chip4_data);
    
    // 发送第三个595的数据（LED16-23）
    HC595_SendByte(chip3_data);
    
    // 发送第二个595的数据（LED8-15）
    HC595_SendByte(chip2_data);
    
    // 发送第一个595的数据（LED0-7）
    HC595_SendByte(chip1_data);
    
    // 产生锁存时钟上升沿，更新所有输出
    HC595_STCP_HIGH();
    __NOP(); __NOP(); __NOP(); __NOP();
    HC595_STCP_LOW();
}



// 设置单个LED状态（0-15）
void HC595_SetLED(uint8_t led_num, uint8_t state)
{
    uint8_t chip_index, bit_index;
    
    if(led_num >= HC595_TOTAL_LEDS) return;
    
    // 计算芯片索引和位索引
    chip_index = led_num / 8;      // 0:第一个595, 1:第二个595
    bit_index = led_num % 8;       // 0-7
    
    if(state)
    {
        // 设置位为1（点亮LED）
        hc595_data[chip_index] |= (1 << bit_index);
    }
    else
    {
        // 清除位（熄灭LED）
        hc595_data[chip_index] &= ~(1 << bit_index);
    }
    
    // 更新到595
    HC595_WriteData(hc595_data[0], hc595_data[1], hc595_data[2], 
                   hc595_data[3], hc595_data[4]);
}


// 切换单个LED状态
void HC595_ToggleLED(uint8_t led_num)
{
    uint8_t chip_index, bit_index;
    
    if(led_num >= HC595_TOTAL_LEDS) return;
    
    chip_index = led_num / 8;
    bit_index = led_num % 8;
    
    // 切换对应位
    hc595_data[chip_index] ^= (1 << bit_index);
    
    // 更新到595
    HC595_WriteData(hc595_data[0], hc595_data[1], hc595_data[2], 
                   hc595_data[3], hc595_data[4]);
}

// 清空所有LED
void HC595_ClearAll(void)
{
    hc595_data[0] = 0x00;
    hc595_data[1] = 0x00;
    hc595_data[2] = 0x00;
    hc595_data[3] = 0x00;
    hc595_data[4] = 0x00;
	
	
	// 清空辉光管当前显示状态
    for(uint8_t i = 0; i < NIXIE_TUBE_COUNT; i++)
    {
        nixie_current_digits[i] = 255;  // 255表示无显示
    }
	
	
    HC595_WriteData(hc595_data[0], hc595_data[1], hc595_data[2],
                   hc595_data[3], hc595_data[4]);
}


// 左移所有LED（新位为0）- 现在处理40位
void HC595_ShiftLeft(void)
{
    uint8_t i;
    uint8_t carry = 0, next_carry;
    
    // 从第一个芯片开始左移（最低字节）
    for(i = 0; i < HC595_CHIP_COUNT; i++)
    {
        next_carry = (hc595_data[i] & 0x80) >> 7;  // 保存最高位
        hc595_data[i] = (hc595_data[i] << 1) | carry;  // 左移并加上前一个芯片的进位
        carry = next_carry;  // 更新进位
    }
    
    HC595_WriteData(hc595_data[0], hc595_data[1], hc595_data[2],
                   hc595_data[3], hc595_data[4]);
}


// 右移所有LED（新位为0）- 现在处理40位
void HC595_ShiftRight(void)
{
    uint8_t i;
    uint8_t carry = 0, next_carry;
    
    // 从最后一个芯片开始右移（最高字节）
    for(i = HC595_CHIP_COUNT; i > 0; i--)
    {
        next_carry = hc595_data[i-1] & 0x01;  // 保存最低位
        hc595_data[i-1] = (hc595_data[i-1] >> 1) | (carry << 7);  // 右移并加上前一个芯片的进位
        carry = next_carry;  // 更新进位
    }
    
    HC595_WriteData(hc595_data[0], hc595_data[1], hc595_data[2],
                   hc595_data[3], hc595_data[4]);
}


// 循环左移所有LED - 现在处理40位
void HC595_RotateLeft(void)
{
    uint8_t i;
    uint8_t carry = (hc595_data[HC595_CHIP_COUNT-1] & 0x80) >> 7;  // 保存最后一个芯片的最高位
    
    // 从第一个芯片开始左移
    for(i = 0; i < HC595_CHIP_COUNT; i++)
    {
        uint8_t next_carry = (hc595_data[i] & 0x80) >> 7;  // 保存当前芯片的最高位
        hc595_data[i] = (hc595_data[i] << 1) | carry;  // 左移并加上进位
        carry = next_carry;  // 更新进位
    }
    
    HC595_WriteData(hc595_data[0], hc595_data[1], hc595_data[2],
                   hc595_data[3], hc595_data[4]);
}


// 循环右移所有LED - 现在处理40位
void HC595_RotateRight(void)
{
    uint8_t i;
    uint8_t carry = hc595_data[0] & 0x01;  // 保存第一个芯片的最低位
    
    // 从最后一个芯片开始右移
    for(i = HC595_CHIP_COUNT; i > 0; i--)
    {
        uint8_t next_carry = hc595_data[i-1] & 0x01;  // 保存当前芯片的最低位
        hc595_data[i-1] = (hc595_data[i-1] >> 1) | (carry << 7);  // 右移并加上进位
        carry = next_carry;  // 更新进位
    }
    
    HC595_WriteData(hc595_data[0], hc595_data[1], hc595_data[2],
                   hc595_data[3], hc595_data[4]);
}



// 设置40位图案（使用64位存储，只取低40位）
void HC595_SetPattern40(uint64_t pattern)
{
    // 只取低40位
    pattern &= 0xFFFFFFFFFFULL;
    
    hc595_data[0] = pattern & 0xFF;                // 低8位（第一个595）
    hc595_data[1] = (pattern >> 8) & 0xFF;         // 第二个595
    hc595_data[2] = (pattern >> 16) & 0xFF;        // 第三个595
    hc595_data[3] = (pattern >> 24) & 0xFF;        // 第四个595
    hc595_data[4] = (pattern >> 32) & 0xFF;        // 第五个595
    
    HC595_WriteData(hc595_data[0], hc595_data[1], hc595_data[2],
                   hc595_data[3], hc595_data[4]);
}

// 设置32位图案（只控制前四个芯片，第五个芯片清零）
void HC595_SetPattern32(uint32_t pattern)
{
    hc595_data[0] = pattern & 0xFF;                // 低8位（第一个595）
    hc595_data[1] = (pattern >> 8) & 0xFF;         // 第二个595
    hc595_data[2] = (pattern >> 16) & 0xFF;        // 第三个595
    hc595_data[3] = (pattern >> 24) & 0xFF;        // 第四个595
    hc595_data[4] = 0x00;                          // 第五个595清零
    
    HC595_WriteData(hc595_data[0], hc595_data[1], hc595_data[2],
                   hc595_data[3], hc595_data[4]);
}

// 设置24位图案（只控制前三个芯片，第四、五个芯片清零）
void HC595_SetPattern24(uint32_t pattern)
{
    // 只取低24位
    pattern &= 0xFFFFFF;
    
    hc595_data[0] = pattern & 0xFF;                // 低8位（第一个595）
    hc595_data[1] = (pattern >> 8) & 0xFF;         // 第二个595
    hc595_data[2] = (pattern >> 16) & 0xFF;        // 第三个595
    hc595_data[3] = 0x00;                          // 第四个595清零
    hc595_data[4] = 0x00;                          // 第五个595清零
    
    HC595_WriteData(hc595_data[0], hc595_data[1], hc595_data[2],
                   hc595_data[3], hc595_data[4]);
}


// 获取当前LED状态
uint8_t HC595_GetLEDState(uint8_t led_num)
{
    uint8_t chip_index, bit_index;
    
    if(led_num >= HC595_TOTAL_LEDS) return 0;
    
    chip_index = led_num / 8;
    bit_index = led_num % 8;
    
    return (hc595_data[chip_index] >> bit_index) & 0x01;
}


// 直接发送40位数据（不使用缓冲区）
void HC595_Write40Bits(uint64_t data)
{
    // 只取低40位
    data &= 0xFFFFFFFFFFULL;
    
    // 注意发送顺序：从第五个芯片开始（高8位）
    HC595_SendByte((data >> 32) & 0xFF);  // 第五个595
    HC595_SendByte((data >> 24) & 0xFF);  // 第四个595
    HC595_SendByte((data >> 16) & 0xFF);  // 第三个595
    HC595_SendByte((data >> 8) & 0xFF);   // 第二个595
    HC595_SendByte(data & 0xFF);          // 第一个595
    
    // 锁存输出
    HC595_STCP_HIGH();
    __NOP(); __NOP(); __NOP(); __NOP();
    HC595_STCP_LOW();
}


// 批量设置LED（可以设置一段连续的LED）
void HC595_SetLEDs(uint8_t start_led, uint8_t end_led, uint8_t state)
{
    uint8_t i;
    
    if(start_led >= HC595_TOTAL_LEDS || end_led >= HC595_TOTAL_LEDS || start_led > end_led)
        return;
    
    for(i = start_led; i <= end_led; i++)
    {
        HC595_SetLED(i, state);
    }
}


// 设置单个辉光管的数字（0-9）
void HC595_SetNixieTubeByIndex(uint8_t tube_index, uint8_t number)
{
    if(tube_index >= NIXIE_TUBE_COUNT) return;
    if(number >= 10) return;  // 只能是0-9
    
    // 关闭当前显示的数字（如果有）
    if(nixie_current_digits[tube_index] != 255)
    {
        uint8_t old_led_num = tube_index * 10 + nixie_current_digits[tube_index];
        HC595_SetLED(old_led_num, 0);
    }
    
    // 点亮新的数字
    uint8_t new_led_num = tube_index * 10 + number;
    HC595_SetLED(new_led_num, 1);
    
    // 更新当前显示状态
    nixie_current_digits[tube_index] = number;
}


// 设置所有辉光管的数字（主函数）
void HC595_SetNixieTube(uint8_t tube1_num, uint8_t tube2_num, uint8_t tube3_num, uint8_t tube4_num)
{
    // 设置每个辉光管的数字
    HC595_SetNixieTubeByIndex(0, tube1_num);
    HC595_SetNixieTubeByIndex(1, tube2_num);
    HC595_SetNixieTubeByIndex(2, tube3_num);
    HC595_SetNixieTubeByIndex(3, tube4_num);
}


// 清除单个辉光管（关闭所有数字）
void HC595_ClearNixieTube(uint8_t tube_index)
{
    if(tube_index >= NIXIE_TUBE_COUNT) return;
    
    // 关闭当前显示的数字
    if(nixie_current_digits[tube_index] != 255)
    {
        uint8_t led_num = tube_index * 10 + nixie_current_digits[tube_index];
        HC595_SetLED(led_num, 0);
        nixie_current_digits[tube_index] = 255;
    }
}

// 清除所有辉光管
void HC595_ClearAllNixieTubes(void)
{
    for(uint8_t i = 0; i < NIXIE_TUBE_COUNT; i++)
    {
        HC595_ClearNixieTube(i);
    }
}


// 显示4位数字（0-9999）
void HC595_ShowNumber(uint16_t number)
{
    if(number > 9999) number = 9999;  // 限制范围
    
    // 分解数字
    uint8_t digit4 = number % 10;          // 个位
    uint8_t digit3 = (number / 10) % 10;   // 十位
    uint8_t digit2 = (number / 100) % 10;  // 百位
    uint8_t digit1 = (number / 1000) % 10; // 千位
    
    // 设置辉光管
    HC595_SetNixieTube(digit1, digit2, digit3, digit4);
}

// 辅助函数：获取LED编号对应的辉光管和数字
void HC595_GetNixieInfo(uint8_t led_num, uint8_t *tube_index, uint8_t *digit)
{
    if(led_num >= NIXIE_TOTAL_DIGITS)
    {
        *tube_index = 255;
        *digit = 255;
        return;
    }
    
    *tube_index = led_num / 10;
    *digit = led_num % 10;
}

// 检查数字是否有效（0-9）
uint8_t HC595_IsValidDigit(uint8_t digit)
{
    return (digit < 10);
}


// 获取辉光管当前显示的数字
uint8_t HC595_GetNixieTubeDigit(uint8_t tube_index)
{
    if(tube_index >= NIXIE_TUBE_COUNT) return 255;
    return nixie_current_digits[tube_index];
}


void HC595_0_9_In_Order(uint32_t ms)
{
	HC595_ClearAll();
	HC595_SetNixieTube(0, 0, 0, 0);
	Delay_ms(ms);
	HC595_SetNixieTube(1, 1, 1, 1);
	Delay_ms(ms);
	HC595_SetNixieTube(2, 2, 2, 2);
	Delay_ms(ms);
	HC595_SetNixieTube(3, 3, 3, 3);
	Delay_ms(ms);
	HC595_SetNixieTube(4, 4, 4, 4);
	Delay_ms(ms);
	HC595_SetNixieTube(5, 5, 5, 5);
	Delay_ms(ms);
	HC595_SetNixieTube(6, 6, 6, 6);
	Delay_ms(ms);
	HC595_SetNixieTube(7, 7, 7, 7);
	Delay_ms(ms);
	HC595_SetNixieTube(8, 8, 8, 8);
	Delay_ms(ms);
	HC595_SetNixieTube(9, 9, 9, 9);
	Delay_ms(ms);
	HC595_ClearAll();
	
}


// 生成0-9的随机数
static uint8_t HC595_GetRandomNumber(void)
{
    // 简单的伪随机数生成
    random_seed = random_seed * 1103515245 + 12345;
    return (random_seed >> 16) % 10;  // 返回0-9
}

// 辉光管随机显示函数
// 功能：让4个辉光管的数字随机亮起来，每个辉光管里只能亮一个数字
// 参数：ms - 两个数字显示之间的时间间隔（毫秒）
void HC595_RandomNixieShow(uint32_t ms)
{
    uint8_t digits[4];
    
    // 为每个辉光管生成0-9的随机数
    for(uint8_t i = 0; i < 4; i++)
    {
        digits[i] = HC595_GetRandomNumber();
    }
    
    // 显示随机数字
    HC595_SetNixieTube(digits[0], digits[1], digits[2], digits[3]);
    
    // 使用已有的延时函数
    Delay_ms(ms);
}


// 带计数器的随机显示函数
void HC595_RandomNixieShowTimes(uint32_t ms, uint16_t times)
{
    for(uint16_t i = 0; i < times; i++)
    {
        // 生成4个随机数
        uint8_t digits[4];
        for(uint8_t j = 0; j < 4; j++)
        {
            digits[j] = HC595_GetRandomNumber();
        }
        
        // 显示
        HC595_SetNixieTube(digits[0], digits[1], digits[2], digits[3]);
        
        // 延时
        Delay_ms(ms);
    }
}


// 从DS1302时间结构体获取时间并分解为4位数字
void HC595_UpdateTimeFromDS1302(void)
{
    // 分解小时为两位数字
    uint8_t hour = TimeData.hour;
    nixie_hour_tens = hour / 10;
    nixie_hour_ones = hour % 10;
    
    // 分解分钟为两位数字
    uint8_t minute = TimeData.minute;
    nixie_min_tens = minute / 10;
    nixie_min_ones = minute % 10;
    
    // 调试输出（可选）
    // UsartPrintf(USART_DEBUG, "[Nixie] Time: %02d:%02d -> %d%d:%d%d\r\n", 
    //             hour, minute, nixie_hour_tens, nixie_hour_ones, nixie_min_tens, nixie_min_ones);
}


// 在辉光管上显示时间
void HC595_ShowTimeOnNixie(void)
{
    if(!nixie_show_time) return;
    
    // 更新辉光管显示
    HC595_SetNixieTube(nixie_hour_tens, nixie_hour_ones, nixie_min_tens, nixie_min_ones);
}

// 启动时间显示
void HC595_StartTimeDisplay(void)
{
    nixie_show_time = 1;
    HC595_UpdateTimeFromDS1302();
    HC595_ShowTimeOnNixie();
    
    // 初始化氖管状态为熄灭
    NE_OFF();
    neon_state = 0;
    neon_timer_ms = 0;
}


// 停止时间显示
void HC595_StopTimeDisplay(void)
{
    nixie_show_time = 0;
    HC595_ClearAllNixieTubes();
    
    // 关闭氖管
    NE_OFF();
    neon_state = 0;
}


// 氖管控制函数（在定时器中断中调用）
void HC595_NeonControl(uint32_t elapsed_ms)
{
    // 更新氖管计时器
    neon_timer_ms += elapsed_ms;
    
    // 检查是否达到1秒间隔
    if(neon_timer_ms >= 1000)  // 1000ms = 1秒
    {
        neon_timer_ms = 0;  // 重置计时器
        
        // 切换氖管状态
        if(neon_state == 0)
        {
            NE_ON();       // 点亮氖管
            neon_state = 1;
        }
        else
        {
            NE_OFF();      // 熄灭氖管
            neon_state = 0;
        }
    }
}


// 获取当前氖管状态
uint8_t HC595_GetNeonState(void)
{
    return neon_state;
}



// 防中毒模式启动
void HC595_AntiPoisonStart(void)
{
    anti_poison_running = 1;
    anti_poison_timer = 0;
    anti_poison_count = 0;
    
    // 生成4个随机数字
    for(uint8_t i = 0; i < 4; i++)
    {
        anti_poison_digits[i] = HC595_GetRandomNumber();
    }
    
    // 显示随机数字
    HC595_SetNixieTube(anti_poison_digits[0], anti_poison_digits[1], 
                       anti_poison_digits[2], anti_poison_digits[3]);
    
    
}


// 防中毒模式停止
void HC595_AntiPoisonStop(void)
{
    anti_poison_running = 0;
    // 恢复到正常时间显示
    HC595_UpdateTimeFromDS1302();
    HC595_ShowTimeOnNixie();
    
   
}


// 检查防中毒模式是否运行
uint8_t HC595_AntiPoisonIsRunning(void)
{
    return anti_poison_running;
}


// 防中毒模式更新（在TIM3中断中调用）
void HC595_AntiPoisonUpdate(void)
{
    if(!anti_poison_running) return;
    
    // 防中毒模式持续10秒
    if(anti_poison_timer < 20000)  // 10秒 = 10000ms（每次中断增加10ms）
    {
        anti_poison_timer += 10;  // TIM3中断每10ms一次
        
        // 每200ms更新一次随机数字（可选）
        static uint8_t update_counter = 0;
        update_counter++;
        if(update_counter >= 20)  // 20 * 10ms = 200ms
        {
            update_counter = 0;
            // 生成新的随机数字
            for(uint8_t i = 0; i < 4; i++)
            {
                anti_poison_digits[i] = HC595_GetRandomNumber();
            }
            
            // 显示随机数字
            HC595_SetNixieTube(anti_poison_digits[0], anti_poison_digits[1], 
                               anti_poison_digits[2], anti_poison_digits[3]);
        }
    }
    else
    {
        // 10秒后停止防中毒模式
        HC595_AntiPoisonStop();
    }
}

