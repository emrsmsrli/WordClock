
#ifndef WORDCLOCK_BUTTON_H
#define WORDCLOCK_BUTTON_H

#include "Arduino.h"

#define BTN_DEBOUNCE_THRESHOLD  50
#define BTN_CLICK_THRESHOLD     300

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
    Button(uint8_t _pin, void(*_isr)(), void(*_single)(), void(*_double)() = NULL);

    /// Call this method to actually perform click actions
    /// when the button is clicked (usually at the end of the \c loop()).
    void perform_clicks();

    /// Updates the state of the button. Should be called in the ISR functions.
    void update();
};

#endif //WORDCLOCK_BUTTON_H
