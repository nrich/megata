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

KeyboardKey UI::ImGuiKeyToKeyboardKey(ImGuiKey imgui_key) {
    switch (imgui_key) {
        case ImGuiKey_Apostrophe: return KEY_APOSTROPHE;
        case ImGuiKey_Comma: return KEY_COMMA;
        case ImGuiKey_Minus: return KEY_MINUS;
        case ImGuiKey_Period: return KEY_PERIOD;
        case ImGuiKey_Slash: return KEY_SLASH;
        case ImGuiKey_0: return KEY_ZERO;
        case ImGuiKey_1: return KEY_ONE;
        case ImGuiKey_2: return KEY_TWO;
        case ImGuiKey_3: return KEY_THREE;
        case ImGuiKey_4: return KEY_FOUR;
        case ImGuiKey_5: return KEY_FIVE;
        case ImGuiKey_6: return KEY_SIX;
        case ImGuiKey_7: return KEY_SEVEN;
        case ImGuiKey_8: return KEY_EIGHT;
        case ImGuiKey_9: return KEY_NINE;
        case ImGuiKey_Semicolon: return KEY_SEMICOLON;
        case ImGuiKey_Equal: return KEY_EQUAL;
        case ImGuiKey_A: return KEY_A;
        case ImGuiKey_B: return KEY_B;
        case ImGuiKey_C: return KEY_C;
        case ImGuiKey_D: return KEY_D;
        case ImGuiKey_E: return KEY_E;
        case ImGuiKey_F: return KEY_F;
        case ImGuiKey_G: return KEY_G;
        case ImGuiKey_H: return KEY_H;
        case ImGuiKey_I: return KEY_I;
        case ImGuiKey_J: return KEY_J;
        case ImGuiKey_K: return KEY_K;
        case ImGuiKey_L: return KEY_L;
        case ImGuiKey_M: return KEY_M;
        case ImGuiKey_N: return KEY_N;
        case ImGuiKey_O: return KEY_O;
        case ImGuiKey_P: return KEY_P;
        case ImGuiKey_Q: return KEY_Q;
        case ImGuiKey_R: return KEY_R;
        case ImGuiKey_S: return KEY_S;
        case ImGuiKey_T: return KEY_T;
        case ImGuiKey_U: return KEY_U;
        case ImGuiKey_V: return KEY_V;
        case ImGuiKey_W: return KEY_W;
        case ImGuiKey_X: return KEY_X;
        case ImGuiKey_Y: return KEY_Y;
        case ImGuiKey_Z: return KEY_Z;
        case ImGuiKey_LeftBracket: return KEY_LEFT_BRACKET;
        case ImGuiKey_Backslash: return KEY_BACKSLASH;
        case ImGuiKey_RightBracket: return KEY_RIGHT_BRACKET;
        case ImGuiKey_GraveAccent: return KEY_GRAVE;
        // Function keys
        case ImGuiKey_Space: return KEY_SPACE;
        case ImGuiKey_Escape: return KEY_ESCAPE;
        case ImGuiKey_Enter: return KEY_ENTER;
        case ImGuiKey_Tab: return KEY_TAB;
        case ImGuiKey_Backspace: return KEY_BACKSPACE;
        case ImGuiKey_Insert: return KEY_INSERT;
        case ImGuiKey_Delete: return KEY_DELETE;
        case ImGuiKey_RightArrow: return KEY_RIGHT;
        case ImGuiKey_LeftArrow: return KEY_LEFT;
        case ImGuiKey_DownArrow: return KEY_DOWN;
        case ImGuiKey_UpArrow: return KEY_UP;
        case ImGuiKey_PageUp: return KEY_PAGE_UP;
        case ImGuiKey_PageDown: return KEY_PAGE_DOWN;
        case ImGuiKey_Home: return KEY_HOME;
        case ImGuiKey_End: return KEY_END;
        case ImGuiKey_CapsLock: return KEY_CAPS_LOCK;
        case ImGuiKey_ScrollLock: return KEY_SCROLL_LOCK;
        case ImGuiKey_NumLock: return KEY_NUM_LOCK;
        case ImGuiKey_PrintScreen: return KEY_PRINT_SCREEN;
        case ImGuiKey_Pause: return KEY_PAUSE;
        case ImGuiKey_F1: return KEY_F1;
        case ImGuiKey_F2: return KEY_F2;
        case ImGuiKey_F3: return KEY_F3;
        case ImGuiKey_F4: return KEY_F4;
        case ImGuiKey_F5: return KEY_F5;
        case ImGuiKey_F6: return KEY_F6;
        case ImGuiKey_F7: return KEY_F7;
        case ImGuiKey_F8: return KEY_F8;
        case ImGuiKey_F9: return KEY_F9;
        case ImGuiKey_F10: return KEY_F10;
        case ImGuiKey_F11: return KEY_F11;
        case ImGuiKey_F12: return KEY_F12;
        case ImGuiKey_LeftShift: return KEY_LEFT_SHIFT;
        case ImGuiKey_LeftCtrl: return KEY_LEFT_CONTROL;
        case ImGuiKey_LeftAlt: return KEY_LEFT_ALT;
        case ImGuiKey_LeftSuper: return KEY_LEFT_SUPER;
        case ImGuiKey_RightShift: return KEY_RIGHT_SHIFT;
        case ImGuiKey_RightCtrl: return KEY_RIGHT_CONTROL;
        case ImGuiKey_RightAlt: return KEY_RIGHT_ALT;
        case ImGuiKey_RightSuper: return KEY_RIGHT_SUPER;
        case ImGuiKey_Menu: return KEY_KB_MENU;
        // Keypad keys
        case ImGuiKey_Keypad0: return KEY_KP_0;
        case ImGuiKey_Keypad1: return KEY_KP_1;
        case ImGuiKey_Keypad2: return KEY_KP_2;
        case ImGuiKey_Keypad3: return KEY_KP_3;
        case ImGuiKey_Keypad4: return KEY_KP_4;
        case ImGuiKey_Keypad5: return KEY_KP_5;
        case ImGuiKey_Keypad6: return KEY_KP_6;
        case ImGuiKey_Keypad7: return KEY_KP_7;
        case ImGuiKey_Keypad8: return KEY_KP_8;
        case ImGuiKey_Keypad9: return KEY_KP_9;
        case ImGuiKey_KeypadDecimal: return KEY_KP_DECIMAL;
        case ImGuiKey_KeypadDivide: return KEY_KP_DIVIDE;
        case ImGuiKey_KeypadMultiply: return KEY_KP_MULTIPLY;
        case ImGuiKey_KeypadSubtract: return KEY_KP_SUBTRACT;
        case ImGuiKey_KeypadAdd: return KEY_KP_ADD;
        case ImGuiKey_KeypadEnter: return KEY_KP_ENTER;
        case ImGuiKey_KeypadEqual: return KEY_KP_EQUAL;
        default: return KEY_NULL;
    }
}

std::string UI::KeyboardKeyToName(const KeyboardKey key) {
    switch (key) {
        case KEY_APOSTROPHE: return "'";
        case KEY_COMMA: return ",";
        case KEY_MINUS: return "-";
        case KEY_PERIOD: return ".";
        case KEY_SLASH: return "/";
        case KEY_ZERO: return "0";
        case KEY_ONE: return "1";
        case KEY_TWO: return "2";
        case KEY_THREE: return "3";
        case KEY_FOUR: return "4";
        case KEY_FIVE: return "5";
        case KEY_SIX: return "6";
        case KEY_SEVEN: return "7";
        case KEY_EIGHT: return "8";
        case KEY_NINE: return "9";
        case KEY_SEMICOLON: return ";";
        case KEY_EQUAL: return "=";
        case KEY_A: return "A";
        case KEY_B: return "B";
        case KEY_C: return "C";
        case KEY_D: return "D";
        case KEY_E: return "E";
        case KEY_F: return "F";
        case KEY_G: return "G";
        case KEY_H: return "H";
        case KEY_I: return "I";
        case KEY_J: return "J";
        case KEY_K: return "K";
        case KEY_L: return "L";
        case KEY_M: return "M";
        case KEY_N: return "N";
        case KEY_O: return "O";
        case KEY_P: return "P";
        case KEY_Q: return "Q";
        case KEY_R: return "R";
        case KEY_S: return "S";
        case KEY_T: return "T";
        case KEY_U: return "U";
        case KEY_V: return "V";
        case KEY_W: return "W";
        case KEY_X: return "X";
        case KEY_Y: return "Y";
        case KEY_Z: return "Z";
        case KEY_LEFT_BRACKET: return "[";
        case KEY_BACKSLASH: return "\\";
        case KEY_RIGHT_BRACKET: return "]";
        case KEY_GRAVE: return "`";
        // Function keys
        case KEY_SPACE: return "Space";
        case KEY_ESCAPE: return "Escape";
        case KEY_ENTER: return "Enter";
        case KEY_TAB: return "Tab";
        case KEY_BACKSPACE: return "Backspace";
        case KEY_INSERT: return "INS";
        case KEY_DELETE: return "DEL";
        case KEY_RIGHT: return "Right";
        case KEY_LEFT: return "Left";
        case KEY_DOWN: return "Down";
        case KEY_UP: return "Up";
        case KEY_PAGE_UP: return "PG UP";
        case KEY_PAGE_DOWN: return "PG DN";
        case KEY_HOME: return "HOME";
        case KEY_END: return "END";
        case KEY_CAPS_LOCK: return "CAPS LOCK";
        case KEY_SCROLL_LOCK: return "SCRL LOCK";
        case KEY_NUM_LOCK: return "NUM Lock";
        case KEY_PRINT_SCREEN: return "PRT SCN";
        case KEY_PAUSE: return "PAUSE";
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
        case KEY_F11: return "F11";
        case KEY_F12: return "F12";
        case KEY_LEFT_SHIFT: return "Left Shift";
        case KEY_LEFT_CONTROL: return "Left CTRL";
        case KEY_LEFT_ALT: return "Left ALT";
        case KEY_LEFT_SUPER: return "Left Super";
        case KEY_RIGHT_SHIFT: return "Right Shift";
        case KEY_RIGHT_CONTROL: return "Right CTRL";
        case KEY_RIGHT_ALT: return "Right ALT";
        case KEY_RIGHT_SUPER: return "Right Super";
        case KEY_KB_MENU: return "MENU";
        // Keypad keys
        case KEY_KP_0: return "0";
        case KEY_KP_1: return "1";
        case KEY_KP_2: return "2";
        case KEY_KP_3: return "3";
        case KEY_KP_4: return "4";
        case KEY_KP_5: return "5";
        case KEY_KP_6: return "6";
        case KEY_KP_7: return "7";
        case KEY_KP_8: return "8";
        case KEY_KP_9: return "9";
        case KEY_KP_DECIMAL: return "DEC";
        case KEY_KP_DIVIDE: return "DIV";
        case KEY_KP_MULTIPLY: return "MUL";
        case KEY_KP_SUBTRACT: return "SUB";
        case KEY_KP_ADD: return "ADD";
        case KEY_KP_ENTER: return "ENT";
        case KEY_KP_EQUAL: return "EQ";
        default:
            return "";
    }
}

static int32_t *configured_key = nullptr;

void UI::KeyboardPopup() {
    if (ImGui::BeginPopupModal("Keyboard Configuration", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Press any key to assign...\n\n");
        ImGui::Separator();

        for (ImGuiKey i = ImGuiKey_NamedKey_BEGIN; i < ImGuiKey_NamedKey_END; i = (ImGuiKey)(i + 1)) {
            if (ImGui::IsKeyDown(i)) {
                KeyboardKey new_key = ImGuiKeyToKeyboardKey(i);

                if (new_key && configured_key) {
                    *configured_key = new_key;
                }

                ImGui::CloseCurrentPopup();
            }
        }

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void UI::KeyboardConfig(const std::string &text, int32_t *key) {
    std::cerr << key << "\n";

    ImGui::Text("%s", text.c_str());
    ImGui::SameLine(120);

    std::string button_label = KeyboardKeyToName((KeyboardKey)*key) + "##" + text;

    std::cerr << button_label << "\n";

    if (ImGui::Button(button_label.c_str(), ImVec2(90, 0))) {
        configured_key = key;
        ImGui::OpenPopup("Keyboard Configuration");
    }
}

bool UI::Draw(RunningState &running_state, LCD &lcd, CPU &cpu, Emulator &emulator, KeyboardInput &keyboard_input) {
    bool should_exit = false;

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
                    should_exit = true;
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

            if (ImGui::BeginMenu("Input")) {
                if (ImGui::BeginMenu("Keyboard")) {
                    KeyboardConfig("Left:", &keyboard_input.left);
                    KeyboardConfig("Right:", &keyboard_input.right);
                    KeyboardConfig("Up:", &keyboard_input.up);
                    KeyboardConfig("Down:", &keyboard_input.down);
                    KeyboardConfig("Button A:", &keyboard_input.a);
                    KeyboardConfig("Button B:", &keyboard_input.b);
                    KeyboardConfig("Start:", &keyboard_input.start);
                    KeyboardConfig("Select:", &keyboard_input.select);

                    KeyboardPopup();

                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Gamepad")) {
                    ImGui::EndMenu();
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

    return should_exit;
}
