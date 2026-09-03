// LAF OS Library
// Copyright (C) 2026  darumin (wasm milestone 2)
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.
//
// Translates SDL2 scancodes/buttons/modifiers (as delivered by
// Emscripten's SDL2 port, which itself maps them from browser
// keyboard/mouse events) into laf's platform-independent os::Event
// vocabulary. Deliberately covers the common keys only -- anything
// unmapped falls back to kKeyNil, matching how other backends treat
// keys they don't recognize.

#ifndef OS_WASM_KEYS_INCLUDED
#define OS_WASM_KEYS_INCLUDED
#pragma once

#include "os/event.h"
#include "os/keys.h"

#include <SDL2/SDL.h>

namespace os {

inline KeyScancode sdl_scancode_to_os(SDL_Scancode sc)
{
  switch (sc) {
    case SDL_SCANCODE_A: return kKeyA;
    case SDL_SCANCODE_B: return kKeyB;
    case SDL_SCANCODE_C: return kKeyC;
    case SDL_SCANCODE_D: return kKeyD;
    case SDL_SCANCODE_E: return kKeyE;
    case SDL_SCANCODE_F: return kKeyF;
    case SDL_SCANCODE_G: return kKeyG;
    case SDL_SCANCODE_H: return kKeyH;
    case SDL_SCANCODE_I: return kKeyI;
    case SDL_SCANCODE_J: return kKeyJ;
    case SDL_SCANCODE_K: return kKeyK;
    case SDL_SCANCODE_L: return kKeyL;
    case SDL_SCANCODE_M: return kKeyM;
    case SDL_SCANCODE_N: return kKeyN;
    case SDL_SCANCODE_O: return kKeyO;
    case SDL_SCANCODE_P: return kKeyP;
    case SDL_SCANCODE_Q: return kKeyQ;
    case SDL_SCANCODE_R: return kKeyR;
    case SDL_SCANCODE_S: return kKeyS;
    case SDL_SCANCODE_T: return kKeyT;
    case SDL_SCANCODE_U: return kKeyU;
    case SDL_SCANCODE_V: return kKeyV;
    case SDL_SCANCODE_W: return kKeyW;
    case SDL_SCANCODE_X: return kKeyX;
    case SDL_SCANCODE_Y: return kKeyY;
    case SDL_SCANCODE_Z: return kKeyZ;
    case SDL_SCANCODE_0: return kKey0;
    case SDL_SCANCODE_1: return kKey1;
    case SDL_SCANCODE_2: return kKey2;
    case SDL_SCANCODE_3: return kKey3;
    case SDL_SCANCODE_4: return kKey4;
    case SDL_SCANCODE_5: return kKey5;
    case SDL_SCANCODE_6: return kKey6;
    case SDL_SCANCODE_7: return kKey7;
    case SDL_SCANCODE_8: return kKey8;
    case SDL_SCANCODE_9: return kKey9;
    case SDL_SCANCODE_KP_0: return kKey0Pad;
    case SDL_SCANCODE_KP_1: return kKey1Pad;
    case SDL_SCANCODE_KP_2: return kKey2Pad;
    case SDL_SCANCODE_KP_3: return kKey3Pad;
    case SDL_SCANCODE_KP_4: return kKey4Pad;
    case SDL_SCANCODE_KP_5: return kKey5Pad;
    case SDL_SCANCODE_KP_6: return kKey6Pad;
    case SDL_SCANCODE_KP_7: return kKey7Pad;
    case SDL_SCANCODE_KP_8: return kKey8Pad;
    case SDL_SCANCODE_KP_9: return kKey9Pad;
    case SDL_SCANCODE_F1:  return kKeyF1;
    case SDL_SCANCODE_F2:  return kKeyF2;
    case SDL_SCANCODE_F3:  return kKeyF3;
    case SDL_SCANCODE_F4:  return kKeyF4;
    case SDL_SCANCODE_F5:  return kKeyF5;
    case SDL_SCANCODE_F6:  return kKeyF6;
    case SDL_SCANCODE_F7:  return kKeyF7;
    case SDL_SCANCODE_F8:  return kKeyF8;
    case SDL_SCANCODE_F9:  return kKeyF9;
    case SDL_SCANCODE_F10: return kKeyF10;
    case SDL_SCANCODE_F11: return kKeyF11;
    case SDL_SCANCODE_F12: return kKeyF12;
    case SDL_SCANCODE_ESCAPE:      return kKeyEsc;
    case SDL_SCANCODE_GRAVE:       return kKeyTilde;
    case SDL_SCANCODE_MINUS:       return kKeyMinus;
    case SDL_SCANCODE_EQUALS:      return kKeyEquals;
    case SDL_SCANCODE_BACKSPACE:   return kKeyBackspace;
    case SDL_SCANCODE_TAB:         return kKeyTab;
    case SDL_SCANCODE_LEFTBRACKET: return kKeyOpenbrace;
    case SDL_SCANCODE_RIGHTBRACKET:return kKeyClosebrace;
    case SDL_SCANCODE_RETURN:      return kKeyEnter;
    case SDL_SCANCODE_SEMICOLON:   return kKeyColon;
    case SDL_SCANCODE_APOSTROPHE:  return kKeyQuote;
    case SDL_SCANCODE_BACKSLASH:   return kKeyBackslash;
    case SDL_SCANCODE_COMMA:       return kKeyComma;
    case SDL_SCANCODE_PERIOD:      return kKeyStop;
    case SDL_SCANCODE_SLASH:       return kKeySlash;
    case SDL_SCANCODE_SPACE:       return kKeySpace;
    case SDL_SCANCODE_INSERT:      return kKeyInsert;
    case SDL_SCANCODE_DELETE:      return kKeyDel;
    case SDL_SCANCODE_HOME:        return kKeyHome;
    case SDL_SCANCODE_END:         return kKeyEnd;
    case SDL_SCANCODE_PAGEUP:      return kKeyPageUp;
    case SDL_SCANCODE_PAGEDOWN:    return kKeyPageDown;
    case SDL_SCANCODE_LEFT:        return kKeyLeft;
    case SDL_SCANCODE_RIGHT:       return kKeyRight;
    case SDL_SCANCODE_UP:          return kKeyUp;
    case SDL_SCANCODE_DOWN:        return kKeyDown;
    case SDL_SCANCODE_KP_DIVIDE:   return kKeySlashPad;
    case SDL_SCANCODE_KP_MULTIPLY: return kKeyAsterisk;
    case SDL_SCANCODE_KP_MINUS:    return kKeyMinusPad;
    case SDL_SCANCODE_KP_PLUS:     return kKeyPlusPad;
    case SDL_SCANCODE_KP_PERIOD:   return kKeyDelPad;
    case SDL_SCANCODE_KP_ENTER:    return kKeyEnterPad;
    case SDL_SCANCODE_PRINTSCREEN: return kKeyPrtscr;
    case SDL_SCANCODE_PAUSE:       return kKeyPause;
    case SDL_SCANCODE_LSHIFT:      return kKeyLShift;
    case SDL_SCANCODE_RSHIFT:      return kKeyRShift;
    case SDL_SCANCODE_LCTRL:       return kKeyLControl;
    case SDL_SCANCODE_RCTRL:       return kKeyRControl;
    case SDL_SCANCODE_LALT:        return kKeyAlt;
    case SDL_SCANCODE_RALT:        return kKeyAltGr;
    case SDL_SCANCODE_LGUI:        return kKeyLWin;
    case SDL_SCANCODE_RGUI:        return kKeyRWin;
    case SDL_SCANCODE_MENU:
    case SDL_SCANCODE_APPLICATION: return kKeyMenu;
    case SDL_SCANCODE_SCROLLLOCK:  return kKeyScrLock;
    case SDL_SCANCODE_NUMLOCKCLEAR:return kKeyNumLock;
    case SDL_SCANCODE_CAPSLOCK:    return kKeyCapsLock;
    default:                       return kKeyNil;
  }
}

inline KeyModifiers sdl_modstate_to_os(Uint16 sdlMod)
{
  int m = kKeyNoneModifier;
  if (sdlMod & KMOD_SHIFT) m |= kKeyShiftModifier;
  if (sdlMod & KMOD_CTRL)  m |= kKeyCtrlModifier;
  if (sdlMod & KMOD_ALT)   m |= kKeyAltModifier;
  if (sdlMod & KMOD_GUI)   m |= kKeyWinModifier;
  return (KeyModifiers)m;
}

inline Event::MouseButton sdl_button_to_os(Uint8 sdlButton)
{
  switch (sdlButton) {
    case SDL_BUTTON_LEFT:   return Event::LeftButton;
    case SDL_BUTTON_RIGHT:  return Event::RightButton;
    case SDL_BUTTON_MIDDLE: return Event::MiddleButton;
    case SDL_BUTTON_X1:     return Event::X1Button;
    case SDL_BUTTON_X2:     return Event::X2Button;
    default:                return Event::NoneButton;
  }
}

} // namespace os

#endif
