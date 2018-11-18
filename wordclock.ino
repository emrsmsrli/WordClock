
#include "wordclock.hpp"
#include "EEPROM.h"

RTC_DS1307 rtc;
DateTime time;
uint32_t total_accuracy_secs;
TimeSpan ONE_SEC(0, 0, 0, 1);
TimeSpan ONE_MIN(0, 0, 1, 0);
TimeSpan ONE_HOUR(0, 1, 0, 0);

Button *TIME_BUTTON;
Button *COLOR_BUTTON;

LedArray IT(105, 107);
LedArray IS(108, 110);
LedArray O_OCLOCK(6, 12);
LedArray O_PAST(61, 65);
LedArray O_TO(72, 74);

LedArray M_NONE(0, 0);
LedArray M_5(90, 94);
LedArray M_10(75, 78);
LedArray M_15(97, 104);
LedArray M_20(83, 89);
LedArray M_25(83, 94);
LedArray M_30(79, 83);

/// Array of LedArrays for hours.
LedArray H[] = {
        LedArray(28, 34)     // H_12
        LedArray(58, 61),    // H_1
        LedArray(55, 58),    // H_2
        LedArray(50, 55),    // H_3
        LedArray(39, 43),    // H_4
        LedArray(43, 47),    // H_5
        LedArray(47, 50),    // H_6
        LedArray(66, 71),    // H_7
        LedArray(17, 22),    // H_8
        LedArray(35, 39),    // H_9
        LedArray(14, 17),    // H_10
        LedArray(22, 28),    // H_11
};

uint8_t seconds_led = 1;
LedArray minute_led = M_NONE;
LedArray hour_led = H[0];
LedArray oclock_led = O_OCLOCK;

uint8_t last_second_led = seconds_led;
LedArray last_minute_led = minute_led;
LedArray last_hour_led = hour_led;
LedArray last_oclock_led = oclock_led;

/// The brightness multiplier variable.
uint8_t brightness = BRIGHTNESS_HIGH;

/// Index of the current color of the leds.
uint8_t led_color_idx = 0;

/// Array of colors. Change \c N_COLORS in wordclock.h if
/// this array changes.
uint32_t colors[] = {
        pixels.Color(247, 139, 15),     // orange
        pixels.Color(127, 127, 0),      // yellow
        pixels.Color(127, 0, 127),    // magenta
        pixels.Color(0, 127, 127),    // cyan
        pixels.Color(0, 0, 255),    // blue
        pixels.Color(0, 255, 0),      // green
        pixels.Color(255, 0, 0)       // red
};

void delay_short() {
    delayMicroseconds(DELAY_SHORT_US);
}

uint32_t current_color() {
    float brightness_mult = brightness / static_cast<float>(BRIGHTNESS_HIGH);
    color c = extract_color(colors[led_color_idx]);

    return pixels.Color(
            static_cast<uint8_t>(c.r * brightness_mult),
            static_cast<uint8_t>(c.g * brightness_mult),
            static_cast<uint8_t>(c.b * brightness_mult)
    );
}

uint8_t smooth_step(uint8_t i, uint8_t N, uint8_t min, uint8_t max) {
    float v = SMOOTH_STEP(i / static_cast<float>(N));
    return static_cast<uint8_t>((max * v) + (min * (1 - v)));
}

color extract_color(uint32_t c) {
    color clr;
    clr.r = static_cast<uint8_t>(c >> 16);
    clr.g = static_cast<uint8_t>(c >> 8 & 0xFF);
    clr.b = static_cast<uint8_t>(c & 0xFF);
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
        uint32_t shifted = shift_color(i, ANIMATION_TIME_MS, old_color, new_color);

        IT.paint(shifted);
        IS.paint(shifted);
        pixels.setPixelColor(seconds_led, shifted);
        minute_led.paint(shifted);
        hour_led.paint(shifted);
        oclock_led.paint(shifted);

        pixels.show();
        delay_short();
    }
}

void tick() {
    time = rtc.now();
}

inline void save_last_leds() {
    last_second_led = seconds_led;
    last_minute_led = minute_led;
    last_hour_led = hour_led;
    last_oclock_led = oclock_led;
}

void calculate_next_leds() {
    uint8_t hour = time.hour();
    uint8_t min = time.minute();
    uint8_t sec = time.second();

    seconds_led = static_cast<uint8_t>(sec % N_SECONDS_LED + 1);

    if(min < 5) {                   /// 0 - 5
        oclock_led = O_OCLOCK;
        minute_led = M_NONE;
    } else if(min < 35) {           /// 5 - 35
        oclock_led = O_PAST;
        if(min < 10)                // 5 - 10
            minute_led = M_5;
        else if(min < 15)           // 10 - 15
            minute_led = M_10;
        else if(min < 20)           // 15 - 20
            minute_led = M_15;
        else if(min < 25)           // 20 - 25
            minute_led = M_20;
        else if(min < 30)           // 25 - 30
            minute_led = M_25;
        else                        // 30 - 35
            minute_led = M_30;
    } else {                        /// 35 - 60
        hour = static_cast<uint8_t>(hour + 1);
        oclock_led = O_TO;
        if(min < 40)                // 35 - 40
            minute_led = M_25;
        else if(min < 45)           // 40 - 45
            minute_led = M_20;
        else if(min < 50)           // 45 - 50
            minute_led = M_15;
        else if(min < 55)           // 50 - 55
            minute_led = M_10;
        else                        // 55 - 60
            minute_led = M_5;
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
            if(last_minute_led == M_20 && minute_led == M_25)
                M_5.paint(to_color);
            else if(last_minute_led == M_25 && minute_led == M_20)
                M_5.paint(to_black);
            else {
                last_minute_led.paint(to_black);
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
        delay_short();
    }
}

/// Birthday class is for playing "Happy Birthday To You" song
/// and displaying a huge red heart on the board.
class Birthday {
    /// Shifts between the \c old_color to \c new_color but only affets heart leds.
    /// \param old_color Color to shift from.
    /// \param new_color Color to shift to.
    /// \param dly Delay after the shifting in millis.
    static void shift_color_heart(uint32_t old_color, uint32_t new_color, uint16_t dly) {
        uint8_t heart[] = {11, 21, 23, 31, 35, 41, 47, 51, 59, 61, 71,
                           72, 77, 82, 84, 87, 89, 92, 96, 97, 101, 102};

        ANIMATE(i, ANIMATION_TIME_MS) {
            uint32_t shift = shift_color(i, ANIMATION_TIME_MS, old_color, new_color);
            for(uint8_t h_l = 0; h_l < 22; ++h_l)
                pixels.setPixelColor(heart[h_l], shift);
            pixels.show();
            delay_short();
        }
        delay(dly);
    }

    /// Checks whether birthday mode is cancelled or not.
    /// \return \c true if birthday mode is cancelled, \c false otherwise.
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

    /// Sets up the birthday mode. Should be called on \c setup().
    static void begin() {
        begun = false;
        cancelled = false;
        manual_begin = false;
        pinMode(PIN_SPEAKER, OUTPUT);
    }

    /// Checks if birthday mode should be activated now.
    /// \return \c true if birthday mode is manually activated or
    /// the date is 12/05 and time is 9 am, 1 pm or 6 pm, \c false otherwise.
    static bool is_today() {
        return manual_begin || (time.day() == 4 && time.month() == 11
                                && (time.hour() == 8 || time.hour() == 12 || time.hour() == 17)
                                && time.minute() == 0 && time.second() == 0);
    }

    /// Begins the birthday mode. Changes the board schema to a big heart and plays "Happy Birthday To You" song.
    /// <br><br> Birthday mode lasts 3 hour.
    static void celebrate() {
        begun = true;
        shift_color_all(current_color(), COLOR_BLACK);
        shift_color_heart(COLOR_BLACK, COLOR_RED, 500);
        play_happy_birthday();
        for(uint16_t i = 0; i < 10800; ++i) {
            delay_short();
            if(cancel()) break;
        }
        shift_color_heart(COLOR_RED, COLOR_BLACK, 0);
        manual_begin = false;
        begun = false;
    }
};

bool Birthday::begun;
volatile bool Birthday::cancelled;
volatile bool Birthday::manual_begin;

void add_time(TimeSpan ts) {
	rtc.adjust(time + ts);
	tick();
}

void on_color_button_pressed() {
    uint32_t old_color = current_color();

    led_color_idx = static_cast<uint8_t>((led_color_idx + 1) % N_COLORS);
    EEPROM.update(ADDRESS_EEPROM_COLOR, led_color_idx);

    shift_color_all(old_color, current_color());
}

void on_color_button_double_pressed() {
    if(!Birthday::begun)
        Birthday::manual_begin = true;
}

void on_time_button_pressed() {
	add_time(ONE_MIN);
}

void on_time_button_double_pressed() {
    add_time(ONE_HOUR);
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

bool is_night() {
    return time.hour() >= 21 || time.hour() < 7;
}

void adjust_brightness() {
    uint8_t old_brightness = EEPROM.read(ADDRESS_EEPROM_BRIGHTN);
    uint32_t old_color = current_color();

    if(is_night())
        brightness = BRIGHTNESS_LOW;
    else
        brightness = BRIGHTNESS_HIGH;
    EEPROM.update(ADDRESS_EEPROM_BRIGHTN, brightness);

    if(old_brightness != brightness)
        shift_color_all(old_color, current_color());
}

void setup() {
    rtc.begin();
    pixels.begin();
    Birthday::begin();

    pixels.show();

    COLOR_BUTTON = new Button(PIN_COLOR_BUTTON, color_isr, on_color_button_pressed, on_color_button_double_pressed);
    TIME_BUTTON = new Button(PIN_TIME_BUTTON, time_isr, on_time_button_pressed, on_time_button_double_pressed);

    led_color_idx = EEPROM.read(ADDRESS_EEPROM_COLOR);
    if(led_color_idx > (N_COLORS - 1))
        led_color_idx = 0;

    tick();

    // total_accuracy_secs = time.unixtime();
    // EEPROM.put(ADDRESS_EEPROM_SECS, total_accuracy_secs);
    EEPROM.get(ADDRESS_EEPROM_SECS, total_accuracy_secs);

    brightness = EEPROM.read(ADDRESS_EEPROM_BRIGHTN);
    if(brightness != BRIGHTNESS_LOW || brightness != BRIGHTNESS_HIGH)
        brightness = BRIGHTNESS_HIGH;
    if(is_night())
        brightness = BRIGHTNESS_LOW;

    calculate_next_leds();
    shift_color_all(COLOR_BLACK, current_color());
}

void loop() {
    tick();

    uint32_t unix_time = time.unixtime();
    if(unix_time != total_accuracy_secs && (time - DateTime(total_accuracy_secs)).hours() == 1) {
        total_accuracy_secs = unix_time;
        EEPROM.put(ADDRESS_EEPROM_SECS, total_accuracy_secs);
		add_time(ONE_SEC);
    }

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

    adjust_brightness();
}