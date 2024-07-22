#ifndef IR_HELPER_H
#define IR_HELPER_H

#include "Arduino.h"

#define MAX_PIN_BUFFER 4

enum IrCommand {
    Key1,       Key2,       Key3, 
    Key4,       Key5,       Key6, 
    Key7,       Key8,       Key9, 
    KeyStar,    Key0,       KeyHash, 
                KeyUp, 
    KeyLeft,    KeyOK,      KeyRight, 
                KeyDown,
    KeyUnknown
};

String get_command_string(IrCommand command);

#endif // IR_HELPER_H
