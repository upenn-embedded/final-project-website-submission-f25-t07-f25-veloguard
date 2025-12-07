#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/wdt.h>
#include <math.h>

#include "config.h"
#include "common.h"
#include "timer.h"
#include "us_sensor.h"
#include "rear_ble.h"
#include "state_machine.h"
#include "buzzer.h"
#include "imu.h"
#include "brake_led.h"
#include "turn_left.h"
#include "turn_right.h"

static void uart_putc(char c)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

static int uart_putc_printf(char c, FILE *stream)
{
    uart_putc(c);
    return 0;
}

FILE mystdout = FDEV_SETUP_STREAM(uart_putc_printf, NULL, _FDEV_SETUP_WRITE);

int main(void)
{
    stdout = &mystdout;

    // 打印复位原因，方便定位是否发生了反复复位
    uint8_t reset_flags = MCUSR;
    if (reset_flags) {
        printf("Reset flags: 0x%02X -", reset_flags);
        if (reset_flags & (1 << PORF)) printf(" POR");
        if (reset_flags & (1 << EXTRF)) printf(" EXT");
        if (reset_flags & (1 << BORF)) printf(" BOR");
        if (reset_flags & (1 << WDRF)) printf(" WDT");
        printf("\n");
    }
    // 清除复位标志并禁用看门狗（避免上次未清的 WDT 导致循环复位）
    MCUSR = 0;
    wdt_disable();

    cli();
    timer_init();
    us_sensor_init();
    buzzer_init();
    rear_ble_init(9600);
    brake_led_init();
    turn_left_init();
    turn_right_init();
    imu_init();
    sei();

    state_machine_init();

    printf("Rear module boot OK\n");

    while (1)
    {
        rear_ble_poll();
        float dist = us_sensor_get_distance_m();
        rear_turn_mode_t turn = rear_ble_get_turn_cmd();
        bool link_ok = rear_ble_is_link_ok();

        // 更新 IMU 状态（读取传感器）
        imu_update();

        // 读取 X 轴加速度（去除偏差，单位 g）
        float ax_g = imu_get_ax_g();

        // 判断是否刹车（X 轴负加速度）
        bool decel = (ax_g < -DECEL_THRESHOLD_G);

        // reset for testing
        //dist = 3;
        link_ok = 1;
        //turn = TURN_LEFT;

        state_machine_update(dist, turn, link_ok, decel);

        rear_state_t s = state_machine_get();
        buzzer_update(s.brake, dist);
        brake_led_update(s.brake);
        
        // Turn LED: waterfall animation updates internally
        switch (turn) {
        case TURN_LEFT:
            turn_right_set_group(6);
            for (uint8_t i = 0; i < 6; i++) {
                turn_left_set_group(i);
                _delay_ms(120);
                _delay_ms(500);
            }
            turn_left_set_group(6);
            break;
        case TURN_RIGHT:
            turn_left_set_group(6);
            for (uint8_t i = 0; i < 6; i++) {
                turn_right_set_group(i);
                _delay_ms(120);
                _delay_ms(500);
            }
            turn_right_set_group(6);
            break; // <--- ?????
        default:
            turn_left_set_group(6);
            turn_right_set_group(6);
            break;
        }

        // 输出条件：距离报警 OR 刹车激活 OR 无链接
        if (dist < D_ALERT || s.brake != BRAKE_IDLE || !link_ok) {
            printf("ax=%.2f dist=%.2f brake=%d link=%d\n", ax_g, dist, s.brake, link_ok);
        }

        _delay_ms(50);
    }
    
    return 0;
}
