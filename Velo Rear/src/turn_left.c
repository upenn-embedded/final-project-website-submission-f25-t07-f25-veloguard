// turn_left.c
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "turn_left.h"

// 1. 修改灯珠总数为 15
#define NUM_PIXELS   15
#define LEFT_DDR      DDRB
#define LEFT_PORT     PORTB
#define LEFT_MASK     (1 << PB3)

// 颜色定义
#define TURN_COLOR_R 255
#define TURN_COLOR_G 120
#define TURN_COLOR_B 0

// 2. 定义分组逻辑：4, 3, 3, 2, 2, 1
static const uint8_t group_sizes[] = {4, 3, 3, 2, 2, 1};
#define TOTAL_GROUPS (sizeof(group_sizes) / sizeof(group_sizes[0]))

static uint8_t pixels[NUM_PIXELS][3];

// --- WS2812 底层驱动保持不变 (因时序敏感，这里省略重复代码，请保留你原文件中的 ws2812_send_byte) ---
static inline void ws2812_send_byte(uint8_t b) {
    // ... 请保留你原本的底层汇编/NOP代码 ...
    for (uint8_t i = 0; i < 8; i++) {
        if (b & 0x80) {
            LEFT_PORT |= LEFT_MASK;
            asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
            LEFT_PORT &= ~LEFT_MASK;
            asm volatile ("nop\nnop\nnop\n");
        } else {
            LEFT_PORT |= LEFT_MASK;
            asm volatile ("nop\nnop\nnop\n");
            LEFT_PORT &= ~LEFT_MASK;
            asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\nnop\n");
        }
        b <<= 1;
    }
}

static void ws2812_show(void) {
    cli();
    for (uint8_t i = 0; i < NUM_PIXELS; i++) {
        uint8_t r = pixels[i][0];
        uint8_t g = pixels[i][1];
        uint8_t b = pixels[i][2];
        ws2812_send_byte(g); // WS2812 is GRB
        ws2812_send_byte(r);
        ws2812_send_byte(b);
    }
    sei();
    _delay_us(60);
}

static void clear_pixels(void) {
    for (uint8_t i = 0; i < NUM_PIXELS; i++) {
        pixels[i][0] = 0;
        pixels[i][1] = 0;
        pixels[i][2] = 0;
    }
}

void turn_left_init(void) {
    LEFT_DDR |= LEFT_MASK;
    LEFT_PORT &= ~LEFT_MASK;
    clear_pixels();
    ws2812_show();
}

// 3. 重写 set_group 逻辑以支持动态分组
void turn_left_set_group(uint8_t phase) {
    clear_pixels(); // 每次先清空缓冲区

    // 如果 phase 超过了组数(6)，则保持全灭（即上面的 clear_pixels 生效后直接 show）
    if (phase < TOTAL_GROUPS) {
        uint8_t current_pixel_idx = 0;

        // 遍历所有组
        for (uint8_t g = 0; g < TOTAL_GROUPS; g++) {
            uint8_t count = group_sizes[g];

            // 核心逻辑：流水灯通常是累积点亮。
            // 只要当前组索引 g 小于等于目标阶段 phase，这组灯就该亮。
            // (如果你只想要“跑马灯”即只有当前那一段亮，把 <= 改为 == 即可)
            if (g <= phase) {
                for (uint8_t k = 0; k < count; k++) {
                    // 防止数组越界（虽然数学上 4+3+3+2+2+1=15 不会越界，但加个保护是好习惯）
                    if (current_pixel_idx + k < NUM_PIXELS) {
                        pixels[current_pixel_idx + k][0] = TURN_COLOR_R;
                        pixels[current_pixel_idx + k][1] = TURN_COLOR_G;
                        pixels[current_pixel_idx + k][2] = TURN_COLOR_B;
                    }
                }
            }
            // 移动起始坐标到下一组
            current_pixel_idx += count;
        }
    }

    ws2812_show();
}