
#ifndef WORDCLOCK_MUSIC_H
#define WORDCLOCK_MUSIC_H

#define PIN_SPEAKER             PIN7

#define NOTE_G                  1275
#define NOTE_A                  1136
#define NOTE_B                  1014
#define NOTE_c                  956
#define NOTE_d                  834
#define NOTE_e                  765
#define NOTE_x                  655
#define NOTE_y                  715
#define NOTE_REST               0

#define SONG_TEMPO              175
#define SPEE                    5

#include "Arduino.h"

/// Plays a given note on a given duration.
/// \param tone Note to play.
/// \param duration Note play duration.
void play_note(uint16_t tone, uint16_t duration);

/// Plays the "Happy Birthday To You" song.
void play_happy_birthday();

#endif //WORDCLOCK_MUSIC_H
