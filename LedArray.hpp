
#ifndef WORDCLOCK_LEDARRAY_H
#define WORDCLOCK_LEDARRAY_H

#define UNUSED_LED_FOR_25       89
#define N_PIXELS                116
#define PIN_NEOPIXELS           PIN4

#define COLOR_BLACK             static_cast<uint32_t>(0x000000)
#define COLOR_RED               static_cast<uint32_t>(0xFF0000)

#include <Adafruit_NeoPixel/Adafruit_NeoPixel.h>

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(N_PIXELS, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);

/// LedArray class represents an array of leds. It has a start and an end index in the matrix.
class LedArray {
    uint8_t start;
    uint8_t end;

public:
    LedArray(uint8_t s, uint8_t e);

    /// Paints the led array to the specified color.
    /// \param color Color to be painted.
    void paint(uint32_t color);

    bool operator==(const LedArray& other) {
        return start == other.start && end == other.end;
    }

    bool operator!=(const LedArray& other) {
        return start != other.start || end != other.end;
    }
};

#endif //WORDCLOCK_LEDARRAY_H
