/*
 * display.c
 *
 *  Created on: Jan 15, 2025
 *      Author: DevMachine
 */

#include "display.h"
#include "main.h"     // Dostęp do htim1, hspi1, GPIO do latcha itd.
#include "slider.h"

// Można tu zadeklarować: extern SPI_HandleTypeDef hspi1; extern TIM_HandleTypeDef htim1;
// Upewnij się, że w main.h są te deklaracje

volatile bool spiTransferInProgress = false;  // Flaga transmisji SPI
static uint8_t spiTxBuffer[24];  // Bufor na 192 bity (24 bajty)
// static Bits192 regValue;  // Przechowuje 192 bity rozbite na 12×16

// Konwersja znaku na wzór 7-segmentowy
uint8_t charToSegment(char c) {
    switch (c) {
    case '0' ... '9':
        return segmentMap[c - '0'];
    case '-':
        return segmentMap[11];
    case '*':  // symbol stopnia
        return segmentMap[12];
    case 'C':
        return segmentMap[13];
    case 'c':
        return segmentMap[29];
    case 'r':
    case 'R':
        return segmentMap[14];
    case 'h':
        return segmentMap[15];
    case 's':
    case 'S':
        return segmentMap[5];
    case 'F':
        return segmentMap[16];
    case 'A':
    case 'a':
        return segmentMap[17];
    case 't':
    case 'T':
        return segmentMap[18];
    case 'V':
    case 'U':
    case 'W':
        return segmentMap[19];
    case 'n':
    case 'N':
        return segmentMap[20];
    case 'i':
        return segmentMap[21];
    case 'E':
    case 'e':
        return segmentMap[22];
    case 'd':
        return segmentMap[23];
    case 'P':
    case 'p':
        return segmentMap[24];
    case 'o':
        return segmentMap[25];
    case 'O':
        return segmentMap[0];
    case 'u':
    case 'w':
    case 'v':
        return segmentMap[26];
    case 'L':
        return segmentMap[27];
    case 'z':
    case 'Z':
        return segmentMap[2];
    case 'b':
    case 'B':
        return segmentMap[28];
    case 'y':
        return segmentMap[29];
    case 'H':
        return segmentMap[30];
    case 'j':
    case 'J':
        return segmentMap[31];
    default:
        return segmentMap[10];  // Spacja lub nieobsługiwany znak
    }
}

/* Implementacja funkcji obsługujących 192-bitowy rejestr wyświetlaczy */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI1)
  {
    HAL_GPIO_WritePin(SPI1_LATCH_GPIO_Port, SPI1_LATCH_Pin, GPIO_PIN_SET);   // Impuls na LATCH
    HAL_GPIO_WritePin(SPI1_LATCH_GPIO_Port, SPI1_LATCH_Pin, GPIO_PIN_RESET); // Reset LATCH
    spiTransferInProgress = false;
  }
}

void Send192_struct(const Bits192 *data)
{
  memset(spiTxBuffer, 0, sizeof(spiTxBuffer));  // Wyczyść bufor
  int byteIndex = 0;
  for (int partIndex = 11; partIndex >= 0; partIndex--)
  {
    uint16_t val = data->part[partIndex];
    spiTxBuffer[byteIndex++] = (uint8_t)((val >> 8) & 0xFF);  // MSB
    spiTxBuffer[byteIndex++] = (uint8_t)(val & 0xFF);         // LSB
  }
  HAL_SPI_Transmit_DMA(&hspi1, spiTxBuffer, 24);
}

void ClearClockBits(MyClockBitFields* clockBits)
{
  memset(clockBits, 0, sizeof(MyClockBitFields));
}

void SetSecondLedSingle(MyClockBitFields* clockBits, uint8_t second)
{
    if (second >= 60) second = 59;
    clockBits->secondsRing = 0ULL;
    uint64_t mask = (1ULL << second);
    clockBits->secondsRing = mask;
}

void SetSecondLedAccumulating(MyClockBitFields* clockBits, uint8_t second)
{
    if (second >= 60) second = 59;
    if (second == 0) {
        clockBits->secondsRing = 0ULL;
    } else {
        clockBits->secondsRing |= (1ULL << second);
    }
}

void SetSecondLedAccumulating2(MyClockBitFields* clockBits, uint8_t second)
{
    if (second >= 60) second = 59;
    clockBits->secondsRing = 0ULL;
    for (uint8_t i = 0; i <= second; i++) {
        clockBits->secondsRing |= (1ULL << i);
    }
}

void SetSecondsDots(MyClockBitFields* clockBits, uint8_t second)
{
  if (second == 0) {
    SetDots(clockBits, false, false);
  } else {
    SetDots(clockBits, true, true);
  }
}

void SetHourRing(MyClockBitFields* clockBits, uint8_t hour, bool outerRing, bool innerRing)
{
  uint8_t h12 = hour % 12;
  clockBits->hoursRingOuter = outerRing ? (1U << h12) : 0;
  clockBits->hoursRingInner = innerRing ? (1U << h12) : 0;
}

void SetHoursRing(MyClockBitFields* clockBits, uint8_t hour)
{
  uint8_t h12 = hour % 12;
  clockBits->hoursRingOuter = (1U << h12);
  clockBits->hoursRingInner = (1U << h12);
}

/* Ustawia 6 wyświetlaczy 7-seg (top) na HH:MM:SS */
void SetTime7Seg_Top(MyClockBitFields* clockBits, uint8_t h, uint8_t m, uint8_t s)
{
    uint8_t backBuffer[6] = {0};  /* Bufor segmentów */

    /* Ustawianie godzin */
    if (h < 10) {
        backBuffer[0] = segmentMap[10];  /* Puste dziesiątki */
        backBuffer[1] = charToSegment('0' + h);
    } else {
        backBuffer[0] = charToSegment('0' + (h / 10));
        backBuffer[1] = charToSegment('0' + (h % 10));
    }

    /* Ustawianie minut */
    backBuffer[2] = charToSegment('0' + (m / 10));
    backBuffer[3] = charToSegment('0' + (m % 10));

    /* Ustawianie sekund */
    backBuffer[4] = charToSegment('0' + (s / 10));
    backBuffer[5] = charToSegment('0' + (s % 10));

    uint64_t displayVal = 0ULL;
    displayVal |= ((uint64_t)backBuffer[5] << 40);
    displayVal |= ((uint64_t)backBuffer[4] << 32);
    displayVal |= ((uint64_t)backBuffer[0] << 24);
    displayVal |= ((uint64_t)backBuffer[1] << 16);
    displayVal |= ((uint64_t)backBuffer[2] << 8);
    displayVal |= ((uint64_t)backBuffer[3] << 0);
    clockBits->topDisplay = displayVal;
}

void SetTime7Seg_Void(MyClockBitFields* clockBits)
{
    uint8_t backBuffer[6] = {0};
    backBuffer[0] = segmentMap[10];
    backBuffer[1] = segmentMap[10];
    backBuffer[2] = segmentMap[10];
    backBuffer[3] = segmentMap[10];
    backBuffer[4] = segmentMap[10];
    backBuffer[5] = segmentMap[10];

    uint64_t displayVal = 0ULL;
    displayVal |= ((uint64_t)backBuffer[5] << 40);
    displayVal |= ((uint64_t)backBuffer[4] << 32);
    displayVal |= ((uint64_t)backBuffer[0] << 24);
    displayVal |= ((uint64_t)backBuffer[1] << 16);
    displayVal |= ((uint64_t)backBuffer[2] << 8);
    displayVal |= ((uint64_t)backBuffer[3] << 0);
    clockBits->topDisplay = displayVal;
}

void Set7Seg_Bot3(MyClockBitFields* clockBits, uint8_t h, uint8_t m, uint8_t s)
{
    uint8_t backBuffer[6] = {0};

    if (h < 10) {
        backBuffer[0] = segmentMap[10];
        backBuffer[1] = charToSegment('0' + h);
    } else {
        backBuffer[0] = charToSegment('0' + (h / 10));
        backBuffer[1] = charToSegment('0' + (h % 10));
    }

    backBuffer[2] = charToSegment('0' + (m / 10));
    backBuffer[3] = charToSegment('0' + (m % 10));

    backBuffer[4] = charToSegment('0' + (s / 10));
    backBuffer[5] = charToSegment('0' + (s % 10));

    uint64_t displayVal = 0ULL;
    displayVal |= ((uint64_t)backBuffer[5] << 40);
    displayVal |= ((uint64_t)backBuffer[4] << 32);
    displayVal |= ((uint64_t)backBuffer[3] << 24);
    displayVal |= ((uint64_t)backBuffer[2] << 16);
    displayVal |= ((uint64_t)backBuffer[1] << 8);
    displayVal |= ((uint64_t)backBuffer[0] << 0);
    clockBits->bottomDisplay = displayVal;
}

void Set7Seg_DisplayLargeNumber(MyClockBitFields* clockBits, uint64_t number) {
    uint8_t backBuffer[6] = {0};

    for (int i = 5; i >= 0; i--) {
        if (number > 0) {
            backBuffer[i] = charToSegment('0' + (number % 10));
            number /= 10;
        } else {
            backBuffer[i] = segmentMap[10];  /* Puste miejsce */
        }
    }

    uint64_t displayVal = 0ULL;
    displayVal |= ((uint64_t)backBuffer[5] << 40);
    displayVal |= ((uint64_t)backBuffer[4] << 32);
    displayVal |= ((uint64_t)backBuffer[3] << 24);
    displayVal |= ((uint64_t)backBuffer[2] << 16);
    displayVal |= ((uint64_t)backBuffer[1] << 8);
    displayVal |= ((uint64_t)backBuffer[0] << 0);
    clockBits->bottomDisplay = displayVal;
}

void UpdateAllDisplays(const MyClockBitFields* clockBits)
{
    if (spiTransferInProgress) {
        return;
    }

    spiTransferInProgress = true;
    static uint8_t spiTxBuffer[24];
    memset(spiTxBuffer, 0, sizeof(spiTxBuffer));

    const uint8_t* p = (const uint8_t*)clockBits;
    int byteIndex = 0;
    for (int partIndex = 11; partIndex >= 0; partIndex--)
    {
        spiTxBuffer[byteIndex++] = p[2 * partIndex + 1];
        spiTxBuffer[byteIndex++] = p[2 * partIndex + 0];
    }

    HAL_SPI_Transmit_DMA(&hspi1, spiTxBuffer, 24);
}

static const uint8_t gamma_table[101] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4,
    5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 11, 11, 12, 13, 13, 14, 15, 16, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 33, 34, 35, 36, 37,
    39, 40, 41, 43, 44, 46, 47, 49, 50, 52, 53, 55, 56, 58, 60, 61, 63, 65, 66,
    68, 70, 72, 74, 75, 77, 79, 81, 83, 85, 87, 89, 91, 94, 96, 98, 100
};

void SetPWMValue(uint16_t value) {
  if (value > __HAL_TIM_GET_AUTORELOAD(&htim1)) {
    value = __HAL_TIM_GET_AUTORELOAD(&htim1);
  }
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, value);
}

void SetPWMPercent(uint8_t percent) {
  if (percent > 100) {
    percent = 100;
  }
  uint32_t period = __HAL_TIM_GET_AUTORELOAD(&htim1);
  uint32_t compare_value = (period + 1) - ((period + 1) * percent / 100);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, compare_value);
}

void SetPWMPercentGamma(uint8_t percent) {
  if (percent > 100) {
      percent = 100;
  }
  uint32_t period = __HAL_TIM_GET_AUTORELOAD(&htim1);
  uint8_t gamma_percent = gamma_table[percent];
  uint32_t compare_value = (period + 1) * gamma_percent / 100;
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, compare_value);
}

void FadeEffect(void)
{
    HAL_Delay(200);  /* Delay 200ms */
    for (int p = 10; p >= 0; p--) {
        SetPWMPercentGamma(p);
        HAL_Delay(50);
    }
    for (int p = 0; p <= 100; p++) {
        SetPWMPercentGamma(p);
        HAL_Delay(50);
    }
    for (int p = 100; p >= 10; p--) {
        SetPWMPercentGamma(p);
        HAL_Delay(50);
    }
}

void SetSecondLedEvenOdd(MyClockBitFields* clockBits, uint8_t second, uint8_t minute)
{
    if (second >= 60) {
        second = 59;
    }
    if ((minute % 2) == 0)
    {
        clockBits->secondsRing = 0ULL;
        for (uint8_t i = 0; i <= second; i++)
        {
            clockBits->secondsRing |= (1ULL << i);
        }
    }
    else
    {
        clockBits->secondsRing = 0ULL;
        for (uint8_t i = second + 1; i < 60; i++)
        {
            clockBits->secondsRing |= (1ULL << i);
        }
    }
}

/* Alternative SetDots implementation (commented out)
void SetDots(MyClockBitFields* clockBits, bool dot1, bool dot2)
{
  uint64_t val = 0ULL;
  if (dot1) val |= (1ULL << 0);
  if (dot2) val |= (1ULL << 1);
  clockBits->dots = val;
}
*/
void SetDots(MyClockBitFields* clockBits, bool dot1, bool dot2) {
  uint64_t val = 0ULL;
  if (dot1) val |= (1ULL << 0);
  if (dot2) val |= (1ULL << 1);
  clockBits->dots = val;
}

void SetHourRingCustom(MyClockBitFields* clockBits, uint8_t outerMode, uint8_t innerMode)
{
    uint16_t fullMask = 0x0FFF;   /* Pełny pierścień 12 godzin */
    uint16_t quarterMask = (1U << 0) | (1U << 3) | (1U << 6) | (1U << 9);  /* Kwadranse */

    switch(outerMode)
    {
        case 1:
            clockBits->hoursRingOuter = fullMask;
            break;
        case 2:
            clockBits->hoursRingOuter = quarterMask;
            break;
        default:
            clockBits->hoursRingOuter = 0;
            break;
    }

    switch(innerMode)
    {
        case 1:
            clockBits->hoursRingInner = fullMask;
            break;
        case 2:
            clockBits->hoursRingInner = quarterMask;
            break;
        default:
            clockBits->hoursRingInner = 0;
            break;
    }
}
