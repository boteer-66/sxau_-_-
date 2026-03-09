//
// Created by wpk on 2026/2/10.
//

#ifndef FINISHSERVO_TEXT_SERVOTEXT_BOTEER_H
#define FINISHSERVO_TEXT_SERVOTEXT_BOTEER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "tim.h"
#include <stdbool.h>

    // 舵机参数定义
    typedef struct {
        TIM_HandleTypeDef *htim;    // 定时器句柄
        uint32_t channel;           // 定时器通道
        uint16_t current_pulse;     // 当前脉宽(单位:0.1us)
        uint16_t target_pulse;      // 目标脉宽(单位:0.1us)
        uint16_t min_pulse;         // 最小脉宽(500us = 5000)
        uint16_t max_pulse;         // 最大脉宽(2500us = 25000)
        float move_speed;           // 移动速度(脉宽变化/毫秒)
        uint32_t last_update_time;  // 上次更新时间
        bool is_moving;             // 是否正在移动
    } ServoMotor;

    // 函数声明
    void Servo_Init(ServoMotor *servo, TIM_HandleTypeDef *htim, uint32_t channel);
    void Servo_SetAngle(ServoMotor *servo, uint8_t angle);
    void Servo_SetPulse(ServoMotor *servo, uint16_t pulse_01us);
    void Servo_Update(ServoMotor *servo);
    void Servo_Stop(ServoMotor *servo);
    uint16_t Servo_AngleToPulse(uint8_t angle);
    uint8_t Servo_PulseToAngle(uint16_t pulse);
    void Servo_SetSpeed(ServoMotor *servo, float speed);
    void Servo_SmoothMoveToAngle(ServoMotor *servo, uint8_t target_angle, uint16_t duration_ms);
    void Servo_SmoothMoveToPulse(ServoMotor *servo, uint16_t target_pulse, uint16_t duration_ms);

#ifdef __cplusplus
}
#endif


#endif //FINISHSERVO_TEXT_SERVOTEXT_BOTEER_H