#include "timer.h"
#include <avr/io.h>
#include <avr/interrupt.h>

static volatile uint32_t ms_counter = 0;

void timer_init(void)
{
    // Timer0: CTC mode, 1ms tick
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS01) | (1 << CS00);   // clk/64
    OCR0A = 249;                           // 16MHz/64 = 250kHz → 250 counts = 1ms
    TIMSK0 = (1 << OCIE0A);
}

ISR(TIMER0_COMPA_vect)
{
    ms_counter++;
}

uint32_t millis(void)
{
    uint32_t temp;
    cli();
    temp = ms_counter;
    sei();
    return temp;
}
