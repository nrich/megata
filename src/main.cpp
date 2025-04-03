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
#include <filesystem>

#include <raylib-cpp.hpp>
#include <cmdline.h>
#include <emu2149.h>
#include <nfd.hpp>
#include <imgui.h>
#include <rlImGui.h>

#include "CPU.h"
#include "LCD.h"
#include "Emulation.h"
#include "UI.h"

std::array<uint8_t, 524288> ROM; // biggest rom is 512KiB
std::array<uint8_t, 4096> BIOS;

static LCD lcd;

PSG psg;

static RunningState running_state;

extern Palette green_palette;
extern Palette grey_palette;
extern Palette gb_palette;
extern Palette gbp_palette;

uint8_t read6502(uint16_t address) {
    if (address >= 0x0000 && address <= 0x1FFF) {
        // 1KiB RAM mirrored 8 times
        return running_state.RAM[address & 0x03FF];
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
        return running_state.button_state;
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
        if (running_state.protection_check) {
            uint8_t check = 0;

            check = ((0x47 >> --running_state.protection_check) & 0x01) << 1;

            return check;
        }

        return ROM[running_state.bank0_offset + (address - 0x6000)];
    }

    if (address >= 0xA000 && address <= 0xDFFF) {
        // ROM (cartridge) data (bank 1)
        return ROM[running_state.bank1_offset + (address - 0xA000)];
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
        running_state.RAM[address & 0x03FF] = value;
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
        if (address == 0xC000) {
            // Standard bank switcher
            running_state.bank1_offset = 0x4000 * value;
        }

        if (address == 0x8000) {
            // 4 in 1 Regular bank switcher
            running_state.bank0_offset = 0x4000 * value;
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
    if (running_state.audio_enabled) {
        PSG_calc_stereo(&psg, (int16_t *)buffer, frames * 2);
    } else {
        std::memset(buffer, 0, sizeof(int16_t) * frames * 2);
    }
}

int main(int argc, char *argv[]) {
    cmdline::parser argparser;
    argparser.add<std::string>("rom", 'r', "ROM", false, "");
    argparser.add<std::string>("bios", 'b', "BIOS", false, "gamate.zip");
    argparser.add<int>("scale", 's', "Screen scale", false, 4);
    argparser.add<int>("colour", 'c', "Colour", false, 0);
    argparser.parse_check(argc, argv);

    Emulator emulator;

    NFD::Init();

    emulator.rom = argparser.get<std::string>("rom");
    emulator.bios = argparser.get<std::string>("bios");
    emulator.scale = argparser.get<int>("scale");

    SetConfigFlags(FLAG_MSAA_4X_HINT|FLAG_WINDOW_RESIZABLE);
    raylib::Window window(1280, 960, "Megata" + std::string(" (v") + std::string(VERSION) + ")");
    SetTargetFPS(68);

    if (emulator.rom.length()) {
        if (!load_file(emulator.rom, ROM.data(), ROM.size())) {
            std::cerr << "Could not open ROM file " << emulator.rom << "\n";
            emulator.rom = "";
        }
    }

    if (emulator.bios.length()) {
        if (!load_file(emulator.bios, BIOS.data(), BIOS.size())) {
            std::cerr << "Could not open BIOS file " << emulator.bios << "\n";
            emulator.bios = "";
        }
    }

    running_state.paused = !emulator.ready();

    std::array<uint32_t, LCD::ScreenWidth*LCD::ScreenHeight> screen = {0xFF};

    raylib::Image screen_image(screen.data(), LCD::ScreenWidth, LCD::ScreenHeight);
    raylib::TextureUnmanaged screen_texture(screen_image);

    int gamepad = 0;

    PSG_init(&psg, 4433000/4, 44100);
    PSG_setVolumeMode(&psg, 2);
    PSG_set_quality(&psg, true);
    PSG_setFlags(&psg, EMU2149_ZX_STEREO);
    PSG_reset(&psg);

    std::unique_ptr<raylib::AudioDevice> audiodevice = nullptr;
    std::unique_ptr<raylib::AudioStream> audio_stream = nullptr;

    try {
        audiodevice = std::make_unique<raylib::AudioDevice>();
        audio_stream = std::make_unique<raylib::AudioStream>(44100, 16, 2);

        audio_stream->Play();
        audio_stream->SetCallback(AudioInputCallback);
        running_state.audio_enabled = true;
    } catch (raylib::RaylibException &rle) {
        std::cerr << "Disabling sound\n";
        running_state.audio_enabled = false;
    }

    switch (argparser.get<int>("colour")) {
        case 1:
            emulator.palette = grey_palette;
            break;
        case 2:
            emulator.palette = gb_palette;
            break;
        case 3:
            emulator.palette = gbp_palette;
            break;
        default:
            emulator.palette = green_palette;
    }

    rlImGuiSetup(true);

    auto &imgui_io = ImGui::GetIO();
    imgui_io.IniFilename = nullptr;

    CPU cpu(read6502, write6502, [](){return INT::QUIT;});
    cpu.reset();
    cpu.setPeriod(32768);

    while (!window.ShouldClose()) {
        running_state.button_state = 0xFF;
        if (IsKeyDown(KEY_UP)) {
            running_state.button_state ^= 0b00000001;
        }
        if (IsKeyDown(KEY_DOWN)) {
            running_state.button_state ^= 0b00000010;
        }
        if (IsKeyDown(KEY_LEFT)) {
            running_state.button_state ^= 0b00000100;
        }
        if (IsKeyDown(KEY_RIGHT)) {
            running_state.button_state ^= 0b00001000;
        }
        if (IsKeyDown(KEY_A)) {
            running_state.button_state ^= 0b00010000;
        }
        if (IsKeyDown(KEY_S)) {
            running_state.button_state ^= 0b00100000;
        }
        if (IsKeyDown(KEY_Q)) {
            running_state.button_state ^= 0b01000000;
        }
        if (IsKeyDown(KEY_W)) {
            running_state.button_state ^= 0b10000000;
        }

        if (IsGamepadAvailable(gamepad)) {
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_UP)) {
                running_state.button_state ^= 0b00000001;
            }

            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) {
                running_state.button_state ^= 0b00000010;
            }

            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) {
                running_state.button_state ^= 0b00000100;
            }

            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) {
                running_state.button_state ^= 0b00001000;
            }

            if (GetGamepadButtonPressed() == GAMEPAD_BUTTON_RIGHT_FACE_LEFT) {
                running_state.button_state ^= 0b00010000;
            }

            if (GetGamepadButtonPressed() == GAMEPAD_BUTTON_RIGHT_FACE_DOWN) {
                running_state.button_state ^= 0b00100000;
            }

            if (GetGamepadButtonPressed() == GAMEPAD_BUTTON_MIDDLE_LEFT) {
                running_state.button_state ^= 0b10000000;
            }

            if (GetGamepadButtonPressed() == GAMEPAD_BUTTON_MIDDLE_RIGHT) {
                running_state.button_state ^= 0b01000000;
            }
        }

        if (!running_state.paused) {
            cpu.run();
            cpu.interupt(INT::IRQ);
            cpu.setPeriod(32768);
            cpu.run();
            cpu.interupt(INT::IRQ);
            cpu.setPeriod(7364);
            cpu.run();
            cpu.setPeriod(32768 - 7364);
        }

        lcd.update(emulator.palette, screen);
        screen_texture.Update(screen.data());

        BeginDrawing();
        {
            int width = window.GetRenderWidth();
            int height = window.GetRenderHeight();

            Vector2 lcd_origin(-((width / 2) - (LCD::ScreenWidth*emulator.scale/2)), -((height / 2) - (LCD::ScreenHeight*emulator.scale / 2)));

            window.ClearBackground(BLACK);
            screen_texture.Draw(raylib::Rectangle(Vector2(LCD::ScreenWidth, LCD::ScreenHeight)), raylib::Rectangle(Vector2(LCD::ScreenWidth*emulator.scale, LCD::ScreenHeight*emulator.scale)), lcd_origin);

            if (UI::Draw(running_state, lcd, cpu, emulator))
                break;
        }
        EndDrawing();
    }
    rlImGuiShutdown();

    NFD::Quit();
    exit (0);
}
