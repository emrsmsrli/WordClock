
#ifndef WORDCLOCK_CONSTS_H
#define WORDCLOCK_CONSTS_H

#include "Arduino.h"

#define UNUSED_LED_FOR_25       89

#define ADDRESS_EEPROM_COLOR    0x0
#define ADDRESS_EEPROM_BRIGHTN  0x4
#define PIN_TIME_BUTTON         PIN3
#define PIN_COLOR_BUTTON        PIN2
#define PIN_NEOPIXELS           PIN4
#define PIN_SPEAKER             PIN7

#define N_PIXELS                116
#define N_COLORS                7
#define N_SECONDS_LED           5

#define COLOR_BLACK             (uint32_t) 0x000000
#define COLOR_RED               (uint32_t) 0xFF0000

#define BRIGHTNESS_LOW          0x20    // 0.125
#define BRIGHTNESS_HIGH         0xFF    // 1.000

#define BTN_DEBOUNCE_THRESHOLD  50
#define BTN_CLICK_THRESHOLD     300

#define ANIMATION_TIME_MS       200
#define ANIMATE(i, duration)    for(uint8_t (i) = 0; (i) <= (duration); (i)++)
#define SMOOTH_STEP(x)          ((x) * (x) * (3 - 2 * (x)))

#define NOTE_G                  1275
#define NOTE_A                  1136
#define NOTE_B                  1014
#define NOTE_c                  956
#define NOTE_d                  834
#define NOTE_e                  765
#define NOTE_x                  655
#define NOTE_y                  715
#define NOTE_REST               0

#define SONG_TEMPO              175
#define SPEE                    5

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color;

uint32_t current_color();
uint8_t smooth_step(uint8_t i, uint8_t N, uint8_t min, uint8_t max);
color extract_color(uint32_t c);
uint32_t shift_color(uint8_t i, uint8_t N, uint32_t old_color, uint32_t new_color);
void shift_color_all(uint32_t old_color, uint32_t new_color);
void tick();
void calculate_next_leds();
void display_time();
void on_color_button_pressed();
void on_color_button_double_pressed();
void on_time_button_pressed();
void on_time_button_double_pressed();
void color_isr();
void time_isr();
void adjust_brightness();

#endif //WORDCLOCK_CONSTS_H
