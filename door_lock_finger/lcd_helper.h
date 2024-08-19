#ifndef LCD_HELPER_H
#define LCD_HELPER_H

#include <Arduino.h>

#define LCD_ROWS 4
#define LCD_COLS 20

#define FINGERPRINT_EMOJI 0
#define NO_FINGERPRINT_EMOJI 1
#define LOCKED_EMOJI 2
#define UNLOCKED_EMOJI 3
#define UNSELECTED_EMOJI 4
#define SELECTED_EMOJI 5

extern uint8_t fingerprint_emoji[];
extern uint8_t no_fingerprint_emoji[];
extern uint8_t locked_emoji[];
extern uint8_t unlocked_emoji[];
extern uint8_t unselected_emoji[];
extern uint8_t selected_emoji[];

uint8_t get_i2c_address();

#endif // LCD_HELPER_H
