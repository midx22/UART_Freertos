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
#include "stream_buffer.h"

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

// 流缓冲区相关变量
#define STREAM_BUFFER_SIZE 512
#define STREAM_TRIGGER_LEVEL 1          // 触发级别：收到1个字节就唤醒任务
StreamBufferHandle_t uart_stream_buffer;       // 流缓冲区句柄
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
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for Uart_Queue */
osMessageQueueId_t Uart_QueueHandle;
const osMessageQueueAttr_t Uart_Queue_attributes = {
  .name = "Uart_Queue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void process_uart_data(uint8_t* data, size_t length);
void check_task_stack_usage(void);
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
  // 检查初始堆内存
  uint32_t free_heap_before = xPortGetFreeHeapSize();
  
  // 创建流缓冲区
  uart_stream_buffer = xStreamBufferCreate(STREAM_BUFFER_SIZE, STREAM_TRIGGER_LEVEL);
  
  // 检查流缓冲区是否创建成功
  if (uart_stream_buffer == NULL) {
    // 流缓冲区创建失败 - 可能内存不足
    Error_Handler();
  }
  
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
  char welcome[] = "=== Simple Stream Buffer Echo ===\r\n"
                   "Commands:\r\n"
                   "  'stack' - Show task stack usage\r\n"
                   "  Other text will be echoed\r\n"
                   "Type something:\r\n";
  HAL_UART_Transmit(&huart3, (uint8_t*)welcome, strlen(welcome), 1000);
  
  uint8_t rx_buffer[128];
  size_t bytes_read;
  
  /* Infinite loop */
  for(;;)
  {
    /* 从流缓冲区接收数据 - 阻塞等待 */
    bytes_read = xStreamBufferReceive(uart_stream_buffer, 
                                    rx_buffer, 
                                    sizeof(rx_buffer) - 1, 
                                    portMAX_DELAY);
    
    if (bytes_read > 0) {
      rx_buffer[bytes_read] = '\0';
      process_uart_data(rx_buffer, bytes_read);
    }
  }
  osdelay(10);
  /* USER CODE END startUART_task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
 * @brief UART接收事件回调函数（DMA + Idle中断）- 最简版本
 * @param huart: UART句柄
 * @param Size: 接收到的数据长度
 * @retval None
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART3) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    /* 将DMA接收到的数据发送到流缓冲区 */
    xStreamBufferSendFromISR(uart_stream_buffer, 
                           dma_rx_buf, 
                           Size, 
                           &xHigherPriorityTaskWoken);
    
    /* 重新启动DMA接收 */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart3, dma_rx_buf, DMA_RX_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(&hdma_usart3_rx, DMA_IT_HT);
    
    /* 执行任务切换 */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
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
 * @brief 处理UART接收到的数据 - 最简版本
 * @param data: 接收到的数据
 * @param length: 数据长度
 * @retval None
 */
void process_uart_data(uint8_t* data, size_t length)
{
  /* 检查是否是查看栈使用情况的命令 */
  if (strncmp((char*)data, "stack", 5) == 0) {
    check_task_stack_usage();
    return;
  }
  
  /* 简单回显 */
  HAL_UART_Transmit(&huart3, (uint8_t*)"Echo: ", 6, 100);
  HAL_UART_Transmit(&huart3, data, length, 100);
  HAL_UART_Transmit(&huart3, (uint8_t*)"\r\n", 2, 100);
}

/**
 * @brief 检查当前UART任务栈使用情况
 * @retval None
 */
void check_task_stack_usage(void)
{
  /* 获取当前UART任务的栈剩余空间 */
  UBaseType_t stack_remaining = uxTaskGetStackHighWaterMark(UART_taskHandle);
  
  /* 计算栈使用情况 - 使用正确的栈大小 */
  uint32_t total_stack = 256 * 4;  // 总栈大小784字节 (与UART_task_attributes一致)
  uint32_t used_stack = total_stack - (stack_remaining * sizeof(StackType_t));
  uint32_t usage_percent = (used_stack * 100) / total_stack;
  
  /* 简单输出 */
  char info[100];
  int len = sprintf(info, 
    "\r\nUART Task Stack: %lu/%lu bytes (%lu%% used)\r\n\r\n",
    used_stack, total_stack, usage_percent);
  
  HAL_UART_Transmit(&huart3, (uint8_t*)info, len, 500);
}



/* USER CODE END Application */

