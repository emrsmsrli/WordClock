#include "Music.h"

void play_note(uint16_t tone, uint16_t duration) {
    for(long i = 0; i < duration * 1000L; i += tone * 2) {
        digitalWrite(PIN_SPEAKER, HIGH);
        delayMicroseconds(tone);
        digitalWrite(PIN_SPEAKER, LOW);
        delayMicroseconds(tone);
    }
}

void play_happy_birthday() {
    uint16_t notes[] = {NOTE_G, NOTE_G, NOTE_A, NOTE_G, NOTE_c, NOTE_B, NOTE_REST,
                        NOTE_G, NOTE_G, NOTE_A, NOTE_G, NOTE_d, NOTE_c, NOTE_REST,
                        NOTE_G, NOTE_G, NOTE_x, NOTE_e, NOTE_c, NOTE_B, NOTE_A, NOTE_REST,
                        NOTE_y, NOTE_y, NOTE_e, NOTE_c, NOTE_d, NOTE_c};
    uint16_t beats[] = {2, 2, 8, 8, 8, 16, 1, 2, 2, 8, 8, 8, 16, 1,
                        2, 2, 8, 8, 8, 8, 16, 1, 2, 2, 8, 8, 8, 16};
    uint16_t spee_mult = SONG_TEMPO / SPEE;
    for(int i = 0; i < 28; i++) {
        if(!notes[i]) delay(beats[i] * SONG_TEMPO);
        else play_note(notes[i], beats[i] * spee_mult);
        delay(SONG_TEMPO);
    }
}