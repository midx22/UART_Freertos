/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
#include <string.h>
#include <stdio.h>
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
/* USER CODE BEGIN Variables */
// 串口接收变量
uint8_t rx_data;  // 接收到的数据
uint8_t rx_buffer[100];    // 接收缓冲区
uint8_t rx_index = 0;      // 缓冲区索引
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for UART_task */
osThreadId_t UART_taskHandle;
const osThreadAttr_t UART_task_attributes = {
  .name = "UART_task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for Uart_Queue */
osMessageQueueId_t Uart_QueueHandle;
const osMessageQueueAttr_t Uart_Queue_attributes = {
  .name = "Uart_Queue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void startUART_task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  // 启动UART中断接收
  HAL_UART_Receive_IT(&huart3, &rx_data, 1);
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of Uart_Queue */
  Uart_QueueHandle = osMessageQueueNew (16, sizeof(uint16_t), &Uart_Queue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of UART_task */
  UART_taskHandle = osThreadNew(startUART_task, NULL, &UART_task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_startUART_task */
/**
* @brief Function implementing the UART_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_startUART_task */
void startUART_task(void *argument)
{
  /* USER CODE BEGIN startUART_task */
  uint8_t received_char;
  osStatus_t status;
  
  /* 发送欢迎信息 */
  char welcome[] = "=== Advanced UART with Queue ===\r\nType something and press Enter:\r\n";
  HAL_UART_Transmit(&huart3, (uint8_t*)welcome, strlen(welcome), 1000);
  
  /* Infinite loop */
  for(;;)
  {
    /* 从队列中获取数据，等待500ms */
    status = osMessageQueueGet(Uart_QueueHandle, &received_char, NULL, 500);
    
    if (status == osOK) {
      /* 成功从队列获取数据 */
      
      /* 检查是否为回车或换行 */
      if (received_char == '\r' || received_char == '\n') {
        if (rx_index > 0) {
          /* 添加字符串结束符 */
          rx_buffer[rx_index] = '\0';
          
          /* 发送回显信息 */
          HAL_UART_Transmit(&huart3, (uint8_t*)"\r\nYou typed: ", 13, 100);
          HAL_UART_Transmit(&huart3, rx_buffer, rx_index, 100);
          HAL_UART_Transmit(&huart3, (uint8_t*)"\r\nLength: ", 10, 100);
          
          /* 发送长度信息 */
          char length_str[10];
          sprintf(length_str, "%d\r\n\r\n", rx_index);
          HAL_UART_Transmit(&huart3, (uint8_t*)length_str, strlen(length_str), 100);
          
          /* 重置缓冲区 */
          rx_index = 0;
          memset(rx_buffer, 0, sizeof(rx_buffer));
        }
      } else {
        /* 普通字符，存储到缓冲区 */
        if (rx_index < sizeof(rx_buffer) - 1) {
          rx_buffer[rx_index++] = received_char;
          
          /* 回显字符 */
          HAL_UART_Transmit(&huart3, &received_char, 1, 100);
        }
      }
    } else {
      /* 队列超时，发送心跳信息 */
      HAL_UART_Transmit(&huart3, (uint8_t*)".", 1, 100);
    }
  }
  /* USER CODE END startUART_task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
 * @brief UART接收完成中断回调函数
 * @param huart: UART句柄
 * @retval None
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART3) {
    /* 将接收到的字符发送到队列 */
    osMessageQueuePut(Uart_QueueHandle, &rx_data, 0, 0);
    
    /* 继续接收下一个字符 */
    HAL_UART_Receive_IT(&huart3, &rx_data, 1);
  }
}

/**
 * @brief UART错误回调函数
 * @param huart: UART句柄
 * @retval None
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART3) {
    /* 发生错误时重新启动接收 */
    HAL_UART_Receive_IT(&huart3, &rx_data, 1);
  }
}

/* USER CODE END Application */

