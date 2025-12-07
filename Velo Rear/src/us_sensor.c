#include "us_sensor.h"
#include "config.h"
#include "timer.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

static volatile uint16_t echo_start = 0;
static volatile uint16_t echo_width = 0;

void us_sensor_init(void)
{
    // TRIG pin output
    US_TRIG_DDR |= (1 << US_TRIG_PIN);
    US_TRIG_PORT &= ~(1 << US_TRIG_PIN);

    // Timer1 input capture for ECHO
    TCCR1A = 0;
    TCCR1B = (1 << ICES1) | (1 << CS11);  // rising edge, clk/8
    TIMSK1 = (1 << ICIE1);
}

ISR(TIMER1_CAPT_vect)
{
    static uint8_t rising = 1;

    if (rising) {
        echo_start = ICR1;           // rising: latch start
        TCCR1B &= ~(1 << ICES1);     // switch to falling edge
        rising = 0;
    } else {
        uint16_t end = ICR1;
        echo_width = end - echo_start;
        TCCR1B |= (1 << ICES1);      // back to rising
        rising = 1;
    }
}

float us_sensor_get_distance_m(void)
{
    // Send 10us TRIG pulse
    US_TRIG_PORT |= (1 << US_TRIG_PIN);
    _delay_us(10);
    US_TRIG_PORT &= ~(1 << US_TRIG_PIN);

    // Convert echo_width to distance
    // At clk/8: timer tick = 0.5us
    float us = echo_width * 0.5f;
    float distance_m = (us * 0.000343f) / 2.0f;
    return distance_m;
}
