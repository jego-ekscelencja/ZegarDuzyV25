/*
 * gps_parser.h
 *
 *  Created on: Dec 25, 2024
 *      Author: DevMachine
 */

#ifndef INC_GPS_PARSER_H_
#define INC_GPS_PARSER_H_

#include <inttypes.h>
#include <stdbool.h>
#include "rtc.h"
#include "main.h"

#define GPS_DMA_BUFFER_SIZE 1024
extern uint8_t gps_dma_buffer[GPS_DMA_BUFFER_SIZE];

// Structure holding GPS data
typedef struct
{
    uint8_t hours;      // UTC hour
    uint8_t minutes;    // UTC minutes
    uint8_t seconds;    // UTC seconds
    uint8_t day;        // Day (from NMEA: ddmmyy)
    uint8_t month;      // Month
    uint8_t year;       // Year (e.g., 24 for 2024)
    uint8_t satellites; // Number of satellites in use
    char fix;           // GPS fix status ('A' = Active, 'V' = Void)
} gps_data_t;

// Global GPS data structure
extern gps_data_t gps_data;

// Convert UTC to local time
void ConvertUtcToLocalTime(uint8_t utcHours, uint8_t utcMinutes,
		uint8_t utcSeconds, uint8_t utcDay, uint8_t utcMonth, uint8_t utcYear,
		RTC_TimeTypeDef *localTime, RTC_DateTypeDef *localDate);

// Initialize the GPS parser (e.g., clear buffers)
void GPS_Init(void);

// Get day of week for a given date
uint8_t GetDayOfWeek(uint16_t year, uint8_t month, uint8_t day);

// Update RTC time from GPS data
void update_rtc_time_from_gps(void);

// Check if DST is active
bool IsDstActive(uint16_t year, uint8_t month, uint8_t day);

// Get last Sunday of the month
uint8_t GetLastSundayOfMonth(uint16_t year, uint8_t month);

// Get number of days in a given month
uint8_t DaysInMonth(uint16_t year, uint8_t month);

extern uint8_t DOW;  // Global variable for day of week

// Process the DMA buffer: parse new NMEA lines and update gps_data
void GPS_ProcessBuffer(void);

#endif /* INC_GPS_PARSER_H_ */
