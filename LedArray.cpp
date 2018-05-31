#include "LedArray.hpp"

LedArray::LedArray(uint8_t s, uint8_t e) {
    start = s;
    end = e;
}

void LedArray::paint(uint32_t color) {
    for(uint8_t i = start; i < end; ++i) {
        if(i == UNUSED_LED_FOR_25)
            continue;
        pixels.setPixelColor(i, color);
    }
}