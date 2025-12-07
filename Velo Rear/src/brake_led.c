#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "config.h"
#include "brake_led.h"

// ================= ???? =================
// 1. ?????? (???????????? 8 ??? 16 ??????)
#define NUM_PIXELS    8

// 2. ??????????????
// ??????? PORTB ? Pin 5 (???? 13)???? Pin 2?????? 5 ?? 2
#define BRAKE_BIT     PB5  
#define BRAKE_DDR     DDRB
#define BRAKE_PORT    PORTB
#define BRAKE_MASK    (1 << BRAKE_BIT)

// 3. ?????????? (R=255, G=60, B=0)
#define BRAKE_R       255
#define BRAKE_G       20
#define BRAKE_B       0
// ===========================================

static uint8_t pixels[NUM_PIXELS][3];

// WS2812 ???? (??????????)
static inline void ws2812_send_byte(uint8_t b) {
    for (uint8_t i = 0; i < 8; i++) {
        if (b & 0x80) {
            // ?? "1": ??????
            // ???????? 'sbi' ??????? BRAKE_BIT ???
            asm volatile (
                "sbi %0, %1 \n\t"       // ??
                "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n" 
                "cbi %0, %1 \n\t"       // ??
                "nop\nnop\nnop\n" 
                :: "I" (_SFR_IO_ADDR(BRAKE_PORT)), "I" (BRAKE_BIT)
            );
        } else {
            // ?? "0": ??????
            asm volatile (
                "sbi %0, %1 \n\t"       // ??
                "nop\nnop\nnop\n" 
                "cbi %0, %1 \n\t"       // ??
                "nop\nnop\nnop\nnop\nnop\nnop\nnop\n" 
                :: "I" (_SFR_IO_ADDR(BRAKE_PORT)), "I" (BRAKE_BIT)
            );
        }
        b <<= 1;
    }
}

static void ws2812_show(void) {
    cli(); // ???????????
    for (uint8_t i = 0; i < NUM_PIXELS; i++) {
        uint8_t r = pixels[i][0];
        uint8_t g = pixels[i][1];
        uint8_t b = pixels[i][2];
        // WS2812 ????? G -> R -> B
        ws2812_send_byte(g);
        ws2812_send_byte(r);
        ws2812_send_byte(b);
    }
    sei(); // ????
    _delay_us(60); // ????
}

void brake_led_init(void) {
    BRAKE_DDR |= BRAKE_MASK;   // ?????
    BRAKE_PORT &= ~BRAKE_MASK; // ??
    brake_led_update(false);   // ??????
}

// ??????? 1 (true) ????? 0 (false) ??
void brake_led_light(bool is_braking) {
    for (uint8_t i = 0; i < NUM_PIXELS; i++) {
        if (is_braking) {
            pixels[i][0] = BRAKE_R;
            pixels[i][1] = BRAKE_G;
            pixels[i][2] = BRAKE_B;
        } else {
            // ?????? (???????????????)
            pixels[i][0] = 0;
            pixels[i][1] = 0;
            pixels[i][2] = 0;
        }
    }
    ws2812_show();
}

void brake_led_update(rear_brake_mode_t mode)
{
    switch (mode) {
    case BRAKE_STEADY:
    case BRAKE_BURST:
    case BRAKE_LINK_LOSS:
        brake_led_light(1);
        break;
        
    default:
        brake_led_light(0);
        break;
    }
}
