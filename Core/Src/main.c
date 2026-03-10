/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

#include <stdio.h>
#include <string.h>

#include "servotext_boteer.h"   // 你的舵机头文件
#include "servotext_boteer.c"
#include "motortext_boteer.h"

#define RX_RING_SIZE 128                // 环形缓冲区大小
uint8_t rx_ring[RX_RING_SIZE];          // 环形缓冲区
volatile uint16_t rx_ring_head = 0;      // 写指针（中断中更新）
volatile uint16_t rx_ring_tail = 0;      // 读指针（主循环中更新）
uint8_t rx_byte;                         // 单字节接收缓存（用于中断）

//原有的 rx_buffer 现在用作行缓冲区，不再直接参与接收，rx_index 和 rx_complete_flag 继续保留，用法不变
#define RX_BUFFER_SIZE  64      // 接收缓冲区大小
uint8_t rx_buffer[RX_BUFFER_SIZE];      // 串口接收缓冲区
uint8_t rx_index = 0;                  // 当前接收位置
uint8_t rx_complete_flag = 0;          // 收到一帧完成标志


ServoMotor servo1;    //TIM1 CH1  引脚PA8
ServoMotor servo2;    //TIM1 CH2  引脚PA9
ServoMotor servo3;    //TIM2 CH1  引脚PA0
ServoMotor servo4;    //TIM2 CH2  引脚PA1
ServoMotor servo5;    //TIM2 CH3  引脚PB10

Motor my_motor;

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
PUTCHAR_PROTOTYPE {
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}



/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//改版添加了行提取函数
static uint8_t GetLineFromRing(char *line_buf, uint16_t max_len)
{
  static uint16_t idx = 0;  // 当前行缓冲区索引

  while (rx_ring_tail != rx_ring_head)
  {
    uint8_t ch = rx_ring[rx_ring_tail];
    rx_ring_tail = (rx_ring_tail + 1) % RX_RING_SIZE;

    // 遇到换行符或缓冲区满，认为一行结束
    if (ch == '\n' || ch == '\r' || idx >= max_len - 1)
    {
      // 忽略空行（只有换行符）
      if (idx == 0)
      {
        continue;
      }
      line_buf[idx] = '\0';  // 字符串结束
      idx = 0;
      return 1;  // 一行就绪
    }
    else
    {
      line_buf[idx++] = ch;  // 存入行缓冲区
    }
  }
  return 0;  // 没有完整行
}





void Parse_Command(char *cmd, uint16_t len)
{
  // 去除末尾换行
  if (len > 0 && (cmd[len-1] == '\n' || cmd[len-1] == '\r')) cmd[len-1] = '\0';
  if (len > 1 && cmd[len-2] == '\r') cmd[len-2] = '\0';

  printf("Received: [%s]\r\n", cmd);

  int left, right;int id, angle;
  if (sscanf(cmd, "#M%d,%d", &left, &right) == 2) {
    Motor_SetSpeed(&my_motor, (int16_t)left, (int16_t)right);
    printf("Motor set: L=%d, R=%d\r\n", left, right);
  }
   else if (sscanf(cmd, "#%d=%d", &id, &angle) == 2)
  {
    if (id >= 1 && id <= 5 && angle >= 0 && angle <= 180)
    {
      // 根据id选择对应的舵机
      switch (id)
      {
      case 1: Servo_SetAngle(&servo1, angle); break;
      case 2: Servo_SetAngle(&servo2, angle); break;
      case 3: Servo_SetAngle(&servo3, angle); break;
      case 4: Servo_SetAngle(&servo4, angle); break;
      case 5: Servo_SetAngle(&servo5, angle); break;
      default: break;
      }
      printf("OK: Servo%d set to %d\r\n", id, angle);
    }
    else
    {
      printf("Error: ID must be 1-5, angle 0-180\r\n");
    }
  }
  else
  {
    printf("Unknown command. Use #<id>=<angle> (e.g. #1=90)\r\n");
  }
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */


  Servo_Init(&servo1, &htim1, TIM_CHANNEL_1);
  Servo_Init(&servo2, &htim1, TIM_CHANNEL_2);
  Servo_Init(&servo3, &htim2, TIM_CHANNEL_1);
  Servo_Init(&servo4, &htim2, TIM_CHANNEL_2);
  Servo_Init(&servo5, &htim2, TIM_CHANNEL_3);


  //Servo_SetSpeed(&my_servo, 50.0f);
  Servo_SetSpeed(&servo1, 50.0f);
  Servo_SetSpeed(&servo2, 50.0f);
  Servo_SetSpeed(&servo3, 50.0f);
  Servo_SetSpeed(&servo4, 50.0f);
  Servo_SetSpeed(&servo5, 50.0f);


  Motor_Init(&my_motor,
           &htim4, TIM_CHANNEL_1,    // 左PWM
           GPIOA, GPIO_PIN_4,         // AIN1
           GPIOA, GPIO_PIN_5,         // AIN2
           &htim4, TIM_CHANNEL_2,    // 右PWM
           GPIOA, GPIO_PIN_6,         // BIN1
           GPIOA, GPIO_PIN_7);        // BIN2

  // 设置加速度（可选，默认为2.0）
  Motor_SetAccel(&my_motor, 1.5f);




  HAL_UART_Receive_IT(&huart2, &rx_byte, 1);

  printf("System Ready. 5 servos initialized.\r\n");
  printf("\r\n====波特儿 Servo Control System====\r\n");
  printf("   等待输入指令中...............\r\n");
  printf("   输入格式:ANGLE=xxx (0-180)\r\n");
  printf(" ==========祝你调试顺利===========\r\n");


  uint32_t last_time = HAL_GetTick();
  int test_state = 0;  // 0:正转, 1:反转, 2:停止



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {


    Servo_Update(&servo1);
    Servo_Update(&servo2);
    Servo_Update(&servo3);
    Servo_Update(&servo4);
    Servo_Update(&servo5);

    Motor_Update(&my_motor);


    // 【新增】从环形缓冲区读取一行，存入 rx_buffer
    if (GetLineFromRing((char *)rx_buffer, RX_BUFFER_SIZE))
    {
      rx_index = strlen((char *)rx_buffer);  // 计算实际长度
      rx_complete_flag = 1;                   // 触发解析
    }

    if (rx_complete_flag)
    {
      Parse_Command((char *)rx_buffer, rx_index);

      memset(rx_buffer, 0, RX_BUFFER_SIZE);
      rx_index = 0;
      rx_complete_flag = 0;

      // 重新开启接收
      //改版部分该部分代码已被删除，接受将由环形缓冲区持续处理
      //HAL_UART_Receive_IT(&huart2, rx_buffer, 1);
    }



    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    // 将字节存入环形缓冲区（如果未满）
    uint16_t next_head = (rx_ring_head + 1) % RX_RING_SIZE;
    if (next_head != rx_ring_tail)
    {
      rx_ring[rx_ring_head] = rx_byte;
      rx_ring_head = next_head;
    }
    // 重新启动接收（使用 rx_byte 作为缓存）
    HAL_UART_Receive_IT(huart, &rx_byte, 1);
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    __HAL_UART_CLEAR_OREFLAG(huart);
    __HAL_UART_CLEAR_NEFLAG(huart);
    __HAL_UART_CLEAR_FEFLAG(huart);
    HAL_UART_Receive_IT(huart, &rx_byte, 1);
  }
}



/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
