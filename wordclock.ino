#include <EEPROM.h>
#include <RTClib.h>
#include <Adafruit_NeoPixel.h>
#include "wordclock.h"

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(N_PIXELS, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);

RTC_DS1307 rtc;
DateTime time;
TimeSpan ONE_MIN(0, 0, 1, 0);
TimeSpan ONE_HOUR(0, 1, 0, 0);

/// LedArray class represents an array of leds. It has a start and an end index in the matrix.
class LedArray {
    uint8_t start;
    uint8_t end;

public:
    LedArray(uint8_t s, uint8_t e) {
        start = s;
        end = e;
    }

    /// Paints the led array to the specified color.
    /// \param color Color to be painted.
    void paint(uint32_t color) {
        for(uint8_t i = start; i < end; ++i) {
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

/// Button class keeps track of the states of a button and
/// provides action invoking methods based on the state of the button.
class Button {
    /// State of the button
    enum State {
        RELEASED = 0,
        PRESSED,
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
    /// Primary constructor of the Button class. <br><br>
    /// For this project, there will be two buttons named TIME_BUTTON and COLOR_BUTTON.
    /// \param _pin The pin number of the button on the controller.
    /// \param _isr The ISR function necessary to keep track of interrupts from the button.
    /// \param _single The action function which will be invoked when the button is clicked once.
    /// \param _double The action function which will be invoked when the button is clicked twice.
    Button(uint8_t _pin, void(*_isr)(), void(*_single)(), void(*_double)() = NULL) {
        pin = _pin;
        state = RELEASED;
        start_time = 0;
        stop_time = 0;
        single_clicked = false;
        double_clicked = false;
        single_click_action = _single;
        double_click_action = _double;
        pinMode(pin, INPUT);
        attachInterrupt((uint8_t) digitalPinToInterrupt(pin), _isr, FALLING);
    }

    /// Call this method to actually perform click actions
    /// when the button is clicked (usually at the end of the \c loop()).
    void perform_clicks() {
        noInterrupts();
        update();
        if(single_clicked) {
            single_clicked = false;
            interrupts();
            if(single_click_action) single_click_action();
        } else if(double_clicked) {
            double_clicked = false;
            interrupts();
            if(double_click_action) double_click_action();
        } else {
            interrupts();
        }
    }

    /// Updates the state of the button. Should be called in the ISR functions.
    void update() {
        unsigned long now = millis();
        int32_t button_pressed = digitalRead(pin);

        switch(state) {
            case RELEASED:
                if(button_pressed) {
                    if(single_clicked || double_clicked)
                        break;
                    state = PRESSED;
                    start_time = now;
                }
                break;
            case PRESSED:
                if(!button_pressed && now - start_time < BTN_DEBOUNCE_THRESHOLD)
                    state = RELEASED;
                else if(!button_pressed) {
                    state = CLICKED_SINGLE;
                    stop_time = now;
                }
                break;
            case CLICKED_SINGLE:
                if(now - start_time > BTN_CLICK_THRESHOLD) {
                    single_clicked = true;
                    state = RELEASED;
                } else if(button_pressed && now - stop_time > BTN_DEBOUNCE_THRESHOLD) {
                    state = CLICKED_DOUBLE;
                    start_time = now;
                }
                break;
            case CLICKED_DOUBLE:
                if(!button_pressed && now - start_time > BTN_DEBOUNCE_THRESHOLD) {
                    single_clicked = false;
                    double_clicked = true;
                    state = RELEASED;
                }
            default:
                break;
        }
    }
};

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

/// Array of LedArrays for hours. <br> Mapped as H[i] -> (i+1)th hour.
LedArray H[] = {
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
    LedArray(28, 34)     // H_12
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
    pixels.Color(127, 0,   127),    // magenta
    pixels.Color(0,   127, 127),    // cyan
    pixels.Color(0,   0,   255),    // blue
    pixels.Color(0,   255, 0),      // green
    pixels.Color(255, 0,   0)       // red
};

/// Returns the currently shown color with adjusted brightness.
/// \return Current color with brightness.
uint32_t current_color() {
    float brightness_mult = brightness / (float)BRIGHTNESS_HIGH;
    color c = extract_color(colors[led_color_idx]);

    return pixels.Color(
            (uint8_t)(c.r * brightness_mult),
            (uint8_t)(c.g * brightness_mult),
            (uint8_t)(c.b * brightness_mult)
    );
}

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
uint8_t smooth_step(uint8_t i, uint8_t N, uint8_t min, uint8_t max) {
    float v = SMOOTH_STEP(i / (float) N);
    return (uint8_t) ((max * v) + (min * (1 - v)));
}

/// Extracts red, green and blue values from an unsigned integer color value.
/// \param c Color to extract.
/// \return A \c color struct containing the components of the color.
color extract_color(uint32_t c) {
    color clr;
    clr.r = c >> 16;
    clr.g = c >> 8 & 0xFF;
    clr.b = c & 0xFF;
    return clr;
}

/// Shifts the color from \c old_color to \c new_color by \c N smooth steps.
/// \param i Current iteration count.
/// \param N Total iteration count.
/// \param old_color Color to shift from.
/// \param new_color Color to shift to.
/// \return Shifted color in iteration \c i th iteration.
uint32_t shift_color(uint8_t i, uint8_t N, uint32_t old_color, uint32_t new_color) {
    color c_old = extract_color(old_color);
    color c_new = extract_color(new_color);
    return pixels.Color(smooth_step(i, N, c_old.r, c_new.r),
                        smooth_step(i, N, c_old.g, c_new.g),
                        smooth_step(i, N, c_old.b, c_new.b));
}

/// Shifts all the lit LedArrays on the board.
/// \param old_color Color to shift from.
/// \param new_color Color to shift to.
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
        delayMicroseconds(1000);
    }
}

/// Retrieves the current time from the RTC circuit
void tick() {
    time = rtc.now();
}

/// Saves the current lit LedArrays for dimming.
inline void save_last_leds() {
    last_second_led = seconds_led;
    last_minute_led = minute_led;
    last_hour_led = hour_led;
    last_oclock_led = oclock_led;
}

/// Calculates the next LedArrays for brightening.
void calculate_next_leds() {
    uint8_t hour = time.hour();
    uint8_t min = time.minute();
    uint8_t sec = time.second();

    seconds_led = sec % N_SECONDS_LED + 1;

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
        hour = hour + 1;
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

/// Brightens and dims calculated next and previous leds.
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
        delayMicroseconds(900);
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
            delayMicroseconds(1000);
        }
        delay(dly);
    }

    /// Plays a given note on a given duration.
    /// \param tone Note to play.
    /// \param duration Note play duration.
    static void play_note(uint16_t tone, uint16_t duration) {
        for(long i = 0; i < duration * 1000L; i += tone * 2) {
            digitalWrite(PIN_SPEAKER, HIGH);
            delayMicroseconds(tone);
            digitalWrite(PIN_SPEAKER, LOW);
            delayMicroseconds(tone);
        }
    }

    /// Plays the "Happy Birthday To You" song.
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
            delay(950);
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

/// Action function for color button single click.
void on_color_button_pressed() {
    uint32_t old_color = current_color();

    led_color_idx = (led_color_idx + 1) % N_COLORS;
    EEPROM.update(ADDRESS_EEPROM_COLOR, led_color_idx);

    shift_color_all(old_color, current_color());
}

/// Action function for color button double click.
void on_color_button_double_pressed() {
    if(!Birthday::begun)
        Birthday::manual_begin = true;
}

/// Action function for time button single click.
void on_time_button_pressed() {
    rtc.adjust(time + ONE_MIN);
    tick();
}

/// Action function for time button double click.
void on_time_button_double_pressed() {
    rtc.adjust(time + ONE_HOUR);
    tick();
}

/// Color button ISR function.
void color_isr() {
    if(Birthday::begun && !Birthday::cancelled)
        Birthday::cancelled = true;
    else
        COLOR_BUTTON->update();
}

/// Time button ISR function.
void time_isr() {
    if(Birthday::begun && !Birthday::cancelled)
        Birthday::cancelled = true;
    else
        TIME_BUTTON->update();
}

/// Returns the time being night status.
/// \return \c true if it is night now (between 10pm and 8am), \c false otherwise.
bool is_night() {
    return time.hour() >= 21 || time.hour() < 7;
}

/// Checks whether it is night or not and adjusts brightness accordingly.
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