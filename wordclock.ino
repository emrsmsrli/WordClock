#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#define ADDRESS_DS1307          0x68
#define ADDRESS_EEPROM_COLOR    0x0
#define PIN_TIME_BUTTON         PIN3    // FIXME tick 2
#define PIN_COLOR_BUTTON        PIN2    // FIXME color 3?
#define PIN_NEOPIXELS           PIN4

#define N_PIXELS                116
#define N_COLORS                7
#define N_SECONDS_LED           5

#define COLOR_SHIFT_DELAY_US    400
#define TIME_CHANGE_DELAY_US    800
#define BUTTON_REPEAT_ALLOW_MS  500

#define ZERO                    0x0     // workaround for issue #527
#define UNUSED_LED_FOR_25       89

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(N_PIXELS, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);

struct time_s {
    uint8_t s;
    uint8_t m;
    uint8_t h;
} now;

class led {
    uint8_t start;
    uint8_t end;

public:
    led(uint8_t s, uint8_t e) {
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

    bool operator==(const led& other) {
        return start == other.start && end == other.end;
    }

    bool operator!=(const led& other) {
        return start != other.start || end != other.end;
    }
};

led IT(105, 106);
led IS(108, 109);
led O_OCLOCK(6, 11);
led O_PAST(61, 64);
led O_TO(72, 73);

led M_5(90, 93);
led M_10(75, 77);
led M_15(97, 103);
led M_20(83, 88);
led M_25(83, 93);
led M_30(79, 82);

led H[] = {
    led(58, 60),    // H_1
    led(55, 57),    // H_2
    led(50, 54),    // H_3
    led(39, 42),    // H_4
    led(43, 46),    // H_5
    led(47, 49),    // H_6
    led(66, 70),    // H_7
    led(17, 21),    // H_8
    led(35, 38),    // H_9
    led(14, 16),    // H_10
    led(22, 27),    // H_11
    led(28, 33)     // H_12
};

uint8_t seconds_led = 0;
led minute_led = M_5;
led hour_led = H[0];
led oclock_led = O_OCLOCK;

uint8_t last_second_led = seconds_led;
led last_minute_led = minute_led;
led last_hour_led = hour_led;
led last_oclock_led = oclock_led;

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

unsigned long min_time_button_wait_ms = 0;
unsigned long min_color_button_wait_ms = 0;

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
    now.s = bcd2dec(Wire.read());
    now.m = bcd2dec(Wire.read());
    now.h = bcd2dec(Wire.read() & 0b111111);
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

void on_time_button_pressed() {
    unsigned long now_ms = millis();
    if(now_ms - min_time_button_wait_ms < BUTTON_REPEAT_ALLOW_MS)
        return;
    min_time_button_wait_ms = now_ms;

    uint8_t h = now.h;
    uint8_t m = now.m + 1;

    if(m >= 60) {
        h += 1;
        m -= 60;
    }

    if(h >= 24) {
        h -= 24;
    }

    Wire.beginTransmission(ADDRESS_DS1307);
    Wire.write(ZERO); //stop oscillator

    Wire.write(dec2Bcd(0));
    Wire.write(dec2Bcd(m));
    Wire.write(dec2Bcd(h));
    Wire.write(dec2Bcd(1));
    Wire.write(dec2Bcd(1));
    Wire.write(dec2Bcd(1));
    Wire.write(dec2Bcd(0));

    Wire.write(ZERO); //start oscillator
    Wire.endTransmission();
}

void on_color_button_pressed() {
    unsigned long now_ms = millis();
    if(now_ms - min_color_button_wait_ms < BUTTON_REPEAT_ALLOW_MS)
        return;
    min_color_button_wait_ms = now_ms;

    set_brightness(dim);

    led_color_idx = (led_color_idx + 1) % N_COLORS;
    EEPROM.update(ADDRESS_EEPROM_COLOR, led_color_idx);

    set_brightness(bright);
}

inline void save_last_leds() {
    last_second_led = seconds_led;
    last_minute_led = minute_led;
    last_hour_led = hour_led;
    last_oclock_led = oclock_led;
}

void calculate_next_leds() {
    uint8_t hour = now.h;
    uint8_t min = now.m;
    uint8_t sec = now.s;

    seconds_led = sec % N_SECONDS_LED;

    if(min < 5) {                           //??:00 --> ??:04
        oclock_led = O_OCLOCK;
    } else if(min < 35) {                   //??:05 --> ??:34
        oclock_led = O_PAST;
        if(min >= 30) {                     //??:30 --> ??:34
            minute_led = M_30;
        } else if(min >= 15 && min < 20) {  //??:15 --> ??:19
            minute_led = M_15;
        } else {
            if(min >= 20) {                 //??:20 --> ??:29
                minute_led = M_20;
                if(min >= 25) {             //??:25 --> ??:29
                    minute_led = M_25;
                }
            } else if(min >= 10) {          //??:10 --> ??:14
                minute_led = M_10;
            } else {                        //??:05 --> ??:09
                minute_led = M_5;
            }
        }
    } else {                                //??:35 --> ??:59
        hour = hour + 1;
        oclock_led = O_TO;
        if(min >= 45 && min < 50) {         //??:45 --> ??:49
            minute_led = M_15;
        } else {
            if(min < 45) {                  //??:35 --> ??:44
                minute_led = M_20;
                if(min < 40) {              //??:35 --> ??:39
                    minute_led = M_25;
                }
            } else if(min < 55) {           //??:50 --> ??:54
                minute_led = M_10;
            } else {                        //??:55 --> ??:59
                minute_led = M_5;
            }
        }
    }

    while(hour >= 12)
        hour -= 12;
    hour_led = H[hour];
}

void display_time() {
    for(uint16_t brightness = 0; brightness <= 255; brightness++) {
        uint32_t darken = set_pixel_brightness(255 - brightness);
        uint32_t brighten = set_pixel_brightness(brightness);

        if(last_second_led != seconds_led) {
            pixels.setPixelColor(last_second_led + 1, darken);
            pixels.setPixelColor(seconds_led + 1, brighten);
        }
        if(last_minute_led != minute_led) {
            if(last_minute_led == M_20 && minute_led == M_25) {
                M_5.paint(brighten);
            } else if(last_minute_led == M_25 && minute_led == M_20) {
                M_5.paint(darken);
            } else {
                last_minute_led.paint(darken);
                minute_led.paint(brighten);
            }
        }
        if(last_hour_led != hour_led) {
            last_hour_led.paint(darken);
            hour_led.paint(brighten);
        }
        if(last_oclock_led != oclock_led) {
            last_oclock_led.paint(darken);
            oclock_led.paint(brighten);
        }

        pixels.show();
        delayMicroseconds(TIME_CHANGE_DELAY_US);
    }
}

void setup() {
    Wire.begin();

    pinMode(PIN_TIME_BUTTON, INPUT);
    pinMode(PIN_COLOR_BUTTON, INPUT);

    min_color_button_wait_ms = millis();
    min_time_button_wait_ms = min_color_button_wait_ms;

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

    if(digitalRead(PIN_COLOR_BUTTON))
        on_color_button_pressed();
    if(digitalRead(PIN_TIME_BUTTON))
        on_time_button_pressed();
}