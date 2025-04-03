/******************************************************************************

Copyright (C) 2025 Neil Richardson (nrich@neiltopia.com)

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.

******************************************************************************/

#ifndef UI_H
#define UI_H

#include <cstdint>

#include "CPU.h"
#include "Emulation.h"
#include "LCD.h"

class UI {
    static void KeyboardPopup();
    static void KeyboardConfig(const std::string &text, int32_t *key);

    static KeyboardKey ImGuiKeyToKeyboardKey(ImGuiKey imgui_key);
    static std::string KeyboardKeyToName(const KeyboardKey key);
public:
    static bool Draw(RunningState &running_state, LCD &lcd, CPU &cpu, Emulator &emulator, KeyboardInput &keyboard_input);
};

#endif //UI_H
