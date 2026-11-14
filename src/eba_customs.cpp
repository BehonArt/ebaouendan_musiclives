// Customs: songs charted over the story's stages, played as a mode of
// their own. Each custom is a folder (tools/extract_customs.py): the files it
// changes on the cartridge and custom.ini (its title, artist, the stage it
// replaces and that stage's index). Customs mode is a run of its own, started
// by this one: the stock cartridge with the custom's files (--replace-file),
// a fresh copy of a known save (customs/pristine.sav, never the campaign's;
// made by the game itself on the first use: its first boot on an empty save,
// headless, through the tutorial to the main menu, Game::first_boot),
// and an autopilot that takes the game from its boot to the custom's stage at
// turbo speed behind a loading screen: the title, Single Player, the
// difficulty, a stage card (The Anthem's, the one the known save shows
// first), the stage's index set in the session, OK. Its scores go to
// customs/scores.ini, a file of their own beside the campaign's save, which
// Customs mode never opens.
//
// What it reads and writes of the game (EBA's, the stock USA ARM9, 2026-10-22;
// Ouendan 2's in games.cpp):
//   *(*(*0x020CD1FC))   the scene's vtable: title 020B3D78, main menu 020B7288,
//                       difficulty 020B736C, map 020B7528, a stage's opening
//                       comic 020B64F0, its song 020B65A0, then its ending's
//                       comic 020B6448 and the results 020B62C0 after a clear,
//                       Regroup? 020B66C8 after a fail
//   *0x020CD248         the session: +8 the stage's index (0-based in the
//                       table at 0x020B3EE4), +0xF8 the score
//
// Options (ndsrun --port): custom=TAG difficulty=e|n|h|x campaign-save=PATH
// (Customs mode, set by the launch); hd=off (the game's own song, not the HD one);
// customs-first-boot (headless: the first boot that makes the start save, then stop);
// customs-make-start-save (make it as the Customs screen would, then stop: a test);
// customs-quit (a test run ends 2 s after the song is over); customs-dir=PATH (where the customs are:
// in the release by default the game's data folder (%APPDATA%\eba-recomp\EBA\customs\),
// where Customs Studio exports to; in a development build customs/ beside the
// cartridge, else the game's folder in the working directory: customs/ for EBA,
// customs_oe2/ for Ouendan 2).
#include "eba.h"
#include "display.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

namespace eba {
namespace {


std::vector<Custom> g_catalog;
std::string g_dir;
const Custom* g_playing;
char g_difficulty = 'h';
std::map<std::string, CustomScore> g_scores;

// the autopilot: the route through the menus on the known save (Game::route)
bool g_auto;
bool g_failed;
bool g_again_asked;                     // the same custom again, from the map (Game::again)
uint64_t g_again_at;                    // its route's first frame (0: none)
uint32_t g_scene_prev;
bool g_show_after;
uint64_t g_route_shift;                 // frames the route has waited for a scene (a first-time screen)
constexpr uint64_t kRouteWait = 5400;   // the most it waits (90 s of the game's time; turbo)
constexpr uint64_t kRouteSettle = 600;  // frames a scene waited for gets before its tap (a known save gave the map ~1000)
uint64_t g_route_settle;                // frames of settling left
uint64_t g_through_at;                  // after the results: the screens before the map, tapped through from then
constexpr uint64_t kThroughMax = 36000; // at most (10 min of the game's time, a finale's credits; turbo)
const RouteStep* g_route_waiting;       // the step whose scene is being waited for
bool g_first_boot;                      // this run makes the start save (--port customs-first-boot)
SDL_Process* g_prep;                    // the first boot making the start save, in the background
struct { std::string tag; char difficulty = 'n'; bool autoplay = false; } g_pending;   // the Play that waits for it
std::string g_prep_error;

uint32_t session() { uint32_t s = game().session ? rd(game().session) : 0; return in_main(s) ? s : 0; }

// The HD song of a custom (tools/bake_customs.py: hd.wav, hd_offset, clock0,
// clock_rate, hd_gain) in place of the game's: during the song the stream's
// channels (6 and 7: PCM16 looped from 0x020BE940/0x020BF940) are muted, and
// each frame the HD audio is fed up to the song's time now, HD time =
// hd_offset + (clock - clock0) / clock_rate (in the game's own time, as its
// sound is), so its queue ends where the game's sound queue ends. It plays at
// the game's playback ratio, nudged to keep the two queues the same length; a
// jump of more than 0.1 s (a pause, a stall) seeks it again.
struct Hd {
    SDL_AudioSpec spec{};
    Uint8* buf = nullptr;
    Uint32 len = 0;
    double offset = 0, clock0 = 0, rate = 60, gain = 1;
    SDL_AudioStream* stream = nullptr;
    double fed = -1;                    // the HD time fed up to, s (-1: from the next frame's time)
    int seeks = 0;
} g_hd;

bool hd_load(const Custom& c) {
    auto get = [&](const char* k, double d) { auto it = c.extra.find(k); return it == c.extra.end() ? d : std::atof(it->second.c_str()); };
    auto it = c.extra.find("hd");
    if (it == c.extra.end()) return false;
    std::string path = (std::filesystem::path(c.dir) / it->second).string();
    if (!SDL_LoadWAV(path.c_str(), &g_hd.spec, &g_hd.buf, &g_hd.len)) {
        nds_note("eba customs: no HD audio (%s): %s", path.c_str(), SDL_GetError());
        return false;
    }
    g_hd.offset = get("hd_offset", 0);
    g_hd.clock0 = get("clock0", 0);
    g_hd.rate = get("clock_rate", 60);
    g_hd.gain = get("hd_gain", 1);
    nds_note("eba customs: HD audio %s, %d Hz, %.1f s", it->second.c_str(), g_hd.spec.freq,
             g_hd.len / (double)(SDL_AUDIO_FRAMESIZE(g_hd.spec) * g_hd.spec.freq));
    return true;
}

void hd_start() {
    if (!g_hd.buf || g_hd.stream || !g_cfg.window) return;
    g_hd.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &g_hd.spec, nullptr, nullptr);
    if (!g_hd.stream) { nds_note("eba customs: no stream for the HD audio: %s", SDL_GetError()); return; }
    SDL_SetAudioStreamGain(g_hd.stream, (float)g_hd.gain * audio_volume());   // the game's Volume too
    SDL_ResumeAudioStreamDevice(g_hd.stream);
    sound_set_muted(game().song_channels);
    g_hd.fed = -1;
}

void hd_stop() {
    if (!g_hd.stream) return;
    SDL_DestroyAudioStream(g_hd.stream);
    g_hd.stream = nullptr;
    sound_set_muted(0);
    nds_note("eba customs: HD audio stopped (%d seeks)", g_hd.seeks);
}

void hd_frame(uint32_t clock) {
    if (!g_hd.stream) return;
    int fs = SDL_AUDIO_FRAMESIZE(g_hd.spec);
    double t = g_hd.offset + (clock - g_hd.clock0) / g_hd.rate;
    if (g_hd.fed < 0 || std::fabs(g_hd.fed - t) > 0.1) {
        if (g_hd.fed >= 0) { SDL_ClearAudioStream(g_hd.stream); ++g_hd.seeks; }
        g_hd.fed = t;
        return;
    }
    if (t > g_hd.fed) {
        size_t a = (size_t)std::max(0.0, g_hd.fed * g_hd.spec.freq) * fs, b = (size_t)std::max(0.0, t * g_hd.spec.freq) * fs;
        b = std::min<size_t>(b, g_hd.len);
        if (b > a) SDL_PutAudioStreamData(g_hd.stream, g_hd.buf + a, (int)(b - a));
        g_hd.fed = t;
    }
    double hdq = SDL_GetAudioStreamQueued(g_hd.stream) / (double)(fs * g_hd.spec.freq);
    double base = audio_ratio();    // the song's time is the game's: its audio's ratio, as the game's own
    static double gap;                  // the HD queue's length over the game's, s, smoothed
    gap += (hdq - audio_queued_seconds() - gap) * 0.1;
    double nudge = std::clamp(gap * 0.25, -0.01, 0.01);  // 20 ms longer: 0.5% faster
    SDL_SetAudioStreamFrequencyRatio(g_hd.stream, (float)(base * (1 + nudge)));
    static int n;
    if (g_cfg.port.count("timing-log") && ++n % 600 == 0)
        nds_note("eba customs: HD at %.2f s, its queue %.1f ms, the game's %.1f ms, ratio %.4f, %d seeks", t, hdq * 1000,
                 audio_queued_seconds() * 1000, base * (1 + nudge), g_hd.seeks);
}

std::string base_name(const std::string& p) { return std::filesystem::path(p).filename().string(); }

void load_catalog() {
    namespace fs = std::filesystem;
    const auto& o = g_cfg.port;
    if (o.count("customs-dir")) g_dir = o.at("customs-dir");
    else {
        fs::path beside = fs::path(g_cfg.rom).parent_path() / "customs";   // a play folder's
        if (kRelease) g_dir = data_dir() + "customs";                     // the player's: with the save and settings
        else if (fs::exists(beside)) g_dir = beside.string();
        else g_dir = (fs::current_path() / game().customs_dir).string();
    }
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(g_dir, ec)) {
        fs::path ini = e.path() / "custom.ini";
        if (!fs::exists(ini)) continue;
        Custom c;
        c.tag = e.path().filename().string();
        c.dir = e.path().string();
        std::ifstream f(ini);
        std::string line;
        while (std::getline(f, line)) {
            size_t q = line.find('=');
            if (q == std::string::npos) continue;
            std::string k = line.substr(0, q), v = line.substr(q + 1);
            if (k == "title") c.title = v;
            else if (k == "artist") c.artist = v;
            else if (k == "stage") c.stage = v;
            else if (k == "stage_index") c.stage_index = std::atoi(v.c_str());
            else if (k == "replaces") c.replaces = v;
            else if (k == "file") c.files.push_back(v);
            else if (k == "difficulties") {
                std::string d;
                for (char ch : std::string("enhx"))     // in the game's order; what is not a difficulty is ignored
                    if (v.find(ch) != std::string::npos) d += ch;
                if (!d.empty()) c.difficulties = d;
            }
            else c.extra[k] = v;
        }
        g_catalog.push_back(c);
    }
    std::sort(g_catalog.begin(), g_catalog.end(), [](const Custom& a, const Custom& b) { return a.stage_index < b.stage_index; });
}

// ---- import: a custom ROM of this game, or a custom's folder ------------------------------
// A cartridge's files by name (its FNT and FAT), for the files a custom ROM changes.
struct RomFiles {
    std::vector<uint8_t> data;
    std::vector<std::string> names;                     // by file id ("" for the overlays' and others')
    std::vector<std::pair<uint32_t, uint32_t>> fat;     // start, end
    bool load(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;
        data.assign(std::istreambuf_iterator<char>(f), {});
        if (data.size() < 0x200) return false;
        auto u32 = [&](size_t o) { uint32_t v = 0; if (o + 4 <= data.size()) std::memcpy(&v, &data[o], 4); return v; };
        uint32_t fnt = u32(0x40), fat_off = u32(0x48), fat_size = u32(0x4C);
        if (fat_off + fat_size > data.size() || fnt >= data.size()) return false;
        for (uint32_t i = 0; i < fat_size / 8; ++i) fat.emplace_back(u32(fat_off + 8 * i), u32(fat_off + 8 * i + 4));
        names.assign(fat.size(), "");
        walk(fnt, 0xF000, "");
        return true;
    }
    void walk(uint32_t fnt, uint16_t dir, const std::string& prefix) {
        size_t e = fnt + (dir & 0xFFF) * 8;
        if (e + 8 > data.size()) return;
        uint32_t sub;
        uint16_t id;
        std::memcpy(&sub, &data[e], 4);
        std::memcpy(&id, &data[e + 4], 2);
        for (size_t p = fnt + sub; p < data.size() && data[p];) {
            int len = data[p] & 0x7F;
            bool is_dir = data[p] & 0x80;
            std::string name(reinterpret_cast<const char*>(&data[p + 1]), len);
            p += 1 + len;
            if (is_dir) {
                uint16_t d;
                std::memcpy(&d, &data[p], 2);
                p += 2;
                walk(fnt, d, prefix + name + "/");
            } else if (id < names.size()) {
                names[id++] = prefix + name;
            }
        }
    }
    bool same(const RomFiles& o, size_t i) const {
        auto [a, b] = fat[i];
        auto [c, d] = o.fat[i];
        return b - a == d - c && b <= data.size() && d <= o.data.size() && std::equal(&data[a], &data[a] + (b - a), &o.data[c]);
    }
};

std::string sanitize(std::string s) {
    for (char& ch : s)
        if (!std::isalnum((unsigned char)ch) && ch != '-' && ch != '_') ch = '_';
    return s.empty() ? std::string("custom") : s;
}

// The custom's files out of a custom ROM: each that differs from this cartridge's own;
// its stage from the story charts it changes; its title from the file's name.
std::string import_rom(const std::string& path) {
    namespace fs = std::filesystem;
    RomFiles stock, mine;
    if (!stock.load(g_cfg.rom) || !mine.load(path)) return "Could not read " + base_name(path) + ".";
    if (std::memcmp(&stock.data[0xC], &mine.data[0xC], 4) != 0)
        return base_name(path) + " is not a custom of " + game().title + ".";
    if (stock.fat.size() != mine.fat.size()) return base_name(path) + "'s file table is not this game's.";
    std::vector<size_t> changed;
    std::string stage;
    for (size_t i = 0; i < stock.fat.size(); ++i) {
        if (stock.names[i].empty() || stock.same(mine, i)) continue;
        changed.push_back(i);
        const std::string& n = stock.names[i];          // data/seq/SS05_n.seq_: the stem, if a story stage's
        if (n.rfind("data/seq/", 0) == 0 && n.size() > 14 && n.compare(n.size() - 5, 5, ".seq_") == 0) {
            std::string stem = n.substr(9, n.find('_', 9) - 9);
            for (const char* st : game().stages)
                if (stem == st) stage = stem;
        }
    }
    if (changed.empty()) return base_name(path) + " changes nothing of this game's files.";
    if (stage.empty()) return base_name(path) + " changes no story stage's chart (not a custom Customs can play).";
    int index = 0;
    for (size_t k = 0; k < game().stages.size(); ++k)
        if (stage == game().stages[k]) index = (int)k;
    std::string stem = fs::path(path).stem().string(), title = stem;   // "Elite Beat Agents (FG_SS05_AllStar)": FG_SS05_AllStar
    size_t a = stem.rfind('('), b = stem.rfind(')');
    if (a != std::string::npos && b != std::string::npos && b > a) title = stem.substr(a + 1, b - a - 1);
    if (title.rfind("FG_", 0) == 0) title = title.substr(3);
    if (title.size() > stage.size() + 1 && title.compare(0, stage.size() + 1, stage + "_") == 0) title = title.substr(stage.size() + 1);
    std::string tag = stage + "_" + sanitize(title);
    std::string words;                                  // AllStar_Final: All Star Final
    for (size_t i = 0; i < title.size(); ++i) {
        char ch = title[i] == '_' ? ' ' : title[i];
        if (i && std::isupper((unsigned char)ch) && std::islower((unsigned char)title[i - 1])) words += ' ';
        words += ch;
    }
    title = words;
    fs::path dir = fs::path(g_dir) / tag;
    for (int n = 2; fs::exists(dir); ++n) dir = fs::path(g_dir) / (tag + "_" + std::to_string(n));
    std::error_code ec;
    fs::create_directories(dir / "files", ec);
    if (ec) return "Could not make " + dir.string() + ".";
    std::ofstream ini(dir / "custom.ini", std::ios::binary);
    ini << "title=" << title << "\nartist=\nstage=" << stage << "\nstage_index=" << index << "\nreplaces=" << stage
        << "\nimported_from=" << path << "\n";
    for (size_t i : changed) {
        const std::string& file = stock.names[i];       // files/ holds each by its name, '/' as "__"
        std::string out;
        for (char ch : file) { if (ch == '/') out += "__"; else out += ch; }
        std::ofstream f(dir / "files" / out, std::ios::binary);
        auto [s0, s1] = mine.fat[i];
        f.write(reinterpret_cast<const char*>(&mine.data[s0]), s1 - s0);
        ini << "file=" << file << "\n";
    }
    nds_note("eba customs: imported %s: %zu files, over %s", path.c_str(), changed.size(), stage.c_str());
    return "Imported \"" + title + "\" over " + stage + " (" + std::to_string(changed.size()) +
           " files). Its title and artist are in " + (dir / "custom.ini").string() + ".";
}

// A custom's folder (a shared one: custom.ini and files/), copied in as it is.
std::string import_folder(const std::string& path) {
    namespace fs = std::filesystem;
    fs::path src(path);
    if (!fs::exists(src / "custom.ini")) return src.filename().string() + " has no custom.ini: not a custom's folder.";
    fs::path dst = fs::path(g_dir) / src.filename();
    if (fs::exists(dst)) return "A custom named " + src.filename().string() + " is already there.";
    std::error_code ec;
    fs::copy(src, dst, fs::copy_options::recursive, ec);
    if (ec) return "Could not copy " + src.string() + ": " + ec.message();
    nds_note("eba customs: imported the folder %s", path.c_str());
    return "Imported the folder " + src.filename().string() + ".";
}

std::string scores_path() { return (std::filesystem::path(g_dir) / "scores.ini").string(); }

void load_scores() {
    std::ifstream f(scores_path());
    std::string line;
    while (std::getline(f, line)) {
        size_t q = line.find('=');
        if (q == std::string::npos || line[0] == '#') continue;
        CustomScore s;
        int cleared = 0;
        if (std::sscanf(line.c_str() + q + 1, "%u %d %d %d %d %d", &s.score, &s.grades[0], &s.grades[1], &s.grades[2],
                        &s.grades[3], &cleared) >= 5) {
            s.cleared = cleared != 0;
            g_scores[line.substr(0, q)] = s;
        }
    }
}

void save_scores() {
    std::FILE* f = std::fopen(scores_path().c_str(), "w");
    if (!f) { nds_note("eba customs: cannot write %s", scores_path().c_str()); return; }
    std::fprintf(f, "# Customs (%s): the best score of each custom and difficulty (score 300 100 50 miss cleared).\n"
                    "# Its own file: the campaign's save is never touched by Customs.\n", game().id);
    for (const auto& [k, s] : g_scores)
        std::fprintf(f, "%s=%u %d %d %d %d %d\n", k.c_str(), s.score, s.grades[0], s.grades[1], s.grades[2], s.grades[3],
                     s.cleared ? 1 : 0);
    std::fclose(f);
}

std::string exe_path() {
#ifdef _WIN32
    char p[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, p, MAX_PATH);
    return std::string(p, n);
#else
    return std::filesystem::read_symlink("/proc/self/exe").string();
#endif
}

// A new run of this program with these arguments, its output to `log`: the process (nullptr: it could not start).
SDL_Process* spawn(const std::vector<std::string>& args, const std::string& log) {
    std::vector<const char*> argv;
    for (const auto& a : args) argv.push_back(a.c_str());
    argv.push_back(nullptr);
    SDL_PropertiesID p = SDL_CreateProperties();
    SDL_SetPointerProperty(p, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, (void*)argv.data());
    SDL_IOStream* out = SDL_IOFromFile(log.c_str(), "w");
    if (out) {
        SDL_SetNumberProperty(p, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_REDIRECT);
        SDL_SetPointerProperty(p, SDL_PROP_PROCESS_CREATE_STDOUT_POINTER, out);
        SDL_SetBooleanProperty(p, SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN, true);
    }
    SDL_SetBooleanProperty(p, SDL_PROP_PROCESS_CREATE_BACKGROUND_BOOLEAN, true);
    SDL_Process* proc = SDL_CreateProcessWithProperties(p);
    SDL_DestroyProperties(p);
    if (out) SDL_CloseIO(out);                      // (the child holds its own handle)
    if (!proc) nds_note("eba customs: cannot start %s: %s", args[0].c_str(), SDL_GetError());
    else nds_note("eba customs: started %s (%s)", base_name(args[0]).c_str(), log.c_str());
    return proc;
}

// A new run of this program with these arguments; this one then closes.
void relaunch(const std::vector<std::string>& args, const std::string& log) {
    if (!spawn(args, log)) return;
    SDL_Event e{};
    e.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&e);
}

// The start save: made by the game's own first boot on an empty save (Game::first_boot), headless,
// in the background. True when it is being made (the Play waits for it), false when it is there.
bool prepare_start_save(const std::string& tag, char d, bool autoplay) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path pristine = fs::path(g_dir) / "pristine.sav";
    if (fs::exists(pristine, ec) && fs::file_size(pristine, ec) > 0) return false;
    g_pending = {tag, d, autoplay};
    if (g_prep) return true;
    g_prep_error.clear();
    if (!game().first_boot.limit) { g_prep_error = "no first boot known for this game"; return true; }
    fs::create_directories(g_dir, ec);
    std::string out = (fs::path(g_dir) / "first-boot").string();
    fs::create_directories(out, ec);
    fs::remove(pristine, ec);
    std::vector<std::string> a = {exe_path(), g_cfg.rom, "--save", pristine.string(), "--out", out,
                                  "--vblanks", "9000", "--no-dump", "--port", "customs-first-boot"};
    nds_note("eba customs: making the start save: the game's first boot, headless, in %s", g_dir.c_str());
    g_prep = spawn(a, (fs::path(out) / "first-boot.log").string());
    if (!g_prep) g_prep_error = "the first boot could not start";
    return true;
}

// each frame: the first boot done, the save checked, the Play that waited for it
void poll_start_save() {
    if (!g_prep) return;
    int code = 0;
    if (!SDL_WaitProcess(g_prep, false, &code)) return;
    SDL_DestroyProcess(g_prep);
    g_prep = nullptr;
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path pristine = fs::path(g_dir) / "pristine.sav";
    bool ok = code == 0 && fs::exists(pristine, ec) && fs::file_size(pristine, ec) > 0;
    if (ok) {                                           // and it reached the main menu (a run that gave up saves too)
        std::ifstream log(fs::path(g_dir) / "first-boot" / "first-boot.log");
        std::string text((std::istreambuf_iterator<char>(log)), {});
        ok = text.find("first boot: the main menu") != std::string::npos;
    }
    if (!ok) {
        fs::remove(pristine, ec);
        g_prep_error = "the game's first boot did not make a save (its log: " +
                       (fs::path(g_dir) / "first-boot" / "first-boot.log").string() + ")";
        nds_note("eba customs: %s", g_prep_error.c_str());
        return;
    }
    nds_note("eba customs: the start save made: %s", pristine.string().c_str());
    if (g_cfg.port.count("customs-make-start-save")) { g_cfg.stop_vblanks = 1; return; }   // (a test: done)
    for (const Custom& c : g_catalog)
        if (c.tag == g_pending.tag) customs_launch(c, g_pending.difficulty, g_pending.autoplay);
}

// the custom's stage in the session (and Ouendan 2's team for it, from the game's own table)
void set_stage(uint32_t s) {
    const Game& g = game();
    g_main[(s + g.stage) & 0x3FFFFF] = (uint8_t)g_playing->stage_index;
    if (g.team_table) {
        uint32_t team = rd(g.team_table + 4 * (uint32_t)g_playing->stage_index);
        std::memcpy(g_main + ((s + g.team) & 0x3FFFFF), &team, 4);
    }
}

// the four difficulties, as the charts' letters (SSxx_e/n/h/x.seq_)
const char* difficulty_name(char d) { return game().difficulties[difficulty_index(d)]; }

}  // namespace

const std::vector<Custom>& customs() { return g_catalog; }

// The Volume option changed: the HD song follows it at once.
void customs_volume_changed() {
    if (g_hd.stream) SDL_SetAudioStreamGain(g_hd.stream, (float)g_hd.gain * audio_volume());
}

char Custom::pick(char want) const {
    if (has(want) || difficulties.empty()) return difficulties.empty() ? want : want;
    int w = difficulty_index(want), best = -1, dist = 99;
    for (char d : difficulties) {                       // (e n h x order: a tie keeps the easier)
        int n = std::abs(difficulty_index(d) - w);
        if (n < dist) { dist = n; best = d; }
    }
    return (char)best;
}
const std::string& customs_folder() { return g_dir; }
bool customs_preparing() { return g_prep != nullptr; }
const std::string& customs_prepare_error() { return g_prep_error; }

std::string customs_import(const std::string& path) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(g_dir, ec);
    std::string msg = fs::is_directory(path) ? import_folder(path) : import_rom(path);
    std::string playing = g_playing ? g_playing->tag : "";
    g_catalog.clear();                                  // the list again, with the new one
    load_catalog();
    g_playing = nullptr;
    for (const Custom& c : g_catalog)
        if (c.tag == playing) g_playing = &c;
    return msg;
}
const Custom* custom_playing() { return g_playing; }
char custom_difficulty() { return g_difficulty; }
bool customs_autopilot() { return g_auto || g_again_at || g_again_asked || g_through_at; }
bool customs_autopilot_failed() { return g_failed; }
const char* customs_difficulty_name(char d) { return difficulty_name(d); }
uint32_t scene() {
    const Game& g = game();
    if (!g.scene_root) return 0;
    uint32_t a = g.scene_root;
    for (int i = 0; i < g.scene_depth; ++i) {
        if (!in_main(a)) return 0;
        a = rd(a);
    }
    return a;
}
bool scene_is_main_menu() { return game().scenes.menu && scene() == game().scenes.menu; }
bool scene_is_song() { return game().scenes.song && scene() == game().scenes.song; }
bool scene_has_skip() {
    uint32_t s = scene();
    const Scenes& k = game().scenes;
    return s && (s == k.intro || s == k.ending);
}
bool customs_take_show_request() { bool r = g_show_after; g_show_after = false; return r; }

CustomScore custom_best(const std::string& tag, char d) {
    auto it = g_scores.find(tag + "." + d);
    return it == g_scores.end() ? CustomScore() : it->second;
}

void customs_init() {
    load_catalog();
    if (g_cfg.port.count("customs-import"))           // a test's import, as the Customs screen's
        nds_note("eba customs: %s", customs_import(g_cfg.port.at("customs-import")).c_str());
    load_scores();
    const auto& o = g_cfg.port;
    if (o.count("custom")) {
        for (const Custom& c : g_catalog)
            if (c.tag == o.at("custom")) g_playing = &c;
        if (!g_playing) nds_note("eba customs: no custom %s in %s", o.at("custom").c_str(), g_dir.c_str());
        if (o.count("difficulty") && !o.at("difficulty").empty()) g_difficulty = o.at("difficulty")[0];
        if (g_playing && !g_playing->has(g_difficulty)) {
            char d = g_playing->pick(g_difficulty);
            nds_note("eba customs: %s has no %s chart: %s instead (charted: %s)", g_playing->title.c_str(),
                     difficulty_name(g_difficulty), difficulty_name(d), g_playing->difficulties.c_str());
            g_difficulty = d;
        }
    }
    if (o.count("customs-first-boot")) {              // this run makes the start save (headless, from the Play)
        g_first_boot = true;
        nds_note("eba customs: first boot: the tutorial to the main menu, then the save");
    }
    if (o.count("customs-make-start-save")) prepare_start_save("", 'n', false);
    if (g_playing && game().route.empty()) {
        nds_note("eba customs: no way to a stage known in %s", game().id);
        g_playing = nullptr;
    }
    if (g_playing) {
        if (!o.count("hd") || o.at("hd") != "off") hd_load(*g_playing);
        g_auto = true;
        display_set_turbo(true);
        nds_note("eba customs: %s (%s, over %s), %s", g_playing->title.c_str(), g_playing->artist.c_str(),
                 g_playing->stage.c_str(), difficulty_name(g_difficulty));
    }
    nds_note("eba customs: %zu in %s (%s)", g_catalog.size(), g_dir.c_str(), game().id);
}

void customs_launch(const Custom& c, char d, bool autoplay) {
    namespace fs = std::filesystem;
    d = c.pick(d);                                      // only what it charts can be played
    set_autoplay(autoplay ? 1 : 0);
    const Game& g = game();
    if (g_playing && g_playing->tag == c.tag && g.again.ok && g.scenes.map && scene() == g.scenes.map && !g_auto) {
        g_difficulty = d;                               // this run has the custom's files: the map again, no new run
        g_again_asked = true;
        nds_note("eba customs: %s again (%s%s), from the map", c.title.c_str(), difficulty_name(d),
                 autoplay ? ", autoplay" : "");
        return;
    }
    if (prepare_start_save(c.tag, d, autoplay)) return;   // no start save yet: made first, this Play after it
    std::string session_save = (fs::path(g_dir) / "session.sav").string();
    std::error_code ec;
    fs::copy_file(fs::path(g_dir) / "pristine.sav", session_save, fs::copy_options::overwrite_existing, ec);
    if (ec) { nds_note("eba customs: no pristine.sav in %s", g_dir.c_str()); return; }
    std::string campaign = g_playing ? g_cfg.port.at("campaign-save") : g_cfg.save;
    std::vector<std::string> a = {exe_path(), g_cfg.rom, "--window", "--save", session_save, "--out", g_cfg.out,
                                  "--port", "customs-dir=" + g_dir, "--port", "custom=" + c.tag,
                                  "--port", std::string("difficulty=") + d, "--port", "campaign-save=" + campaign};
    if (display_settings().fullscreen) a.push_back("--fullscreen");   // the new run starts as this one is (F11 too)
    if (autoplay) { a.push_back("--port"); a.push_back("autoplay=1"); }
    for (const auto& f : c.files) {                     // files/ holds each by its name, '/' as "__"
        std::string name = f;
        for (size_t i = 0; (i = name.find('/', i)) != std::string::npos; i += 2) name.replace(i, 1, "__");
        a.push_back("--replace-file");
        a.push_back(f + "=" + (fs::path(c.dir) / "files" / name).string());
    }
    for (const auto& [k, v] : g_cfg.port)               // keep the other --port options (buttons-log, offset...)
        if (k != "custom" && k != "difficulty" && k != "campaign-save" && k != "customs-dir" && k != "autoplay" &&
            k != "replay") {
            a.push_back("--port");
            a.push_back(v.empty() ? k : k + "=" + v);
        }
    relaunch(a, (fs::path(g_cfg.out) / "EBA_customs.log").string());
}

void customs_back_to_campaign() {
    namespace fs = std::filesystem;
    if (!g_playing) return;
    std::vector<std::string> a = {exe_path(), g_cfg.rom, "--window", "--save", g_cfg.port.at("campaign-save"),
                                  "--out", g_cfg.out};
    if (display_settings().fullscreen) a.push_back("--fullscreen");
    for (const auto& [k, v] : g_cfg.port)
        if (k != "custom" && k != "difficulty" && k != "campaign-save" && k != "poke" && k != "autoplay" && k != "replay") {
            a.push_back("--port");
            a.push_back(v.empty() ? k : k + "=" + v);
        }
    relaunch(a, (fs::path(g_cfg.out) / "EBA.log").string());
}

// Each frame, before the input reaches the game: true while the autopilot drives it.
bool customs_frame(NdsInput& in, uint64_t vbl) {
    const Game& g = game();
    const Scenes& k = g.scenes;
    uint32_t sc = scene();
    poll_start_save();
    if (g_first_boot) {                                 // the first boot: the title, SKIP through the tutorial, the menu
        const auto& fb = g.first_boot;
        static uint64_t done_at;
        in = NdsInput();
        if (sc != g_scene_prev && g_cfg.port.count("scene-log"))
            nds_note("eba customs: scene %08X at VBlank %llu", sc, (unsigned long long)vbl);
        g_scene_prev = sc;
        if (!done_at && k.menu && sc == k.menu && vbl > fb.title) {
            done_at = vbl + 300;                        // the menu: the save is written; a moment, then stop
            g_cfg.stop_vblanks = done_at;
            nds_note("eba customs: first boot: the main menu at VBlank %llu", (unsigned long long)vbl);
        }
        if (done_at) return true;
        if (vbl >= fb.title && vbl < fb.title + (uint64_t)g.route_tap) { in.pen = true; in.x = 128; in.y = 150; }
        else if (vbl >= fb.skip_from && (vbl - fb.skip_from) % 250 < (uint64_t)g.route_tap) {
            in.pen = true; in.x = fb.skip_x; in.y = fb.skip_y;
        }
        if (vbl > fb.limit) {
            nds_note("eba customs: first boot: no main menu by VBlank %llu (scene %08X): giving up", (unsigned long long)vbl, sc);
            g_first_boot = false;
            g_cfg.stop_vblanks = vbl + 1;
        }
        return true;
    }
    bool song = k.song && sc == k.song, was_song = k.song && g_scene_prev == k.song;
    // a song of Customs mode ending: its score to the scores file, the Customs screen after the results
    static bool pending;                                // a song over, its score kept until the next scene says how
    static CustomScore now;
    static const char* unranked;                        // why its score is not kept (autoplay, a replay, an assist)
    if (g_playing && song && !was_song) { timing_reset(); hd_start(); }
    static uint32_t last_clock;                         // the song's clock at its last frame
    if (g_playing && song) { hd_frame(rd(g.clock)); last_clock = rd(g.clock); }
    if (g_playing && !song && was_song) {
        hd_stop();
        nds_note("eba customs: song over at frame %u", last_clock);   // (tools: dead time = this - the last note)
    }
    // --port customs-quit: a test run (the Studio's validation) ends 2 s after the song is over
    static bool quit_armed;
    if (g_playing && g_cfg.port.count("customs-quit") && !quit_armed && sc && (sc == k.ending || sc == k.results || sc == k.regroup)) {
        quit_armed = true;
        g_cfg.stop_vblanks = vbl + 120;
    }
    if (g_playing && was_song && !song) {
        uint32_t s = session();
        now = CustomScore();
        now.score = s && g.score ? rd(s + g.score) : 0;
        TimingStats t = timing_stats();
        std::copy(t.grades, t.grades + 4, now.grades);
        pending = true;
        unranked = autoplay() ? "autoplay" : replay_playing() ? "a replay"
                 : assists().hit_extra > 0 ? "a wider hit circle" : nullptr;
    }
    if (pending && sc && !song) {                       // the ending's comic after a clear; Regroup? after a fail
        pending = false;
        now.cleared = k.ending ? sc == k.ending : sc != k.regroup;
        std::string key = g_playing->tag + "." + g_difficulty;
        CustomScore& best = g_scores[key];
        nds_note("eba customs: %s %s: score %u (300 x%d, 100 x%d, 50 x%d, miss x%d)%s", g_playing->title.c_str(),
                 difficulty_name(g_difficulty), now.score, now.grades[0], now.grades[1], now.grades[2], now.grades[3],
                 now.cleared ? ", cleared" : "");
        if (unranked) nds_note("eba customs: played with %s: the score is not kept", unranked);
        int notes = now.grades[0] + now.grades[1] + now.grades[2] + now.grades[3];
        auto better = [&](const CustomScore& a, const CustomScore& b) {   // the score, else (none known) the grades
            if (a.score != b.score) return a.score > b.score;
            return a.grades[0] * 3 + a.grades[1] * 2 + a.grades[2] > b.grades[0] * 3 + b.grades[1] * 2 + b.grades[2];
        };
        if (notes && !unranked && (better(now, best) || (now.cleared && !best.cleared))) {
            best = now;
            save_scores();
        }
    }
    // After the results, Customs mode goes back to the map (and the Customs screen); what the story puts
    // between them (Ouendan 2's rank-up 020ADAC8, a stage's story screens, the credits after a finale)
    // is not the custom's: tapped through at turbo speed behind the loading screen.
    static bool results_seen;
    if (g_playing && k.results && sc == k.results) results_seen = true;
    if (g_playing && results_seen && sc && sc != k.results) {
        results_seen = false;
        if (sc != k.map && sc != k.title && sc != k.menu) {
            g_through_at = vbl;
            display_set_turbo(true);
            nds_note("eba customs: scene %08X after the results: through it to the map", sc);
        }
    }
    if (g_through_at) {
        bool done = sc == k.map || sc == k.title || sc == k.menu;
        if (done || vbl - g_through_at > kThroughMax) {
            if (!done) nds_note("eba customs: no map after %llu frames (scene %08X): stopped", (unsigned long long)(vbl - g_through_at), sc);
            g_through_at = 0;
            display_set_turbo(false);
            if (sc == k.title || sc == k.menu) g_show_after = true;   // (the map's own check below covers the map)
        } else {
            in = NdsInput();
            uint64_t t = (vbl - g_through_at) % 60;         // "next" at the lower right, then A
            if (t < 9) { in.pen = true; in.x = 236; in.y = 183; }
            else if (t >= 30 && t < 39) in.keys |= 1u;   // (A: KEYINPUT bit 0)
            g_scene_prev = sc;
            return true;
        }
    }
    if (g_playing && k.map && g_scene_prev != k.map && sc == k.map && !g_auto) g_show_after = true;
    if (g_cfg.port.count("clock-log") && song && vbl % 30 == 0)   // tools/bake_customs.py's measure
        nds_note("eba clock %llu %u", (unsigned long long)vbl, rd(g.clock));
    if (sc != g_scene_prev && (g_cfg.port.count("scene-log") || (g_playing && g_cfg.port.count("timing-log"))))
        nds_note("eba customs: scene %08X at VBlank %llu", sc, (unsigned long long)vbl);
    g_scene_prev = sc;
    if (g_playing && g_cfg.port.count("customs-again") && vbl == std::strtoull(g_cfg.port.at("customs-again").c_str(), nullptr, 10))
        customs_launch(*g_playing, g_difficulty, autoplay() != 0);   // a test's Play once more, at that VBlank
    if (g_again_asked) {                                // the same custom again: the map's card, its stage set, OK
        g_again_asked = false;
        g_again_at = vbl + 1;
        display_set_turbo(true);
    }
    if (g_again_at) {
        const auto& a = g.again;
        in = NdsInput();
        if (vbl < g_again_at) return true;
        uint64_t t = vbl - g_again_at;
        if ((t == 0 || t == (uint64_t)a.ok) && sc != k.map) {
            nds_note("eba customs: the map's route found scene %08X, not the map: stopped", sc);
            g_again_at = 0;
            g_failed = true;
            display_set_turbo(false);
            return false;
        }
        const uint64_t hold = 9;                        // a script's tap, as the map was learned with
        if (t < hold) { in.pen = true; in.x = a.x; in.y = a.y; }
        if (t >= (uint64_t)a.ok && t < a.ok + hold) { in.pen = true; in.x = a.ok_x; in.y = a.ok_y; }
        if (t == (uint64_t)a.card) {
            uint32_t s = session();
            if (s) {
                uint32_t d = (uint32_t)difficulty_index(g_difficulty);
                set_stage(s);
                std::memcpy(g_main + (s & 0x3FFFFF), &d, 4);
            }
        }
        if (t >= a.ok + hold + 1) {
            g_again_at = 0;
            display_set_turbo(false);
            nds_note("eba customs: on its way into %s again", g_playing->stage.c_str());
        }
        return true;
    }
    if (!g_auto) return false;

    // the autopilot: the route on the known save, each step checked against the scene it needs; a
    // scene not there yet (a first-time screen before the map on a new save) is waited for, the
    // rest of the route shifted by the wait
    in = NdsInput();
    if (g_route_settle) { --g_route_settle; ++g_route_shift; return true; }   // the scene waited for, settling
    uint64_t t = vbl - g_route_shift;
    for (const RouteStep& st : g.route) {
        if (t < st.at || t >= st.at + g.route_tap) continue;
        if (t == st.at && sc != st.scene) {
            if (g_route_shift < kRouteWait) {
                if (g_route_waiting != &st)
                    nds_note("eba customs: the route found scene %08X at VBlank %llu, not %08X: waiting for it", sc,
                             (unsigned long long)vbl, st.scene);
                g_route_waiting = &st;
                ++g_route_shift;
                return true;
            }
            nds_note("eba customs: the route found scene %08X at VBlank %llu, not %08X: autopilot stopped", sc,
                     (unsigned long long)vbl, st.scene);
            g_auto = false;
            g_failed = true;
            display_set_turbo(false);
            return false;
        }
        if (t == st.at && g_route_waiting == &st) {   // found: it settles before the tap
            nds_note("eba customs: scene %08X at VBlank %llu, after a %llu-frame wait: %llu frames to settle", sc,
                     (unsigned long long)vbl, (unsigned long long)g_route_shift, (unsigned long long)kRouteSettle);
            g_route_waiting = nullptr;
            g_route_settle = kRouteSettle;
            ++g_route_shift;
            return true;
        }
        in.pen = true;
        in.x = st.x;
        in.y = st.y ? st.y : g.difficulty_rows[difficulty_index(g_difficulty)];
    }
    if (t >= g.card_from && t <= g.card_to) {           // the card open: the custom's stage instead, and its difficulty
        uint32_t s = session();
        if (s) {
            uint32_t d = (uint32_t)difficulty_index(g_difficulty);
            if (t == g.card_from && rd(s) != d) nds_note("eba customs: difficulty %u on the card, set to %u", rd(s), d);
            set_stage(s);
            std::memcpy(g_main + (s & 0x3FFFFF), &d, 4);
        }
    }
    if (t >= g.hand_over) {
        g_auto = false;
        display_set_turbo(false);
        nds_note("eba customs: on its way into %s", g_playing->stage.c_str());
    }
    return true;
}

}  // namespace eba
