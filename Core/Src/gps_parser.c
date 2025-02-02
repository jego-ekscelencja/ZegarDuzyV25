/*
 * gps_parser.c
 *
 *  Created on: Dec 25, 2024
 *      Author: DevMachine
 *
 *  This file implements a parser for GPS data received in NMEA format.
 */

#include "gps_parser.h"
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include "usart.h"
#include "rtc.h"
#include "main.h"

uint8_t DOW;                      // Global day-of-week variable
volatile uint8_t colon = 0;       // Global colon flag

extern DMA_HandleTypeDef hdma_usart1_rx;  // DMA handle for USART1 RX
uint8_t gps_dma_buffer[GPS_DMA_BUFFER_SIZE];  // DMA buffer for GPS data
gps_data_t gps_data = {0};        // Global GPS data structure
static uint16_t old_pos = 0;      // Previous buffer position

// Returns true if character is a NMEA separator (',' or '*')
static bool IsNmeaSeparator(char c)
{
    return (c == ',' || c == '*');  // Check for comma or asterisk
}

// Parses an unsigned 8-bit integer from a string, up to maxLen digits
static uint8_t ParseUInt8(const char *startingPtr, uint8_t maxLen)
{
    uint8_t value = 0;  // Initialize result
    for (uint8_t i = 0; i < maxLen; i++) {
        if (startingPtr[i] < '0' || startingPtr[i] > '9')
            break;  // Stop if non-digit encountered
        value = (uint8_t)(value * 10 + (startingPtr[i] - '0'));
    }
    return value;  // Return parsed value
}

// Parse GPRMC sentence from NMEA and update gps_data fields
static void ParseGPRMC(const char *nmeaLine)
{
    const char *p = nmeaLine;
    uint8_t fieldIndex = 0;
    const char *fieldPtr = NULL;
    for (; *p != '\0'; p++) {
        if (IsNmeaSeparator(*p) || *p == '\r' || *p == '\n') {
            if (fieldPtr) {
                switch(fieldIndex) {
                    case 1: // TIME (hhmmss.ss)
                        gps_data.hours   = ParseUInt8(fieldPtr, 2);
                        gps_data.minutes = ParseUInt8(fieldPtr+2, 2);
                        gps_data.seconds = ParseUInt8(fieldPtr+4, 2);
                        break;
                    case 2: // Fix status
                        gps_data.fix = *fieldPtr;
                        break;
                    case 9: // DATE (ddmmyy)
                        gps_data.day   = ParseUInt8(fieldPtr, 2);
                        gps_data.month = ParseUInt8(fieldPtr+2, 2);
                        gps_data.year  = ParseUInt8(fieldPtr+4, 2);
                        break;
                    default:
                        break;
                }
            }
            fieldPtr = NULL;
            fieldIndex++;
        } else {
            if (!fieldPtr) fieldPtr = p;
        }
        if (*p == '*') break;
    }
    // Update RTC immediately if fix is active
    if (gps_data.fix == 'A') {
        RTC_TimeTypeDef localTime;
        RTC_DateTypeDef localDate;
        ConvertUtcToLocalTime(gps_data.hours, gps_data.minutes, gps_data.seconds,
                              gps_data.day, gps_data.month, gps_data.year,
                              &localTime, &localDate);
        HAL_RTC_SetTime(&hrtc, &localTime, RTC_FORMAT_BIN);
        HAL_RTC_SetDate(&hrtc, &localDate, RTC_FORMAT_BIN);
        colon = 1;
    }
}

// Parse GPGGA sentence from NMEA and update satellites count
static void ParseGPGGA(const char *nmeaLine)
{
    const char *p = nmeaLine;      // Pointer for iterating through the sentence
    uint8_t fieldIndex = 0;        // Current field index
    const char *fieldPtr = NULL;   // Pointer to the start of the field
    for (; *p != '\0'; p++) {
        if (IsNmeaSeparator(*p) || *p == '\r' || *p == '\n') {
            if (fieldPtr && fieldIndex == 7) {
                gps_data.satellites = ParseUInt8(fieldPtr, 2);  // Parse satellites count
            }
            fieldPtr = NULL;
            fieldIndex++;
        } else {
            if (!fieldPtr)
                fieldPtr = p;
        }
        if (*p == '*')
            break;
    }
}

// Process the DMA buffer and parse complete NMEA sentences
void GPS_ProcessBuffer(void)
{
    uint16_t now_pos = GPS_DMA_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);  // Current buffer position
    while (old_pos != now_pos) {
        char c = (char)gps_dma_buffer[old_pos];
        static char lineBuf[128];
        static uint8_t lineIndex = 0;
        if (lineIndex < sizeof(lineBuf) - 1) {
            lineBuf[lineIndex++] = c;
        }
        if (c == '\n' || c == '\r') {
            lineBuf[lineIndex] = '\0';
            if (strncmp(lineBuf, "$GPRMC", 6) == 0) {
                ParseGPRMC(lineBuf);
            }
            if (strncmp(lineBuf, "$GPGGA", 6) == 0) {
                ParseGPGGA(lineBuf);
            }
            if (strncmp(lineBuf, "$GPRMC", 6) == 0 || strncmp(lineBuf, "$GNRMC", 6) == 0) {
                ParseGPRMC(lineBuf);
            }
            else if (strncmp(lineBuf, "$GPGGA", 6) == 0 || strncmp(lineBuf, "$GNGGA", 6) == 0) {
                ParseGPGGA(lineBuf);
            }
            lineIndex = 0;
        }
        old_pos++;
        if (old_pos >= GPS_DMA_BUFFER_SIZE)
            old_pos = 0;
    }
}

void GPS_Init(void)
{
    memset(&gps_data, 0, sizeof(gps_data));
    memset(gps_dma_buffer, 0, GPS_DMA_BUFFER_SIZE);
    old_pos = 0;
}

void ConvertUtcToLocalTime(uint8_t utcHours, uint8_t utcMinutes,
		uint8_t utcSeconds, uint8_t utcDay, uint8_t utcMonth, uint8_t utcYear,
		RTC_TimeTypeDef *localTime, RTC_DateTypeDef *localDate) {
	// For Poland: UTC+1 in winter, UTC+2 in summer
	uint16_t fullYear = 2000 + utcYear;
	int offset = 1;
	if (IsDstActive(fullYear, utcMonth, utcDay)) {
		offset = 2;
	}

	int hour = utcHours + offset;
	int minute = utcMinutes;
	int second = utcSeconds;
	int day = utcDay;
	int month = utcMonth;
	int yearXX = utcYear;  // 0..99 in RTC

	DOW = GetDayOfWeek(fullYear, utcMonth, utcDay);

	if (hour >= 24) {
		hour -= 24;
		day++;
		uint8_t mdays = DaysInMonth(fullYear, month);
		if (day > mdays) {
			day = 1;
			month++;
			if (month > 12) {
				month = 1;
				fullYear++;
			}
		}
	} else if (hour < 0) {
		hour += 24;
		day--;
		if (day < 1) {
			month--;
			if (month < 1) {
				month = 12;
				fullYear--;
			}
			day = DaysInMonth(fullYear, month);
		}
	}
	yearXX = (uint8_t)(fullYear % 100);
	localTime->Hours = hour;
	localTime->Minutes = minute;
	localTime->Seconds = second;
	localTime->DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	localTime->StoreOperation = RTC_STOREOPERATION_RESET;
	localDate->Date = (uint8_t)day;
	localDate->Month = (uint8_t)month;
	localDate->Year = (uint8_t)yearXX;
	localDate->WeekDay = RTC_WEEKDAY_MONDAY;  // Set static weekday (or compute it)
}

uint8_t DaysInMonth(uint16_t year, uint8_t month) {
	static const uint8_t daysTable[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	if (month == 2) {
		bool leap = false;
		if ((year % 400) == 0) {
			leap = true;
		} else if ((year % 100) == 0) {
			leap = false;
		} else if ((year % 4) == 0) {
			leap = true;
		}
		return (leap ? 29 : 28);
	}
	return daysTable[month - 1];
}

uint8_t GetLastSundayOfMonth(uint16_t year, uint8_t month) {
	uint8_t d = DaysInMonth(year, month);
	while (1) {
		uint8_t dow = GetDayOfWeek(year, month, d);
		if (dow == 0) { // 0 = Sunday
			return d;
		}
		d--;
	}
}

bool IsDstActive(uint16_t year, uint8_t month, uint8_t day) {
	uint8_t lastSundayMarch = GetLastSundayOfMonth(year, 3);
	uint8_t lastSundayOctober = GetLastSundayOfMonth(year, 10);
	if (month < 3) {
		return false;
	}
	if (month > 10) {
		return false;
	}
	if (month == 3) {
		return (day >= lastSundayMarch);
	}
	if (month == 10) {
		return (day < lastSundayOctober);
	}
	return true;
}

void update_rtc_time_from_gps(void) {
	if (gps_data.fix == 'A') {
		RTC_TimeTypeDef sTime = {0};
		RTC_DateTypeDef sDate = {0};
		sTime.Hours = gps_data.hours;
		sTime.Minutes = gps_data.minutes;
		sTime.Seconds = gps_data.seconds;
		sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
		sTime.StoreOperation = RTC_STOREOPERATION_RESET;
		if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
			Error_Handler();
		}
		sDate.WeekDay = RTC_WEEKDAY_MONDAY;  // Static weekday; can be computed
		sDate.Month = (uint8_t)gps_data.month;
		sDate.Date = gps_data.day;
		sDate.Year = gps_data.year;
		if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
			Error_Handler();
		}
	}
}

uint8_t GetDayOfWeek(uint16_t year, uint8_t month, uint8_t day) {
	if (month < 3) {
		month += 12;
		year -= 1;
	}
	uint16_t K = (uint16_t)(year % 100);
	uint16_t J = (uint16_t)(year / 100);
	int32_t h = (int32_t)(day + (13 * (month + 1)) / 5 + K + (K / 4) + (J / 4) - (2 * J));
	h = h % 7;
	if (h < 0) {
		h += 7;
	}
	return (uint8_t)h;  // Returns 0..6 (0 = Sunday)
}
