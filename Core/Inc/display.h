/*
 * display.h
 *
 *  Created on: Jan 15, 2025
 *      Author: DevMachine
 */

#ifndef INC_DISPLAY_H_
#define INC_DISPLAY_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* 192-bit structure layout:
   - 60 bits: seconds ring
   - 12 bits: outer hours ring
   - 12 bits: inner hours ring
   - 2 bits: dots
   - 10 bits: filler
   - 48 bits: top 7-seg display
   - 48 bits: bottom 7-seg display
*/
typedef struct __attribute__((packed,aligned(1)))
{
  uint64_t secondsRing       : 60;  /* Seconds ring bits */
  uint64_t hoursRingOuter    : 12;  /* Outer hours ring */
  uint64_t hoursRingInner    : 12;  /* Inner hours ring */
  uint64_t dots              : 2;   /* Dot segments */
  uint64_t filler            : 10;  /* Filler bits */
  uint64_t topDisplay        : 48;  /* Top display segments */
  uint64_t bottomDisplay     : 48;  /* Bottom display segments */
} MyClockBitFields;

/*!!!!!!!! PAMIĘTAĆ O  0ULL I 1ULL GDZIE TRZEBA */

/* 192-bit value represented as 12 x 16-bit parts */
typedef struct {
  uint16_t part[12];
} Bits192;

/* Function prototypes for SPI data sending */
void Send192_struct(const Bits192 *data);

/* Functions to display bits */
void ClearClockBits(MyClockBitFields* clockBits);
void SetSecondLedSingle(MyClockBitFields* clockBits, uint8_t second);
void SetSecondLedAccumulating(MyClockBitFields* clockBits, uint8_t second);
void SetSecondLedAccumulating2(MyClockBitFields* clockBits, uint8_t second);
void SetSecondsDots(MyClockBitFields* clockBits, uint8_t second);
void SetHourRing(MyClockBitFields* clockBits, uint8_t hour, bool outerRing, bool innerRing);
void SetHoursRing(MyClockBitFields* clockBits, uint8_t hour);
void SetTime7Seg_Top(MyClockBitFields* clockBits, uint8_t h, uint8_t m, uint8_t s);
void Set7Seg_Bot3(MyClockBitFields* clockBits, uint8_t h, uint8_t m, uint8_t s);
void UpdateAllDisplays(const MyClockBitFields* clockBits);
void SetSecondLedEvenOdd(MyClockBitFields* clockBits, uint8_t second, uint8_t minute);
void SetHourRingCustom(MyClockBitFields* clockBits, uint8_t outerMode, uint8_t innerMode);
void Set7Seg_DisplayLargeNumber(MyClockBitFields* clockBits, uint64_t number);
void SetTime7Seg_Void(MyClockBitFields* clockBits);

/* Functions for PWM and brightness control */
void DisplayScrollingText(const char* text);
void SetPWMValue(uint16_t value);
void SetPWMPercent(uint8_t percent);
void SetPWMPercentGamma(uint8_t percent);
void FadeEffect(void);
void SetDots(MyClockBitFields* clockBits, bool dot1, bool dot2);

/* 7-seg segment map and character conversion */
static const uint8_t segmentMap[42] = {
    0b00111111,   /* 0 */
    0b00000110,   /* 1 */
    0b01011011,   /* 2 */
    0b01001111,   /* 3 */
    0b01100110,   /* 4 */
    0b01101101,   /* 5 */
    0b01111101,   /* 6 */
    0b00000111,   /* 7 */
    0b01111111,   /* 8 */
    0b01101111,   /* 9 */
    0b00000000,   /* Blank (10) */
    0b01000000,   /* Minus (11) */
    0b01100011,   /* Degree (12) */
    0b00111001,   /* C (13) */
    0b01010000,   /* r (14) */
    0b01110100,   /* h (15) */
    0b01110001,   /* F (16) */
    0b01110111,   /* A (17) */
    0b01111000,   /* t (18) */
    0b00111110,   /* U/V (19) */
    0b01010100,   /* n (20) */
    0b00010000,   /* i (21) */
    0b01111001,   /* e (22) */
    0b01011110,   /* d (23) */
    0b01110011,   /* p (24) */
    0b01011100,   /* o (25) */
    0b00011100,   /* u/w/v (26) */
    0b00111000,   /* L (27) */
    0b01111100,   /* b (28) */
    0b01101110,   /* y (29) */
    0b01110110,   /* H (30) */
    0b00011110    /* J (31) */
};

uint8_t charToSegment(char c);

#endif /* INC_DISPLAY_H_ */
