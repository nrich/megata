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

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include <vector>
#include <iomanip>
#include <thread>
#include <memory>

#include <raylib-cpp.hpp>

#include <cmdline.h>

#include <emu2149.h>

#include "LCD.h"
#include "CPU.h"

static std::array<uint8_t, 1024> RAM = {0xFF};
static std::array<uint8_t, 524288> ROM; // biggest rom is 512KiB
static std::array<uint8_t, 4096> BIOS;

static LCD lcd;
static uint8_t button_state = 0xFF;

static uint32_t bank0_offset = 0x0000;
static uint32_t bank1_offset = 0x4000;

static PSG psg;

static uint32_t color_to_u32(const raylib::Color &color) {
    uint8_t r = color.GetR();
    uint8_t g = color.GetG();
    uint8_t b = color.GetB();
    uint8_t a = color.GetA();

    uint32_t c = ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;

    return c;
}

static const std::array<uint32_t, 4> green_palette = {
    color_to_u32(raylib::Color(0x6B, 0xA6, 0x4A, 0xFF)),
    color_to_u32(raylib::Color(0x43, 0x7A, 0x63, 0xFF)),
    color_to_u32(raylib::Color(0x25, 0x59, 0x55, 0xFF)),
    color_to_u32(raylib::Color(0x12, 0x42, 0x4C, 0xFF)),
};

static const std::array<uint32_t, 4> grey_palette = {
    color_to_u32(raylib::Color(0xBA, 0xBA, 0xBA, 0xFF)),
    color_to_u32(raylib::Color(0x75, 0x75, 0x75, 0xFF)),
    color_to_u32(raylib::Color(0x35, 0x35, 0x35, 0xFF)),
    color_to_u32(raylib::Color(0x1A, 0x1A, 0x1A, 0xFF)),
};

static const std::array<uint32_t, 4> gb_palette = {
    color_to_u32(raylib::Color(0x7b, 0x82, 0x10, 0xFF)),
    color_to_u32(raylib::Color(0x5a, 0x79, 0x42, 0xFF)),
    color_to_u32(raylib::Color(0x39, 0x59, 0x4A, 0xFF)),
    color_to_u32(raylib::Color(0x29, 0x41, 0x39, 0xFF)),
};

static const std::array<uint32_t, 4> gbp_palette = {
    color_to_u32(raylib::Color(0xC5, 0xCA, 0xA4, 0xFF)),
    color_to_u32(raylib::Color(0x8C, 0x92, 0x6B, 0xFF)),
    color_to_u32(raylib::Color(0x4A, 0x51, 0x38, 0xFF)),
    color_to_u32(raylib::Color(0x18, 0x18, 0x18, 0xFF)),
};

uint8_t read6502(uint16_t address) {
    if (address >= 0x0000 && address <= 0x1FFF) {
        // 1KiB RAM mirrored 8 times
        return RAM[address & 0x03FF];
    }

    if (address >= 0x2000 && address <= 0x3FFF) {
        // peripheral space
        return 0xFF;
    }

    if (address >= 0x4000 && address <= 0x43FF) {
        // audio
        return 0xFF;
    }

    if (address >= 0x4400 && address <= 0x47FF) {
        // UART TX
        return button_state;
    }

    if (address >= 0x4800 && address <= 0x4BFF) {
        // UART RX
        return 0x00;
    }

    if (address >= 0x4C00 && address <= 0x4FFF) {
        // TX shift register?
        return 0xFF;
    }

    if (address >= 0x5000 && address <= 0x53FF) {
        // LCD (8) registers
        return lcd.read(address);
    }

    if (address >= 0x5400 && address <= 0x57FF) {
        // reads external space, which is usually 0xFF
        return 0xFF;
    }

    if (address >= 0x5800 && address <= 0x58FF) {
        //reads open bus
        return 0xFF;
    }

    if (address >= 0x5900 && address <= 0x59FF) {
        // Write address?
        return 0xFF;
    }

    if (address >= 0x5A00 && address <= 0x5AFF) {
        //always returns 11b in bits 1:0, the other 6 bits are open bus (i.e. reads 5Bh)
        return 0x5B;
    }

    if (address >= 0x5B00 && address <= 0x5FFF) {
        // Open bus
        return 0x5B;
    }

    /*
        Split ROM address space into 2 16KiB banks
    */
    if (address >= 0x6000 && address <= 0x9FFF) {
        // ROM (cartridge) data (bank 0)
        static int protection_check = 8;

        if (protection_check) {
            uint8_t check = 0;

            check = ((0x47 >> --protection_check) & 0x01) << 1;

            return check;
        }

        return ROM[bank0_offset + (address - 0x6000)];
    }

    if (address >= 0xA000 && address <= 0xDFFF) {
        // ROM (cartridge) data (bank 1)
        return ROM[bank1_offset + (address - 0xA000)];
    }

    if (address >= 0xE000 && address <= 0xFFFF) {
        // BIOS (4k repeated twice)
        return BIOS[address & 0x0FFF];
    }

    std::cerr << "ADDRESS " << std::hex << address << " NOT HANDLED\n";
    exit(-1);

    return 0x00;
}

void write6502(uint16_t address, uint8_t value) {
    if (address >= 0x0000 && address <= 0x1FFF) {
        RAM[address & 0x03FF] = value;
        return;
    }

    if (address >= 0x2000 && address <= 0x3FFF) {
        // peripheral space
        return;
    }

    if (address >= 0x4000 && address <= 0x43FF) {
        // Audio
        PSG_writeReg(&psg, address & 0x0F, value);
        return;
    }

    if (address >= 0x4400 && address <= 0x47FF) {
        // UART TX
        return;
    }

    if (address >= 0x4800 && address <= 0x4BFF) {
        // UART RX
        return;
    }

    if (address >= 0x4C00 && address <= 0x4FFF) {
        // TX shift register?
        return;
    }

    if (address >= 0x5000 && address <= 0x53FF) {
        // LCD (8) registers
        
        lcd.write(address, value);
        return;
    }

    if (address >= 0x5400 && address <= 0x57FF) {
        // reads external space, which is usually 0xFF
        return;
    }

    if (address >= 0x5800 && address <= 0x58FF) {
        //reads open bus
        return;
    }

    if (address >= 0x5900 && address <= 0x59FF) {
        // Write address?
        return;
    }

    if (address >= 0x5A00 && address <= 0x5AFF) {
        //always returns 11b in bits 1:0, the other 6 bits are open bus (i.e. reads 5Bh)
        return;
    }

    if (address >= 0x5B00 && address <= 0x5FFF) {
        // Open bus
        return;
    }

    if (address >= 0x6000 && address <= 0xDFFF) {
        // ROM (cartridge) data
        if (address >= 0xC000 && address <= 0xDFFF) {
            bank1_offset = 0x4000 * value;
        }

        if (address >= 0x8000 && address <=  0x9FFF) {
            bank0_offset = 0x4000 * value;
        }
        return;
    }

    if (address >= 0xE000 && address <= 0xFFFF) {
        // BIOS (4k repeated twice)
        BIOS[address & 0x0FFF] = value;
        return;
    }

    std::cerr << "ADDRESS " << std::hex << address << " NOT HANDLED\n";
    exit(-1);

}

static void AudioInputCallback(void *buffer, unsigned int frames) {
    PSG_calc_stereo(&psg, (int16_t *)buffer, frames);
}

int main(int argc, char *argv[]) {
    cmdline::parser argparser;
    argparser.add<std::string>("rom", 'r', "ROM", true, "");
    argparser.add<std::string>("bios", 'b', "BIOS", true, "");
    argparser.add<int>("scale", 's', "Screen scale", false, 4);
    argparser.add<int>("colour", 'c', "Colour", false, 0);
    argparser.parse_check(argc, argv);

    std::string rom_filename = argparser.get<std::string>("rom");
    std::string bios_filename = argparser.get<std::string>("bios");
    int scale = argparser.get<int>("scale");

    SetConfigFlags(FLAG_MSAA_4X_HINT|FLAG_VSYNC_HINT);
    raylib::Window window(LCD::ScreenWidth*scale, LCD::ScreenHeight*scale, "Megata" + std::string(" (v") + std::string(VERSION) + ")");
    SetTargetFPS(60);

    std::ifstream rom_fh(rom_filename, std::ios::binary|std::ios::in);
    std::ifstream bios_fh(bios_filename, std::ios::binary|std::ios::in);

    rom_fh.read(reinterpret_cast<char*>(ROM.data()), ROM.size());
    bios_fh.read(reinterpret_cast<char*>(BIOS.data()), BIOS.size());

    std::array<uint32_t, LCD::ScreenWidth*LCD::ScreenHeight> screen = {0xFF};

    raylib::Image screen_image(screen.data(), LCD::ScreenWidth, LCD::ScreenHeight);
    raylib::TextureUnmanaged screen_texture(screen_image);

    RAM.fill(0xFF);

    CPU cpu(read6502, write6502, [](){ return INT::QUIT; });

    cpu.reset();
    cpu.setPeriod(32768);

    int gamepad = 0;

    PSG_init(&psg, 4433000, 44100);
    PSG_setVolumeMode(&psg, 2);
    PSG_set_quality(&psg, true);
    PSG_setFlags(&psg, EMU2149_ZX_STEREO);
    PSG_reset(&psg);

    std::unique_ptr<raylib::AudioDevice> audiodevice = nullptr;
    std::unique_ptr<raylib::AudioStream> audio_stream = nullptr;

    try {
        audiodevice = std::make_unique<raylib::AudioDevice>();
        audio_stream = std::make_unique<raylib::AudioStream>(44100, 16, 1);

        audio_stream->Play();
        audio_stream->SetCallback(AudioInputCallback);
    } catch (raylib::RaylibException &rle) {
        std::cerr << "Disabling sound\n";
    }

    std::array<uint32_t, 4> palette;
    switch (argparser.get<int>("colour")) {
        case 1:
            palette = grey_palette;
            break;
        case 2:
            palette = gb_palette;
            break;
        case 3:
            palette = gbp_palette;
            break;
        default:
            palette = green_palette;
    }

    while (!window.ShouldClose()) {
        button_state = 0xFF;
        if (IsKeyDown(KEY_UP)) {
            button_state ^= 0b00000001;
        }
        if (IsKeyDown(KEY_DOWN)) {
            button_state ^= 0b00000010;
        }
        if (IsKeyDown(KEY_LEFT)) {
            button_state ^= 0b00000100;
        }
        if (IsKeyDown(KEY_RIGHT)) {
            button_state ^= 0b00001000;
        }
        if (IsKeyDown(KEY_A)) {
            button_state ^= 0b00010000;
        }
        if (IsKeyDown(KEY_S)) {
            button_state ^= 0b00100000;
        }
        if (IsKeyDown(KEY_Q)) {
            button_state ^= 0b01000000;
        }
        if (IsKeyDown(KEY_W)) {
            button_state ^= 0b10000000;
        }

        if (IsGamepadAvailable(gamepad)) {
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_UP)) {
                button_state ^= 0b00000001;
            }

            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) {
                button_state ^= 0b00000010;
            }

            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) {
                button_state ^= 0b00000100;
            }

            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) {
                button_state ^= 0b00001000;
            }

            if (GetGamepadButtonPressed() == GAMEPAD_BUTTON_RIGHT_FACE_LEFT) {
                button_state ^= 0b00010000;
            }

            if (GetGamepadButtonPressed() == GAMEPAD_BUTTON_RIGHT_FACE_DOWN) {
                button_state ^= 0b00100000;
            }

            if (GetGamepadButtonPressed() == GAMEPAD_BUTTON_MIDDLE_LEFT) {
                button_state ^= 0b10000000;
            }

            if (GetGamepadButtonPressed() == GAMEPAD_BUTTON_MIDDLE_RIGHT) {
                button_state ^= 0b01000000;
            }
        }

        cpu.run();
        cpu.interupt(INT::IRQ);
        cpu.setPeriod(32768);
        cpu.run();
        cpu.interupt(INT::IRQ);
        cpu.setPeriod(7364);
        cpu.run();
        cpu.setPeriod(32768 - 7364);

        lcd.update(palette, screen);
        screen_texture.Update(screen.data());

        BeginDrawing();
        {
            window.ClearBackground(RAYWHITE);
            screen_texture.Draw(raylib::Rectangle(Vector2(LCD::ScreenWidth, LCD::ScreenHeight)), raylib::Rectangle(Vector2(LCD::ScreenWidth*scale, LCD::ScreenHeight*scale)));
        }
        EndDrawing();
    }

    exit (0);
}
