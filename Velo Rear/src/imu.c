#include "imu.h"
#include "config.h"
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "timer.h"

// -------- LSM6DSO 地址 & 寄存器 --------
#define IMU_ADDR_7BIT     0x6B       // 如果 SA0 接 3V3 改成 0x6B
#define IMU_ADDR_WRITE    (IMU_ADDR_7BIT << 1)
#define IMU_ADDR_READ     ((IMU_ADDR_7BIT << 1) | 1)

#define REG_WHO_AM_I      0x0F
#define REG_CTRL1_XL      0x10
#define REG_CTRL3_C       0x12
#define REG_OUTX_L_A      0x28   // 连续到 OUTZ_H_A

// ±4g 灵敏度：0.122 mg/LSB = 0.000122 g/LSB
#define LSM6DSO_SENS_4G_G (0.122f / 1000.0f)

// Internal cached state (declare early so functions above can use them)
static float _imu_cached_ax_g = 0.0f;

// accelerometer startup biases (in g) — measured during imu_calibrate()
static float _imu_bias_ax = 0.0f;
static float _imu_bias_ay = 0.0f;
static float _imu_bias_az = 1.0f; // assume 1g on Z by default

// -------- TWI0 (I2C) 基本操作 --------
static void twi0_init(void)
{
    // 关掉 TWI0 的省电 (PRR0 的 PRTWI0 位)
    PRR0 &= ~(1 << PRTWI0);    // 使能 TWI0 外设:contentReference[oaicite:4]{index=4}

    // PC4/PC5 设为输入，关掉内部上拉（Qwiic 那边有 3V3 上拉）
    DDRC  &= ~((1 << PC4) | (1 << PC5));
    PORTC &= ~((1 << PC4) | (1 << PC5));

    // 预分频 = 1
    TWSR0 = 0x00;

    // SCL = F_CPU / (16 + 2*TWBR0) 这里假设 F_CPU=16MHz, TWBR0=72 -> ~100kHz:contentReference[oaicite:5]{index=5}
    TWBR0 = 72;

    // 使能 TWI
    TWCR0 = (1 << TWEN);
}

static bool twi0_wait(void)
{
    uint32_t timeout = 0;
    const uint32_t TIMEOUT_MAX = 100000UL;
    while (!(TWCR0 & (1 << TWINT))) {
        if (++timeout >= TIMEOUT_MAX) {
            return false;
        }
    }
    return true;
}

static bool twi0_start(void)
{
    TWCR0 = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    if (!twi0_wait()) {
        return false;
    }
    return true;
}

static void twi0_stop(void)
{
    TWCR0 = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

static bool twi0_write(uint8_t data)
{
    TWDR0 = data;
    TWCR0 = (1 << TWINT) | (1 << TWEN);
    if (!twi0_wait()) {
        return false;
    }
    // 检查 TWI 状态，判断是否收到 ACK（仅在写地址/数据时有用）
    uint8_t status = TWSR0 & 0xF8;
    if (status == 0x20) { // SLA+W 或 数据 NACK
        return false;
    }
    return true;
}

static int twi0_read_ack(void)
{
    TWCR0 = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    if (!twi0_wait()) {
        printf("imu: twi0_read_ack timeout\n");
        return -1;
    }
    return (int)TWDR0;
}

static int twi0_read_nack(void)
{
    TWCR0 = (1 << TWINT) | (1 << TWEN);
    if (!twi0_wait()) {
        printf("imu: twi0_read_nack timeout\n");
        return -1;
    }
    return (int)TWDR0;
}

// 写单个寄存器
static void imu_write_reg(uint8_t reg, uint8_t value)
{
    if (!twi0_start()) { twi0_stop(); return; }
    if (!twi0_write(IMU_ADDR_WRITE)) { twi0_stop(); return; }
    if (!twi0_write(reg)) { twi0_stop(); return; }
    if (!twi0_write(value)) { twi0_stop(); return; }
    twi0_stop();
}

// 连续读多个寄存器（IF_INC 已开）
static void imu_read_regs(uint8_t reg, uint8_t *buf, uint8_t len)
{
    if (!twi0_start()) { twi0_stop(); return; }
    if (!twi0_write(IMU_ADDR_WRITE)) { twi0_stop(); return; }
    if (!twi0_write(reg)) { twi0_stop(); return; }

    // 重复起始，切到读
    if (!twi0_start()) { twi0_stop(); return; }
    if (!twi0_write(IMU_ADDR_READ)) { twi0_stop(); return; }

    for (uint8_t i = 0; i < len; i++) {
        int v;
        if (i == (len - 1))
            v = twi0_read_nack();
        else
            v = twi0_read_ack();

        if (v < 0) { twi0_stop(); return; }
        buf[i] = (uint8_t)v;
    }
    twi0_stop();
}

void imu_init(void)
{
    twi0_init();

    // perform a short calibration to estimate accelerometer bias (device should be stationary)
    imu_calibrate();



    // Initialization complete, subsequent data will be obtained by reading registers

    // CTRL3_C：BDU=1, IF_INC=1 其余用默认值
    // BOOT BDU H_LACTIVE PP_OD SIM IF_INC 0 SW_RESET
    // 0    1   0         0     0   1      0 0  => 0b01000100 = 0x44:contentReference[oaicite:7]{index=7}
    imu_write_reg(REG_CTRL3_C, 0x44);

    // CTRL1_XL：加速度计 ODR=104Hz，FS=±4g，高性能模式（XL_HM_MODE=0）
    // [7:0] = ODR_XL3..0, FS1_XL, FS0_XL, LPF2_XL_EN, 0
    // ODR = 0100b -> 104Hz；FS=10b -> ±4g；LPF2=0 -> 0
    // 即 0b01001000 = 0x48:contentReference[oaicite:8]{index=8}
    imu_write_reg(REG_CTRL1_XL, 0x48);

    _delay_ms(50);  // 等 IMU 上电稳定
}

float imu_get_ax_g(void)
{
    // 返回最近一次更新的 X 轴读数（如果未更新则执行一次更新）
    // 为简单起见，直接调用 imu_update() 保证数据是最新的
    imu_update();
    return _imu_cached_ax_g;
}

// 更新 IMU 状态（读取传感器），应在主循环定期调用
void imu_update(void)
{
    uint8_t buf[6];
    imu_read_regs(REG_OUTX_L_A, buf, 6);

    int16_t raw_x = (int16_t)((buf[1] << 8) | buf[0]);
    int16_t raw_y = (int16_t)((buf[3] << 8) | buf[2]);
    int16_t raw_z = (int16_t)((buf[5] << 8) | buf[4]);

    float ax = raw_x * LSM6DSO_SENS_4G_G;
    float ay = raw_y * LSM6DSO_SENS_4G_G;
    float az = raw_z * LSM6DSO_SENS_4G_G;

    // Subtract startup bias per-axis (simple static calibration model)
    float ax_corr = ax - _imu_bias_ax;
    float ay_corr = ay - _imu_bias_ay;
    float az_corr = az - _imu_bias_az;

    // Small deadzone to suppress sensor noise on X
    if (fabsf(ax_corr) < 0.02f) ax_corr = 0.0f;

    _imu_cached_ax_g = ax_corr;
}

// Simple static calibration: average IMU_CAL_SAMPLES readings while stationary
void imu_calibrate(void)
{
    uint32_t samples = IMU_CAL_SAMPLES;
    float sumx = 0.0f, sumy = 0.0f, sumz = 0.0f;
    for (uint32_t i = 0; i < samples; i++) {
        uint8_t buf[6];
        imu_read_regs(REG_OUTX_L_A, buf, 6);
        int16_t rx = (int16_t)((buf[1] << 8) | buf[0]);
        int16_t ry = (int16_t)((buf[3] << 8) | buf[2]);
        int16_t rz = (int16_t)((buf[5] << 8) | buf[4]);
        float ax = rx * LSM6DSO_SENS_4G_G;
        float ay = ry * LSM6DSO_SENS_4G_G;
        float az = rz * LSM6DSO_SENS_4G_G;
        sumx += ax;
        sumy += ay;
        sumz += az;
        _delay_ms(5);
    }
    
        // Compute average gravity vector components (g)
        float avg_ax = sumx / (float)samples;
        float avg_ay = sumy / (float)samples;
        float avg_az = sumz / (float)samples;

        // Store per-axis startup bias (includes gravity). Later we subtract this
        // bias from raw readings to obtain linear acceleration.
        _imu_bias_ax = avg_ax;
        _imu_bias_ay = avg_ay;
        _imu_bias_az = avg_az;

        // Debug: print calibration results so user can verify orientation
        printf("imu_calibrate: bias_ax=%.4f g, ay=%.4f g, az=%.4f g\n",
            _imu_bias_ax, _imu_bias_ay, _imu_bias_az);
}
