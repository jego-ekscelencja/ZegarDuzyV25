#ifndef SLIDER_H
#define SLIDER_H

#include <stdint.h>
#include <stdbool.h>

// Scroll direction
typedef enum {
    SCROLL_RIGHT_TO_LEFT = 0,  // Scroll from right to left
    SCROLL_LEFT_TO_RIGHT     // Scroll from left to right
} ScrollDirection;

extern volatile uint8_t disp_mode;  // Display mode flag

// Initializes the slider module (clears buffers, sets initial flags)
void SLIDER_Init(void);

// Sets a scrolling string on the bottom display with the specified direction
// and pause time (in units of 10 ms)
void SLIDER_SetString(const char* text, ScrollDirection dir, uint32_t pauseTime);

// Must be called periodically (e.g., every 10 ms in TIM5 interrupt) to update scrolling
void SLIDER_Update(void);

// Immediately stops the scrolling and resets the scroll index
void SLIDER_Stop(void);

// Displays a temperature value on the slider
void SLIDER_DisplayTemperature(int32_t temperature);

// Displays a humidity value on the slider
void SLIDER_DisplayHumidity(uint32_t humidity);

// Sets a string that scrolls in and then stays (does not scroll out)
void SLIDER_SetStringAndStay(const char* text, ScrollDirection direction);

// Returns true if the slider is stopped
bool SLIDER_IsStopped(void);

// Displays a number on the slider immediately
void SLIDER_DisplayNumber(uint32_t number);

// Displays averaged humidity value on the slider
void SLIDER_DisplayHumidity_AVG (uint32_t currentHumidity);

// Displays averaged temperature value on the slider
void SLIDER_DisplayTemperature_AVG(int32_t currentTemperature);

// Sets a string that scrolls in, pauses for a given number of update cycles, then scrolls out.
// pauseTicks indicates how many times SLIDER_Update() (every 10 ms) should wait (e.g., 200 means 2 seconds).
void SLIDER_SetStringPauseAndOut(const char* text, ScrollDirection direction, uint32_t pauseTicks);

#endif // SLIDER_H
