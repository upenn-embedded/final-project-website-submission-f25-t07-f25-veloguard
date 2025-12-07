// #define F_CPU 16000000UL
// #include <avr/io.h>
// #include <avr/interrupt.h>
// #include <util/delay.h>
// #include <string.h> // for memset
// #include "turn_led.h"
// #include "config.h"

// // --- 配置部分 ---
// #define NUM_PIXELS    15
// #define COLOR_R       255
// #define COLOR_G       120
// #define COLOR_B       0

// // 动画分组: 4, 3, 3, 2, 2, 1
// static const uint8_t group_sizes[] = {4, 3, 3, 2, 2, 1};
// #define TOTAL_PHASES  (sizeof(group_sizes) / sizeof(group_sizes[0]))

// // 动画速度控制
// // 如果 main 循环约 10ms 执行一次，TICK_THRESHOLD = 15 意味着 150ms 切换一帧
// #define TICK_THRESHOLD 15 

// // 像素缓存 (GRB 格式)
// static uint8_t pixels[NUM_PIXELS][3];

// // --- 底层 WS2812 驱动 (时序敏感，请勿随意修改) ---

// // 发送一个字节到 PB3 (Left)
// static inline void ws2812_send_byte_left(uint8_t b) {
//     for (uint8_t i = 0; i < 8; i++) {
//         if (b & 0x80) {
//             // 发送 1: 高电平持续时间长
//             TURN_LEFT_PORT |= (1 << TURN_LEFT_PIN);
//             asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"); // ~500ns
//             TURN_LEFT_PORT &= ~(1 << TURN_LEFT_PIN);
//             asm volatile ("nop\nnop\nnop\n");
//         } else {
//             // 发送 0: 高电平持续时间短
//             TURN_LEFT_PORT |= (1 << TURN_LEFT_PIN);
//             asm volatile ("nop\nnop\nnop\n"); // ~250ns
//             TURN_LEFT_PORT &= ~(1 << TURN_LEFT_PIN);
//             asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\nnop\n");
//         }
//         b <<= 1;
//     }
// }

// // 发送一个字节到 PB4 (Right)
// static inline void ws2812_send_byte_right(uint8_t b) {
//     for (uint8_t i = 0; i < 8; i++) {
//         if (b & 0x80) {
//             TURN_RIGHT_PORT |= (1 << TURN_RIGHT_PIN);
//             asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
//             TURN_RIGHT_PORT &= ~(1 << TURN_RIGHT_PIN);
//             asm volatile ("nop\nnop\nnop\n");
//         } else {
//             TURN_RIGHT_PORT |= (1 << TURN_RIGHT_PIN);
//             asm volatile ("nop\nnop\nnop\n");
//             TURN_RIGHT_PORT &= ~(1 << TURN_RIGHT_PIN);
//             asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\nnop\n");
//         }
//         b <<= 1;
//     }
// }

// // 刷新左侧灯带
// static void show_left(void) {
//     cli(); // 关闭中断以保证时序
//     for (uint8_t i = 0; i < NUM_PIXELS; i++) {
//         ws2812_send_byte_left(pixels[i][1]); // G
//         ws2812_send_byte_left(pixels[i][0]); // R
//         ws2812_send_byte_left(pixels[i][2]); // B
//     }
//     sei(); // 恢复中断
//     _delay_us(60); // Reset time
// }

// // 刷新右侧灯带
// static void show_right(void) {
//     cli();
//     for (uint8_t i = 0; i < NUM_PIXELS; i++) {
//         ws2812_send_byte_right(pixels[i][1]); // G
//         ws2812_send_byte_right(pixels[i][0]); // R
//         ws2812_send_byte_right(pixels[i][2]); // B
//     }
//     sei();
//     _delay_us(60);
// }

// static void clear_buffer(void) {
//     memset(pixels, 0, sizeof(pixels));
// }

// // --- 逻辑层 ---

// // 初始化 IO
// void turn_led_init(void)
// {
//     // 配置引脚为输出
//     TURN_LEFT_DDR |= (1 << TURN_LEFT_PIN);
//     TURN_RIGHT_DDR |= (1 << TURN_RIGHT_PIN);

//     // 初始状态拉低 (灭)
//     TURN_LEFT_PORT &= ~(1 << TURN_LEFT_PIN);
//     TURN_RIGHT_PORT &= ~(1 << TURN_RIGHT_PIN);
    
//     clear_buffer();
//     show_left();
//     show_right();
// }

// // 填充像素缓存的核心算法
// // phase: 当前动画阶段 (0 到 TOTAL_PHASES-1)
// static void fill_buffer_for_phase(uint8_t phase) {
//     clear_buffer();
    
//     // 如果 phase 超过最大阶段，说明是全灭状态，直接返回全黑 buffer
//     if (phase >= TOTAL_PHASES) return;

//     uint8_t current_idx = 0;
//     for (uint8_t g = 0; g < TOTAL_PHASES; g++) {
//         uint8_t count = group_sizes[g];
//         // 累积点亮：如果当前组索引 g <= 当前阶段 phase，则点亮该组
//         if (g <= phase) {
//             for (uint8_t k = 0; k < count; k++) {
//                 if (current_idx + k < NUM_PIXELS) {
//                     pixels[current_idx + k][0] = COLOR_R;
//                     pixels[current_idx + k][1] = COLOR_G;
//                     pixels[current_idx + k][2] = COLOR_B;
//                 }
//             }
//         }
//         current_idx += count;
//     }
// }

// // 主逻辑更新函数
// void turn_led_update(rear_turn_mode_t mode)
// {
//     // 静态变量保持状态
//     static uint8_t phase = 0;       // 当前动画阶段 (0 ~ 6)
//     static uint8_t last_tick = 0;   // 计时器
//     static rear_turn_mode_t last_mode = 255; // 记录上一次模式，用于检测切换

//     // 1. 检测模式是否发生变化 (例如从 LEFT 变成 NONE)
//     // 如果变了，立即重置状态，并强制关灯
//     if (mode != last_mode) {
//         phase = 0;
//         last_tick = 0;
        
//         // 强制熄灭两边
//         clear_buffer();
//         show_left();
//         show_right();
        
//         last_mode = mode; // 更新记录
//     }

//     // 2. 如果是 TURN_NONE，直接返回，保持全灭
//     if (mode != TURN_LEFT && mode != TURN_RIGHT) {
//         return; 
//     }

//     // 3. 动画计时逻辑
//     last_tick++;
//     if (last_tick >= TICK_THRESHOLD) {
//         last_tick = 0; // 重置计时
        
//         // 计算当前帧的像素数据
//         fill_buffer_for_phase(phase);
        
//         // 根据方向发送到对应的灯带
//         if (mode == TURN_LEFT) {
//             show_left();
//             // 确保右边是灭的 (防止意外)
//             // 实际应用中可以省略这步如果确定右边没信号
//         } else if (mode == TURN_RIGHT) {
//             show_right();
//         }

//         // 阶段递增
//         phase++;
        
//         // 动画循环逻辑：
//         // 阶段 0~5: 逐渐点亮
//         // 阶段 6: 全灭 (让 clear_buffer 生效一次)
//         // 阶段 >6: 重置回 0
//         if (phase > TOTAL_PHASES) { 
//             phase = 0; 
//         }
//     }
// }