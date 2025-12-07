#ifndef IMU_H
#define IMU_H

void imu_init(void);

// 读取 X 轴加速度，单位 g（假设 X 轴沿车前进方向）
// 自动移除校准时测得的偏差
float imu_get_ax_g(void);

// 更新 IMU 状态（读取传感器），应在主循环定期调用
void imu_update(void);

// 执行静态校准以估计加速度偏差（在启动时调用一次）
void imu_calibrate(void);

#endif
