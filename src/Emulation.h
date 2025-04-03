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

#ifndef EMULATION_H
#define EMULATION_H

#include <cstdint>
#include <array>
#include <string>

typedef std::array<uint32_t, 4> Palette;

struct RunningState {
    std::array<uint8_t, 1024> RAM;
    uint8_t button_state;
    int32_t protection_check;
    uint32_t bank0_offset;
    uint32_t bank1_offset;
    bool paused;
    bool audio_enabled;

    RunningState();

    void reset();

    void reset(bool is_paused, bool is_audio_enabled);
};

struct Emulator {
    int32_t scale;
    std::string rom;
    std::string bios;
    Palette palette;

    bool ready() {
        return rom.length() && bios.length();
    }
};

bool load_file(const std::string &filename, uint8_t *data, size_t size);

#endif //EMULATION_H
