#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#define ZERO                    0x0     // workaround for issue #527
#define UNUSED_LED_FOR_25       89

#define ADDRESS_DS1307          0x68
#define ADDRESS_EEPROM_COLOR    ZERO
#define PIN_TIME_BUTTON         PIN3    // FIXME time 2
#define PIN_COLOR_BUTTON        PIN2    // FIXME color 3?
#define PIN_NEOPIXELS           PIN4

#define N_PIXELS                116
#define N_COLORS                7
#define N_SECONDS_LED           5

#define COLOR_SHIFT_DELAY_US    400
#define TIME_CHANGE_DELAY_US    800

#define BTN_STATE_PRESSED       HIGH
#define BTN_STATE_RELEASED      LOW
#define BTN_DEBOUNCE_THRESHOLD  50
#define BTN_CLICK_THRESHOLD     300

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(N_PIXELS, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);

class Time {
public:
    static uint8_t s;
    static uint8_t m;
    static uint8_t h;
};

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
        WAIT_PRESS = 0,
        WAIT_RELEASE,
        CLICKED_SINGLE,
        CLICKED_DOUBLE
    };

    uint8_t pin;
    uint8_t state;
    volatile unsigned long start_time;
    volatile unsigned long stop_time;
    volatile bool single_clicked;
    volatile bool double_clicked;

    void(*single_click_action)();
    void(*double_click_action)();

public:
    Button(uint8_t _pin, void(*_isr)(), void(*_single)(), void(*_double)() = nullptr) {
        pin = _pin;
        state = WAIT_PRESS;
        start_time = 0;
        stop_time = 0;
        single_clicked = false;
        double_clicked = false;
        single_click_action = _single;
        double_click_action = _double;
        pinMode(pin, INPUT);
        attachInterrupt((uint8_t) digitalPinToInterrupt(pin), _isr, FALLING);
    }

    void perform_clicks() {
        noInterrupts();
        if(single_clicked) {
            single_clicked = false;
            interrupts();
            if(single_click_action) single_click_action();
        } else if(double_clicked) {
            double_clicked = false;
            interrupts();
            if(double_click_action) double_click_action();
        }
    }

    void tick() {
        unsigned long now = millis();
        int32_t button_state = digitalRead(pin);

        switch(state) {
            case WAIT_PRESS:
                if(button_state == BTN_STATE_PRESSED) {
                    if(single_clicked || double_clicked)
                        break;
                    state = WAIT_RELEASE;
                    start_time = now;
                }
                break;
            case WAIT_RELEASE:
                if(button_state == BTN_STATE_RELEASED && now - start_time < BTN_DEBOUNCE_THRESHOLD) {
                    state = WAIT_PRESS;
                } else if(button_state == BTN_STATE_RELEASED) {
                    state = CLICKED_SINGLE;
                    stop_time = now;
                }
                break;
            case CLICKED_SINGLE:
                if(now - start_time > BTN_CLICK_THRESHOLD) {
                    single_clicked = true;
                    state = WAIT_PRESS;
                } else if(button_state == BTN_STATE_PRESSED && now - stop_time > BTN_DEBOUNCE_THRESHOLD) {
                    state = CLICKED_DOUBLE;
                    start_time = now;
                }
                break;
            case CLICKED_DOUBLE:
                if(button_state == BTN_STATE_RELEASED && now - start_time > BTN_DEBOUNCE_THRESHOLD) {
                    single_clicked = false;
                    double_clicked = true;
                    state = WAIT_PRESS;
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

uint8_t seconds_led = 0;
LedArray minute_led = M_5;
LedArray hour_led = H[0];
LedArray oclock_led = O_OCLOCK;

uint8_t last_second_led = seconds_led;
LedArray last_minute_led = minute_led;
LedArray last_hour_led = hour_led;
LedArray last_oclock_led = oclock_led;

uint8_t led_color_idx = 0;
uint32_t colors[] = {
    pixels.Color(247, 139, 15),     // orange
    // pixels.Color(71,  5,   20),     // claret

    pixels.Color(127, 127, 0),      // yellow
    pixels.Color(127, 0,   127),    // magenta
    pixels.Color(0,   127, 127),    // cyan
    pixels.Color(0,   0,   255),    // blue
    pixels.Color(0,   255, 0),      // green
    pixels.Color(255, 0,   0)       // red
};

int dec2Bcd(uint8_t val) {
    return (val / 10 * 16) + (val % 10);
}

uint8_t bcd2dec(int val) {
    return (uint8_t) (val / 16 * 10) + (val % 16);
}

void tick() {
    Wire.beginTransmission(ADDRESS_DS1307);
    Wire.write(ZERO);
    Wire.endTransmission();

    Wire.requestFrom(ADDRESS_DS1307, 7);
    Time::s = bcd2dec(Wire.read());
    Time::m = bcd2dec(Wire.read());
    Time::h = bcd2dec(Wire.read() & 0b111111);
    Wire.read();
    Wire.read();
    Wire.read();
    Wire.read();
}

uint32_t set_pixel_brightness(uint8_t brightness) {
    uint32_t color = colors[led_color_idx];
    float b = brightness / (float) 255;
    return pixels.Color(
            (uint8_t) ((color >> 16) * b),
            (uint8_t) ((color >> 8 & 0xFF) * b),
            (uint8_t) ((color & 0xFF) * b));
}

inline uint8_t bright(uint16_t b) {
    return (uint8_t) b;
}

inline uint8_t dim(uint16_t b) {
    return 255 - (uint8_t) b;
}

void set_brightness(uint8_t (*setting)(uint16_t)) {
    for(uint16_t brightness = 0; brightness <= 255; brightness++) {
        uint32_t b = set_pixel_brightness(setting(brightness));

        IT.paint(b);
        IS.paint(b);
        pixels.setPixelColor(seconds_led + 1, b);
        minute_led.paint(b);
        hour_led.paint(b);
        oclock_led.paint(b);

        pixels.show();
        delayMicroseconds(COLOR_SHIFT_DELAY_US);
    }
}

void on_color_button_pressed() {
    set_brightness(dim);

    led_color_idx = (led_color_idx + 1) % N_COLORS;
    EEPROM.update(ADDRESS_EEPROM_COLOR, led_color_idx);

    set_brightness(bright);
}

void write_time(uint8_t m, uint8_t h) {
    Wire.beginTransmission(ADDRESS_DS1307);
    Wire.write(ZERO);       // stop oscillator

    Wire.write(dec2Bcd(0));
    Wire.write(dec2Bcd(m));
    Wire.write(dec2Bcd(h));
    Wire.write(dec2Bcd(1));
    Wire.write(dec2Bcd(1));
    Wire.write(dec2Bcd(1));
    Wire.write(dec2Bcd(0));

    Wire.write(ZERO);       // start oscillator
    Wire.endTransmission();
}

void on_time_button_pressed() {
    uint8_t h = Time::h;
    uint8_t m = Time::m + 1;

    if(m == 60) {
        h += 1;
        m -= 60;
    }

    if(h == 24) {
        h -= 24;
    }

    write_time(m, h);
}

void on_time_button_double_pressed() {
    uint8_t h = Time::h + 1;
    uint8_t m = Time::m;

    if(h == 24) {
        h -= 24;
    }

    write_time(m, h);
}

inline void save_last_leds() {
    last_second_led = seconds_led;
    last_minute_led = minute_led;
    last_hour_led = hour_led;
    last_oclock_led = oclock_led;
}

void calculate_next_leds() {
    uint8_t hour = Time::h;
    uint8_t min = Time::m;
    uint8_t sec = Time::s;

    seconds_led = sec % N_SECONDS_LED;  // should this be 12-by-1 timer or 5-by-1?

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
    for(uint16_t brightness = 0; brightness <= 255; brightness++) {
        uint32_t darken = set_pixel_brightness(255 - brightness);
        uint32_t brighten = set_pixel_brightness(brightness);

        if(last_second_led != seconds_led) {
            pixels.setPixelColor(last_second_led + 1, darken);
            pixels.setPixelColor(seconds_led + 1, brighten);
            no_led_changed = false;
        }
        if(last_minute_led != minute_led) {
            if(last_minute_led == M_20 && minute_led == M_25) {
                M_5.paint(brighten);
            } else if(last_minute_led == M_25 && minute_led == M_20) {
                M_5.paint(darken);
            } else {
                if(last_minute_led != M_NONE)
                    last_minute_led.paint(darken);
                if(minute_led != M_NONE)
                    minute_led.paint(brighten);
            }
            no_led_changed = false;
        }
        if(last_hour_led != hour_led) {
            last_hour_led.paint(darken);
            hour_led.paint(brighten);
            no_led_changed = false;
        }
        if(last_oclock_led != oclock_led) {
            last_oclock_led.paint(darken);
            oclock_led.paint(brighten);
            no_led_changed = false;
        }

        if(no_led_changed)
            break;

        pixels.show();
        delayMicroseconds(TIME_CHANGE_DELAY_US);
    }
}

void setup() {
    Wire.begin();

    COLOR_BUTTON = new Button(PIN_COLOR_BUTTON, COLOR_BUTTON->tick, on_color_button_pressed);
    TIME_BUTTON = new Button(PIN_TIME_BUTTON, TIME_BUTTON->tick, on_time_button_pressed, on_time_button_double_pressed);

    // Read color if stored in EEPROM
    led_color_idx = EEPROM.read(ADDRESS_EEPROM_COLOR);
    if(led_color_idx > (N_COLORS - 1))
        led_color_idx = 0;

    pixels.begin();
    pixels.clear();

    tick();
    calculate_next_leds();
    set_brightness(bright);
}

void loop() {
    tick();
    save_last_leds();
    calculate_next_leds();
    display_time();

    COLOR_BUTTON->perform_clicks();
    TIME_BUTTON->perform_clicks();
}