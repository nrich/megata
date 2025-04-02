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

#ifndef LCD_H
#define LCD_H

#include <cstdint>
#include <cstddef>
#include <array>
#include <utility>

#include <raylib-cpp.hpp>

class LCD {
    std::array<std::array<uint8_t, 0x2000>, 2> bitPlanes;

    int bitPlaneSelected = 0;

    bool displayBlank = true;
    bool incrementVertical = false;
    bool swapBitPlanes = false;
    bool windowMode = false;
    bool noRefresh = false;

    uint16_t vramAddress = 0;

    uint8_t xScroll = 0;
    uint8_t yScroll = 0;

    void control(const uint8_t control_byte);
    void scrollHorizontal(const uint8_t scroll);
    void scrollVertical(const uint8_t scroll);
    void positionX(const uint8_t x);
    void positionY(const uint8_t y);
    void raw(const uint8_t data);

    std::pair<int32_t, int32_t> translate(const int scan_line) const;

    uint32_t getPixel(const int x_pos, const int y_pos) const;

    void vramAddressIncrement();
public:
    const static int ScreenWidth = 160;
    const static int ScreenHeight = 150;

    LCD();

    void update(const std::array<uint32_t, 4> &palette, std::array<uint32_t, ScreenWidth*ScreenHeight> &screen);

    void write(uint16_t address, uint8_t value);
    uint8_t read(uint16_t address);

    void reset() {
        bitPlanes[0].fill(0x00);
        bitPlanes[1].fill(0x00);

        bitPlaneSelected = 0;

        displayBlank = true;
        incrementVertical = false;
        swapBitPlanes = false;
        windowMode = false;
        noRefresh = false;

        vramAddress = 0;

        xScroll = 0;
        yScroll = 0;
    }

    ~LCD();

};

#endif //LCD_H
