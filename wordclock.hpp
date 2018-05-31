
#ifndef WORDCLOCK_CONSTS_H
#define WORDCLOCK_CONSTS_H

#include <EEPROM.h>
#include <RTClib.h>
#include "Arduino.h"
#include "Button.hpp"
#include "LedArray.hpp"
#include "Music.h"

#define ADDRESS_EEPROM_COLOR    0x0
#define ADDRESS_EEPROM_BRIGHTN  0x4
#define PIN_TIME_BUTTON         PIN3
#define PIN_COLOR_BUTTON        PIN2

#define N_COLORS                7
#define N_SECONDS_LED           5

#define BRIGHTNESS_LOW          0x20    // 0.125
#define BRIGHTNESS_HIGH         0xFF    // 1.000

#define ANIMATION_TIME_MS       100
#define ANIMATE(i, duration)    for(uint8_t (i) = 0; (i) <= (duration); (i)++)
#define SMOOTH_STEP(x)          ((x) * (x) * (3 - 2 * (x)))

#define DELAY_SHORT_US          800

/// \c color struct for destructuring integer
/// color values into R, G and B components.
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color;

/// Returns the currently shown color with adjusted brightness.
/// \return Current color with brightness.
uint32_t current_color();

/// Implementation of the smooth step interpolation. <br>
/// Use this in loops. For example:
/// \code
///     for(uint8_t i = 0; i <= LENGTH; i++) {
///         uint8 interpolated = smooth_step(i, LENGTH, 0, 1);
///         ...
///     }
/// \endcode <br>
/// divides 0..1 region into LENGTH pieces according to smooth step rules.
/// \param i Current iteration count.
/// \param N Total iteration count.
/// \param min The minimum value there can be.
/// \param max The maximum value there can be.
/// \return Smooth step interpolated value given the parameters.
uint8_t smooth_step(uint8_t i, uint8_t N, uint8_t min, uint8_t max);

/// Extracts red, green and blue values from an unsigned integer color value.
/// \param c Color to extract.
/// \return A \c color struct containing the components of the color.
color extract_color(uint32_t c);

/// Shifts the color from \c old_color to \c new_color by \c N smooth steps.
/// \param i Current iteration count.
/// \param N Total iteration count.
/// \param old_color Color to shift from.
/// \param new_color Color to shift to.
/// \return Shifted color in iteration \c i th iteration.
uint32_t shift_color(uint8_t i, uint8_t N, uint32_t old_color, uint32_t new_color);

/// Shifts all the lit LedArrays on the board.
/// \param old_color Color to shift from.
/// \param new_color Color to shift to.
void shift_color_all(uint32_t old_color, uint32_t new_color);

/// Retrieves the current time from the RTC circuit
void tick();

/// Saves the current lit LedArrays for dimming.
inline void save_last_leds();

/// Calculates the next LedArrays for brightening.
void calculate_next_leds();

/// Brightens and dims calculated next and previous leds.
void display_time();

/// Action function for color button single click.
void on_color_button_pressed();

/// Action function for color button double click.
void on_color_button_double_pressed();

/// Action function for time button single click.
void on_time_button_pressed();

/// Action function for time button double click.
void on_time_button_double_pressed();

/// Color button ISR function.
void color_isr();

/// Time button ISR function.
void time_isr();

/// Returns the time being night status.
/// \return \c true if it is night now (between 10pm and 8am), \c false otherwise.
bool is_night();

/// Checks whether it is night or not and adjusts brightness accordingly.
void adjust_brightness();

/// Delays execution a short amount of time.
void delay_short();

#endif //WORDCLOCK_CONSTS_H
