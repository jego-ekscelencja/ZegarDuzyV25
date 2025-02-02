/*
 * sht30.h
 *
 *  Created on: Jan 22, 2025
 *      Author: DevMachine
 */

#ifndef INC_SHT30_H_
#define INC_SHT30_H_

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h" // Adjust for your MCU

// I2C address (SHT30 default: 0x44 or 0x45 based on pins)
#define SHT30_I2C_ADDR  0x44 // 7-bit address

// Structure for measurement results
// Temperature is represented as int32_t, where 3456 means 34.56°C
// Humidity is represented as uint32_t, where 4567 means 45.67% RH
typedef struct
{
    int32_t  temperature; // [0.01°C]
    uint32_t humidity;    // [0.01%RH]
    bool     valid;
} SHT30_Data_t;

// State machine for DMA-based measurements
typedef enum
{
    SHT30_STATE_IDLE = 0,        // Waiting for measurement period start
    SHT30_STATE_TX_IN_PROGRESS,  // Transmitting command (DMA)
    SHT30_STATE_WAITING_FOR_MEAS, // Waiting for conversion time
    SHT30_STATE_RX_IN_PROGRESS,  // Receiving data (DMA)
    SHT30_STATE_DONE             // Measurement complete; waiting for next cycle
} SHT30_MeasState_t;

// API functions
void SHT30_Init(void);                           // Initialize the SHT30 sensor
void SHT30_10msHandler(void);                    // Called every 10 ms for state handling
bool SHT30_GetLatestData(SHT30_Data_t *pData);     // Get the latest measurement result

// HAL I2C DMA callbacks
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c); // TX complete callback
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c); // RX complete callback
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c);        // Error callback

#endif /* INC_SHT30_H_ */
