#include "lcd_helper.h"

#include "Wire.h"

uint8_t fingerprint_emoji[] = {
    B01110,
    B10011,
    B10111,
    B11111,
    B11101,
    B11001,
    B01110,
    B00000
};

uint8_t no_fingerprint_emoji[] = {
    B01110,
    B11001,
    B10001,
    B10001,
    B10001,
    B10011,
    B01110,
    B00000
};

uint8_t locked_emoji[] = {
    B11111,
    B10001,
    B10001,
    B11111,
    B11111,
    B11111,
    B11111,
    B00000
};

uint8_t unlocked_emoji[] = {
    B11111,
    B00001,
    B00001,
    B11111,
    B11111,
    B11111,
    B11111,
    B00000
};


uint8_t get_i2c_address()
{
    uint8_t address = -1;
    Wire.begin();
    for (uint8_t i = 0; i < 127; ++i) {
        Wire.beginTransmission(i);
        if (Wire.endTransmission() == 0) {
            address = i;
            break;
        }
    }
    Wire.end();
    return address;
}
