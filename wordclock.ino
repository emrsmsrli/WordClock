#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#define ADDRESS_DS1307          0x68
#define ADDRESS_EEPROM_COLOR    0x0
#define PIN_TIME_BUTTON         PIN3    // FIXME tick 2
#define PIN_COLOR_BUTTON        PIN2    // FIXME color 3?
#define PIN_NEOPIXELS           PIN4

#define N_PIXELS                115
#define N_COLORS                7
#define N_SECONDS_LED           5

#define ZERO                    0x0     // workaround for issue #527
#define UNUSED_TWENTY_FIVE_LED  88

struct time_s {
    uint8_t s;
    uint8_t m;
    uint8_t h;
};

struct time_s now;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(N_PIXELS, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);

typedef struct led_s {
    uint8_t start;
    uint8_t end;

    led_s(uint8_t s, uint8_t e) {
        start = s;
        end = e;
    }

    void paint(uint32_t color) {
        for(uint8_t i = start; i <= end; ++i) {
            if(i == UNUSED_TWENTY_FIVE_LED)
                continue;
            pixels.setPixelColor(i, color);
        }
    }
} led;

led O_IT(104, 105);
led O_IS(107, 108);
led O_OCLOCK(5, 10);
led O_PAST(60, 63);
led O_TO(71, 72);

led M_5(89, 92);
led M_10(74, 76);
led M_15(96, 102);
led M_20(82, 87);
led M_25(82, 92);
led M_30(78, 81);

led H[] = {
    led(57, 59),    // H_1
    led(54, 56),    // H_2
    led(49, 53),    // H_3
    led(38, 41),    // H_4
    led(42, 45),    // H_5
    led(46, 48),    // H_6
    led(65, 69),    // H_7
    led(16, 20),    // H_8
    led(34, 37),    // H_9
    led(13, 15),    // H_10
    led(21, 26),    // H_11
    led(27, 32)     // H_12
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

void on_time_button_pressed() {
    tick();
    uint8_t h = now.h;
    uint8_t m = now.m + 1;

    if(m >= 60) {
        h += 1;
        m -= 60;
    }

    if(h >= 24)
        h -= 24;

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
    led_color_idx = (led_color_idx + 1) % N_COLORS;
    EEPROM.update(ADDRESS_EEPROM_COLOR, led_color_idx);
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

uint32_t set_pixel_brightness(uint8_t brightness) {
    float b = brightness / (float) 255;
    return pixels.Color(
            (uint8_t) ((colors[led_color_idx] >> 16) * b),
            (uint8_t) ((colors[led_color_idx] >> 8 & 0xFF) * b),
            (uint8_t) ((colors[led_color_idx] & 0xFF) * b));
}

inline bool equals(led lhs, led rhs) {
    return lhs.start == rhs.start && lhs.end == rhs.end;
}

void display_time() {
    O_IT.paint(colors[led_color_idx]);
    O_IS.paint(colors[led_color_idx]);

    for(uint16_t brightness = 0; brightness <= 255; brightness++) {
        uint32_t darken = set_pixel_brightness(255 - brightness);
        uint32_t brighten = set_pixel_brightness(brightness);

        if(last_second_led != seconds_led) {
            pixels.setPixelColor(last_second_led, darken);
            pixels.setPixelColor(seconds_led, brighten);
        }
        if(!equals(last_minute_led, minute_led)) {
            last_minute_led.paint(darken);
            minute_led.paint(brighten);
        }
        if(!equals(last_hour_led, hour_led)) {
            last_hour_led.paint(darken);
            hour_led.paint(brighten);
        }
        if(!equals(last_oclock_led, oclock_led)) {
            last_oclock_led.paint(darken);
            oclock_led.paint(brighten);
        }

        pixels.show();
        delayMicroseconds(700);
    }
}

void setup() {
    Wire.begin();

    pinMode(PIN_TIME_BUTTON, INPUT);
    pinMode(PIN_COLOR_BUTTON, INPUT);

    // Read color if stored in EEPROM
    led_color_idx = EEPROM.read(ADDRESS_EEPROM_COLOR);
    if(led_color_idx > (N_COLORS - 1))
        led_color_idx = 0;

    pixels.begin();
}


void loop() {
    tick();
    save_last_leds();
    calculate_next_leds();
    display_time();

    if(digitalRead(PIN_COLOR_BUTTON)) {
        on_color_button_pressed();
        delay(500);
    }
    if(digitalRead(PIN_TIME_BUTTON)) {
        on_time_button_pressed();
        delay(50);
    }
}