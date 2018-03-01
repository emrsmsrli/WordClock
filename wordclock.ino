#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include "include/consts.h"

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(N_PIXELS, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);

struct {
    uint8_t s;
    uint8_t m;
    uint8_t h;
    uint8_t dd;
    uint8_t mm;
    uint8_t yy;
} time;

class LedArray {
    uint8_t start;
    uint8_t end;

public:
    LedArray(uint8_t s, uint8_t e) {
        start = s;
        end = e;
    }

    void paint(uint32_t color) {
        for(uint8_t i = start; i <= end; ++i) {
            if(i == UNUSED_LED_FOR_25)
                continue;
            pixels.setPixelColor(i, color);
        }
    }

    bool operator==(const LedArray& other) {
        return start == other.start && end == other.end;
    }

    bool operator!=(const LedArray& other) {
        return start != other.start || end != other.end;
    }
};

class Button {
    enum State {
        RELEASED = 0,
        PRESSED,
        PRESSED_SINGLE,
        PRESSED_DOUBLE,
        PRESSED_LONG
    };

    uint8_t pin;
    uint8_t state;
    volatile unsigned long start_time;
    volatile unsigned long stop_time;
    volatile bool single_pressed;
    volatile bool double_pressed;
    volatile bool long_pressed;

    void(*single_pressed_action)();
    void(*double_pressed_action)();

public:
    Button(uint8_t _pin, void(*_isr)(), void(*_single)(), void(*_double)() = NULL) {
        pin = _pin;
        state = RELEASED;
        start_time = 0;
        stop_time = 0;
        single_pressed = false;
        double_pressed = false;
        long_pressed = false;
        single_pressed_action = _single;
        double_pressed_action = _double;
        pinMode(pin, INPUT);
        attachInterrupt((uint8_t) digitalPinToInterrupt(pin), _isr, FALLING);
    }

    void perform_clicks() {
        noInterrupts();
        update();
        if(single_pressed) {
            single_pressed = false;
            interrupts();
            if(single_pressed_action) single_pressed_action();
        } else if(double_pressed) {
            double_pressed = false;
            interrupts();
            if(double_pressed_action) double_pressed_action();
        }
    }

    bool isLongPressed() {
        return long_pressed;
    }

    void update() {
        unsigned long now = millis();
        int32_t button_pressed = digitalRead(pin);

        switch(state) {
            case RELEASED:
                if(button_pressed) {
                    if(single_pressed || double_pressed)
                        break;
                    state = PRESSED;
                    start_time = now;
                }
                break;
            case PRESSED:
                if(!button_pressed && now - start_time < BTN_DEBOUNCE_THRESHOLD) {
                    state = RELEASED;
                } else if(!button_pressed) {
                    state = PRESSED_SINGLE;
                    stop_time = now;
                } else if(now - start_time > BTN_PRESS_L_THRESHOLD) {
                    long_pressed = true;
                    state = PRESSED_LONG;
                    stop_time = now;
                }
                break;
            case PRESSED_SINGLE:
                if(now - start_time > BTN_PRESS_THRESHOLD) {
                    single_pressed = true;
                    state = RELEASED;
                } else if(button_pressed && now - stop_time > BTN_DEBOUNCE_THRESHOLD) {
                    state = PRESSED_DOUBLE;
                    start_time = now;
                }
                break;
            case PRESSED_DOUBLE:
                if(!button_pressed && now - start_time > BTN_DEBOUNCE_THRESHOLD) {
                    single_pressed = false;
                    double_pressed = true;
                    state = RELEASED;
                }
                break;
            case PRESSED_LONG:
                if(!button_pressed) {
                    long_pressed = false;
                    state = RELEASED;
                }
            default:
                break;
        }
    }
};

Button *TIME_BUTTON;
Button *COLOR_BUTTON;

LedArray IT(105, 106);
LedArray IS(108, 109);
LedArray O_OCLOCK(6, 11);
LedArray O_PAST(61, 64);
LedArray O_TO(72, 73);

LedArray M_NONE(0, 0);
LedArray M_5(90, 93);
LedArray M_10(75, 77);
LedArray M_15(97, 103);
LedArray M_20(83, 88);
LedArray M_25(83, 93);
LedArray M_30(79, 82);

LedArray H[] = {
        LedArray(58, 60),    // H_1
        LedArray(55, 57),    // H_2
        LedArray(50, 54),    // H_3
        LedArray(39, 42),    // H_4
        LedArray(43, 46),    // H_5
        LedArray(47, 49),    // H_6
        LedArray(66, 70),    // H_7
        LedArray(17, 21),    // H_8
        LedArray(35, 38),    // H_9
        LedArray(14, 16),    // H_10
        LedArray(22, 27),    // H_11
        LedArray(28, 33)     // H_12
};

uint8_t seconds_led = 1;
LedArray minute_led = M_NONE;
LedArray hour_led = H[0];
LedArray oclock_led = O_OCLOCK;

uint8_t last_second_led = seconds_led;
LedArray last_minute_led = minute_led;
LedArray last_hour_led = hour_led;
LedArray last_oclock_led = oclock_led;

uint8_t led_color_idx = 0;
uint32_t colors[] = {
        pixels.Color(247, 139, 15),     // orange
        // pixels.Color(71,  5,   20),     // claret dont forget to increment N_COLORS
        pixels.Color(127, 127, 0),      // yellow
        pixels.Color(127, 0,   127),    // magenta
        pixels.Color(0,   127, 127),    // cyan
        pixels.Color(0,   0,   255),    // blue
        pixels.Color(0,   255, 0),      // green
        pixels.Color(255, 0,   0)       // red
};

uint8_t heart[] = {11, 21, 23, 31, 35, 41, 47, 51, 59, 61, 71, 72, 77, 82, 84, 87, 89, 92};

uint32_t current_color() {
    return colors[led_color_idx];
}

int dec2bcd(uint8_t val) {
    return val / 10 * 16 + val % 10;
}

uint8_t bcd2dec(int val) {
    return (uint8_t) (val / 16 * 10 + val % 16);
}

uint8_t smooth_step(uint8_t i, uint8_t N, uint8_t min, uint8_t max) {
    float v = SMOOTH_STEP(i / (float) N);
    return (uint8_t) ((min * v) + (max * (1 - v)));
}

color extract_color(uint32_t c) {
    color clr;
    clr.r = c >> 16;
    clr.g = c >> 8 & 0xFF;
    clr.b = c & 0xFF;
    return clr;
}

uint32_t shift_color(uint8_t i, uint8_t N, uint32_t old_color, uint32_t new_color) {
    color c_old = extract_color(old_color);
    color c_new = extract_color(new_color);
    return pixels.Color(smooth_step(i, N, c_old.r, c_new.r),
                        smooth_step(i, N, c_old.g, c_new.g),
                        smooth_step(i, N, c_old.b, c_new.b));
}

void shift_color_all(uint32_t old_color, uint32_t new_color) {
    ANIMATE(i, ANIMATION_TIME_MS) {
        uint32_t b = shift_color(i, ANIMATION_TIME_MS, old_color, new_color);

        IT.paint(b);
        IS.paint(b);
        pixels.setPixelColor(seconds_led, b);
        minute_led.paint(b);
        hour_led.paint(b);
        oclock_led.paint(b);

        pixels.show();
        delayMicroseconds(1000);
    }
}

void tick() {
    Wire.beginTransmission(ADDRESS_DS1307);
    Wire.write(ZERO);
    Wire.endTransmission();

    Wire.requestFrom(ADDRESS_DS1307, 7);
    time.s = bcd2dec(Wire.read() & 0x7F);
    time.m = bcd2dec(Wire.read());
    time.h = bcd2dec(Wire.read() & 0x3F);
    Wire.read();
    time.dd = bcd2dec(Wire.read());
    time.mm = bcd2dec(Wire.read());
    time.yy = bcd2dec(Wire.read());
}

void write_time(uint8_t m, uint8_t h) {
    Wire.beginTransmission(ADDRESS_DS1307);
    Wire.write(ZERO);

    Wire.write(dec2bcd(0));
    Wire.write(dec2bcd(m));
    Wire.write(dec2bcd(h));
    Wire.write(dec2bcd(1));
    Wire.write(dec2bcd(time.dd));
    Wire.write(dec2bcd(time.mm));
    Wire.write(dec2bcd(time.yy));

    Wire.endTransmission();
}

inline void save_last_leds() {
    last_second_led = seconds_led;
    last_minute_led = minute_led;
    last_hour_led = hour_led;
    last_oclock_led = oclock_led;
}

void calculate_next_leds() {
    uint8_t hour = time.h;
    uint8_t min = time.m;
    uint8_t sec = time.s;

    seconds_led = sec % N_SECONDS_LED + 1;

    if(min < 5) {                   /// 0 - 5
        oclock_led = O_OCLOCK;
        minute_led = M_NONE;
    } else if(min < 35) {           /// 5 - 35
        oclock_led = O_PAST;
        if(min < 10) {              // 5 - 10
            minute_led = M_5;
        } else if(min < 15) {       // 10 - 15
            minute_led = M_10;
        } else if(min < 20) {       // 15 - 20
            minute_led = M_15;
        } else if(min < 25) {       // 20 - 25
            minute_led = M_20;
        } else if(min < 30) {       // 25 - 30
            minute_led = M_25;
        } else {                    // 30 - 35
            minute_led = M_30;
        }
    } else {                        /// 35 - 60
        hour = hour + 1;
        oclock_led = O_TO;
        if(min < 40) {              // 35 - 40
            minute_led = M_25;
        } else if(min < 45) {       // 40 - 45
            minute_led = M_20;
        } else if(min < 50) {       // 45 - 50
            minute_led = M_15;
        } else if(min < 55) {       // 50 - 55
            minute_led = M_10;
        } else {                    // 55 - 60
            minute_led = M_5;
        }
    }

    while(hour >= 12)
        hour -= 12;
    hour_led = H[hour];
}

void display_time() {
    bool no_led_changed = true;
    uint32_t color = current_color();
    ANIMATE(i, ANIMATION_TIME_MS) {
        uint32_t to_black = shift_color(i, ANIMATION_TIME_MS, color, COLOR_BLACK);
        uint32_t to_color = shift_color(i, ANIMATION_TIME_MS, COLOR_BLACK, color);

        if(last_second_led != seconds_led) {
            pixels.setPixelColor(last_second_led, to_black);
            pixels.setPixelColor(seconds_led, to_color);
            no_led_changed = false;
        }
        if(last_minute_led != minute_led) {
            if(last_minute_led == M_20 && minute_led == M_25) {
                M_5.paint(to_color);
            } else if(last_minute_led == M_25 && minute_led == M_20) {
                M_5.paint(to_black);
            } else {
                if(last_minute_led != M_NONE)
                    last_minute_led.paint(to_black);
                if(minute_led != M_NONE)
                    minute_led.paint(to_color);
            }
            no_led_changed = false;
        }
        if(last_hour_led != hour_led) {
            last_hour_led.paint(to_black);
            hour_led.paint(to_color);
            no_led_changed = false;
        }
        if(last_oclock_led != oclock_led) {
            last_oclock_led.paint(to_black);
            oclock_led.paint(to_color);
            no_led_changed = false;
        }

        if(no_led_changed)
            break;

        pixels.show();
        delayMicroseconds(1000);
    }
}

class Birthday {
    static void shift_color_heart(uint32_t old_color, uint32_t new_color, uint16_t dly) {
        ANIMATE(i, ANIMATION_TIME_MS) {
            uint32_t shift = shift_color(i, ANIMATION_TIME_MS, old_color, new_color);
            for(uint8_t h_l = 0; h_l < 18; ++h_l)
                pixels.setPixelColor(heart[h_l], shift);
            pixels.show();
            delayMicroseconds(1000);
        }
        delay(dly);
    }

    static void play_note(uint16_t tone, uint16_t duration) {
        for(long i = 0; i < duration * 1000L; i += tone * 2) {
            digitalWrite(PIN_SPEAKER, HIGH);
            delayMicroseconds(tone);
            digitalWrite(PIN_SPEAKER, LOW);
            delayMicroseconds(tone);
        }
    }

    static void play_happy_birthday() {
        uint16_t notes[] = {NOTE_G, NOTE_G, NOTE_A, NOTE_G, NOTE_c, NOTE_B, NOTE_REST,
                            NOTE_G, NOTE_G, NOTE_A, NOTE_G, NOTE_d, NOTE_c, NOTE_REST,
                            NOTE_G, NOTE_G, NOTE_x, NOTE_e, NOTE_c, NOTE_B, NOTE_A, NOTE_REST,
                            NOTE_y, NOTE_y, NOTE_e, NOTE_c, NOTE_d, NOTE_c};
        uint16_t beats[] = {2, 2, 8, 8, 8, 16, 1, 2, 2, 8, 8, 8, 16, 1,
                            2, 2, 8, 8, 8 ,8, 16, 1, 2, 2, 8, 8, 8, 16};
        uint16_t spee_mult = SONG_TEMPO / SPEE;
        for (int i = 0; i < 28; i++) {
            if (!notes[i]) delay(beats[i] * SONG_TEMPO);
            else play_note(notes[i], beats[i] * spee_mult);
            delay(SONG_TEMPO);
        }
    }

    static bool cancel() {
        if(cancelled) {
            begun = false;
            cancelled = false;
            return true;
        }
        return false;
    }

public:
    static bool begun;
    static volatile bool cancelled;
    static volatile bool manual_begin;

    static void begin() {
        begun = false;
        cancelled = false;
        manual_begin = false;
        pinMode(PIN_SPEAKER, OUTPUT);
    }

    static bool is_today() {
        return (time.dd == 4 && time.mm == 11
                && (time.h == 8 || time.h == 12 || time.h == 17)
                && time.m == 0 && time.s == 0) || manual_begin;
    }

    static void celebrate() {
        begun = true;
        shift_color_all(current_color(), COLOR_BLACK);
        shift_color_heart(COLOR_BLACK, COLOR_RED, 500);
        play_happy_birthday();
        for(uint16_t i = 0; i < 10800; ++i) {
            delay(1000);
            if(cancel()) {
                shift_color_heart(COLOR_RED, COLOR_BLACK, 0);
                break;
            }
        }
        manual_begin = false;
        begun = false;
    }
};

bool Birthday::begun;
volatile bool Birthday::cancelled;
volatile bool Birthday::manual_begin;

void on_color_button_pressed() {
    uint8_t old_led_color_idx = led_color_idx;

    led_color_idx = (led_color_idx + 1) % N_COLORS;
    EEPROM.update(ADDRESS_EEPROM_COLOR, led_color_idx);

    shift_color_all(colors[old_led_color_idx], current_color());
}

void on_color_button_double_pressed() {
    if(!Birthday::begun) {
        Birthday::manual_begin = true;
    }
}

void on_time_button_pressed() {
    uint8_t h = time.h;
    uint8_t m = time.m + 1;

    if(m == 60) {
        m = 0;
        if(++h == 24) {
            h = 0;
        }
    }

    write_time(m, h);
}

void on_time_button_double_pressed() {
    uint8_t h = time.h + 1;
    uint8_t m = time.m;

    if(h == 24) {
        h = 0;
    }

    write_time(m, h);
}

void color_isr() {
    if(Birthday::begun && !Birthday::cancelled)
        Birthday::cancelled = true;
    else
        COLOR_BUTTON->update();
}

void time_isr() {
    if(Birthday::begun && !Birthday::cancelled)
        Birthday::cancelled = true;
    else
        TIME_BUTTON->update();
}

void setup() {
    Wire.begin();
    pixels.begin();
    Birthday::begin();

    pixels.show();

    COLOR_BUTTON = new Button(PIN_COLOR_BUTTON, color_isr, on_color_button_pressed, on_color_button_double_pressed);
    TIME_BUTTON = new Button(PIN_TIME_BUTTON, time_isr, on_time_button_pressed, on_time_button_double_pressed);

    led_color_idx = EEPROM.read(ADDRESS_EEPROM_COLOR);
    if(led_color_idx > (N_COLORS - 1))
        led_color_idx = 0;

    tick();
    calculate_next_leds();
    shift_color_all(COLOR_BLACK, current_color());
}

void loop() {
    tick();

    if(Birthday::is_today()) {
        Birthday::celebrate();
        tick();
        calculate_next_leds();
        shift_color_all(COLOR_BLACK, current_color());
        return;
    }

    save_last_leds();
    calculate_next_leds();
    display_time();

    COLOR_BUTTON->perform_clicks();
    TIME_BUTTON->perform_clicks();

    if(TIME_BUTTON->isLongPressed()) {
        // TODO implement digital clock mode
    }
}