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

#include "LCD.h"

#include <ios>
#include <iostream>

enum Control {
    N = 0x80,
    X = 0x40,
    W = 0x20,
    S = 0x10,
    E = 0x01,
};

LCD::LCD() {

}

void LCD::control(const uint8_t control_byte) {
    displayBlank = control_byte & Control::N;
    incrementVertical = control_byte & Control::X;
    windowMode = control_byte & Control::W;
    swapBitPlanes = control_byte & Control::S;
    noRefresh = control_byte & Control::E;
}

void LCD::scrollHorizontal(const uint8_t scroll) {
    xScroll = scroll;
}

void LCD::scrollVertical(const uint8_t scroll) {
    yScroll = scroll;
}

void LCD::positionX(const uint8_t x) {
    bitPlaneSelected = (x & 0x80) >> 7;

    vramAddress = (vramAddress & 0x1FE0) | (x & 0x1F);
}

void LCD::positionY(const uint8_t y) {
    vramAddress = ((vramAddress & 0x001F) | (y << 5)) & 0x1FFF;
}

void LCD::raw(const uint8_t data) {
    bitPlanes[bitPlaneSelected][vramAddress & 0x1FFF] = data;
    vramAddressIncrement();
}

void LCD::vramAddressIncrement() {
    if (incrementVertical) {
        vramAddress += 0x0020;
    } else {
        vramAddress += 0x0001;
    }

    vramAddress &= 0x1FFF;
}

std::pair<int32_t, int32_t> LCD::translate(const int scan_line) const {
    std::pair<int32_t, int32_t> pos(0, 0);

    if (yScroll < 0xC8) {
        pos.second = scan_line + yScroll;

        if (pos.second >= 0xC8) {
            pos.second -= 0xC8;
        }

        pos.first = xScroll;

        if (windowMode) {
            if (scan_line < 0x10) {
                pos.first = 0x00;
                pos.second = 0xD0 + scan_line;
            }
        }
    } else {
        pos.first = xScroll;

        if (yScroll & 0x08) {
            pos.second = 0x00;
            pos.first = xScroll;
        } else {
            int fixed_scan_lines = yScroll & 0x07;

            if (scan_line <= fixed_scan_lines) {
                pos.first = 0x00;
                pos.second = 0xF8 + scan_line + (7 - fixed_scan_lines);
            } else {
                pos.first = xScroll;
                pos.second = scan_line;
            }
        }
    }

    return pos;
}

uint32_t LCD::getPixel(const int x_pos, const int y_pos) const {
    uint8_t x = x_pos & 0xFF;
    uint8_t y = y_pos & 0xFF;

    int address = (y * 0x20) + (x >> 3);

    int bit_lower = (bitPlanes[0][address]) >> (7 - (x & 0x07)) & 0x01;
    int bit_upper = (bitPlanes[1][address]) >> (7 - (x & 0x07)) & 0x01;

    if (swapBitPlanes)
        return bit_upper | (bit_lower << 1);
    else
        return bit_lower | (bit_upper << 1);
}

void LCD::update(const std::array<uint32_t, 4> &palette, std::array<uint32_t, ScreenWidth*ScreenHeight> &screen) {
    if (displayBlank) {
        screen.fill(palette[0]);
        return;
    }

    for (int scan_line = 0; scan_line < ScreenHeight; scan_line += 1) {
        auto real = translate(scan_line);

        for (int x = 0; x < ScreenWidth; x += 1) {
            int color_idx = getPixel(real.first + x, real.second);

            screen[(scan_line * ScreenWidth) + x] = palette[color_idx];
        }
    }
}

void LCD::write(uint16_t address, uint8_t value) {
    switch (address & 0x0007) {
        case 1:
            control(value);
            break;
        case 2:
            scrollHorizontal(value);
            break;
        case 3:
            scrollVertical(value);
            break;
        case 4:
            positionX(value);
            break;
        case 5:
            positionY(value);
            break;
        case 7:
            raw(value);
            break;
    }
}

uint8_t LCD::read(uint16_t address) {
    uint8_t data = bitPlanes[bitPlaneSelected][vramAddress & 0x1FFF];
    vramAddressIncrement();
    return data;
}

LCD::~LCD() {

}
