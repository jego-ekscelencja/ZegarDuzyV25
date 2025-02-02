#ifndef MENU_H
#define MENU_H

#include <stdbool.h>
#include <stdint.h>

// Enum representing menu items; we have 6 items.
typedef enum
{
    MENU_ITEM_HOUR = 0,
    MENU_ITEM_SECD,
    MENU_ITEM_COLN,
    MENU_ITEM_TOP,
    MENU_ITEM_CUST,
    MENU_ITEM_END,
    MENU_ITEM_COUNT // Always last: total number of items
} MenuItem_t;


// Registers the encoder callback and resets state variables.
void MENU_Init(void);
void Display(void);

// Returns whether the menu is active (i.e., the user is navigating the menu).
bool MENU_IsActive(void);

// Enter the menu (e.g., on long button press).
void MENU_Enter(void);

// Exit the menu (e.g., on long press or when reaching "END").
void MENU_Exit(void);

// Go to the next menu item (e.g., on short button press).
void MENU_Next(void);

// Call this function periodically (e.g., every 10 ms) to refresh the menu display.
// This function is non-blocking.
void MENU_Process(void);

// Encoder callback function; 'direction' is +1 (clockwise) or -1 (counter-clockwise).
void MENU_OnEncoderRotate(int8_t direction);

// Returns the current mode (0..4) for the specified menu item.
// This can be used in the main loop to control rings, displays, etc.
uint8_t MENU_GetMode(MenuItem_t item);

#endif // MENU_H
