//
// Created by wpk boteer  波特儿 on 2026/2/10.
//
#include "servotext_boteer.h"
#include <math.h>

// 舵机初始化
void Servo_Init(ServoMotor *servo, TIM_HandleTypeDef *htim, uint32_t channel) {
    servo->htim = htim;
    servo->channel = channel;
    servo->current_pulse = 15000;  // 初始1.5ms (中心位置)
    servo->target_pulse = 15000;
    servo->min_pulse = 5000;       // 0.5ms
    servo->max_pulse = 25000;      // 2.5ms
    servo->move_speed = 10.0f;     // 默认速度: 10us/ms (0-180度约200ms)
    servo->last_update_time = HAL_GetTick();
    servo->is_moving = false;

    // 启动PWM
    HAL_TIM_PWM_Start(servo->htim, servo->channel);

    // 设置初始位置
    __HAL_TIM_SET_COMPARE(servo->htim, servo->channel, servo->current_pulse / 10);
}

// 设置舵机角度(0-180度)
void Servo_SetAngle(ServoMotor *servo, uint8_t angle) {
    if (angle > 180) angle = 180;

    uint16_t pulse = Servo_AngleToPulse(angle);
    Servo_SetPulse(servo, pulse);
}

// 设置舵机脉宽(单位: 微秒)
void Servo_SetPulse(ServoMotor *servo, uint16_t pulse_01us) {
    // 限制脉宽范围
    if (pulse_01us < servo->min_pulse ) {
        pulse_01us = servo->min_pulse ;
    } else if (pulse_01us > servo->max_pulse ) {
        pulse_01us = servo->max_pulse ;
    }

    // // 转换为0.1us单位
    // uint16_t pulse_01us = pulse_us * 10;

    servo->target_pulse = pulse_01us;
    servo->current_pulse = pulse_01us;
    servo->is_moving = false;

    // 立即设置PWM占空比
    __HAL_TIM_SET_COMPARE(servo->htim, servo->channel, pulse_01us/10);
}

// 角度转脉宽(0.1us单位)
uint16_t Servo_AngleToPulse(uint8_t angle) {
    // 角度范围限制
    if (angle > 180) angle = 180;

    // 线性映射: 0°→500us, 180°→2500us
    float pulse_us = 500.0f + (angle / 180.0f) * 2000.0f;

    return (uint16_t)(pulse_us * 10);  // 返回0.1us单位
}

// 脉宽转角度
uint8_t Servo_PulseToAngle(uint16_t pulse_01us) {
    float pulse_us = pulse_01us / 10.0f;

    // 限制脉宽范围
    if (pulse_us < 500.0f) pulse_us = 500.0f;
    if (pulse_us > 2500.0f) pulse_us = 2500.0f;

    // 线性映射
    float angle = ((pulse_us - 500.0f) / 2000.0f) * 180.0f;

    return (uint8_t)angle;
}

// 设置移动速度(单位: 0.1us/ms)
void Servo_SetSpeed(ServoMotor *servo, float speed) {
    if (speed > 0) {
        servo->move_speed = speed;
    }
}

// 平滑移动到指定角度
void Servo_SmoothMoveToAngle(ServoMotor *servo, uint8_t target_angle, uint16_t duration_ms) {
    uint16_t target_pulse = Servo_AngleToPulse(target_angle);
    Servo_SmoothMoveToPulse(servo, target_pulse, duration_ms);
}

// 平滑移动到指定脉宽
void Servo_SmoothMoveToPulse(ServoMotor *servo, uint16_t target_pulse_01us, uint16_t duration_ms) {
    // 限制脉宽范围
    if (target_pulse_01us < servo->min_pulse) {
        target_pulse_01us = servo->min_pulse;
    } else if (target_pulse_01us > servo->max_pulse) {
        target_pulse_01us = servo->max_pulse;
    }

    servo->target_pulse = target_pulse_01us;

    // 计算需要的移动速度
    if (duration_ms > 0) {
        int32_t delta = (int32_t)target_pulse_01us - (int32_t)servo->current_pulse;
        float required_speed = fabsf((float)delta / duration_ms);

        // 如果用户设置的速度更快，使用用户设置的速度
        if (required_speed > servo->move_speed) {
            servo->move_speed = required_speed;
        }
    }

    servo->is_moving = true;
    servo->last_update_time = HAL_GetTick();
}

// 停止舵机移动
void Servo_Stop(ServoMotor *servo) {
    servo->is_moving = false;
}

// 非阻塞更新函数(需要在主循环中定期调用)
void Servo_Update(ServoMotor *servo) {
    if (!servo->is_moving) return;

    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed_time = current_time - servo->last_update_time;

    if (elapsed_time == 0) return;

    // 计算应该移动的距离
    int32_t delta_pulse = (int32_t)servo->target_pulse - (int32_t)servo->current_pulse;

    if (delta_pulse == 0) {
        servo->is_moving = false;
        return;
    }

    // 计算本次更新应该移动的距离
    float move_distance = servo->move_speed * elapsed_time;

    if (fabsf((float)delta_pulse) <= move_distance) {
        // 已经到达或超过目标位置
        servo->current_pulse = servo->target_pulse;
        servo->is_moving = false;
    } else {
        // 向目标位置移动
        if (delta_pulse > 0) {
            servo->current_pulse += (int32_t)move_distance;
        } else {
            servo->current_pulse -= (int32_t)move_distance;
        }
    }

    // 更新PWM占空比(转换为us单位)
    __HAL_TIM_SET_COMPARE(servo->htim, servo->channel, servo->current_pulse / 10);

    servo->last_update_time = current_time;

}