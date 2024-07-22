#include "ir_helper.h"


String get_command_string(IrCommand command)
{
    switch (command) {
        case Key1:
            return String("1");
        case Key2:
            return String("2");
        case Key3:
            return String("3");
        case Key4:
            return String("4");
        case Key5:
            return String("5");
        case Key6:
            return String("6");
        case Key7:
            return String("7");
        case Key8:
            return String("8");
        case Key9:
            return String("9");
        case KeyStar:
            return String("*");
        case Key0:
            return String("0");
        case KeyHash:
            return String("#");
        case KeyUp:
            return String("Up");
        case KeyLeft:
            return String("Left");
        case KeyOK:
            return String("OK");
        case KeyRight:
            return String("Right");
        case KeyDown:
            return String("Down");
        case KeyUnknown:
            return String("Unknown");
    }
}
