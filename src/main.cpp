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
#include <miniz.h>
#include <imgui.h>
#include <rlImGui.h>
#include <nfd.hpp>
#include <license.h>

#include "LCD.h"
#include "CPU.h"

#define MEGATA_TITLE_ASCII "" \
" __  __                  _        \n" \
"|  \\/  | ___  __ _  __ _| |_ __ _ \n" \
"| |\\/| |/ _ \\/ _` |/ _` | __/ _` |\n" \
"| |  | |  __/ (_| | (_| | || (_| |\n" \
"|_|  |_|\\___|\\__, |\\__,_|\\__\\__,_|\n" \
"             |___/                \n"



static std::array<uint8_t, 524288> ROM; // biggest rom is 512KiB
static std::array<uint8_t, 4096> BIOS;

static LCD lcd;

static PSG psg;

struct RunningState {
    std::array<uint8_t, 1024> RAM;
    uint8_t button_state;
    int protection_check;
    uint32_t bank0_offset;
    uint32_t bank1_offset;
    bool paused;
    bool audio_enabled;

    RunningState() {
        reset();
    }

    void reset() {
        reset(true, true);
    };

    void reset(bool is_paused, bool is_audio_enabled) {
        bank0_offset = 0x0000;
        bank1_offset = 0x4000;
        button_state = 0xFF;
        protection_check = 8;
        RAM.fill(0xFF);

        paused = is_paused;
        audio_enabled = is_audio_enabled;
    }
};

static RunningState running_state;

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
        if (address >= 0xC000 && address <= 0xDFFF) {
            running_state.bank1_offset = 0x4000 * value;
        }

        if (address >= 0x8000 && address <=  0x9FFF) {
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
        PSG_calc_stereo(&psg, (int16_t *)buffer, frames);
    } else {
        std::memset(buffer, 0, sizeof(int16_t) * frames);
    }
}

static bool load_file(const std::string &filename, uint8_t *data, size_t size) {
    std::string lc_filename = filename;

    std::transform(lc_filename.begin(), lc_filename.end(), lc_filename.begin(), (int(*)(int)) tolower);

    std::string extension = lc_filename.substr(lc_filename.find_last_of(".") + 1);

    if (extension == "zip") {
        mz_zip_archive zip_archive = {0};

        if (!mz_zip_reader_init_file(&zip_archive, filename.c_str(), 0))
            return false;

        for (uint32_t i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++) {
            mz_zip_archive_file_stat file_stat;

            if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
                mz_zip_reader_end(&zip_archive);
                return false;
            }

            std::string lc_filename((const char*) file_stat.m_filename);
            std::transform(lc_filename.begin(), lc_filename.end(), lc_filename.begin(), (int(*)(int)) tolower);
            std::string extension = lc_filename.substr(lc_filename.find_last_of(".") + 1);

            if (extension == "bin") {
                void *buffer;
                size_t uncomp_size;

                buffer = mz_zip_reader_extract_file_to_heap(&zip_archive, file_stat.m_filename, &uncomp_size, 0);
                if (!buffer) {
                    mz_zip_reader_end(&zip_archive);
                    return false;
                }

                bool success = false;
                if (uncomp_size <= size) {
                    std::memcpy(data, buffer, uncomp_size);
                    success = true;
                }

                mz_zip_reader_end(&zip_archive);
                free(buffer);

                return success;
            }
        }

        mz_zip_reader_end(&zip_archive);
        return false;
    } else {
        std::filesystem::path file_path = filename;

        if (std::filesystem::file_size(file_path) <= size) {
            std::ifstream fh(filename, std::ios::binary|std::ios::in);
            fh.read(reinterpret_cast<char*>(data), size);
            return true;
        }
    }

    return false;
}

int main(int argc, char *argv[]) {
    cmdline::parser argparser;
    argparser.add<std::string>("rom", 'r', "ROM", false, "");
    argparser.add<std::string>("bios", 'b', "BIOS", false, "");
    argparser.add<int>("scale", 's', "Screen scale", false, 4);
    argparser.add<int>("colour", 'c', "Colour", false, 0);
    argparser.parse_check(argc, argv);

    NFD::Init();

    std::string rom_filename = argparser.get<std::string>("rom");
    std::string bios_filename = argparser.get<std::string>("bios");
    int scale = argparser.get<int>("scale");

    SetConfigFlags(FLAG_MSAA_4X_HINT|FLAG_VSYNC_HINT|FLAG_WINDOW_RESIZABLE);
    raylib::Window window(1280, 720, "Megata" + std::string(" (v") + std::string(VERSION) + ")");
    SetTargetFPS(60);

    bool has_bios = false;
    bool has_rom = false;

    if (rom_filename.length()) {
        if (load_file(rom_filename, ROM.data(), ROM.size())) {
            has_rom = true;
        } else {
            std::cerr << "Could not open ROM file " << rom_filename << "\n";
        }
    }

    if (bios_filename.length()) {
        if (load_file(bios_filename, BIOS.data(), BIOS.size())) {
            has_bios = true;
        } else {
            std::cerr << "Could not open BIOS file " << bios_filename << "\n";
        }
    }

    running_state.paused = !(has_bios && has_rom);

    std::array<uint32_t, LCD::ScreenWidth*LCD::ScreenHeight> screen = {0xFF};

    raylib::Image screen_image(screen.data(), LCD::ScreenWidth, LCD::ScreenHeight);
    raylib::TextureUnmanaged screen_texture(screen_image);

    CPU cpu(read6502, write6502, [](){return INT::QUIT;});

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

    rlImGuiSetup(true);

    bool show_about = false;

    auto &imgui_io = ImGui::GetIO();
    imgui_io.IniFilename = nullptr;

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

        lcd.update(palette, screen);
        screen_texture.Update(screen.data());

        BeginDrawing();
        {
            int width = window.GetRenderWidth();
            int height = window.GetRenderHeight();

            Vector2 lcd_origin(-((width / 2) - (LCD::ScreenWidth*scale/2)), -((height / 2) - (LCD::ScreenHeight*scale / 2)));

            window.ClearBackground(BLACK);
            screen_texture.Draw(raylib::Rectangle(Vector2(LCD::ScreenWidth, LCD::ScreenHeight)), raylib::Rectangle(Vector2(LCD::ScreenWidth*scale, LCD::ScreenHeight*scale)), lcd_origin);
            rlImGuiBegin();
            {
                if (ImGui::BeginMainMenuBar()) {
                    if (ImGui::BeginMenu("File")) {
                        if (ImGui::MenuItem("Open ROM")) {
                            bool is_audio_enabled = running_state.audio_enabled;
                            running_state.audio_enabled = false;

                            NFD::UniquePath out_path;

                            nfdu8filteritem_t filters[2] = {{"BIN", "bin"}, {"ZIP", "zip"}};

                            nfdresult_t result = NFD::OpenDialog(out_path, filters);

                            if (result == NFD_OKAY) {
                                std::string rom_path = out_path.get();

                                ROM = {0};
                                if (load_file(rom_path, ROM.data(), ROM.size())) {
                                    has_rom = true;
                                } else {
                                    std::cerr << "Could not open ROM file " << rom_filename << "\n";
                                }

                                has_rom = true;

                                PSG_reset(&psg);

                                running_state.reset(!(has_bios && has_rom), is_audio_enabled);
                                lcd.reset();

                                cpu.reset();
                                cpu.setPeriod(32768);
                            }
                        }
                        if (ImGui::MenuItem("Load BIOS")) {
                            bool is_audio_enabled = running_state.audio_enabled;
                            running_state.audio_enabled = false;

                            NFD::UniquePath out_path;

                            nfdu8filteritem_t filters[2] = {{"BIN", "bin"}, {"ZIP", "zip"}};

                            nfdresult_t result = NFD::OpenDialog(out_path, filters);

                            if (result == NFD_OKAY) {
                                std::string rom_path = out_path.get();

                                ROM = {0};
                                if (load_file(rom_path, BIOS.data(), BIOS.size())) {
                                    has_bios = true;
                                } else {
                                    std::cerr << "Could not open BIOS file " << rom_filename << "\n";
                                }

                                PSG_reset(&psg);

                                running_state.reset(!(has_bios && has_rom), is_audio_enabled);
                                lcd.reset();

                                cpu.reset();
                                cpu.setPeriod(32768);
                            }
                        }

                        if (ImGui::MenuItem("Quit", "ESC")) {
                            break;
                            window.Close();
                        }

                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Display")) {
                        if (ImGui::BeginMenu("Scale")) {
                            if (ImGui::MenuItem("1x")) {
                                scale = 1;
                            }
                            if (ImGui::MenuItem("2x")) {
                                scale = 2;
                            }
                            if (ImGui::MenuItem("4x")) {
                                scale = 4;
                            }
                            if (ImGui::MenuItem("8x")) {
                                scale = 8;
                            }
                            ImGui::EndMenu();
                        }

                        if (ImGui::BeginMenu("Palette")) {
                            if (ImGui::MenuItem("Default")) {
                                palette = green_palette;
                            }
                            if (ImGui::MenuItem("Greyscale")) {
                                palette = grey_palette;
                            }
                            if (ImGui::MenuItem("GB")) {
                                palette = gb_palette;
                            }
                            if (ImGui::MenuItem("GB Pocket")) {
                                palette = gbp_palette;
                            }
                            ImGui::EndMenu();
                        }

                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Audio")) {
                        if (ImGui::MenuItem("Enable Audio", "", &running_state.audio_enabled)) {
                        }

                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("System")) {
                        if (ImGui::MenuItem("Pause")) {
                        }
                        if (ImGui::MenuItem("Reset")) {
                            running_state.reset(!(has_bios && has_rom), running_state.audio_enabled);
                            lcd.reset();

                            cpu.reset();
                            cpu.setPeriod(32768);
                        }
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("About")) {
                        if (ImGui::MenuItem("About Megate")) {
                            show_about = true;
                            std::cerr << "HERE\n";
                        }

                        ImGui::EndMenu();
                    }

                    ImGui::EndMainMenuBar();
                }

                if (show_about) {
                    ImGui::OpenPopup("About Megate");

                    static const ImVec4 cyan =          ImVec4(0.10f, 0.90f, 0.90f, 1.0f);

                    if (ImGui::BeginPopupModal("About Megate", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                        ImGui::TextColored(cyan, "%s\n", MEGATA_TITLE_ASCII);

                        if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None)) {
                            if (ImGui::BeginTabItem("Build Info")) {
                                ImGui::BeginChild("build", ImVec2(0, 100), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

                                ImGui::Text("Build: %s", VERSION);

                                #if defined(__DATE__) && defined(__TIME__)
                                ImGui::Text("Built on: %s - %s", __DATE__, __TIME__);
                                #endif

                                #if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
                                ImGui::Text("GCC %d.%d.%d", (int)__GNUC__, (int)__GNUC_MINOR__, (int)__GNUC_PATCHLEVEL__);
                                #endif

                                ImGui::Text("raylib %s (%d.%d.%d)", RAYLIB_VERSION, RAYLIB_VERSION_MAJOR, RAYLIB_VERSION_MINOR, RAYLIB_VERSION_PATCH);
                                ImGui::Text("Dear ImGui %s (%d)", IMGUI_VERSION, IMGUI_VERSION_NUM);

                                ImGui::EndChild();
                                ImGui::EndTabItem();
                            }

                            if (ImGui::BeginTabItem("LICENSE")) {
                                ImGui::BeginChild("license", ImVec2(0, 100), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                                ImGui::TextUnformatted(GPL_LICENSE_STR);
                                ImGui::EndChild();
                                ImGui::EndTabItem();
                            }

                            ImGui::EndTabBar();
                        }

                        if (ImGui::Button("OK", ImVec2(120, 0))) {
                            ImGui::CloseCurrentPopup();
                            show_about = false;
                        }

                        ImGui::EndPopup();
                    }
                }
            }
            rlImGuiEnd();
        }
        EndDrawing();
    }
    rlImGuiShutdown();

    NFD::Quit();
    exit (0);
}
