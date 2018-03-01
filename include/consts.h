
#ifndef WORDCLOCK_CONSTS_H
#define WORDCLOCK_CONSTS_H

#include "Arduino.h"

#define ZERO                    (uint8_t)(0x00)     // workaround for issue #527
#define UNUSED_LED_FOR_25       89

#define ADDRESS_DS1307          0x68
#define ADDRESS_EEPROM_COLOR    0
#define PIN_TIME_BUTTON         PIN3    // FIXME time 2
#define PIN_COLOR_BUTTON        PIN2    // FIXME color 3?
#define PIN_NEOPIXELS           PIN4
#define PIN_SPEAKER             9

#define N_PIXELS                116
#define N_COLORS                7
#define N_SECONDS_LED           5

#define COLOR_BLACK             (uint32_t) 0x000000
#define COLOR_RED               (uint32_t) 0xFF0000

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

#endif //WORDCLOCK_CONSTS_H
