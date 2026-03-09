//
// Created by wpk on 2026/2/19.
//
//
// Created by wpk on 2026/2/19.
// 电机控制模块实现（TB6612，双路PWM + 方向引脚）
//

#include "motortext_boteer.h"

#include <stdio.h>
#include <stdlib.h>
#define MOTOR_PWM_MAX 19999

// 内部函数：根据速度值设置左电机方向引脚和PWM
static void Motor_SetLeftHardware(Motor *motor, int16_t speed) {
    // 限幅 -255~255
    if (speed > 255) speed = 255;
    if (speed < -255) speed = -255;

    // 设置方向引脚（根据TB6612真值表）
    if (speed > 0) {
        // 正转：AIN1=1, AIN2=0
        HAL_GPIO_WritePin(motor->ain1_port, motor->ain1_pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(motor->ain2_port, motor->ain2_pin, GPIO_PIN_RESET);
    } else if (speed < 0) {
        // 反转：AIN1=0, AIN2=1
        HAL_GPIO_WritePin(motor->ain1_port, motor->ain1_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(motor->ain2_port, motor->ain2_pin, GPIO_PIN_SET);
    } else {
        // 停止：AIN1=0, AIN2=0（或都高，都停止）
        HAL_GPIO_WritePin(motor->ain1_port, motor->ain1_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(motor->ain2_port, motor->ain2_pin, GPIO_PIN_RESET);
    }

    // 设置PWM占空比（取绝对值）
    uint32_t compare = (abs(speed) * motor->pwm_max) / 255;
    __HAL_TIM_SET_COMPARE(motor->htim_left, motor->channel_left, compare);
   // __HAL_TIM_SET_COMPARE(motor->htim_left, motor->channel_left, abs(speed));
    printf("Left PWM compare: %lu\n", compare);
}

// 内部函数：根据速度值设置右电机方向引脚和PWM
static void Motor_SetRightHardware(Motor *motor, int16_t speed) {
    // 限幅 -255~255
    if (speed > 255) speed = 255;
    if (speed < -255) speed = -255;

    // 设置方向引脚
    if (speed > 0) {
        // 正转：BIN1=1, BIN2=0
        HAL_GPIO_WritePin(motor->bin1_port, motor->bin1_pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(motor->bin2_port, motor->bin2_pin, GPIO_PIN_RESET);
    } else if (speed < 0) {
        // 反转：BIN1=0, BIN2=1
        HAL_GPIO_WritePin(motor->bin1_port, motor->bin1_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(motor->bin2_port, motor->bin2_pin, GPIO_PIN_SET);
    } else {
        // 停止：BIN1=0, BIN2=0
        HAL_GPIO_WritePin(motor->bin1_port, motor->bin1_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(motor->bin2_port, motor->bin2_pin, GPIO_PIN_RESET);
    }

    // 设置PWM占空比
    uint32_t compare = (abs(speed) * motor->pwm_max) / 255;
    __HAL_TIM_SET_COMPARE(motor->htim_right, motor->channel_right, compare);
   // __HAL_TIM_SET_COMPARE(motor->htim_right, motor->channel_right, abs(speed));
    printf("Right PWM compare: %lu\n", compare);
}

// 初始化电机
void Motor_Init(Motor *motor,
                TIM_HandleTypeDef *htim_left, uint32_t ch_left,
                GPIO_TypeDef *ain1_port, uint16_t ain1_pin,
                GPIO_TypeDef *ain2_port, uint16_t ain2_pin,
                TIM_HandleTypeDef *htim_right, uint32_t ch_right,
                GPIO_TypeDef *bin1_port, uint16_t bin1_pin,
                GPIO_TypeDef *bin2_port, uint16_t bin2_pin) {

    // 保存硬件资源
    motor->htim_left = htim_left;
    motor->channel_left = ch_left;
    motor->ain1_port = ain1_port;
    motor->ain1_pin = ain1_pin;
    motor->ain2_port = ain2_port;
    motor->ain2_pin = ain2_pin;

    motor->htim_right = htim_right;
    motor->channel_right = ch_right;
    motor->bin1_port = bin1_port;
    motor->bin1_pin = bin1_pin;
    motor->bin2_port = bin2_port;
    motor->bin2_pin = bin2_pin;

    // 初始化状态
    motor->target_left = 0;
    motor->target_right = 0;
    motor->current_left = 0;
    motor->current_right = 0;
    motor->accel_step = 2.0f;       // 默认加速度，可根据需要调整
    motor->last_update = HAL_GetTick();
    motor->is_moving = false;

    // 启动PWM输出（假设定时器已在CubeMX中初始化）
    HAL_TIM_PWM_Start(motor->htim_left, motor->channel_left);
    HAL_TIM_PWM_Start(motor->htim_right, motor->channel_right);

    // 初始停止
    Motor_SetLeftHardware(motor, 0);
    Motor_SetRightHardware(motor, 0);
    motor->pwm_max = __HAL_TIM_GET_AUTORELOAD(motor->htim_left);
}

// 设置目标速度（立即模式，不平滑）
void Motor_SetSpeed(Motor *motor, int16_t left_speed, int16_t right_speed) {
    // 限幅
    if (left_speed > 255) left_speed = 255;
    if (left_speed < -255) left_speed = -255;
    if (right_speed > 255) right_speed = 255;
    if (right_speed < -255) right_speed = -255;

    // 更新目标速度和当前速度（立即生效）
    motor->target_left = left_speed;
    motor->target_right = right_speed;
    motor->current_left = left_speed;
    motor->current_right = right_speed;

    // 直接设置硬件
    Motor_SetLeftHardware(motor, left_speed);
    Motor_SetRightHardware(motor, right_speed);

    motor->is_moving = (left_speed != 0 || right_speed != 0);
}

// 设置加速度（用于平滑模式）
void Motor_SetAccel(Motor *motor, float accel_step) {
    if (accel_step > 0) {
        motor->accel_step = accel_step;
    }
}

// 平滑更新函数（需要在主循环中定期调用，实现加减速）
void Motor_Update(Motor *motor) {
    if (!motor->is_moving) return;  // 如果目标速度已经等于当前速度，直接返回

    uint32_t now = HAL_GetTick();
    uint32_t dt = now - motor->last_update;
    if (dt == 0) return;

    // 左电机平滑处理
    if (motor->target_left != motor->current_left) {
        int16_t diff = motor->target_left - motor->current_left;
        int16_t step = (int16_t)(motor->accel_step * dt);
        if (abs(diff) <= abs(step)) {
            motor->current_left = motor->target_left;
        } else {
            motor->current_left += (diff > 0 ? step : -step);
        }
        Motor_SetLeftHardware(motor, motor->current_left);
    }

    // 右电机平滑处理
    if (motor->target_right != motor->current_right) {
        int16_t diff = motor->target_right - motor->current_right;
        int16_t step = (int16_t)(motor->accel_step * dt);
        if (abs(diff) <= abs(step)) {
            motor->current_right = motor->target_right;
        } else {
            motor->current_right += (diff > 0 ? step : -step);
        }
        Motor_SetRightHardware(motor, motor->current_right);
    }

    // 检查是否到达目标
    if (motor->current_left == motor->target_left &&
        motor->current_right == motor->target_right) {
        motor->is_moving = false;
    }

    motor->last_update = now;
}

// 停止电机（立即停止）
void Motor_Stop(Motor *motor) {
    Motor_SetSpeed(motor, 0, 0);
}