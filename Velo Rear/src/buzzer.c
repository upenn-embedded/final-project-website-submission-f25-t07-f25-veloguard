// New implementation: use Timer2 CTC + ISR to toggle buzzer pin.
// This avoids using Timer1 so it doesn't conflict with ultrasonic input-capture.

#include "buzzer.h"
#include "config.h"
#include "timer.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>

// Timer2 prescaler options and corresponding CS bits
struct prescaler_option { uint16_t div; uint8_t cs_bits; };
static const struct prescaler_option prescalers[] = {
    {1,   (1 << CS20)},
    {8,   (1 << CS21)},
    {32,  (1 << CS21) | (1 << CS20)},
    {64,  (1 << CS22)},
    {128, (1 << CS22) | (1 << CS20)},
    {256, (1 << CS22) | (1 << CS21)},
    {1024,(1 << CS22) | (1 << CS21) | (1 << CS20)}
};

static volatile bool buzzer_on = false;

ISR(TIMER2_COMPA_vect)
{
    // toggle buzzer pin
    if (buzzer_on) {
        BUZZER_PORT ^= (1 << BUZZER_PIN);
    } else {
        BUZZER_PORT &= ~(1 << BUZZER_PIN);
    }
}

static void timer2_stop(void)
{
    TIMSK2 &= ~(1 << OCIE2A);
    TCCR2A = 0;
    TCCR2B = 0;
    BUZZER_PORT &= ~(1 << BUZZER_PIN);
}

// Start Timer2 to generate approx `freq_hz` toggles (square wave frequency = freq_hz)
// We toggle the pin in ISR, so OCR2A should be set so that toggle frequency = F_CPU/(2*prescaler*(OCR2A+1))
static void timer2_start_freq(uint32_t freq_hz)
{
    if (freq_hz == 0) {
        timer2_stop();
        return;
    }

    const uint32_t F = F_CPU;
    for (int i = 0; i < sizeof(prescalers)/sizeof(prescalers[0]); ++i) {
        uint32_t pres = prescalers[i].div;
        uint32_t top = (F / (2UL * pres * freq_hz));
        if (top == 0) continue;
        if (top - 1 <= 0xFF) {
            uint8_t ocr = (uint8_t)(top - 1);

            // Configure Timer2 CTC mode (WGM21=1)
            TCCR2A = (1 << WGM21);
            // Set prescaler bits
            TCCR2B = prescalers[i].cs_bits;
            OCR2A = ocr;
            // enable compare match A interrupt
            TIMSK2 |= (1 << OCIE2A);
            return;
        }
    }

    // If no prescaler fits, stop timer
    timer2_stop();
}

void buzzer_init(void)
{
    BUZZER_DDR |= (1 << BUZZER_PIN);
    BUZZER_PORT &= ~(1 << BUZZER_PIN);
    buzzer_on = false;
    timer2_stop();
}

void buzzer_update(rear_brake_mode_t mode, float dist_m)
{
    static uint32_t last_toggle = 0;
    static bool burst_on = false;
    uint32_t now = millis();

    if (mode == BRAKE_BURST) {
        if (now - last_toggle >= 100) {
            burst_on = !burst_on;
            last_toggle = now;
            if (burst_on) {
                buzzer_on = true;
                timer2_start_freq(1500);
            } else {
                buzzer_on = false;
                timer2_stop();
            }
        }
        return;
    }

    if (dist_m < D_ALERT) {
        buzzer_on = true;
        timer2_start_freq(2000);
        return;
    }

    if (dist_m < D_WARN) {
        buzzer_on = true;
        timer2_start_freq(800);
        return;
    }

    // Idle
    buzzer_on = false;
    timer2_stop();
}
