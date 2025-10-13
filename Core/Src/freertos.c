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
#include "dma.h"
#include "ringbuffer.h"

/* 外部变量声明 */
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
// DMA接收相关变量
#define DMA_RX_BUF_SIZE 128
uint8_t dma_rx_buf[DMA_RX_BUF_SIZE];  // DMA接收缓冲区

// 环形缓冲区相关变量
#define RING_BUFFER_SIZE 512
uint8_t ring_buffer_pool[RING_BUFFER_SIZE];     // 环形缓冲区内存池
struct rt_ringbuffer uart_rx_ringbuf;          // 环形缓冲区对象
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
void process_uart_data(uint8_t* data, int length);
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
  // 初始化环形缓冲区
  rt_ringbuffer_init(&uart_rx_ringbuf, ring_buffer_pool, RING_BUFFER_SIZE);
  
  // 启动DMA + Idle中断接收
  HAL_UARTEx_ReceiveToIdle_DMA(&huart3, dma_rx_buf, DMA_RX_BUF_SIZE);
  
  // 禁用DMA的半完成中断（只保留完成中断和Idle中断）
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
  /* 发送欢迎信息 */
  char welcome[] = "=== Simple UART Echo ===\r\nType something:\r\n";
  HAL_UART_Transmit(&huart3, (uint8_t*)welcome, strlen(welcome), 1000);
  
  uint8_t rx_buffer[256];
  int bytes_read;  // 改为int类型，更简单
  
  /* Infinite loop */
  for(;;)
  {
    /* 检查是否有数据 */
    if (rt_ringbuffer_data_len(&uart_rx_ringbuf) > 0) {
      /* 读取数据 */
      bytes_read = rt_ringbuffer_get(&uart_rx_ringbuf, rx_buffer, sizeof(rx_buffer) - 1);
      
      if (bytes_read > 0) {
        rx_buffer[bytes_read] = '\0';
        process_uart_data(rx_buffer, bytes_read);
      }
    }
    
    osDelay(50);  // 增加延时，降低CPU占用
  }
  /* USER CODE END startUART_task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
 * @brief UART接收事件回调函数（DMA + Idle中断）
 * @param huart: UART句柄
 * @param Size: 接收到的数据长度
 * @retval None
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART3) {
    /* 方案A：继续使用环形缓冲区（推荐） */
    rt_size_t written = rt_ringbuffer_put(&uart_rx_ringbuf, dma_rx_buf, Size);
    if (written < Size) {
      char warning[] = "\r\n[WARNING] Ring buffer overflow!\r\n";
      HAL_UART_Transmit(&huart3, (uint8_t*)warning, strlen(warning), 100);
    }
    
    /* 方案B：最简单 - 直接在中断中回显（取消下面注释即可使用）
    dma_rx_buf[Size] = '\0';
    HAL_UART_Transmit(&huart3, (uint8_t*)"\r\nEcho: ", 8, 100);
    HAL_UART_Transmit(&huart3, dma_rx_buf, Size, 100);
    HAL_UART_Transmit(&huart3, (uint8_t*)"\r\n", 2, 100);
    */
    
    /* 重新启动DMA接收 */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart3, dma_rx_buf, DMA_RX_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(&hdma_usart3_rx, DMA_IT_HT);
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
    /* 发生错误时重新启动DMA接收 */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart3, dma_rx_buf, DMA_RX_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(&hdma_usart3_rx, DMA_IT_HT);
  }
}

/**
 * @brief 处理UART接收到的数据 - 超简化版本
 * @param data: 接收到的数据
 * @param length: 数据长度
 * @retval None
 */
void process_uart_data(uint8_t* data, int length)
{
  /* 最简单的回显：发送什么，返回什么 */
  HAL_UART_Transmit(&huart3, (uint8_t*)"Echo: ", 6, 100);
  HAL_UART_Transmit(&huart3, data, length, 100);
  HAL_UART_Transmit(&huart3, (uint8_t*)"\r\n", 2, 100);
}

/* USER CODE END Application */

