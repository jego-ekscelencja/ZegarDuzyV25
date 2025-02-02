// button.h

#ifndef BUTTON_H
#define BUTTON_H

#include "stm32f4xx_hal.h"  // Dostosuj do swojej serii STM32
#include <stdbool.h>

// Stan przycisku
typedef enum {
    BUTTON_RELEASED = 0,  // Przycisk nie naciśnięty
    BUTTON_PRESSED      // Przycisk naciśnięty
} ButtonState;

// Deklaracja funkcji obsługi przerwania enkodera
void Encoder_HandleInterrupt(int8_t direction);

// Typ funkcji callback dla przycisków
typedef void (*ButtonCallback)(void);

// Struktura przechowująca stan przycisku
typedef struct {
    GPIO_TypeDef* port;      // Port GPIO
    uint16_t pin;            // Numer pinu
    ButtonState state;       // Aktualny stan przycisku
    uint8_t debounceCounter; // Licznik drgań styków
    uint32_t pressTime;      // Czas naciśnięcia
    uint32_t nextRepeatTime; // Następny czas powtórzenia
    bool holdTriggered;      // Flaga wykrycia długiego naciśnięcia
    uint8_t clickCount;      // Liczba kliknięć (dla dwukliku)
    uint32_t lastClickTime;  // Czas ostatniego kliknięcia
    bool waitingForDoubleClick; // Flaga oczekiwania na dwuklik
    // Callbacki przycisku
    ButtonCallback onPress;
    ButtonCallback onRelease;
    ButtonCallback onHold;
    ButtonCallback onRepeat;
    ButtonCallback onDoubleClick;
} Button_t;

// Definicje stałych dla przycisków (czasowe)
#define DEBOUNCE_TICKS          5     // 5 x 10ms = 50ms
#define HOLD_THRESHOLD         100    // 100 x 10ms = 1s
#define REPEAT_INTERVAL         20    // 20 x 10ms = 200ms
#define DOUBLE_CLICK_THRESHOLD  30    // 30 x 10ms = 300ms

// Definicje portu i pinu dla przycisku 1
#define BUTTON1_PORT GPIOB
#define BUTTON1_PIN  GPIO_PIN_5

#define NUM_BUTTONS 1

extern Button_t buttons[NUM_BUTTONS];

// Funkcje rejestrujące callbacki dla przycisków
void Button_RegisterPressCallback(uint8_t buttonIndex, ButtonCallback cb);
void Button_RegisterReleaseCallback(uint8_t buttonIndex, ButtonCallback cb);
void Button_RegisterHoldCallback(uint8_t buttonIndex, ButtonCallback cb);
void Button_RegisterRepeatCallback(uint8_t buttonIndex, ButtonCallback cb);
void Button_RegisterDoubleClickCallback(uint8_t buttonIndex, ButtonCallback cb);

// Funkcja przetwarzająca stany przycisków
void Button_Process(void);

// Inicjalizacja przycisków (opcjonalnie)
void Button_Init(void);

// Typ funkcji callback dla obrotu enkodera
typedef void (*EncoderRotateCallback_t)(int8_t steps);

// Rejestracja i wyrejestrowanie callbacka enkodera
void Encoder_RegisterRotateCallback(EncoderRotateCallback_t cb);
void Encoder_UnregisterRotateCallback(void);

#endif // BUTTON_H
