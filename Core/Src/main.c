/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "display.h"   /* Biblioteka wyświetlacza */
#include "button.h"    /* Obsługa przycisków */
#include "gps_parser.h"
#include "slider.h"    /* Obsługa przewijania tekstu */
#include "sht30.h"     /* Czujnik temperatury i wilgotności */
#include "menu.h"      /* Moduł menu */
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
/* Globalne zmienne systemowe */
MyClockBitFields clockReg = { 0 };
RTC_TimeTypeDef sTime;
RTC_DateTypeDef sDate;
uint32_t adcValue = 0;
volatile int32_t encoderValue = 0;
volatile uint32_t systemTicks = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Set_RTC_Time(void);  /* Ustawia przykładowy czas RTC */
void Get_RTC_Time(void);  /* Odczytuje czas z RTC */
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  Main program entry point
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* Inicjalizacja zmiennych i kod użytkownika */
  /* USER CODE END 1 */

  HAL_Init();  /* Reset peryferiów i inicjalizacja Systick */

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  SystemClock_Config();  /* Konfiguracja zegara systemowego */

  /* USER CODE BEGIN SysInit */
  /* Odblokowanie backupu i konfiguracja RTC */
  HAL_PWR_EnableBkUpAccess();
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
  /* USER CODE END SysInit */

  /* Inicjalizacja peryferiów */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_TIM1_Init();
  MX_RTC_Init();
  MX_I2C2_Init();
  MX_ADC1_Init();
  MX_TIM4_Init();
  MX_TIM5_Init();
  MX_USART1_UART_Init();
  MX_SPI2_Init();

  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (htim1.Init.Period + 1) / 2);

  SetPWMPercentGamma(30);
  ClearClockBits(&clockReg);
  UpdateAllDisplays(&clockReg);
  SLIDER_Init();
  SHT30_Init();
  //Set_RTC_Time();
  HAL_TIM_Encoder_Start_IT(&htim4, TIM_CHANNEL_ALL);
  HAL_TIM_Base_Start_IT(&htim5);
  GPS_Init();
  if (HAL_UART_Receive_DMA(&huart1, gps_dma_buffer, GPS_DMA_BUFFER_SIZE) != HAL_OK)
  {
    Error_Handler();
  }
  MENU_Init();
  /* Rejestracja callbacków przycisków */
  Button_RegisterPressCallback(0, Button1_Pressed);
  Button_RegisterDoubleClickCallback(0, Button1_DoubleClicked);
  Button_RegisterHoldCallback(0, Button1_Held);
  /* USER CODE END 2 */

  /* USER CODE BEGIN WHILE */
  while (1)
  {
    GPS_ProcessBuffer();  /* Przetwarzanie danych GPS */
    Get_RTC_Time();       /* Odczyt RTC */
    Display();            /* Obertas Egzekutas */

    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
      Error_Handler();
    }
    Button_Process();     /* Przetwarzanie stanów przycisków */
    HAL_Delay(10);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief  Konfiguracja zegara systemowego.
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void Set_RTC_Time(void)
{
  RTC_TimeTypeDef time = {0};
  RTC_DateTypeDef date = {0};

  time.Hours = 11;
  time.Minutes = 33;
  time.Seconds = 0;
  time.TimeFormat = RTC_HOURFORMAT_24;
  if (HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  date.WeekDay = RTC_WEEKDAY_TUESDAY;
  date.Month = RTC_MONTH_JANUARY;
  date.Date = 14;
  date.Year = 25;  /* 2025 */
  if (HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
}


void Get_RTC_Time(void)
{
  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM4)
  {
    int8_t direction = __HAL_TIM_IS_TIM_COUNTING_DOWN(htim) ? -1 : +1;
    Encoder_HandleInterrupt(direction);
    if (direction < 0)
      encoderValue--;
    else
      encoderValue++;
  }
}

void Button1_Pressed(void)
{
  /* Jeśli menu jest aktywne, przechodzimy do kolejnej pozycji menu */
  if (MENU_IsActive())
  {
    MENU_Next();
  }
  else
  {
    /* W przeciwnym razie - cos innego
    SLIDER_SetString("HELLO", SCROLL_RIGHT_TO_LEFT, 10); */
  }
}

/* Callback dla dwukrotnego kliknięcia przycisku. */
void Button1_DoubleClicked(void)
{
  /* Jeśli menu jest aktywne, wychodzimy z menu */
  if (MENU_IsActive())
  {
    MENU_Exit();
  }
  else
  {
    /* Opcjonalna akcja przy dwukliku, np. inicjalizacja innej funkcji */
  }
}

/** @brief Callback dla długiego przytrzymania przycisku. */
void Button1_Held(void)
{
  /* Jeśli menu nie jest aktywne, wchodzimy do menu; w przeciwnym razie wychodzimy */
  if (!MENU_IsActive())
    MENU_Enter();
  else
    MENU_Exit();
}
/* USER CODE END 4 */


void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
    /* USER CODE END Error_Handler_Debug */
  }
  /* USER CODE BEGIN Error_Handler_Debug */
  /* USER CODE END Error_Handler_Debug */
}


