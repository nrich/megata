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


#include <iostream>

#include <emu2149.h>
#include <imgui.h>
#include <rlImGui.h>
#include <nfd.hpp>
#include <license.h>

#include "UI.h"

#define MEGATA_TITLE_ASCII "" \
" __  __                  _        \n" \
"|  \\/  | ___  __ _  __ _| |_ __ _ \n" \
"| |\\/| |/ _ \\/ _` |/ _` | __/ _` |\n" \
"| |  | |  __/ (_| | (_| | || (_| |\n" \
"|_|  |_|\\___|\\__, |\\__,_|\\__\\__,_|\n" \
"             |___/                \n"

static bool show_about;

extern std::array<uint8_t, 524288> ROM; // biggest rom is 512KiB
extern std::array<uint8_t, 4096> BIOS;
extern PSG psg;


static uint32_t color_to_u32(const raylib::Color &color) {
    uint8_t r = color.GetR();
    uint8_t g = color.GetG();
    uint8_t b = color.GetB();
    uint8_t a = color.GetA();

    uint32_t c = ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;

    return c;
}

Palette green_palette = {
    color_to_u32(raylib::Color(0x6B, 0xA6, 0x4A, 0xFF)),
    color_to_u32(raylib::Color(0x43, 0x7A, 0x63, 0xFF)),
    color_to_u32(raylib::Color(0x25, 0x59, 0x55, 0xFF)),
    color_to_u32(raylib::Color(0x12, 0x42, 0x4C, 0xFF)),
};

Palette grey_palette = {
    color_to_u32(raylib::Color(0xBA, 0xBA, 0xBA, 0xFF)),
    color_to_u32(raylib::Color(0x75, 0x75, 0x75, 0xFF)),
    color_to_u32(raylib::Color(0x35, 0x35, 0x35, 0xFF)),
    color_to_u32(raylib::Color(0x1A, 0x1A, 0x1A, 0xFF)),
};

Palette gb_palette = {
    color_to_u32(raylib::Color(0x7b, 0x82, 0x10, 0xFF)),
    color_to_u32(raylib::Color(0x5a, 0x79, 0x42, 0xFF)),
    color_to_u32(raylib::Color(0x39, 0x59, 0x4A, 0xFF)),
    color_to_u32(raylib::Color(0x29, 0x41, 0x39, 0xFF)),
};

Palette gbp_palette = {
    color_to_u32(raylib::Color(0xC5, 0xCA, 0xA4, 0xFF)),
    color_to_u32(raylib::Color(0x8C, 0x92, 0x6B, 0xFF)),
    color_to_u32(raylib::Color(0x4A, 0x51, 0x38, 0xFF)),
    color_to_u32(raylib::Color(0x18, 0x18, 0x18, 0xFF)),
};

bool UI::Draw(RunningState &running_state, LCD &lcd, CPU &cpu, Emulator &emulator) {
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
                            emulator.rom = rom_path;
                        } else {
                            std::cerr << "Could not open ROM file " << rom_path << "\n";
                        }

                        PSG_reset(&psg);

                        running_state.reset(!emulator.ready(), is_audio_enabled);
                        lcd.reset();

                        cpu.reset();
                        cpu.setPeriod(32768);
                    } else {
                        running_state.audio_enabled = is_audio_enabled;
                    }
                }
                if (ImGui::MenuItem("Load BIOS")) {
                    bool is_audio_enabled = running_state.audio_enabled;
                    running_state.audio_enabled = false;

                    NFD::UniquePath out_path;

                    nfdu8filteritem_t filters[2] = {{"BIN", "bin"}, {"ZIP", "zip"}};

                    nfdresult_t result = NFD::OpenDialog(out_path, filters);

                    if (result == NFD_OKAY) {
                        std::string bios_path = out_path.get();

                        ROM = {0};
                        if (load_file(bios_path, BIOS.data(), BIOS.size())) {
                            emulator.bios = bios_path;
                        } else {
                            std::cerr << "Could not open BIOS file " << bios_path << "\n";
                        }

                        PSG_reset(&psg);

                        running_state.reset(!emulator.ready(), is_audio_enabled);
                        lcd.reset();

                        cpu.reset();
                        cpu.setPeriod(32768);
                    } else {
                        running_state.audio_enabled = is_audio_enabled;
                    }
                }

                if (ImGui::MenuItem("Quit", "ESC")) {
                    return false;
                }

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Video")) {
                if (ImGui::BeginMenu("Scale")) {
                    if (ImGui::MenuItem("1x")) {
                        emulator.scale = 1;
                    }
                    if (ImGui::MenuItem("2x")) {
                        emulator.scale = 2;
                    }
                    if (ImGui::MenuItem("4x")) {
                        emulator.scale = 4;
                    }
                    if (ImGui::MenuItem("8x")) {
                        emulator.scale = 8;
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Palette")) {
                    if (ImGui::MenuItem("Default")) {
                        emulator.palette = green_palette;
                    }
                    if (ImGui::MenuItem("Greyscale")) {
                        emulator.palette = grey_palette;
                    }
                    if (ImGui::MenuItem("GB")) {
                        emulator.palette = gb_palette;
                    }
                    if (ImGui::MenuItem("GB Pocket")) {
                        emulator.palette = gbp_palette;
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
                    running_state.reset(!emulator.ready(), running_state.audio_enabled);
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

            if (ImGui::BeginPopupModal("About Megate", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("%s\n", MEGATA_TITLE_ASCII);

                ImGui::Text("  By Neil Richardson (nrich@neiltopia.com)");
                ImGui::Text(" "); ImGui::SameLine();
                ImGui::TextLinkOpenURL("https://github.com/nrich/megata");
                ImGui::NewLine();


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

    return false;
}
