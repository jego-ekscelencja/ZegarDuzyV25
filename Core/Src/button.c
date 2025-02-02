// button.c

#include "button.h"
#include "stdbool.h"
#include "gps_parser.h"
#include "main.h"
#include "display.h"
#include "slider.h"
#include "sht30.h"

extern MyClockBitFields clockReg;  /* Globalny rejestr wyświetlacza */
volatile uint8_t counter = 0;

static EncoderRotateCallback_t s_encoderCb = NULL;  /* Callback enkodera */

void Encoder_RegisterRotateCallback(EncoderRotateCallback_t cb)
{
    s_encoderCb = cb;
}

void Encoder_UnregisterRotateCallback(void)
{
    s_encoderCb = NULL;
}

/* Inicjalizacja przycisków */
Button_t buttons[NUM_BUTTONS] = {
    { BUTTON1_PORT, BUTTON1_PIN, BUTTON_RELEASED, 0, 0, 0, false, 0, 0, false, NULL, NULL, NULL, NULL, NULL }
};

// Rejestracja callbacków przycisku
void Button_RegisterPressCallback(uint8_t buttonIndex, ButtonCallback cb) {
    if (buttonIndex < NUM_BUTTONS) {
        buttons[buttonIndex].onPress = cb;
    }
}

void Button_RegisterReleaseCallback(uint8_t buttonIndex, ButtonCallback cb) {
    if (buttonIndex < NUM_BUTTONS) {
        buttons[buttonIndex].onRelease = cb;
    }
}

void Button_RegisterHoldCallback(uint8_t buttonIndex, ButtonCallback cb) {
    if (buttonIndex < NUM_BUTTONS) {
        buttons[buttonIndex].onHold = cb;
    }
}

void Button_RegisterRepeatCallback(uint8_t buttonIndex, ButtonCallback cb) {
    if (buttonIndex < NUM_BUTTONS) {
        buttons[buttonIndex].onRepeat = cb;
    }
}

void Button_RegisterDoubleClickCallback(uint8_t buttonIndex, ButtonCallback cb) {
    if (buttonIndex < NUM_BUTTONS) {
        buttons[buttonIndex].onDoubleClick = cb;
    }
}

extern volatile uint32_t systemTicks;  /* Globalny licznik taktów */

// Funkcja przetwarzająca stany przycisków
void Button_Process(void) {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        Button_t *btn = &buttons[i];
        bool rawState = (HAL_GPIO_ReadPin(btn->port, btn->pin) == GPIO_PIN_RESET); /* Odczyt stanu */

        /* Debounce */
        if (rawState != (btn->state == BUTTON_PRESSED)) {
            btn->debounceCounter++;
            if (btn->debounceCounter >= DEBOUNCE_TICKS) {
                btn->debounceCounter = 0;
                if (rawState) {
                    btn->state = BUTTON_PRESSED;
                    btn->pressTime = systemTicks;
                    btn->holdTriggered = false;
                } else {
                    btn->state = BUTTON_RELEASED;
                    /* Obsługa kliknięć (jeśli nie było hold) */
                    if (!btn->holdTriggered) {
                        if (btn->waitingForDoubleClick) {
                            if ((systemTicks - btn->lastClickTime) <= DOUBLE_CLICK_THRESHOLD) {
                                btn->clickCount++;
                            } else {
                                btn->clickCount = 1;
                            }
                        } else {
                            btn->clickCount = 1;
                            btn->waitingForDoubleClick = true;
                            btn->lastClickTime = systemTicks;
                        }
                        if (btn->clickCount == 2) {
                            btn->waitingForDoubleClick = false;
                            btn->clickCount = 0;
                            if (btn->onDoubleClick) {
                                btn->onDoubleClick();
                            }
                        }
                    }
                    if (btn->onRelease)
                        btn->onRelease();
                }
            }
        } else {
            btn->debounceCounter = 0;
        }

        /* Timeout dwukliku */
        if (btn->waitingForDoubleClick && ((systemTicks - btn->lastClickTime) > DOUBLE_CLICK_THRESHOLD)) {
            if (btn->clickCount == 1) {
                if (!btn->holdTriggered && btn->onPress) {
                    btn->onPress();
                }
            }
            btn->waitingForDoubleClick = false;
            btn->clickCount = 0;
        }

        /* Obsługa hold i repeat */
        if (btn->state == BUTTON_PRESSED) {
            uint32_t elapsed = systemTicks - btn->pressTime;
            if (!btn->holdTriggered && (elapsed >= HOLD_THRESHOLD)) {
                btn->holdTriggered = true;
                btn->clickCount = 0;
                btn->waitingForDoubleClick = false;
                if (btn->onHold)
                    btn->onHold();
                btn->nextRepeatTime = systemTicks + REPEAT_INTERVAL;
            }
            if (btn->holdTriggered && (systemTicks >= btn->nextRepeatTime)) {
                if (btn->onRepeat)
                    btn->onRepeat();
                btn->nextRepeatTime += REPEAT_INTERVAL;
            }
        }
    }
}


/* @brief  Callback wywoływany w przerwaniu timera (TIM5). */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM5) {
        SHT30_10msHandler();         /* Obsługa czujnika SHT30 */
        systemTicks++;               /* Inkrementacja globalnego licznika */
        SLIDER_Update();             /* Aktualizacja slidera */
    }
    if (colon == 1) {
        if (counter > 0) {
            counter--;             /* Odliczanie */
        } else {
            counter = 50;          /* Reset licznika */
            colon = 0;             /* Reset stanu colon */
        }
    }
    static volatile uint16_t cnter = 0;
    cnter++;
    if (cnter > 400) {
        cnter = 0;
        disp_mode++;
        if (disp_mode > 1)
            disp_mode = 0;
    }
}

void Encoder_HandleInterrupt(int8_t direction)
{
    if (s_encoderCb != NULL) {
        s_encoderCb(direction);
    }
}
