#include "slider.h"
#include "display.h"     // For UpdateAllDisplays, charToSegment
#include "main.h"        // For clockReg, timer/HSPI access, etc.
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

// External variable from main.c
extern MyClockBitFields clockReg;
volatile uint8_t disp_mode;

// Enum for scroll phases
typedef enum {
    SCROLL_PHASE_NONE = 0,  // No scrolling
    SCROLL_PHASE_IN,        // Scrolling in (text enters)
    SCROLL_PHASE_PAUSE,     // Pause phase
    SCROLL_PHASE_OUT        // Scrolling out (text leaves)
} ScrollPhase;

#define NUM_TEMPERATURE_READINGS 10
#define NUM_HUMIDITY_READINGS    10

// TOTAL_LEN: 18 elements; indices 0..5 and 12..17 are empty, 6..11 hold the text (max 6 chars)
#define TOTAL_LEN 18
static uint8_t buffer[TOTAL_LEN];

// State variables
static bool isScrolling = false;                         // Indicates if scrolling is active
static ScrollPhase scrollPhase = SCROLL_PHASE_NONE;      // Current scroll phase
static ScrollDirection currentDirection = SCROLL_RIGHT_TO_LEFT;  // Current scroll direction
static bool displayNumberPending = false;                // Flag: pending number display
static uint32_t pendingNumberToDisplay = 0;              // Pending number to display
static int16_t windowIndex = 0;                          // Index for the current 6-character window in buffer

// Counter to limit update frequency in SLIDER_Update()
static uint8_t scrollSpeedCounter = 0;
static const uint8_t scrollSpeedTicks = 5;  // Advance window every 5 updates (e.g., every 10ms => 50ms)

// Options for post-entry behavior
static bool doStayForever = false;   // Stay on screen after entering fully
static bool doPauseThenOut = false;  // Pause then scroll out

// Pause variables
static uint32_t pauseCounter = 0;  // Countdown for pause duration
static uint32_t pauseTicks = 0;    // Number of update ticks to pause

// Fills the 18-element buffer with: indices 0..5 empty, indices 6..11 with up to 6 characters of text, indices 12..17 empty
static void PrepareBuffer18(const char* text)
{
    memset(buffer, 0, sizeof(buffer));  // Clear entire buffer

    char temp[7];
    strncpy(temp, text, 6);  // Copy up to 6 characters from text
    temp[6] = '\0';          // Ensure null termination

    // Indices 6..11: fill with converted characters from temp
    for (int i = 0; i < 6; i++)
    {
        if (temp[i] == '\0')
        {
            // If text ends, fill with 0 (blank)
            buffer[6 + i] = 0;
        }
        else {
            buffer[6 + i] = (uint8_t)charToSegment(temp[i]);
        }
    }
    // Indices 0..5 and 12..17 remain 0 from memset
}

// Displays exactly 6 bytes from buffer (from windowIndex to windowIndex+5) on bottomDisplay
static void ShowWindow(void)
{
    uint64_t val = 0ULL;

    // Safe read from buffer (indices 0..17)
    uint8_t d0 = 0;
    if (windowIndex + 0 >= 0 && windowIndex + 0 < TOTAL_LEN)
        d0 = buffer[windowIndex + 0];
    uint8_t d1 = 0;
    if (windowIndex + 1 >= 0 && windowIndex + 1 < TOTAL_LEN)
        d1 = buffer[windowIndex + 1];
    uint8_t d2 = 0;
    if (windowIndex + 2 >= 0 && windowIndex + 2 < TOTAL_LEN)
        d2 = buffer[windowIndex + 2];
    uint8_t d3 = 0;
    if (windowIndex + 3 >= 0 && windowIndex + 3 < TOTAL_LEN)
        d3 = buffer[windowIndex + 3];
    uint8_t d4 = 0;
    if (windowIndex + 4 >= 0 && windowIndex + 4 < TOTAL_LEN)
        d4 = buffer[windowIndex + 4];
    uint8_t d5 = 0;
    if (windowIndex + 5 >= 0 && windowIndex + 5 < TOTAL_LEN)
        d5 = buffer[windowIndex + 5];

    // Compose a 48-bit value from 6 bytes; digit0 is placed in bits [47..40] and digit5 in bits [7..0]
    val |= ((uint64_t)d5 << 40);
    val |= ((uint64_t)d4 << 32);
    val |= ((uint64_t)d3 << 24);
    val |= ((uint64_t)d2 << 16);
    val |= ((uint64_t)d1 << 8);
    val |= ((uint64_t)d0 << 0);

    clockReg.bottomDisplay = val;
    // Optionally call UpdateAllDisplays(&clockReg);
}

// Initializes all slider variables to a resting state
void SLIDER_Init(void)
{
    isScrolling = false;
    scrollPhase = SCROLL_PHASE_NONE;
    currentDirection = SCROLL_RIGHT_TO_LEFT;
    windowIndex = 0;
    scrollSpeedCounter = 0;

    doStayForever = false;
    doPauseThenOut = false;
    pauseCounter = 0;
    pauseTicks = 0;

    memset(buffer, 0, sizeof(buffer));
}

// Sets a string that scrolls in, pauses, then scrolls out.
// For R->L: start at index 12 (empty) and move left until index 6 (fully visible), then pause and continue to index 0.
// For L->R: start at index 0 and move right until index 6, then pause and continue to index 12.
void SLIDER_SetStringPauseAndOut(const char* text, ScrollDirection dir, uint32_t pauseTime)
{
    if (!text) return;

    PrepareBuffer18(text); // Fill buffer with text segments

    currentDirection = dir;
    scrollPhase = SCROLL_PHASE_IN;
    isScrolling = true;
    doStayForever = false;
    doPauseThenOut = true;
    pauseTicks = pauseTime;
    pauseCounter = 0;

    if (dir == SCROLL_RIGHT_TO_LEFT)
        windowIndex = 12;  // Start with empty part on the left
    else
        windowIndex = 0;   // Start with empty part on the right

    ShowWindow();
}

// Sets a string that scrolls in and stays (does not scroll out)
void SLIDER_SetStringAndStay(const char* text, ScrollDirection dir)
{
    if (!text) return;

    PrepareBuffer18(text);
    currentDirection = dir;
    scrollPhase = SCROLL_PHASE_IN;
    isScrolling = true;
    doStayForever = true;
    doPauseThenOut = false;
    pauseCounter = 0;
    pauseTicks = 0;

    if (dir == SCROLL_RIGHT_TO_LEFT)
        windowIndex = 12;
    else
        windowIndex = 0;

    ShowWindow();
}

// Sets a string that scrolls out only; assumes the text is fully visible initially
void SLIDER_SetString(const char* text, ScrollDirection dir, uint32_t pauseTime)
{
    if (!text) return;

    PrepareBuffer18(text);
    currentDirection = dir;
    scrollPhase = SCROLL_PHASE_PAUSE;  // Start with pause phase
    isScrolling = true;
    doStayForever = false;
    doPauseThenOut = true;
    pauseTicks = pauseTime;
    pauseCounter = pauseTime;

    windowIndex = 6;  // Set window index so text is fully visible
    ShowWindow();
}

// Immediately stops scrolling and resets the window index.
// If a number display is pending, displays it immediately.
void SLIDER_Stop(void)
{
    isScrolling = false;
    scrollPhase = SCROLL_PHASE_NONE;
    windowIndex = 0;

    if (displayNumberPending) {
        SLIDER_DisplayNumber(pendingNumberToDisplay);
        displayNumberPending = false;
    }
}

// Called periodically (e.g., every 10 ms) to update the slider's state.
void SLIDER_Update(void)
{
    if (!isScrolling) return;

    if (scrollSpeedCounter < scrollSpeedTicks) {
        scrollSpeedCounter++;
        return;
    }
    scrollSpeedCounter = 0;

    switch (scrollPhase)
    {
    case SCROLL_PHASE_IN:
        if (currentDirection == SCROLL_RIGHT_TO_LEFT) {
            windowIndex--; // Move left
            if (windowIndex == 6) { // Text is fully visible
                if (doStayForever) {
                    isScrolling = false;
                    scrollPhase = SCROLL_PHASE_NONE;
                } else if (doPauseThenOut && pauseTicks > 0) {
                    scrollPhase = SCROLL_PHASE_PAUSE;
                    pauseCounter = pauseTicks;
                } else {
                    scrollPhase = SCROLL_PHASE_OUT;
                }
            }
        } else { // SCROLL_LEFT_TO_RIGHT
            windowIndex++; // Move right
            if (windowIndex == 6) {
                if (doStayForever) {
                    isScrolling = false;
                    scrollPhase = SCROLL_PHASE_NONE;
                } else if (doPauseThenOut && pauseTicks > 0) {
                    scrollPhase = SCROLL_PHASE_PAUSE;
                    pauseCounter = pauseTicks;
                } else {
                    scrollPhase = SCROLL_PHASE_OUT;
                }
            }
        }
        break;

    case SCROLL_PHASE_PAUSE:
        if (pauseCounter > 0) {
            pauseCounter--; // Decrement pause counter
        } else {
            scrollPhase = SCROLL_PHASE_OUT; // End pause phase
        }
        break;

    case SCROLL_PHASE_OUT:
        if (currentDirection == SCROLL_RIGHT_TO_LEFT) {
            windowIndex--; // Continue moving left
            if (windowIndex < 0) {
                SLIDER_Stop();
            }
        } else { // SCROLL_LEFT_TO_RIGHT
            windowIndex++; // Continue moving right
            if (windowIndex > 12) {
                SLIDER_Stop();
            }
        }
        break;

    default:
        return;
    }

    ShowWindow(); // Refresh display after updating windowIndex
}

bool SLIDER_IsStopped(void) {
    return (scrollPhase == SCROLL_PHASE_NONE);
}

// Immediately displays a number on the slider (maximum 6 digits)
void SLIDER_DisplayNumber(uint32_t number)
{
    if (!SLIDER_IsStopped()) {
        displayNumberPending = true;
        pendingNumberToDisplay = (number > 999999) ? 999999 : number;
        return;
    }
    if (number > 999999)
        number = 999999;

    uint8_t digits[6];
    for (int i = 0; i < 6; i++) {
        digits[i] = charToSegment(' '); // Initialize digits as blank
    }
    for (int i = 5; i >= 0; i--) {
        digits[i] = charToSegment('0' + (number % 10));
        number /= 10;
    }
    uint64_t displayVal = 0ULL;
    displayVal |= ((uint64_t)digits[5] << 40);
    displayVal |= ((uint64_t)digits[4] << 32);
    displayVal |= ((uint64_t)digits[3] << 24);
    displayVal |= ((uint64_t)digits[2] << 16);
    displayVal |= ((uint64_t)digits[1] << 8);
    displayVal |= ((uint64_t)digits[0] << 0);
    clockReg.bottomDisplay = displayVal;
    // Optionally call UpdateAllDisplays(&clockReg);
}

// Displays a temperature value on the slider
void SLIDER_DisplayTemperature(int32_t temperature)
{
    if (!SLIDER_IsStopped()) {
        displayNumberPending = true;
        if (temperature > 99999) {
            pendingNumberToDisplay = 99999;
        } else if (temperature < -99999) {
            pendingNumberToDisplay = 99999;
        } else {
            pendingNumberToDisplay = (temperature < 0) ? (uint32_t)(-temperature) : (uint32_t)temperature;
        }
        return;
    }
    if (temperature > 99999)
        temperature = 99999;
    else if (temperature < -99999)
        temperature = -99999;

    uint8_t digits[6];
    for (int i = 0; i < 6; i++) {
        digits[i] = charToSegment(' '); // Initialize digits as blank
    }
    bool isNegative = false;
    if (temperature < 0) {
        isNegative = true;
        temperature = -temperature;
    }
    if (isNegative && temperature >= 10000) {
        digits[5] = charToSegment('*'); // Display degree symbol only for 5-digit negative numbers
    } else {
        digits[5] = charToSegment('C');
        digits[4] = charToSegment('*');
    }
    int digitPos = 3;
    for (int i = 0; i < 4; i++) {
        if (temperature > 0 || i > 0) {
            digits[digitPos - i] = charToSegment('0' + (temperature % 10));
            temperature /= 10;
        } else {
            digits[digitPos - i] = charToSegment('0');
            temperature /= 10;
        }
    }
    if (isNegative) {
        digits[0] = charToSegment('-'); // Display minus sign
    }
    uint64_t displayVal = 0ULL;
    displayVal |= ((uint64_t)digits[5] << 40);
    displayVal |= ((uint64_t)digits[4] << 32);
    displayVal |= ((uint64_t)digits[3] << 24);
    displayVal |= ((uint64_t)digits[2] << 16);
    displayVal |= ((uint64_t)digits[1] << 8);
    displayVal |= ((uint64_t)digits[0] << 0);
    displayVal |= ((uint64_t)0b10000000 << 8); // Set decimal point on digit[1]
    clockReg.bottomDisplay = displayVal;
    // Optionally call UpdateAllDisplays(&clockReg);
}

// Displays averaged temperature on the slider
void SLIDER_DisplayTemperature_AVG(int32_t currentTemperature)
{
    static int32_t temperatureBuffer[NUM_TEMPERATURE_READINGS] = {0};
    static int index = 0;
    static int count = 0;
    static int32_t sum = 0;

    sum -= temperatureBuffer[index];
    temperatureBuffer[index] = currentTemperature;
    sum += currentTemperature;
    index = (index + 1) % NUM_TEMPERATURE_READINGS;
    if (count < NUM_TEMPERATURE_READINGS) {
        count++;
    }
    int32_t averageTemperature = sum / count;
    if (averageTemperature > 99999)
        averageTemperature = 99999;
    else if (averageTemperature < -99999)
        averageTemperature = -99999;

    if (!SLIDER_IsStopped()) {
        displayNumberPending = true;
        if (averageTemperature > 99999) {
            pendingNumberToDisplay = 99999;
        } else if (averageTemperature < -99999) {
            pendingNumberToDisplay = 99999;
        } else {
            pendingNumberToDisplay = (averageTemperature < 0)
                                   ? (uint32_t)(-averageTemperature)
                                   : (uint32_t)averageTemperature;
        }
        return;
    }

    if (averageTemperature > 99999)
        averageTemperature = 99999;
    else if (averageTemperature < -99999)
        averageTemperature = -99999;

    uint8_t digits[6];
    for (int i = 0; i < 6; i++) {
        digits[i] = charToSegment(' '); // Initialize with blank
    }
    bool isNegative = (averageTemperature < 0);
    if (isNegative) {
        averageTemperature = -averageTemperature;
    }
    if (isNegative && averageTemperature >= 10000) {
        digits[5] = charToSegment('*');
    } else {
        digits[5] = charToSegment('C');
        digits[4] = charToSegment('*');
    }
    int digitPos = 3;
    int32_t tempVal = averageTemperature;
    for (int i = 0; i < 4; i++) {
        if (tempVal > 0 || i == 0) {
            digits[digitPos - i] = charToSegment('0' + (tempVal % 10));
            tempVal /= 10;
        } else {
            digits[digitPos - i] = charToSegment('0');
            tempVal /= 10;
        }
    }
    if (isNegative) {
        digits[0] = charToSegment('-'); // Minus sign
    }
    uint64_t displayVal = 0ULL;
    displayVal |= ((uint64_t)digits[5] << 40);
    displayVal |= ((uint64_t)digits[4] << 32);
    displayVal |= ((uint64_t)digits[3] << 24);
    displayVal |= ((uint64_t)digits[2] << 16);
    displayVal |= ((uint64_t)digits[1] << 8);
    displayVal |= ((uint64_t)digits[0] << 0);
    displayVal |= ((uint64_t)0b10000000 << 8); // Decimal point on digit[1]
    clockReg.bottomDisplay = displayVal;
    // Optionally call UpdateAllDisplays(&clockReg);
}

// Displays averaged humidity on the slider
void SLIDER_DisplayHumidity_AVG (uint32_t currentHumidity)
{
    static uint32_t humidityBuffer[NUM_HUMIDITY_READINGS] = {0};
    static int index = 0;
    static int count = 0;
    static uint32_t sum = 0;

    sum -= humidityBuffer[index];
    humidityBuffer[index] = currentHumidity;
    sum += currentHumidity;
    index = (index + 1) % NUM_HUMIDITY_READINGS;
    if (count < NUM_HUMIDITY_READINGS)
        count++;
    uint32_t averageHumidity = sum / count;
    if (averageHumidity > 999999)
        averageHumidity = 999999;

    if (!SLIDER_IsStopped()) {
        displayNumberPending = true;
        pendingNumberToDisplay = (averageHumidity > 999999) ? 999999 : averageHumidity;
        return;
    }

    uint8_t digits[6];
    for (int i = 0; i < 6; i++) {
        digits[i] = charToSegment(' '); // Initialize with blank
    }
    digits[5] = charToSegment('h');
    digits[4] = charToSegment('R');
    int digitPos = 3;
    for (int i = 0; i < 4; i++) { // 4 digits for humidity (0-9999)
        if (averageHumidity > 0 || i > 0) {
            digits[digitPos - i] = charToSegment('0' + (averageHumidity % 10));
            averageHumidity /= 10;
        } else {
            digits[digitPos - i] = charToSegment('0');
            averageHumidity /= 10;
        }
    }
    uint64_t displayVal = 0ULL;
    displayVal |= ((uint64_t)digits[5] << 40);
    displayVal |= ((uint64_t)digits[4] << 32);
    displayVal |= ((uint64_t)digits[3] << 24);
    displayVal |= ((uint64_t)digits[2] << 16);
    displayVal |= ((uint64_t)digits[1] << 8);
    displayVal |= ((uint64_t)digits[0] << 0);
    displayVal |= ((uint64_t)0b10000000 << 8); // Optional decimal point on digit[1]
    clockReg.bottomDisplay = displayVal;
    // Optionally call UpdateAllDisplays(&clockReg);
}

// Displays humidity immediately on the slider
void SLIDER_DisplayHumidity(uint32_t humidity)
{
    if (!SLIDER_IsStopped()) {
        displayNumberPending = true;
        pendingNumberToDisplay = (humidity > 999999) ? 999999 : humidity;
        return;
    }
    if (humidity > 999999)
        humidity = 999999;
    uint8_t digits[6];
    for (int i = 0; i < 6; i++) {
        digits[i] = charToSegment(' '); // Initialize with blank
    }
    digits[5] = charToSegment('h');
    digits[4] = charToSegment('R');
    int digitPos = 3;
    for (int i = 0; i < 4; i++) { // 4 digits for humidity (0-9999)
        if (humidity > 0 || i > 0) {
            digits[digitPos - i] = charToSegment('0' + (humidity % 10));
            humidity /= 10;
        } else {
            digits[digitPos - i] = charToSegment('0');
            humidity /= 10;
        }
    }
    uint64_t displayVal = 0ULL;
    displayVal |= ((uint64_t)digits[5] << 40);
    displayVal |= ((uint64_t)digits[4] << 32);
    displayVal |= ((uint64_t)digits[3] << 24);
    displayVal |= ((uint64_t)digits[2] << 16);
    displayVal |= ((uint64_t)digits[1] << 8);
    displayVal |= ((uint64_t)digits[0] << 0);
    displayVal |= ((uint64_t)0b10000000 << 8); // Optional: decimal point on digit[1]
    clockReg.bottomDisplay = displayVal;
    // Optionally call UpdateAllDisplays(&clockReg);
}
