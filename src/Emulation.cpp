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


#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>

#include <miniz.h>

#include "Emulation.h"

RunningState::RunningState() {
    reset();
}

void RunningState::reset() {
    reset(true, true);
};

void RunningState::reset(bool is_paused, bool is_audio_enabled) {
    bank0_offset = 0x0000;
    bank1_offset = 0x4000;
    button_state = 0xFF;
    protection_check = 8;
    RAM.fill(0xFF);

    paused = is_paused;
    audio_enabled = is_audio_enabled;
}

bool load_file(const std::string &filename, uint8_t *data, size_t size) {
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

