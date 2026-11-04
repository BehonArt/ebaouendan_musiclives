// A stream (STRM) from a cartridge dump's sound archive, decoded to PCM, so the
// launcher can play a game's own menu music from the player's dump. Nothing of
// the games ships with the launcher: the music exists only once the dump is there.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct RomMusic {
    int rate = 0;                       // samples a second
    bool looped = false;
    size_t loop = 0;                    // the loop's start, in frames
    std::vector<int16_t> pcm;           // stereo, interleaved
    size_t frames() const { return pcm.size() / 2; }
};

// data/sound_data.sdat's stream named `strm` (SYMB names: STRM_MENU_B, ...) from
// the dump at `rom_path`. False, with why in `err`, when the dump has no such stream.
bool rom_music_load(const std::string& rom_path, const char* strm, RomMusic& out, std::string* err = nullptr);
