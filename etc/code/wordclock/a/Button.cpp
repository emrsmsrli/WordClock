#include "Button.hpp"

Button::Button(uint8_t _pin, void(*_isr)(), void(*_single)(), void(*_double)() = NULL) {
    pin = _pin;
    state = RELEASED;
    start_time = 0;
    stop_time = 0;
    single_clicked = false;
    double_clicked = false;
    single_click_action = _single;
    double_click_action = _double;
    pinMode(pin, INPUT);
    attachInterrupt(static_cast<uint8_t>(digitalPinToInterrupt(pin)), _isr, FALLING);
}

void Button::perform_clicks() {
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

void Button::update() {
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