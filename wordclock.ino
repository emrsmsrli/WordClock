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
#define BLACK                   ZERO

enum oclock_e {
    O_OCLOCK = 0,
    O_PAST,
    O_TO
};

enum minute_e {
    M_5 = 0,
    M_10,
    M_15,
    M_20,
    M_25,
    M_30
};

enum hour_e {
    H_1 = 0,
    H_2,
    H_3,
    H_4,
    H_5,
    H_6,
    H_7,
    H_8,
    H_9,
    H_10,
    H_11,
    H_12
};

struct time_s {
    uint8_t s;
    uint8_t m;
    uint8_t h;
} now;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(N_PIXELS, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);


uint8_t oclock_led = 0;
uint8_t second_led = 0;
uint8_t minute_led = 0;
uint8_t hour_led = 0;

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

int decToBcd(uint8_t val) {
    return (val / 10 * 16) + (val % 10);
}

uint8_t bcdToDec(int val) {
    return (uint8_t) (val / 16 * 10) + (val % 16);
}

void tick() {
    Wire.beginTransmission(ADDRESS_DS1307);
    Wire.write(ZERO);
    Wire.endTransmission();

    Wire.requestFrom(ADDRESS_DS1307, 7);
    now.s = bcdToDec(Wire.read());
    now.m = bcdToDec(Wire.read());
    now.h = bcdToDec(Wire.read() & 0b111111);
    Wire.read();
    Wire.read();
    Wire.read();
    Wire.read();
}

void onTimeButtonPressed() {
    tick();
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

    Wire.write(decToBcd(0));
    Wire.write(decToBcd(m));
    Wire.write(decToBcd(h));
    Wire.write(decToBcd(1));
    Wire.write(decToBcd(1));
    Wire.write(decToBcd(1));
    Wire.write(decToBcd(0));

    Wire.write(ZERO); //start oscillator
    Wire.endTransmission();
}

void onColorButtonPressed() {
    led_color_idx = (led_color_idx + 1) % N_COLORS;
    EEPROM.update(ADDRESS_EEPROM_COLOR, led_color_idx);
}

void assignLeds() {
    uint8_t hour = now.h;
    uint8_t min = now.m;
    uint8_t sec = now.s;

    second_led = sec % N_SECONDS_LED;

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

    while(hour >= 12) {
        hour -= 12;
    }
    hour_led = hour;
}

void paintLeds(uint16_t lo, uint16_t hi, uint32_t color = colors[led_color_idx]) {
    for(uint16_t i = lo; i <= hi; ++i) {
        pixels.setPixelColor(i, color);
    }
}

void displayTime() {
    paintLeds(0, N_PIXELS - 1, BLACK);

    paintLeds(104, 105); // leds of IT IS
    paintLeds(107, 108); // are always on

    paintLeds(0, 103, BLACK);
    paintLeds(0, second_led); // move seconds led

    switch(oclock_led) {
        case O_OCLOCK:
            paintLeds(5, 10); break;
        case O_PAST:
            paintLeds(60, 63); break;
        case O_TO:
            paintLeds(71, 72); break;
        default: break;
    }

    switch(minute_led) {
        case M_25:
        case M_5:
            paintLeds(89, 92);
            if(minute_led == M_5)
                break;
        case M_20:
            paintLeds(82, 87); break;
        case M_10:
            paintLeds(74, 76); break;
        case M_15:
            paintLeds(96, 102); break;
        case M_30:
            paintLeds(78, 81); break;
        default: break;
    }

    switch(hour_led) {
        case H_1:
            paintLeds(57, 59); break;
        case H_2:
            paintLeds(54, 56); break;
        case H_3:
            paintLeds(49, 53); break;
        case H_4:
            paintLeds(38, 41); break;
        case H_5:
            paintLeds(42, 45); break;
        case H_6:
            paintLeds(46, 48); break;
        case H_7:
            paintLeds(65, 69); break;
        case H_8:
            paintLeds(16, 20); break;
        case H_9:
            paintLeds(34, 37); break;
        case H_10:
            paintLeds(13, 15); break;
        case H_11:
            paintLeds(21, 26); break;
        case H_12:
            paintLeds(27, 32); break;
        default: break;
    }

    pixels.show();
}

void setup() {
    Wire.begin();

    pinMode(PIN_TIME_BUTTON, INPUT);
    pinMode(PIN_COLOR_BUTTON, INPUT);

    // Read color if stored in EEPROM
    led_color_idx = EEPROM.read(ADDRESS_EEPROM_COLOR);
    if(led_color_idx > (N_COLORS - 1)) {
        led_color_idx = 0;
    }

    pixels.begin();
}


void loop() {
    tick();
    assignLeds();
    displayTime();

    if(digitalRead(PIN_COLOR_BUTTON)) {
        onColorButtonPressed();
        delay(500);
    }
    if(digitalRead(PIN_TIME_BUTTON)) {
        onTimeButtonPressed();
        delay(50);
    }
}