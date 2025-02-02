/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file. Contains common defines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms in the LICENSE file in the root
  * directory of this software component. If no LICENSE file is provided, it
  * is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* USER CODE BEGIN Includes */
#include "display.h"            /* Defines MyClockBitFields and display functions */

extern SPI_HandleTypeDef hspi1; /* SPI handle */
extern TIM_HandleTypeDef htim1; /* Timer handle */
extern MyClockBitFields clockReg; /* Global display register */
extern RTC_TimeTypeDef sTime;       /* Global RTC time structure */
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
void MENU_EncoderCallback(int8_t steps); /* Callback for encoder steps */
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
extern volatile uint8_t colon; /* Global variable for colon state */
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SPI1_LATCH_Pin GPIO_PIN_6
#define SPI1_LATCH_GPIO_Port GPIOA
#define ENC_SW_Pin GPIO_PIN_5
#define ENC_SW_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
/* USER CODE BEGIN 0 */
void Button1_Pressed(void);
void Button1_DoubleClicked(void);
void Button1_Held(void);
/* USER CODE END Private defines */

#endif /* __MAIN_H */
