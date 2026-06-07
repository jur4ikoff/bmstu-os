#ifndef KEYS_H
#define KEYS_H

#include <linux/input.h>

const char *get_key_name(unsigned int code) {
    switch (code) {
        case KEY_ESC: return "Esc";
        case KEY_1: return "1";
        case KEY_2: return "2";
        case KEY_3: return "3";
        case KEY_4: return "4";
        case KEY_5: return "5";
        case KEY_6: return "6";
        case KEY_7: return "7";
        case KEY_8: return "8";
        case KEY_9: return "9";
        case KEY_0: return "0";
        case KEY_MINUS: return "-";
        case KEY_EQUAL: return "=";
        case KEY_BACKSPACE: return "Backspace";
        case KEY_TAB: return "Tab";
        case KEY_Q: return "Q";
        case KEY_W: return "W";
        case KEY_E: return "E";
        case KEY_R: return "R";
        case KEY_T: return "T";
        case KEY_Y: return "Y";
        case KEY_U: return "U";
        case KEY_I: return "I";
        case KEY_O: return "O";
        case KEY_P: return "P";
        case KEY_LEFTBRACE: return "[";
        case KEY_RIGHTBRACE: return "]";
        case KEY_ENTER: return "Enter";
        case KEY_LEFTCTRL: return "Left Ctrl";
        case KEY_A: return "A";
        case KEY_S: return "S";
        case KEY_D: return "D";
        case KEY_F: return "F";
        case KEY_G: return "G";
        case KEY_H: return "H";
        case KEY_J: return "J";
        case KEY_K: return "K";
        case KEY_L: return "L";
        case KEY_SEMICOLON: return ";";
        case KEY_APOSTROPHE: return "'";
        case KEY_GRAVE: return "`";
        case KEY_LEFTSHIFT: return "Left Shift";
        case KEY_BACKSLASH: return "\\";
        case KEY_Z: return "Z";
        case KEY_X: return "X";
        case KEY_C: return "C";
        case KEY_V: return "V";
        case KEY_B: return "B";
        case KEY_N: return "N";
        case KEY_M: return "M";
        case KEY_COMMA: return ",";
        case KEY_DOT: return ".";
        case KEY_SLASH: return "/";
        case KEY_RIGHTSHIFT: return "Right Shift";
        case KEY_KPASTERISK: return "Keypad *";
        case KEY_LEFTALT: return "Left Alt";
        case KEY_SPACE: return "Space";
        case KEY_CAPSLOCK: return "CapsLock";
        case KEY_F1: return "F1";
        case KEY_F2: return "F2";
        case KEY_F3: return "F3";
        case KEY_F4: return "F4";
        case KEY_F5: return "F5";
        case KEY_F6: return "F6";
        case KEY_F7: return "F7";
        case KEY_F8: return "F8";
        case KEY_F9: return "F9";
        case KEY_F10: return "F10";
        default: return "Unknown Key";
    }
}

#endif
