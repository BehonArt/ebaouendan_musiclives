// Where the player's files go. A window's run given no --save keeps its save,
// settings, screenshots and log in SDL's folder for the user's settings
// (%APPDATA%\eba-recomp\EBA\, ...\OE2\, ...\OE1\), so the game can live in a
// folder it cannot write to; with a file portable.txt beside the executable,
// in userdata\<game>\ beside it instead. A run given --save (the play folders'
// launchers, tests) keeps the paths it was given; a headless run is left as
// it was asked. The release build also leaves out the memory dumps a run
// writes as it stops (for tests) and titles the window with the game and the
// version.
#include "eba.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <filesystem>
#include <vector>

namespace eba {
namespace {

namespace fs = std::filesystem;

std::string g_data;                     // the run's folder, once known

std::string data_dir_for(const char* id) {
    if (portable()) return (fs::path(exe_dir()) / "userdata" / id).string() + "\\";
    char* d = SDL_GetPrefPath("eba-recomp", id);
    std::string s = d ? d : "";
    SDL_free(d);
    return s;
}

// a save file smaller than the chip (made before the chip's size was known, 2026-11-08): padded
// with erased bytes to the chip's size, so the game finds its data where it left it and the rest blank
void pad_save(const std::string& path, uint32_t size) {
    std::error_code ec;
    if (!fs::exists(path, ec)) return;
    auto have = fs::file_size(path, ec);
    if (ec || have >= size) return;
    if (FILE* f = std::fopen(path.c_str(), "ab")) {
        std::vector<char> pad(size - (size_t)have, (char)0xFF);
        std::fwrite(pad.data(), 1, pad.size(), f);
        std::fclose(f);
        nds_note("eba: the save %s was %llu bytes, padded to the chip's %u", path.c_str(), (unsigned long long)have, size);
    }
}

void config(NdsConfig& c) {
    Game g = game_of(c.rom);
    c.save_size = g.save_size;                      // a save made from nothing: the cartridge's chip
    if (kRelease) c.title = std::string(g.title) + "  (eba-recomp " EBA_VERSION ")";
    if (!c.save.empty()) pad_save(c.save, g.save_size);
    if (!c.window) return;
    if (kRelease) c.dump = false;
    if (!c.save.empty()) return;
    g_data = data_dir_for(g.id);
    std::error_code ec;
    fs::create_directories(g_data, ec);
    c.save = g_data + g.id + ".sav";
    pad_save(c.save, g.save_size);
    if (c.out == NdsConfig().out) c.out = g_data;   // not asked: screenshots and the run's files there too
    nds_note("eba: the save and settings in %s%s", g_data.c_str(), portable() ? " (portable)" : "");
}

struct Register {
    Register() { nds_set_config_hook(config); }
} registered;

}  // namespace

std::string exe_dir() {
    const char* b = SDL_GetBasePath();
    return b ? b : "";
}

bool portable() {
    std::error_code ec;
    return fs::exists(fs::path(exe_dir()) / "portable.txt", ec);
}

std::string data_dir() { return g_data.empty() ? data_dir_for(game().id) : g_data; }

}  // namespace eba
