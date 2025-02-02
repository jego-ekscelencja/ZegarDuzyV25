

//static const uint8_t s_menuMaxModes[MENU_ITEM_COUNT] =
//{
//    4, // HOUR -> dopuszczalne 0..4
//    3, // SECD -> 0..3
//    4, // COLN -> 0..1
//    5, // TOP  -> 0..5
//    9, // CUST -> 0..9
//    0  // END  -> 0..0 (albo i tak nieużywane)
//};

#include "menu.h"
#include "slider.h"
#include "display.h"
#include "button.h"    // for registering encoder callbacks
#include "main.h"      // for clockReg, etc.
#include <string.h>
#include <stdio.h>
#include "sht30.h"
#include "rtc.h"
#include "stm32f4xx_hal_rtc.h"

static const uint8_t s_menuMaxModes[MENU_ITEM_COUNT] = {
    [MENU_ITEM_HOUR] = 4,
    [MENU_ITEM_SECD] = 3,
    [MENU_ITEM_COLN] = 3,
    [MENU_ITEM_TOP]  = 5,
    [MENU_ITEM_CUST] = 9,
    [MENU_ITEM_END]  = 0
};

static uint8_t s_menuModes[MENU_ITEM_COUNT] = {0};  // Internal mode values for each menu item
static bool     s_menuActive = false;               // Menu active flag
static uint8_t  s_currentItem = 0;                  // Current menu item index

#define MAX_MODE_VALUE 4  // Not used; per-item max is in s_menuMaxModes

// 4-letter labels for menu items
static const char* s_menuLabels[MENU_ITEM_COUNT] =
{
    "HOUR",   // MENU_ITEM_HOUR
    "SECD",   // MENU_ITEM_SECD
    "COLN",   // MENU_ITEM_COLN
    "TOP ",   // MENU_ITEM_TOP
    "CUST",   // MENU_ITEM_CUST
    "END "    // MENU_ITEM_END
};

// Display a 4-letter label using the slider (non-blocking)
static void ShowMenuItemLabel(uint8_t itemIndex)
{
    if (itemIndex >= MENU_ITEM_COUNT) return;  // Safety check
    SLIDER_SetStringAndStay(s_menuLabels[itemIndex], SCROLL_RIGHT_TO_LEFT);
}

// Display the current menu item; for items other than END, show "LABEL+mode"
static void MENU_ShowCurrent(void)
{
    if (s_currentItem == MENU_ITEM_END)
    {
        SLIDER_SetStringAndStay("END ", SCROLL_RIGHT_TO_LEFT);
        return;
    }
    char temp[7];  // 6 characters + null terminator
    snprintf(temp, sizeof(temp), "%s%d", s_menuLabels[s_currentItem], (int)s_menuModes[s_currentItem]);
    SLIDER_SetStringAndStay(temp, SCROLL_RIGHT_TO_LEFT);
}

// Initialize the menu module: reset state and register encoder callback
void MENU_Init(void)
{
    s_menuActive  = false;
    s_currentItem = 0;
    for (int i = 0; i < MENU_ITEM_COUNT; i++)
    {
        s_menuModes[i] = 0;
    }
    Encoder_RegisterRotateCallback(MENU_OnEncoderRotate);
}

// Returns true if the menu is active
bool MENU_IsActive(void)
{
    return s_menuActive;
}

// Enter the menu (e.g., on long button press)
void MENU_Enter(void)
{
    if (!s_menuActive)
    {
        s_menuActive  = true;
        s_currentItem = 0;
        MENU_ShowCurrent();
    }
}

// Exit the menu (e.g., on long press or when reaching "END")
void MENU_Exit(void)
{
    if (s_menuActive)
    {
        s_menuActive  = false;
        s_currentItem = 0;
        SLIDER_SetStringAndStay("    ", SCROLL_RIGHT_TO_LEFT);
    }
}

// Advance to the next menu item (e.g., on short button press)
void MENU_Next(void)
{
    if (!s_menuActive) return;
    s_currentItem++;
    if (s_currentItem >= MENU_ITEM_COUNT)
    {
        MENU_Exit();
        return;
    }
    if (s_currentItem == MENU_ITEM_END)
    {
        MENU_ShowCurrent();
        return;
    }
    MENU_ShowCurrent();
}

// Process menu state (non-blocking); can add timeout or auto-exit logic here
void MENU_Process(void)
{
    if (!s_menuActive)
    {
        return;
    }
    // Additional state update logic can be added here.
}

// Return the current mode (0..4) for the specified menu item
uint8_t MENU_GetMode(MenuItem_t item)
{
    if (item >= MENU_ITEM_COUNT) return 0;
    return s_menuModes[item];
}

// Encoder callback: adjust the mode for the current menu item based on direction
void MENU_OnEncoderRotate(int8_t direction)
{
    if (!s_menuActive)
    {
        return;
    }
    if (s_currentItem >= MENU_ITEM_COUNT)
    {
        return;
    }
    uint8_t maxVal = s_menuMaxModes[s_currentItem];
    if (direction > 0)
    {
        if (s_menuModes[s_currentItem] < maxVal)
            s_menuModes[s_currentItem]++;
    }
    else
    {
        if (s_menuModes[s_currentItem] > 0)
            s_menuModes[s_currentItem]--;
    }
    if (s_currentItem != MENU_ITEM_END)
    {
        MENU_ShowCurrent();
    }
}

// Display function called in the main loop to update hardware based on menu settings
void Display(void){
    uint8_t hourMode = MENU_GetMode(MENU_ITEM_HOUR);
    uint8_t secdMode = MENU_GetMode(MENU_ITEM_SECD);
    uint8_t colonMode = MENU_GetMode(MENU_ITEM_COLN);
    uint8_t topMode = MENU_GetMode(MENU_ITEM_TOP);
    uint8_t custMode = MENU_GetMode(MENU_ITEM_CUST);

    // Update hours ring based on hourMode
    switch (hourMode) {
    case 0: /* ring OFF */
        SetHourRingCustom(&clockReg, 1, 1);
        break;
    case 1:
        SetHourRingCustom(&clockReg, 0, 1);
        break;
    case 2:
        SetHourRingCustom(&clockReg, 1, 0);
        break;
    case 3:
        SetHourRingCustom(&clockReg, 0, 0);
        break;
    case 4: /* additional mode */
        // ...
        break;
    }

    // Update seconds ring based on secdMode
    switch (secdMode) {
    case 0:
        SetSecondLedSingle(&clockReg, sTime.Seconds);
        break;
    case 1:
        SetSecondLedAccumulating(&clockReg, sTime.Seconds);
        break;
    case 2:
        SetSecondLedAccumulating2(&clockReg, sTime.Seconds);
        break;
    case 3:
        SetSecondLedEvenOdd(&clockReg, sTime.Seconds, sTime.Minutes);
        break;
    case 4: /* additional mode */
        // ...
        break;
    }

    // Update colon display based on colonMode
    switch (colonMode) {
    case 0:
        SetDots(&clockReg, colon, colon);
        break;
    case 1:
        SetDots(&clockReg, 0, colon);
        break;
    case 2:
        SetDots(&clockReg, colon, 0);
        break;
    case 3:
        SetDots(&clockReg, 0, 0);
        break;
    case 4: /* additional mode */
        // ...
        break;
    }

    // Update top 7-seg display based on topMode
    switch (topMode) {
    case 0:
        SetTime7Seg_Top(&clockReg, sTime.Hours, sTime.Minutes, sTime.Seconds);
        break;
    case 1:
        SetTime7Seg_Void(&clockReg);
        break;
    // additional cases can be added
    }

    // custMode can be used for additional functionality; currently not used.
    switch (custMode) {
    case 0: /* ... */ break;
    case 1: /* ... */ break;
    case 2: /* ... */ break;
    case 3: /* ... */ break;
    case 4: /* ... */ break;
    }

    // If menu is not active, update sensor data display
    SHT30_Data_t data;  // Sensor data variable
    if (!MENU_IsActive()) {
        if (SHT30_GetLatestData(&data)) {
            disp_mode ? SLIDER_DisplayTemperature(data.temperature)
                      : SLIDER_DisplayHumidity(data.humidity);
        }
    }
    UpdateAllDisplays(&clockReg);
}
