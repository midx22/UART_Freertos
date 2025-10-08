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
#include "dma.h"
#include <string.h>
#include <stdio.h>

// 外部DMA句柄声明
extern DMA_HandleTypeDef hdma_usart3_rx;
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
uint8_t rx_data;  // 接收到的数据（保留用于兼容）
uint8_t rx_buffer[100];    // 接收缓冲区
uint8_t rx_index = 0;      // 缓冲区索引

// DMA接收相关变量
#define DMA_RX_BUF_SIZE 64
uint8_t dma_rx_buf[DMA_RX_BUF_SIZE];  // DMA接收缓冲区
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
  // 使用HAL库高级DMA接收函数（自动处理空闲检测）
  HAL_UARTEx_ReceiveToIdle_DMA(&huart3, dma_rx_buf, DMA_RX_BUF_SIZE);
  // 禁用DMA半传输中断，减少中断次数
  __HAL_DMA_DISABLE_IT(&hdma_usart3_rx, DMA_IT_HT);
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
  Uart_QueueHandle = osMessageQueueNew (64, sizeof(uint8_t), &Uart_Queue_attributes);

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
  
  /* 发送欢迎信息 */
  char welcome[] = "=== UART DMA Variable Length Reception ===\r\n";
  char instruction[] = "Send any data (variable length), it will be echoed back!\r\n";
  char separator[] = "===============================================\r\n\r\n";
  
  HAL_UART_Transmit(&huart3, (uint8_t*)welcome, strlen(welcome), 1000);
  HAL_UART_Transmit(&huart3, (uint8_t*)instruction, strlen(instruction), 1000);
  HAL_UART_Transmit(&huart3, (uint8_t*)separator, strlen(separator), 1000);
  
  /* Infinite loop */
  for(;;)
  {
    /* 主要工作由DMA和空闲中断完成，任务只需要保持运行 */
    /* 发送心跳信息，表示系统正常运行 */
    osDelay(5000);  // 5秒间隔
    HAL_UART_Transmit(&huart3, (uint8_t*)"[System Running...]\r\n", 21, 100);
  }
  /* USER CODE END startUART_task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
 * @brief 处理UART接收数据的通用函数
 * @param data_len: 接收到的数据长度
 * @retval None
 */
void UART_ProcessReceivedData(uint16_t data_len)
{
  if (data_len > 0) {
    /* 发送提示信息 */
    char header[50];
    sprintf(header, "\r\n[Received %d bytes]: ", data_len);
    HAL_UART_Transmit(&huart3, (uint8_t*)header, strlen(header), 100);
    
    /* 直接回显接收到的数据 */
    HAL_UART_Transmit(&huart3, dma_rx_buf, data_len, 1000);
    
    /* 发送结束标记 */
    HAL_UART_Transmit(&huart3, (uint8_t*)"\r\n--End--\r\n\r\n", 12, 100);
    
    /* 重新启动DMA接收 - 使用高级函数 */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart3, dma_rx_buf, DMA_RX_BUF_SIZE);
  }
}

/**
 * @brief HAL库高级DMA接收事件回调函数（替代原来的3个回调函数）
 * @param huart: UART句柄
 * @param Size: 接收到的实际数据长度（HAL库自动计算）
 * @retval None
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART3) {
    /* 直接处理数据，Size就是实际接收长度，无需手动计算 */
    UART_ProcessReceivedData(Size);
  }
}

/* USER CODE END Application */

