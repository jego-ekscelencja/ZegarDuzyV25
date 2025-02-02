#include "sht30.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c2; // I2C2 handle from i2c.c

// Single Shot command (High Repeatability, no clock stretching)
static const uint8_t SHT30_CMD_SINGLE_SHOT[2] = {0x24, 0x00};

// Minimum conversion time for High Repeatability (~15 ms), using 90 ms here
#define SHT30_MEAS_TIME_MS     90

// Measurement period: every 120 ms
#define SHT30_PERIOD_MS        120

// Buffer for 6 bytes of sensor data
static uint8_t g_rxBuffer[6];

// State variable for the SHT30 state machine
static SHT30_MeasState_t g_measState = SHT30_STATE_IDLE;

// Latest measurement data
static SHT30_Data_t g_latestData;

// Timer counter in milliseconds
static uint16_t g_timerMs = 0;

// Internal function prototypes
static bool SHT30_ConvertRawData(const uint8_t *raw, int32_t *pTemp, uint32_t *pRH);
static uint8_t SHT30_CalcCrc8(const uint8_t *data, int len);

// SHT30 initialization function
void SHT30_Init(void)
{
    g_measState = SHT30_STATE_IDLE;  // Set state to IDLE
    g_timerMs   = 0;                 // Reset timer
    memset(&g_latestData, 0, sizeof(g_latestData)); // Clear latest data
    g_latestData.valid = false;      // Mark data as invalid

    // Optional soft reset (command 0x30A2) sent synchronously
    uint8_t cmdReset[2] = {0x30, 0xA2};
    HAL_I2C_Master_Transmit(&hi2c2, (SHT30_I2C_ADDR << 1), cmdReset, 2, 100);
    HAL_Delay(10); // Wait a moment after reset
}

// This function is called every 10 ms (from timer interrupt)
void SHT30_10msHandler(void)
{
    switch (g_measState)
    {
    case SHT30_STATE_IDLE:
        // Wait until the measurement period elapses
        g_timerMs += 10;
        if (g_timerMs >= SHT30_PERIOD_MS)
        {
            g_timerMs = 0; // Reset timer
            // Start DMA transmission: send Single Shot command
            if (HAL_I2C_Master_Transmit_DMA(&hi2c2, (SHT30_I2C_ADDR << 1),
                                            (uint8_t*)SHT30_CMD_SINGLE_SHOT, 2) == HAL_OK)
            {
                g_measState = SHT30_STATE_TX_IN_PROGRESS;
            }
            else
            {
                // DMA start error; remain in IDLE (error handling can be added)
            }
        }
        break;

    case SHT30_STATE_TX_IN_PROGRESS:
        // Do nothing; wait for TX callback
        break;

    case SHT30_STATE_WAITING_FOR_MEAS:
        // Count conversion time (90 ms)
        g_timerMs += 10;
        if (g_timerMs >= SHT30_MEAS_TIME_MS)
        {
            g_timerMs = 0; // Reset timer
            // Start DMA reception of 6 bytes of raw data
            if (HAL_I2C_Master_Receive_DMA(&hi2c2, (SHT30_I2C_ADDR << 1),
                                           g_rxBuffer, 6) == HAL_OK)
            {
                g_measState = SHT30_STATE_RX_IN_PROGRESS;
            }
            else
            {
                g_measState = SHT30_STATE_IDLE;
                g_latestData.valid = false;
            }
        }
        break;

    case SHT30_STATE_RX_IN_PROGRESS:
        // Wait for RX callback
        break;

    case SHT30_STATE_DONE:
        // Measurement complete; return to IDLE state
        g_measState = SHT30_STATE_IDLE;
        break;

    default:
        g_measState = SHT30_STATE_IDLE;
        break;
    }
}

// Returns the latest measurement data if valid
bool SHT30_GetLatestData(SHT30_Data_t *pData)
{
    if (pData == NULL)
        return false;
    if (!g_latestData.valid)
        return false;
    *pData = g_latestData;
    return true;
}

// DMA transmission complete callback (MasterTxCplt)
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2)
    {
        if (g_measState == SHT30_STATE_TX_IN_PROGRESS)
        {
            g_measState = SHT30_STATE_WAITING_FOR_MEAS; // Transition to waiting state
            g_timerMs = 0; // Reset timer
        }
    }
}

// DMA reception complete callback (MasterRxCplt)
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2)
    {
        if (g_measState == SHT30_STATE_RX_IN_PROGRESS)
        {
            int32_t temp;   // Temperature in 0.01°C
            uint32_t rh;    // Humidity in 0.01%RH

            bool ok = SHT30_ConvertRawData(g_rxBuffer, &temp, &rh);
            if (ok)
            {
                g_latestData.temperature = temp;
                g_latestData.humidity    = rh;
                g_latestData.valid       = true;
            }
            else
            {
                g_latestData.valid = false;
            }
            g_measState = SHT30_STATE_DONE; // Transition to DONE state
        }
    }
}

// DMA error callback
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2)
    {
        g_measState = SHT30_STATE_IDLE;  // Return to IDLE on error
        g_latestData.valid = false;
    }
}

// Convert raw sensor data to integer temperature and humidity values
static bool SHT30_ConvertRawData(const uint8_t *raw, int32_t *pTemp, uint32_t *pRH)
{
    if (raw == NULL || pTemp == NULL || pRH == NULL)
        return false;
    if (SHT30_CalcCrc8(&raw[0], 2) != raw[2])
        return false; // Temperature CRC error
    if (SHT30_CalcCrc8(&raw[3], 2) != raw[5])
        return false; // Humidity CRC error

    uint16_t rawT = (raw[0] << 8) | raw[1];
    uint16_t rawH = (raw[3] << 8) | raw[4];

    *pTemp = (-4500) + ((17500 * (int32_t)rawT) / 65535);
    *pRH   = (10000 * (uint32_t)rawH) / 65535;
    return true;
}

// Calculate CRC8 per SHT3x specification (polynomial 0x31, init 0xFF)
static uint8_t SHT30_CalcCrc8(const uint8_t *data, int len)
{
    uint8_t crc = 0xFF;
    for (int i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc <<= 1;
        }
    }
    return crc;
}
