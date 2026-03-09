//
// Created by wpk on 2026/2/19.
//
//
// Created by wpk on 2026/2/19.
//

#ifndef FUCKYOUUSART_MOTORTEXT_BOTEER_H
#define FUCKYOUUSART_MOTORTEXT_BOTEER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "tim.h"
#include <stdbool.h>
#include <math.h>

    // 电机状态结构体（类似ServoMotor的设计思路）
    typedef struct {
        // 左电机资源
        TIM_HandleTypeDef *htim_left;       // 左电机PWM定时器（TIM4）
        uint32_t channel_left;               // 左电机PWM通道（TIM_CHANNEL_1）
        GPIO_TypeDef *ain1_port;             // AIN1 引脚端口
        uint16_t ain1_pin;                   // AIN1 引脚号
        GPIO_TypeDef *ain2_port;             // AIN2 引脚端口
        uint16_t ain2_pin;                   // AIN2 引脚号

        // 右电机资源
        TIM_HandleTypeDef *htim_right;       // 右电机PWM定时器（TIM4）
        uint32_t channel_right;               // 右电机PWM通道（TIM_CHANNEL_2）
        GPIO_TypeDef *bin1_port;             // BIN1 引脚端口
        uint16_t bin1_pin;                   // BIN1 引脚号
        GPIO_TypeDef *bin2_port;             // BIN2 引脚端口
        uint16_t bin2_pin;                   // BIN2 引脚号

        // 状态变量（用于平滑加减速）
        int16_t target_left;      // 目标速度 -255~255
        int16_t target_right;
        int16_t current_left;
        int16_t current_right;
        float accel_step;         // 加速度步长（速度变化量/毫秒）
        uint32_t last_update;
        bool is_moving;           // 是否正在运动
/////////////////////////////////
        uint16_t pwm_max;
    } Motor;

    // 函数声明
    void Motor_Init(Motor *motor,
                    TIM_HandleTypeDef *htim_left, uint32_t ch_left,
                    GPIO_TypeDef *ain1_port, uint16_t ain1_pin,
                    GPIO_TypeDef *ain2_port, uint16_t ain2_pin,
                    TIM_HandleTypeDef *htim_right, uint32_t ch_right,
                    GPIO_TypeDef *bin1_port, uint16_t bin1_pin,
                    GPIO_TypeDef *bin2_port, uint16_t bin2_pin);

    void Motor_SetSpeed(Motor *motor, int16_t left_speed, int16_t right_speed);
    void Motor_Update(Motor *motor);
    void Motor_Stop(Motor *motor);
    void Motor_SetAccel(Motor *motor, float accel_step);  // 设置加速度

#ifdef __cplusplus
}
#endif


#endif //FUCKYOUUSART_MOTORTEXT_BOTEER_H