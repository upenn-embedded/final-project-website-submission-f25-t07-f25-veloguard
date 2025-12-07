#include "config.h"
#include "rear_ble.h"
#include "timer.h"
#include <avr/io.h>
#include <util/delay.h>

// Simple polled UART implementation compatible with legacy single-char commands
// Uses: RX polling (non-interrupt) and periodic reporting

static volatile uint8_t cmd = 'N';
static uint32_t last_rx_ms = 0;
static uint32_t last_report_ms = 0;
// last values sent to host to avoid noisy periodic prints
static uint8_t last_sent_cmd = 0;
static bool last_sent_link = false;

// UART0 (PD0 RX / PD1 TX) init (polling-friendly)
void rear_ble_init(uint32_t baud)
{
    uint16_t ubrr = (F_CPU / (16UL * baud)) - 1;
    UBRR0H = (ubrr >> 8);
    UBRR0L = ubrr;

    // Enable TX/RX but no RX interrupt
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

// Non-blocking poll; also sends a periodic status message (every 100ms)
void rear_ble_poll(void)
{
    uint32_t now = millis();
    
    // Poll RX
    if (UCSR0A & (1 << RXC0)) {
        uint8_t b = UDR0;
        // Step 1: Update heartbeat IMMEDIATELY (any byte counts as "alive")
        last_rx_ms = now;
        // Step 2: Only update command for known commands L, R, C
        if (b == 'L' || b == 'R' || b == 'C') {
            cmd = b;
        }
        // Note: if b is anything else (noise, etc.), we ignore it for cmd but still update heartbeat
    }

    // Periodic status report (10Hz) but only send when cmd or link state changed
    if ((now - last_report_ms) >= 100) {
        // Compute current link status WITHOUT calling rear_ble_is_link_ok() yet
        // (to avoid the race condition where RX just happened but status doesn't reflect it)
        bool link_now = (now - last_rx_ms) < LINK_LOSS_MS;
        
        if (cmd != last_sent_cmd || link_now != last_sent_link) {
            last_report_ms = now;
            // Send a short ASCII status (non-blocking write)
            while (!(UCSR0A & (1 << UDRE0)));
            UDR0 = 'S';
            while (!(UCSR0A & (1 << UDRE0))); UDR0 = cmd;
            while (!(UCSR0A & (1 << UDRE0))); UDR0 = '\n';
            last_sent_cmd = cmd;
            last_sent_link = link_now;
        }
    }
}

bool rear_ble_is_link_ok(void)
{
    return (millis() - last_rx_ms) < LINK_LOSS_MS;
}

rear_turn_mode_t rear_ble_get_turn_cmd(void)
{
    switch (cmd) {
    case 'L': return TURN_LEFT;
    case 'R': return TURN_RIGHT;
    default:  return TURN_NONE;
    }
}
