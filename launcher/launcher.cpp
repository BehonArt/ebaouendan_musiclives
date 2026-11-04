// Elite Beat Ouendan: Music LIVES, the launcher (eba-recomp.exe): the three games
// in one window. For each: whether its executable is in the package, the player's
// own cartridge dump (chosen or dropped on the window, checked by
// RetroAchievements' hash of the ROM: its header, ARM9 and ARM7 code and icon,
// which every good dump of that game shares, trimmed or not), Play, and its save
// folder. Once a dump checks out, the card shows the game's own icon and title
// from the dump's banner, the icon popping in, and the game's difficulty-select
// music plays through once from the dump (rom_music.cpp), and again while the
// mouse (or a gamepad's focus) is on the card: nothing of the games ships with
// the launcher. The ROM paths and the music switch are kept in launcher.ini in
// the launcher's own data folder. A header.png beside the launcher (any width;
// launcher/header_template.png is the guide) is drawn across the top in place
// of the title text.
//
// Where the games keep their files is decided by the games themselves
// (src/eba_paths.cpp): %APPDATA%\eba-recomp\<game>\, or userdata\<game>\
// beside the executables when a file portable.txt is there. The launcher
// follows the same rule for its own folder and for "Save folder", and writes
// each game's output to <its folder>\<game>.log.
//
// It draws with SDL's 2D renderer, not OpenGL 4.5, so it opens on any PC, and
// says what went wrong when a game stops with an error. A gamepad moves between
// the buttons (d-pad or stick) and presses them (south button); Windows' own
// Segoe UI and a Japanese font (Yu Gothic, Meiryo or MS Gothic) are used when
// present, for the Ouendan games' titles.
//
// Options (tests): --rom FILE (set a dump, as a drop does); --play ID (EBA, OE1,
// OE2: start it at once); --shot FILE.bmp (the window's picture after a few
// frames, then quit; --shot-frame N picks the frame); --data DIR (the launcher's
// own folder instead: a test's launcher.ini); --music-wav NAME FILE.wav (write the
// first --rom's stream NAME decoded, for checking the decoder, then quit).
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "rc_consoles.h"
#include "rc_hash.h"
#include "rom_music.h"
#include "stb_image.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

#ifndef EBA_VERSION
#define EBA_VERSION "0.0.0-dev"
#endif

namespace fs = std::filesystem;

namespace {

constexpr const char* kName = "Elite Beat Ouendan: Music LIVES";

struct GameInfo {
    const char* id;                     // the data folder's name, the executable's
    const char* title;
    const char* code;                   // the cartridge's game code (its header)
    const char* dump;                   // which dump it needs
    const char* hash;                   // RetroAchievements' hash of that dump
    const char* menu_strm;              // its menu music, by the sound archive's name (EBA's difficulty select; the Ouendan games' stage select loop, which fits better here)
};

const GameInfo kGames[] = {
    {"EBA", "Elite Beat Agents", "AOSE", "the USA cartridge (AOSE)", "bbf10cada0dcc72068179c600fe79267", "STRM_MENU_A"},
    {"OE1", "Osu! Tatakae! Ouendan", "AOSJ", "the Japanese cartridge (AOSJ)", "999654092d72821ef56a7c8de260cc64", "STRM_MENU_B"},
    {"OE2", "Moero! Nekketsu Rhythm Damashii Osu! Tatakae! Ouendan 2", "AO2J", "the Japanese cartridge (AO2J)",
     "899b9c8dd0128097aebb009b49b2a2d5", "STRM_MENU_B"},
};
constexpr int kCount = 3;

enum class Rom { None, Missing, Wrong, Good };

// What a file's header and banner say: its game code, the game's own icon and title.
struct Banner {
    bool ds = false;                    // a DS cartridge dump, by its header
    std::string code;
    uint32_t icon[32 * 32] = {};        // RGBA, index 0 clear
    bool has_icon = false;
    std::string title;                  // the banner's (English slot), lines joined, the publisher left out
};

struct Slot {
    std::string rom;                    // the chosen dump
    Rom state = Rom::None;
    std::string got;                    // its hash, when wrong
    Banner banner;
    SDL_Texture* icon = nullptr;
    int exit_code = 0;                  // the last run's, when it stopped with an error
    bool failed = false;
    RomMusic music;                     // its difficulty-select music, from the dump
    bool has_music = false;
    uint64_t added_at = 0;              // when its dump was added (ms): the icon's pop
};

std::string g_exe_dir, g_data_override;
bool g_portable, g_jp_font;
Slot g_slots[kCount];
SDL_Renderer* g_ren;
SDL_Process* g_proc;                    // the game running, if any
int g_running = -1;
std::string g_note;                     // a message under the games (a dropped file, ...)
bool g_note_bad;
std::mutex g_pick_mu;
std::vector<std::pair<int, std::string>> g_picked;   // file dialogs' answers, for the main thread
ImFont* g_big;
SDL_Texture* g_header;                  // header.png beside the launcher, if any
int g_header_w, g_header_h;
int g_shot_frame = 5;

// the menu music: one game's at a time, ramped in and out (a quarter second), looping
// under the mouse, once through when just added, quiet while a game runs
struct Music {
    SDL_AudioStream* stream = nullptr;
    int playing = -1, want = -1;        // the slot sounding; the one asked for
    bool once = false;                  // the added game's play-through (no loop)
    size_t pos = 0;                     // the frame next
    float gain = 0;
    bool on = true;                     // the option (launcher.ini music=)
} g_music;
constexpr float kMusicVolume = 0.55f;

// paths as UTF-8 (what SDL hands over: drops, dialogs, the ini)
fs::path upath(const std::string& s) { return fs::path((const char8_t*)s.c_str()); }
std::string ustr(const fs::path& p) { auto u = p.u8string(); return std::string(u.begin(), u.end()); }

const ImVec4 kOk(0.45f, 0.85f, 0.45f, 1), kWarn(1, 0.72f, 0.3f, 1), kDim(0.62f, 0.62f, 0.66f, 1);

std::string data_dir(const char* id) {
    std::string d;
    if (!g_data_override.empty() && !std::strcmp(id, "Launcher")) d = g_data_override + "\\";
    else if (g_portable) d = (fs::path(g_exe_dir) / "userdata" / id).string() + "\\";
    else if (char* p = SDL_GetPrefPath("eba-recomp", id)) {
        d = p;
        SDL_free(p);
    }
    std::error_code ec;
    if (!d.empty()) fs::create_directories(d, ec);
    return d;
}

std::string ini_path() { return data_dir("Launcher") + "launcher.ini"; }

void load() {
    std::ifstream f(ini_path());
    std::string line;
    while (std::getline(f, line)) {
        size_t e = line.find('=');
        if (e == std::string::npos) continue;
        std::string k = line.substr(0, e), v = line.substr(e + 1);
        for (int i = 0; i < kCount; ++i)
            if (k == std::string("rom_") + kGames[i].id) g_slots[i].rom = v;
        if (k == "music") g_music.on = v != "0";
    }
}

void save() {
    std::ofstream f(ini_path());
    f << "# " << kName << ", the launcher: the cartridge dumps chosen\n";
    for (int i = 0; i < kCount; ++i) f << "rom_" << kGames[i].id << "=" << g_slots[i].rom << "\n";
    f << "music=" << (g_music.on ? 1 : 0) << "\n";
}

std::string hash_of(const std::string& path) {
    char h[33] = {};
    return rc_hash_generate_from_file(h, RC_CONSOLE_NINTENDO_DS, path.c_str()) ? std::string(h) : std::string();
}

std::string utf16(const uint8_t* p, int chars) {
    std::string s;
    for (int i = 0; i < chars; ++i) {
        uint32_t c = p[2 * i] | p[2 * i + 1] << 8;
        if (!c) break;
        if (c < 0x80) s += (char)c;
        else if (c < 0x800) { s += (char)(0xC0 | c >> 6); s += (char)(0x80 | (c & 63)); }
        else { s += (char)(0xE0 | c >> 12); s += (char)(0x80 | (c >> 6 & 63)); s += (char)(0x80 | (c & 63)); }
    }
    return s;
}

// The header (its game code at 0x0C, the banner's offset at 0x68) and the banner: a
// 32x32 icon in 8x8 tiles of 4 bits (+0x20), its 16 colours (+0x220, BGR555), the
// titles in UTF-16 (+0x240 Japanese, +0x340 English, ...).
Banner read_banner(const std::string& path) {
    Banner b;
    std::ifstream f(upath(path), std::ios::binary);
    uint8_t h[0x200] = {};
    if (!f.read((char*)h, sizeof h)) return b;
    for (int i = 0; i < 4; ++i) {
        char c = (char)h[0x0C + i];
        if (c < '0' || c > 'Z') return b;          // not a DS game code: not a dump
        b.code += c;
    }
    b.ds = true;
    uint32_t off = h[0x68] | h[0x69] << 8 | h[0x6A] << 16 | (uint32_t)h[0x6B] << 24;
    std::vector<uint8_t> bn(0x440);
    if (!off || !f.seekg(off) || !f.read((char*)bn.data(), (std::streamsize)bn.size())) return b;
    uint32_t pal[16];
    for (int i = 0; i < 16; ++i) {
        uint16_t c = bn[0x220 + 2 * i] | bn[0x221 + 2 * i] << 8;
        uint32_t r = (c & 31) * 255 / 31, g = (c >> 5 & 31) * 255 / 31, bl = (c >> 10 & 31) * 255 / 31;
        pal[i] = i ? (r | g << 8 | bl << 16 | 0xFFu << 24) : 0;
    }
    for (int t = 0; t < 16; ++t)
        for (int y = 0; y < 8; ++y)
            for (int x = 0; x < 8; ++x) {
                uint8_t v = bn[0x20 + t * 32 + y * 4 + x / 2];
                b.icon[((t / 4) * 8 + y) * 32 + (t % 4) * 8 + x] = pal[x & 1 ? v >> 4 : v & 15];
            }
    b.has_icon = true;
    std::string t = utf16(&bn[0x340], 128), line, joined;
    std::vector<std::string> lines;
    for (char c : t + "\n")
        if (c == '\n') { if (!line.empty()) lines.push_back(line); line.clear(); }
        else line += c;
    if (lines.size() > 1) lines.pop_back();         // the last line is the publisher
    for (auto& l : lines) joined += (joined.empty() ? "" : " ") + l;
    b.title = joined;
    return b;
}

void set_icon(Slot& s) {
    if (s.icon) { SDL_DestroyTexture(s.icon); s.icon = nullptr; }
    if (!g_ren || s.state != Rom::Good || !s.banner.has_icon) return;
    s.icon = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, 32, 32);
    if (!s.icon) return;
    SDL_UpdateTexture(s.icon, nullptr, s.banner.icon, 32 * 4);
    SDL_SetTextureScaleMode(s.icon, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(s.icon, SDL_BLENDMODE_BLEND);
}

void check(int i) {
    Slot& s = g_slots[i];
    std::error_code ec;
    s.banner = Banner();
    if (s.rom.empty()) s.state = Rom::None;
    else if (!fs::exists(upath(s.rom), ec)) s.state = Rom::Missing;
    else {
        s.got = hash_of(s.rom);
        s.state = s.got == kGames[i].hash ? Rom::Good : Rom::Wrong;
        s.banner = read_banner(s.rom);
    }
    set_icon(s);
    s.music = RomMusic();
    s.has_music = s.state == Rom::Good && rom_music_load(s.rom, kGames[i].menu_strm, s.music);
}

bool exe_present(int i) {
    std::error_code ec;
    return fs::exists(fs::path(g_exe_dir) / (std::string(kGames[i].id) + ".exe"), ec);
}

// a file given for game i (or for whichever game it is: i < 0)
void take(int i, const std::string& path) {
    std::string name = ustr(upath(path).filename());
    std::string h = hash_of(path);
    Banner b = read_banner(path);
    if (i < 0) {
        for (int k = 0; k < kCount; ++k)
            if (h == kGames[k].hash || (b.ds && b.code == kGames[k].code)) i = k;
        if (i < 0) {
            g_note_bad = true;
            g_note = !b.ds ? name + " is not a Nintendo DS cartridge dump."
                           : name + " is a DS dump (" + b.code + (b.title.empty() ? "" : ", " + b.title) +
                                 "), but not one of these three games.";
            return;
        }
    }
    g_slots[i].rom = path;
    check(i);
    save();
    if (g_slots[i].state == Rom::Good) {        // the pop, and its music once through
        g_slots[i].added_at = SDL_GetTicks();
        g_music.want = i;
        g_music.once = true;
        if (g_music.playing == i) g_music.pos = 0;
    }
    g_note_bad = g_slots[i].state != Rom::Good;
    g_note = g_slots[i].state == Rom::Good ? std::string(kGames[i].title) + ": ready to play." : "";
}

void SDLCALL picked(void* user, const char* const* files, int) {
    if (!files || !files[0]) return;
    std::lock_guard<std::mutex> l(g_pick_mu);
    g_picked.push_back({(int)(intptr_t)user, files[0]});
}

void open_folder(const std::string& dir) { SDL_OpenURL(("file:///" + dir).c_str()); }

void play(int i) {
    std::string exe = (fs::path(g_exe_dir) / (std::string(kGames[i].id) + ".exe")).string();
    std::string log = data_dir(kGames[i].id) + kGames[i].id + ".log";
    const char* argv[] = {exe.c_str(), g_slots[i].rom.c_str(), "--window", nullptr};
    SDL_PropertiesID p = SDL_CreateProperties();
    SDL_SetPointerProperty(p, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, (void*)argv);
    SDL_SetStringProperty(p, SDL_PROP_PROCESS_CREATE_WORKING_DIRECTORY_STRING, g_exe_dir.c_str());
    SDL_IOStream* out = SDL_IOFromFile(log.c_str(), "w");
    if (out) {
        SDL_SetNumberProperty(p, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_REDIRECT);
        SDL_SetPointerProperty(p, SDL_PROP_PROCESS_CREATE_STDOUT_POINTER, out);
        SDL_SetBooleanProperty(p, SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN, true);
    }
    g_proc = SDL_CreateProcessWithProperties(p);
    SDL_DestroyProperties(p);
    if (out) SDL_CloseIO(out);
    g_slots[i].failed = false;
    if (!g_proc) {
        g_note_bad = true;
        g_note = std::string("Could not start ") + exe + ": " + SDL_GetError();
        return;
    }
    g_running = i;
    g_note.clear();
}

// each frame: what should sound (the card under the mouse, else a play-through),
// then enough audio queued for the next 150 ms, ramping between games and to silence
void music_frame(int hovered) {
    if (hovered >= 0 && !g_slots[hovered].has_music) hovered = -1;
    if (hovered >= 0 && hovered != g_music.want) g_music.once = false;
    g_music.want = hovered >= 0 ? hovered : g_music.once ? g_music.want : -1;
    if (g_music.want >= 0 && !g_slots[g_music.want].has_music) g_music.want = -1;
    bool quiet = !g_music.on || g_proc;
    if (g_music.playing < 0) {
        if (g_music.want < 0 || quiet) return;
        g_music.playing = g_music.want;
        g_music.pos = 0;
        g_music.gain = 0;
    }
    const RomMusic& m = g_slots[g_music.playing].music;
    SDL_AudioSpec spec = {SDL_AUDIO_S16, 2, m.rate};
    if (!g_music.stream) {
        g_music.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
        if (!g_music.stream) { g_music.playing = -1; g_music.want = -1; g_music.on = false; return; }
        SDL_ResumeAudioStreamDevice(g_music.stream);
    }
    SDL_SetAudioStreamFormat(g_music.stream, &spec, nullptr);
    bool to = g_music.want == g_music.playing && !quiet;
    float step = 1.0f / (0.25f * m.rate);
    int16_t buf[2 * 512];
    while (SDL_GetAudioStreamQueued(g_music.stream) < (int)(m.rate * 4 * 0.15)) {
        int n = 0;
        for (; n < 512; ++n) {
            g_music.gain = to ? std::min(1.0f, g_music.gain + step) : std::max(0.0f, g_music.gain - step);
            float g = g_music.gain * kMusicVolume;
            buf[2 * n] = (int16_t)(m.pcm[2 * g_music.pos] * g);
            buf[2 * n + 1] = (int16_t)(m.pcm[2 * g_music.pos + 1] * g);
            if (++g_music.pos >= m.frames()) {
                if (m.looped && !g_music.once) g_music.pos = m.loop;
                else { g_music.pos = m.frames() - 1; g_music.gain = 0; to = false; }
            }
            if (!to && g_music.gain <= 0) { ++n; break; }
        }
        SDL_PutAudioStreamData(g_music.stream, buf, n * 4);
        if (!to && g_music.gain <= 0) {             // silent: this one is over
            if (g_music.once && g_music.want == g_music.playing) { g_music.once = false; g_music.want = -1; }
            g_music.playing = -1;
            return;
        }
    }
}

void poll_game() {
    if (!g_proc) return;
    int code = 0;
    if (!SDL_WaitProcess(g_proc, false, &code)) return;
    SDL_DestroyProcess(g_proc);
    g_proc = nullptr;
    if (code != 0) {
        g_slots[g_running].failed = true;
        g_slots[g_running].exit_code = code;
    }
    g_running = -1;
}

// a button in the games' accent colour (Play), grey while it cannot be pressed
bool accent_button(const char* label, ImVec2 size, bool on) {
    if (on) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.80f, 0.24f, 0.16f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.92f, 0.33f, 0.20f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.66f, 0.18f, 0.12f, 1));
    }
    ImGui::BeginDisabled(!on);
    bool r = ImGui::Button(label, size);
    ImGui::EndDisabled();
    if (on) ImGui::PopStyleColor(3);
    return r;
}

const ImU32 kAccent = IM_COL32(235, 84, 41, 255);

// the card; true while the mouse (or a gamepad's focus) is on it
bool card(int i, SDL_Window* win, bool& focused) {
    const GameInfo& g = kGames[i];
    Slot& s = g_slots[i];
    float fs_ = ImGui::GetFontSize();
    float side = 32 * std::max(2.0f, std::round(fs_ * 5.2f / 32));   // the icon at a whole multiple of its 32 pixels
    // the pop: 0.9 s after the dump is added the icon springs in (overshooting) with a ring spreading from it
    float pop = 0;
    if (s.added_at) {
        pop = (SDL_GetTicks() - s.added_at) / 900.0f;
        if (pop >= 1) { pop = 0; s.added_at = 0; }
    }
    ImGui::PushID(i);
    ImGui::BeginChild("card", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar);
    // the icon (the game's own, from the dump), or a blank tile
    ImVec2 at = ImGui::GetCursorScreenPos();
    if (s.icon) {                                   // its pixels kept square: nearest sampling (the backend's own switch)
        const ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
        ImDrawList* d = ImGui::GetWindowDrawList();
        if (pio.DrawCallback_SetSamplerNearest) d->AddCallback(pio.DrawCallback_SetSamplerNearest, nullptr);
        if (s.added_at) {                           // springing in: ease-out back, from nothing past its size and settling
            float t = pop - 1, sc = 1 + 2.70158f * t * t * t + 1.70158f * t * t;
            ImVec2 c(at.x + side / 2, at.y + side / 2), h(side * sc / 2, side * sc / 2);
            ImGui::Dummy(ImVec2(side, side));
            d->AddImage((ImTextureID)(intptr_t)s.icon, ImVec2(c.x - h.x, c.y - h.y), ImVec2(c.x + h.x, c.y + h.y));
        } else {
            ImGui::Image((ImTextureID)(intptr_t)s.icon, ImVec2(side, side));
        }
        if (pio.DrawCallback_SetSamplerLinear) d->AddCallback(pio.DrawCallback_SetSamplerLinear, nullptr);
    } else {
        ImDrawList* d = ImGui::GetWindowDrawList();
        d->AddRectFilled(at, ImVec2(at.x + side, at.y + side), IM_COL32(44, 44, 52, 255), fs_ * 0.4f);
        const char* t = "DS";
        ImGui::PushFont(g_big, 0);
        ImVec2 ts = ImGui::CalcTextSize(t);
        d->AddText(ImVec2(at.x + (side - ts.x) / 2, at.y + (side - ts.y) / 2), IM_COL32(90, 90, 104, 255), t);
        ImGui::PopFont();
        ImGui::Dummy(ImVec2(side, side));
    }
    ImGui::SameLine(0, fs_ * 1.2f);
    ImGui::BeginGroup();
    ImGui::PushFont(g_big, ImGui::GetStyle().FontSizeBase * 1.25f);
    ImGui::TextUnformatted(g.title);
    ImGui::PopFont();
    bool exe = exe_present(i);
    if (s.state == Rom::Good && !s.banner.title.empty() && (g_jp_font || i == 0)) ImGui::TextColored(kDim, "%s", s.banner.title.c_str());
    else ImGui::TextColored(kDim, "Needs your dump of %s.", g.dump);
    if (!exe) ImGui::TextColored(kDim, "Not in this package.");
    else if (s.state == Rom::None) ImGui::TextColored(kWarn, "Choose your dump (.nds), or drop it on this window.");
    else if (s.state == Rom::Missing) ImGui::TextColored(kWarn, "The dump chosen is gone: choose it again.");
    else if (s.state == Rom::Wrong)
        ImGui::TextColored(kWarn, s.banner.code == g.code ? "This is the game, but not the dump it needs: an unmodified %s."
                                                         : "That file is not %s: choose its dump.",
                           g.dump);
    else if (g_running == i) ImGui::TextColored(kOk, "Playing...");
    else if (s.failed) ImGui::TextColored(kWarn, "The game stopped with an error (code %d): its log says why.", s.exit_code);
    else ImGui::TextColored(kOk, "Ready.");
    if (ImGui::IsItemHovered() && !s.rom.empty()) ImGui::SetTooltip("%s", s.rom.c_str());
    ImGui::Spacing();
    ImGui::BeginDisabled(!exe);
    bool can = exe && s.state == Rom::Good && !g_proc;
    if (accent_button("Play", ImVec2(fs_ * 7, 0), can)) play(i);
    bool nav = ImGui::IsItemFocused() && ImGui::GetIO().NavVisible;   // a gamepad's or the keyboard's focus
    if (!focused && can) { ImGui::SetItemDefaultFocus(); focused = true; }
    ImGui::SameLine();
    if (ImGui::Button(s.state == Rom::Good ? "Change dump..." : "Choose dump...")) {
        static const SDL_DialogFileFilter f[] = {{"Nintendo DS cartridge dump", "nds"}};
        SDL_ShowOpenFileDialog(picked, (void*)(intptr_t)i, win, f, 1, nullptr, false);
    }
    ImGui::SameLine();
    if (ImGui::Button("Save folder")) open_folder(data_dir(g.id));
    if (s.failed) {
        ImGui::SameLine();
        if (ImGui::Button("Open log")) SDL_OpenURL(("file:///" + data_dir(g.id) + g.id + ".log").c_str());
    }
    ImGui::EndDisabled();
    ImGui::EndGroup();
    ImGui::EndChild();
    ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
    if (s.added_at) {                               // over the card: its glow fading, the ring spreading from the icon
        ImDrawList* d = ImGui::GetWindowDrawList();
        float a = 1 - pop;
        d->AddRectFilled(mn, mx, (kAccent & 0xFFFFFF) | (ImU32)(a * a * 70) << 24, ImGui::GetStyle().ChildRounding);
        ImVec2 c(at.x + side / 2, at.y + side / 2);
        d->AddCircle(c, side * (0.55f + pop * 1.1f), (kAccent & 0xFFFFFF) | (ImU32)(a * 220) << 24, 0, fs_ * 0.22f * a + 1);
    }
    bool hovered = ImGui::IsMouseHoveringRect(mn, mx, false) && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_RootWindow);
    ImGui::PopID();
    return hovered || nav;
}

// Windows' Segoe UI (and its bold for the titles) with a Japanese font merged in, else ImGui's own
void fonts(float size) {
    ImGuiIO& io = ImGui::GetIO();
    std::string win = "C:\\Windows";
    if (const char* w = SDL_getenv("WINDIR")) win = w;
    auto font = [&](const char* name) { return win + "\\Fonts\\" + name; };
    auto exists = [](const std::string& p) { std::error_code ec; return fs::exists(p, ec); };
    const char* jp[] = {"YuGothM.ttc", "meiryo.ttc", "msgothic.ttc"};
    auto add = [&](const char* latin) -> ImFont* {
        if (!exists(font(latin))) return nullptr;
        ImFont* f = io.Fonts->AddFontFromFileTTF(font(latin).c_str(), size);
        for (const char* j : jp)
            if (exists(font(j))) {
                ImFontConfig c;
                c.MergeMode = true;
                io.Fonts->AddFontFromFileTTF(font(j).c_str(), size, &c);
                g_jp_font = true;
                break;
            }
        return f;
    };
    ImFont* body = add("segoeui.ttf");
    if (!body) io.Fonts->AddFontDefault();
    g_big = add("segoeuib.ttf");
    if (!g_big) g_big = io.Fonts->Fonts[0];
}

// header.png beside the launcher: the art across the window's top (header_template.png guides it)
void load_header() {
    std::ifstream f(fs::path(g_exe_dir) / "header.png", std::ios::binary);
    if (!f) return;
    std::vector<unsigned char> png((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    int w = 0, h = 0, n = 0;
    unsigned char* px = stbi_load_from_memory(png.data(), (int)png.size(), &w, &h, &n, 4);
    if (!px) return;
    g_header = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, w, h);
    if (g_header) {
        SDL_UpdateTexture(g_header, nullptr, px, w * 4);
        SDL_SetTextureBlendMode(g_header, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(g_header, SDL_SCALEMODE_LINEAR);
        g_header_w = w;
        g_header_h = h;
    }
    stbi_image_free(px);
}

void style() {
    ImGuiStyle& s = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    s.WindowRounding = 0;
    s.ChildRounding = 8;
    s.FrameRounding = 6;
    s.FramePadding = ImVec2(12, 6);
    s.ItemSpacing = ImVec2(10, 7);
    s.WindowPadding = ImVec2(18, 16);
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.075f, 0.075f, 0.09f, 1);
    c[ImGuiCol_ChildBg] = ImVec4(0.11f, 0.11f, 0.13f, 1);
    c[ImGuiCol_Border] = ImVec4(0.22f, 0.22f, 0.26f, 1);
    c[ImGuiCol_Button] = ImVec4(0.20f, 0.21f, 0.26f, 1);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.30f, 0.37f, 1);
    c[ImGuiCol_ButtonActive] = ImVec4(0.16f, 0.17f, 0.21f, 1);
    c[ImGuiCol_NavCursor] = ImVec4(1, 0.78f, 0.25f, 1);
}

}  // namespace

int main(int argc, char** argv) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) return 1;
    if (const char* b = SDL_GetBasePath()) g_exe_dir = b;
    std::error_code ec;
    g_portable = fs::exists(fs::path(g_exe_dir) / "portable.txt", ec);
    std::string shot, play_id, wav_name, wav_file;
    std::vector<std::string> roms;
    for (int a = 1; a + 1 < argc; a += 2) {
        std::string k = argv[a], v = argv[a + 1];
        if (k == "--rom") roms.push_back(v);
        else if (k == "--play") play_id = v;
        else if (k == "--shot") shot = v;
        else if (k == "--data") g_data_override = v;
        else if (k == "--shot-frame") g_shot_frame = std::max(1, std::atoi(v.c_str()));
        else if (k == "--music-wav" && a + 2 < argc) { wav_name = v; wav_file = argv[a + 2]; ++a; }
    }
    if (!wav_name.empty() && !roms.empty()) {   // the decoder's check: the stream as a WAV
        RomMusic m;
        std::string err;
        if (!rom_music_load(roms[0], wav_name.c_str(), m, &err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 2; }
        std::ofstream w(wav_file, std::ios::binary);
        uint32_t bytes = (uint32_t)(m.pcm.size() * 2), rate = (uint32_t)m.rate, brate = rate * 4, hdr[] = {
            0x46464952, 36 + bytes, 0x45564157, 0x20746d66, 16, 0x00020001, rate, brate, 0x00100004, 0x61746164, bytes};
        w.write((const char*)hdr, sizeof hdr);
        w.write((const char*)m.pcm.data(), bytes);
        std::printf("%s: %zu frames at %d Hz, loop %s at %zu\n", wav_name.c_str(), m.frames(), m.rate, m.looped ? "yes" : "no", m.loop);
        return 0;
    }
    load();

    float dpi = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    if (dpi <= 0) dpi = 1;
    SDL_Window* win = SDL_CreateWindow(kName, (int)(860 * dpi), (int)(720 * dpi), SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    g_ren = win ? SDL_CreateRenderer(win, nullptr) : nullptr;
    if (!g_ren) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, kName, SDL_GetError(), nullptr);
        return 1;
    }
    SDL_SetRenderVSync(g_ren, 1);
    load_header();
    for (int i = 0; i < kCount; ++i) check(i);
    for (auto& r : roms) take(-1, r);
    for (int i = 0; i < kCount; ++i)
        if (play_id == kGames[i].id && g_slots[i].state == Rom::Good && exe_present(i)) play(i);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_NavEnableKeyboard;
    style();
    ImGui::GetStyle().ScaleAllSizes(dpi);
    ImGui::GetStyle().FontScaleDpi = dpi;
    fonts(18);
    ImGui_ImplSDL3_InitForSDLRenderer(win, g_ren);
    ImGui_ImplSDLRenderer3_Init(g_ren);
    int frames = 0;

    for (bool run = true; run;) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL3_ProcessEvent(&e);
            if (e.type == SDL_EVENT_QUIT) run = false;
            if (e.type == SDL_EVENT_DROP_FILE && e.drop.data) take(-1, e.drop.data);
        }
        {
            std::lock_guard<std::mutex> l(g_pick_mu);
            for (auto& [i, p] : g_picked) take(i, p);
            g_picked.clear();
        }
        poll_game();
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::Begin("##main", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
        if (g_header) {                             // the art, the window's width
            float w = ImGui::GetContentRegionAvail().x;
            ImGui::Image((ImTextureID)(intptr_t)g_header, ImVec2(w, w * g_header_h / g_header_w));
        } else {
            ImGui::PushFont(g_big, ImGui::GetStyle().FontSizeBase * 1.6f);
            ImGui::TextUnformatted(kName);
            ImGui::PopFont();
        }
        ImGui::TextColored(kDim, "Elite Beat Agents and both Osu! Tatakae! Ouendan games, recompiled for PC.   v" EBA_VERSION);
        ImGui::TextColored(kDim, "Each game needs your own dump of its cartridge: nothing of the games is included.");
        ImGui::Spacing();
        ImGui::Spacing();
        bool focused = false;
        int hovered = -1;
        for (int i = 0; i < kCount; ++i)
            if (card(i, win, focused)) hovered = i;
        music_frame(hovered);
        if (!g_note.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(g_note_bad ? kWarn : kOk, "%s", g_note.c_str());
        }
        // the footer, at the window's foot
        float fs_ = ImGui::GetFontSize();
        float foot = ImGui::GetTextLineHeightWithSpacing() * 2 + ImGui::GetStyle().WindowPadding.y;
        if (ImGui::GetCursorPosY() < ImGui::GetWindowHeight() - foot) ImGui::SetCursorPosY(ImGui::GetWindowHeight() - foot);
        if (ImGui::Checkbox("Music", &g_music.on)) save();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Each game's difficulty-select music, from your dump:\nonce when it is added, and while the mouse is on its card.");
        ImGui::SameLine(0, fs_ * 1.5f);
        ImGui::TextColored(kDim, "Saves and settings: %s.   In a game, F1 shows or hides the options bar.",
                           g_portable ? "beside this program (portable)" : "%APPDATA%\\eba-recomp");
        ImGui::TextColored(kDim, "A fan project, not affiliated with Nintendo or iNiS.");
        ImGui::End();
        ImGui::Render();
        SDL_SetRenderDrawColor(g_ren, 19, 19, 23, 255);
        SDL_RenderClear(g_ren);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), g_ren);
        if (!shot.empty() && ++frames == g_shot_frame) {
            if (SDL_Surface* s = SDL_RenderReadPixels(g_ren, nullptr)) {
                SDL_SaveBMP(s, shot.c_str());
                SDL_DestroySurface(s);
            }
            if (!g_proc) run = false;           // (a game started: wait for it)
        }
        if (!shot.empty() && frames > g_shot_frame && !g_proc) run = false;
        SDL_RenderPresent(g_ren);
        if (g_proc) SDL_Delay(10);
    }
    for (auto& s : g_slots)
        if (s.icon) SDL_DestroyTexture(s.icon);
    if (g_header) SDL_DestroyTexture(g_header);
    if (g_music.stream) SDL_DestroyAudioStream(g_music.stream);
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(g_ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
