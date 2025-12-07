#ifndef CONFIG_H
#define CONFIG_H

// Distance thresholds (meters)
#define D_WARN      1.0f
#define D_ALERT     0.5f

#define F_CPU 16000000UL
// Link-loss timeout
#define LINK_LOSS_MS   1000

// Buzzer pin
#define BUZZER_PORT PORTB
#define BUZZER_DDR  DDRB
#define BUZZER_PIN  PB2

// Ultrasonic TRIG
#define US_TRIG_PORT PORTB
#define US_TRIG_DDR  DDRB
#define US_TRIG_PIN  PB1

// IMU deceleration threshold (in g)
#define DECEL_THRESHOLD_G   0.20f

// IMU calibration settings
#define IMU_CAL_SAMPLES 200

// LED pins (re-mapped)
// Turn signals: use PB3 (left) and PB4 (right)
#define TURN_LEFT_PORT  PORTB
#define TURN_LEFT_DDR   DDRB
#define TURN_LEFT_PIN   PB3

#define TURN_RIGHT_PORT PORTB
#define TURN_RIGHT_DDR  DDRB
#define TURN_RIGHT_PIN  PB4

// Brake LED: use PB5
#define BRAKE_LED_PORT PORTB
#define BRAKE_LED_DDR  DDRB
#define BRAKE_LED_PIN  PB5


#endif
