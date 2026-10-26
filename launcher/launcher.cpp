// eba-recomp's launcher: the three games in one window. For each: whether its
// executable is in the package, the player's own cartridge dump (chosen or
// dropped on the window, checked by RetroAchievements' hash of the ROM: its
// header, ARM9 and ARM7 code and icon, which every good dump of that game
// shares, trimmed or not), Play, and its save folder. The ROM paths are kept
// in launcher.ini in the launcher's own data folder.
//
// Where the games keep their files is decided by the games themselves
// (src/eba_paths.cpp): %APPDATA%\eba-recomp\<game>\, or userdata\<game>\
// beside the executables when a file portable.txt is there. The launcher
// follows the same rule for its own folder and for "Save folder", and writes
// each game's output to <its folder>\<game>.log.
//
// It draws with SDL's 2D renderer, not OpenGL 4.5, so it opens on any PC, and
// says what went wrong when a game stops with an error.
//
// Options (tests): --rom FILE (set a dump, as a drop does); --play ID (EBA, OE1,
// OE2: start it at once); --shot FILE.bmp (the window's picture after a few
// frames, then quit).
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "rc_consoles.h"
#include "rc_hash.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#ifndef EBA_VERSION
#define EBA_VERSION "0.0.0-dev"
#endif

namespace fs = std::filesystem;

namespace {

struct GameInfo {
    const char* id;                     // the data folder's name, the executable's
    const char* title;
    const char* dump;                   // which dump it needs
    const char* hash;                   // RetroAchievements' hash of that dump
};

const GameInfo kGames[] = {
    {"EBA", "Elite Beat Agents", "the USA cartridge (AOSE)", "bbf10cada0dcc72068179c600fe79267"},
    {"OE1", "Osu! Tatakae! Ouendan", "the Japanese cartridge (AOSJ)", "999654092d72821ef56a7c8de260cc64"},
    {"OE2", "Moero! Nekketsu Rhythm Damashii Osu! Tatakae! Ouendan 2", "the Japanese cartridge (AO2J)",
     "899b9c8dd0128097aebb009b49b2a2d5"},
};
constexpr int kCount = 3;

enum class Rom { None, Missing, Wrong, Good };

struct Slot {
    std::string rom;                    // the chosen dump
    Rom state = Rom::None;
    std::string got;                    // its hash, when wrong
    int exit_code = 0;                  // the last run's, when it stopped with an error
    bool failed = false;
};

std::string g_exe_dir;
bool g_portable;
Slot g_slots[kCount];
SDL_Process* g_proc;                    // the game running, if any
int g_running = -1;
std::string g_note;                     // a message under the games (a dropped file, ...)
std::mutex g_pick_mu;
std::vector<std::pair<int, std::string>> g_picked;   // file dialogs' answers, for the main thread

std::string data_dir(const char* id) {
    std::string d;
    if (g_portable) {
        d = (fs::path(g_exe_dir) / "userdata" / id).string() + "\\";
    } else if (char* p = SDL_GetPrefPath("eba-recomp", id)) {
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
    }
}

void save() {
    std::ofstream f(ini_path());
    f << "# eba-recomp's launcher: the cartridge dumps chosen\n";
    for (int i = 0; i < kCount; ++i) f << "rom_" << kGames[i].id << "=" << g_slots[i].rom << "\n";
}

std::string hash_of(const std::string& path) {
    char h[33] = {};
    return rc_hash_generate_from_file(h, RC_CONSOLE_NINTENDO_DS, path.c_str()) ? std::string(h) : std::string();
}

void check(int i) {
    Slot& s = g_slots[i];
    std::error_code ec;
    if (s.rom.empty()) s.state = Rom::None;
    else if (!fs::exists(s.rom, ec)) s.state = Rom::Missing;
    else {
        s.got = hash_of(s.rom);
        s.state = s.got == kGames[i].hash ? Rom::Good : Rom::Wrong;
    }
}

bool exe_present(int i) {
    std::error_code ec;
    return fs::exists(fs::path(g_exe_dir) / (std::string(kGames[i].id) + ".exe"), ec);
}

// a file given for game i (or for whichever game it is: i < 0)
void take(int i, const std::string& path) {
    std::string h = hash_of(path);
    if (i < 0) {
        for (int k = 0; k < kCount; ++k)
            if (h == kGames[k].hash) i = k;
        if (i < 0) {
            g_note = fs::path(path).filename().string() + " is none of the three games' expected dumps.";
            return;
        }
    }
    g_slots[i].rom = path;
    check(i);
    save();
    g_note = g_slots[i].state == Rom::Good ? std::string(kGames[i].title) + ": dump set." : "";
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
        g_note = std::string("Could not start ") + exe + ": " + SDL_GetError();
        return;
    }
    g_running = i;
    g_note.clear();
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

void card(int i, SDL_Window* win) {
    const GameInfo& g = kGames[i];
    Slot& s = g_slots[i];
    ImGui::PushID(i);
    ImGui::BeginChild("card", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 4.2f), ImGuiChildFlags_Borders);
    ImGui::TextUnformatted(g.title);
    bool exe = exe_present(i);
    const ImVec4 ok(0.45f, 0.85f, 0.45f, 1), warn(1, 0.75f, 0.3f, 1), dim(0.6f, 0.6f, 0.6f, 1);
    if (!exe) ImGui::TextColored(dim, "Not in this package.");
    else if (s.state == Rom::None) ImGui::TextColored(warn, "Choose your dump of %s (.nds).", g.dump);
    else if (s.state == Rom::Missing) ImGui::TextColored(warn, "The dump chosen is gone (%s): choose it again.", s.rom.c_str());
    else if (s.state == Rom::Wrong)
        ImGui::TextColored(warn, "Not the expected dump: it must be %s, unmodified (its hash is %s).", g.dump,
                           s.got.empty() ? "unreadable" : s.got.c_str());
    else if (g_running == i) ImGui::TextColored(ok, "Playing...");
    else if (s.failed) ImGui::TextColored(warn, "The game stopped with an error (code %d): see its log.", s.exit_code);
    else ImGui::TextColored(ok, "Ready.");
    if (s.state != Rom::None) ImGui::TextDisabled("%s", s.rom.c_str());
    else ImGui::NewLine();
    ImGui::BeginDisabled(!exe);
    ImGui::BeginDisabled(s.state != Rom::Good || g_proc);
    if (ImGui::Button("  Play  ")) play(i);
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Choose dump...")) {
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
    ImGui::EndChild();
    ImGui::PopID();
}

}  // namespace

int main(int argc, char** argv) {
    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;
    if (const char* b = SDL_GetBasePath()) g_exe_dir = b;
    std::error_code ec;
    g_portable = fs::exists(fs::path(g_exe_dir) / "portable.txt", ec);
    load();
    for (int i = 0; i < kCount; ++i) check(i);
    std::string shot, play_id;
    for (int a = 1; a + 1 < argc; a += 2) {
        std::string k = argv[a], v = argv[a + 1];
        if (k == "--rom") take(-1, v);
        else if (k == "--play") play_id = v;
        else if (k == "--shot") shot = v;
    }
    for (int i = 0; i < kCount; ++i)
        if (play_id == kGames[i].id && g_slots[i].state == Rom::Good && exe_present(i)) play(i);
    int frames = 0;

    float dpi = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    if (dpi <= 0) dpi = 1;
    SDL_Window* win = SDL_CreateWindow("eba-recomp " EBA_VERSION, (int)(760 * dpi), (int)(560 * dpi),
                                       SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_Renderer* ren = win ? SDL_CreateRenderer(win, nullptr) : nullptr;
    if (!ren) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "eba-recomp", SDL_GetError(), nullptr);
        return 1;
    }
    SDL_SetRenderVSync(ren, 1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGui::GetStyle().ScaleAllSizes(dpi);
    ImGui::GetStyle().FontScaleDpi = dpi * 1.15f;
    ImGui_ImplSDL3_InitForSDLRenderer(win, ren);
    ImGui_ImplSDLRenderer3_Init(ren);

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
        ImGui::Text("eba-recomp %s", EBA_VERSION);
        ImGui::TextDisabled("The Nintendo DS rhythm games, recompiled. Each needs your own dump of its cartridge;");
        ImGui::TextDisabled("nothing of the games is included. Drop a .nds file on this window to set it.");
        ImGui::Spacing();
        for (int i = 0; i < kCount; ++i) card(i, win);
        if (!g_note.empty()) ImGui::TextWrapped("%s", g_note.c_str());
        ImGui::Spacing();
        ImGui::TextDisabled("Saves and settings: %s", g_portable ? "beside this program (portable)" : "%APPDATA%\\eba-recomp");
        ImGui::TextDisabled("In a game: F1 shows or hides the options bar. Not affiliated with Nintendo or iNiS.");
        ImGui::End();
        ImGui::Render();
        SDL_SetRenderDrawColor(ren, 20, 20, 24, 255);
        SDL_RenderClear(ren);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), ren);
        if (!shot.empty() && ++frames == 5) {
            if (SDL_Surface* s = SDL_RenderReadPixels(ren, nullptr)) {
                SDL_SaveBMP(s, shot.c_str());
                SDL_DestroySurface(s);
            }
            if (!g_proc) run = false;           // (a game started: wait for it)
        }
        if (!shot.empty() && frames > 5 && !g_proc) run = false;
        SDL_RenderPresent(ren);
        if (g_proc) SDL_Delay(10);
    }
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
