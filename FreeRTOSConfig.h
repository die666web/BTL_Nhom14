#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>
#include "board_config.h"
void app_assert_failed(const char *file, unsigned long line);
#define configASSERT(x) do { if (!(x)) app_assert_failed(__FILE__, __LINE__); } while (0)

/* Clock va scheduler */
#define configCPU_CLOCK_HZ                   BSP_CPU_CLOCK_HZ
#define configTICK_RATE_HZ                   1000U
#define configUSE_PREEMPTION                 1
#define configUSE_TIME_SLICING               1
#define configUSE_16_BIT_TICKS               0
#define configMAX_PRIORITIES                 5
#define configIDLE_SHOULD_YIELD              1

/* Bo nho: stack tinh theo phan tu 32-bit, heap tinh theo byte */
#define configMINIMAL_STACK_SIZE            128U
#define configTOTAL_HEAP_SIZE                (10U * 1024U)
#define configMAX_TASK_NAME_LEN              16
#define configSUPPORT_DYNAMIC_ALLOCATION     1
#define configSUPPORT_STATIC_ALLOCATION      0

/* Dong bo va trao doi du lieu */
#define configUSE_TASK_NOTIFICATIONS         1
#define configUSE_MUTEXES                    1
#define configUSE_RECURSIVE_MUTEXES           1
#define configUSE_COUNTING_SEMAPHORES        1
#define configUSE_QUEUE_SETS                 1
#define configQUEUE_REGISTRY_SIZE            8
#define configUSE_EVENT_GROUPS               1
#define configUSE_STREAM_BUFFERS             1

/* Software timer */
#define configUSE_TIMERS                     1
#define configTIMER_TASK_PRIORITY            2
#define configTIMER_QUEUE_LENGTH             10
#define configTIMER_TASK_STACK_DEPTH          256U

/* Fail-fast hooks implemented in app/src/rtos_hooks.c */
#define configUSE_IDLE_HOOK                  0
#define configUSE_TICK_HOOK                  0
#define configUSE_MALLOC_FAILED_HOOK          1
#define configCHECK_FOR_STACK_OVERFLOW       2
#define configUSE_CO_ROUTINES                0

/* Priority ngat tren STM32F103 */
#define configPRIO_BITS                      4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY       15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY  5

#define configKERNEL_INTERRUPT_PRIORITY \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* Cac API duoc su dung */
#define INCLUDE_vTaskDelay                   1
#define INCLUDE_xTaskDelayUntil              1
#define INCLUDE_vTaskDelete                  1
#define INCLUDE_vTaskSuspend                 1
#define INCLUDE_vTaskPrioritySet             1
#define INCLUDE_uxTaskPriorityGet            1
#define INCLUDE_xTaskGetSchedulerState       1
#define INCLUDE_xTaskGetCurrentTaskHandle    1
#define INCLUDE_uxTaskGetStackHighWaterMark   1
#define INCLUDE_eTaskGetState                1
#define INCLUDE_xTimerPendFunctionCall       1

/* Noi handler cua FreeRTOS voi ten trong startup.c */
#define vPortSVCHandler                      SVC_Handler
#define xPortPendSVHandler                   PendSV_Handler
#define xPortSysTickHandler                  SysTick_Handler

#endif