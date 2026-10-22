// EBA's Customs: songs charted over the story's stages, played as a mode of
// their own. Each custom is a folder (tools/extract_customs.py): the files it
// changes on the cartridge and custom.ini (its title, artist, the stage it
// replaces and that stage's index). Customs mode is a run of its own, started
// by this one: the stock cartridge with the custom's files (--replace-file),
// a fresh copy of a known save (customs/pristine.sav, never the campaign's),
// and an autopilot that takes the game from its boot to the custom's stage at
// turbo speed behind a loading screen: the title, Single Player, the
// difficulty, a stage card (The Anthem's, the one the known save shows
// first), the stage's index set in the session, OK. Its scores go to
// customs/scores.ini, a file of their own beside the campaign's save, which
// Customs mode never opens.
//
// What it reads and writes of the game (the stock USA ARM9, 2026-10-22):
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
// customs-dir=PATH (where the customs are:
// by default customs/ beside the cartridge, else in the working directory).
#include "eba.h"
#include "display.h"
#include <SDL3/SDL.h>
#include <algorithm>
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

constexpr uint32_t kSceneRoot = 0x020CD1FC, kSession = 0x020CD248;
constexpr uint32_t kSceneTitle = 0x020B3D78, kSceneMenu = 0x020B7288, kSceneDifficulty = 0x020B736C,
                   kSceneMap = 0x020B7528, kSceneIntro = 0x020B64F0, kSceneSong = 0x020B65A0,
                   kSceneEnding = 0x020B6448;

std::vector<Custom> g_catalog;
std::string g_dir;
const Custom* g_playing;
char g_difficulty = 'h';
std::map<std::string, CustomScore> g_scores;

// the autopilot: the route through the menus on the known save
struct Step { uint64_t at; int x, y; uint32_t scene; };
bool g_auto;
bool g_failed;
uint32_t g_scene_prev;
bool g_show_after;

uint32_t session() { uint32_t s = rd(kSession); return in_main(s) ? s : 0; }

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
    SDL_SetAudioStreamGain(g_hd.stream, (float)g_hd.gain);
    SDL_ResumeAudioStreamDevice(g_hd.stream);
    sound_set_muted(0xC0);
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
        fs::path beside = fs::path(g_cfg.rom).parent_path() / "customs";
        g_dir = fs::exists(beside) ? beside.string() : (fs::current_path() / "customs").string();
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
            else c.extra[k] = v;
        }
        g_catalog.push_back(c);
    }
    std::sort(g_catalog.begin(), g_catalog.end(), [](const Custom& a, const Custom& b) { return a.stage_index < b.stage_index; });
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
    std::fprintf(f, "# EBA Customs: the best score of each custom and difficulty (score 300 100 50 miss cleared).\n"
                    "# Its own file: the campaign's save is never touched by Customs.\n");
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

// A new run of this program with these arguments; this one then closes.
void relaunch(const std::vector<std::string>& args, const std::string& log) {
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
    if (!proc) {
        nds_note("eba customs: cannot start %s: %s", args[0].c_str(), SDL_GetError());
        if (out) SDL_CloseIO(out);
        return;
    }
    nds_note("eba customs: started %s (%s)", base_name(args[0]).c_str(), log.c_str());
    SDL_Event e{};
    e.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&e);
}

// EBA's four difficulties and the chart each plays (SSxx_e/n/h/x.seq_)
const char* difficulty_name(char d) {
    switch (d) {
    case 'e': return "Breezin'";
    case 'n': return "Cruisin'";
    case 'x': return "Hard Rock";
    default: return "Sweatin'";
    }
}

// the difficulty screen's rows (lower screen), top to bottom: Hard Rock, Sweatin', Cruisin', Breezin'
int difficulty_row(char d) {
    switch (d) {
    case 'e': return 153;
    case 'n': return 119;
    case 'x': return 45;
    default: return 78;
    }
}

// the session's difficulty (its first word): 0 Breezin' .. 3 Hard Rock. The tap on Sweatin's row,
// already the one chosen on the known save, moves it to Hard Rock: the autopilot sets it instead.
uint32_t difficulty_value(char d) {
    switch (d) {
    case 'e': return 0;
    case 'n': return 1;
    case 'x': return 3;
    default: return 2;
    }
}

}  // namespace

const std::vector<Custom>& customs() { return g_catalog; }
const Custom* custom_playing() { return g_playing; }
char custom_difficulty() { return g_difficulty; }
bool customs_autopilot() { return g_auto; }
bool customs_autopilot_failed() { return g_failed; }
const char* customs_difficulty_name(char d) { return difficulty_name(d); }
uint32_t scene() {
    uint32_t a = rd(kSceneRoot);
    if (!in_main(a)) return 0;
    uint32_t b = rd(a);
    return in_main(b) ? rd(b) : 0;
}
bool scene_is_main_menu() { return scene() == kSceneMenu; }
bool scene_is_song() { return scene() == kSceneSong; }
bool scene_has_skip() { uint32_t s = scene(); return s == kSceneIntro || s == kSceneEnding; }
bool customs_take_show_request() { bool r = g_show_after; g_show_after = false; return r; }

CustomScore custom_best(const std::string& tag, char d) {
    auto it = g_scores.find(tag + "." + d);
    return it == g_scores.end() ? CustomScore() : it->second;
}

void customs_init() {
    load_catalog();
    load_scores();
    const auto& o = g_cfg.port;
    if (o.count("custom")) {
        for (const Custom& c : g_catalog)
            if (c.tag == o.at("custom")) g_playing = &c;
        if (!g_playing) nds_note("eba customs: no custom %s in %s", o.at("custom").c_str(), g_dir.c_str());
        if (o.count("difficulty") && !o.at("difficulty").empty()) g_difficulty = o.at("difficulty")[0];
    }
    if (g_playing) {
        if (!o.count("hd") || o.at("hd") != "off") hd_load(*g_playing);
        g_auto = true;
        display_set_turbo(true);
        nds_note("eba customs: %s (%s, over %s), %s", g_playing->title.c_str(), g_playing->artist.c_str(),
                 g_playing->stage.c_str(), difficulty_name(g_difficulty));
    }
    nds_note("eba customs: %zu in %s", g_catalog.size(), g_dir.c_str());
}

void customs_launch(const Custom& c, char d) {
    namespace fs = std::filesystem;
    std::string session_save = (fs::path(g_dir) / "session.sav").string();
    std::error_code ec;
    fs::copy_file(fs::path(g_dir) / "pristine.sav", session_save, fs::copy_options::overwrite_existing, ec);
    if (ec) { nds_note("eba customs: no pristine.sav in %s", g_dir.c_str()); return; }
    std::string campaign = g_playing ? g_cfg.port.at("campaign-save") : g_cfg.save;
    std::vector<std::string> a = {exe_path(), g_cfg.rom, "--window", "--save", session_save, "--out", g_cfg.out,
                                  "--port", "customs-dir=" + g_dir, "--port", "custom=" + c.tag,
                                  "--port", std::string("difficulty=") + d, "--port", "campaign-save=" + campaign};
    if (display_settings().fullscreen) a.push_back("--fullscreen");   // the new run starts as this one is (F11 too)
    for (const auto& f : c.files) {                     // files/ holds each by its name, '/' as "__"
        std::string name = f;
        for (size_t i = 0; (i = name.find('/', i)) != std::string::npos; i += 2) name.replace(i, 1, "__");
        a.push_back("--replace-file");
        a.push_back(f + "=" + (fs::path(c.dir) / "files" / name).string());
    }
    for (const auto& [k, v] : g_cfg.port)               // keep the other --port options (buttons-log, offset...)
        if (k != "custom" && k != "difficulty" && k != "campaign-save" && k != "customs-dir") {
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
        if (k != "custom" && k != "difficulty" && k != "campaign-save" && k != "poke") {
            a.push_back("--port");
            a.push_back(v.empty() ? k : k + "=" + v);
        }
    relaunch(a, (fs::path(g_cfg.out) / "EBA.log").string());
}

// Each frame, before the input reaches the game: true while the autopilot drives it.
bool customs_frame(NdsInput& in, uint64_t vbl) {
    uint32_t sc = scene();
    // a song of Customs mode ending: its score to the scores file, the Customs screen after the results
    static bool pending;                                // a song over, its score kept until the next scene says how
    static CustomScore now;
    if (g_playing && sc == kSceneSong && g_scene_prev != kSceneSong) { timing_reset(); hd_start(); }
    if (g_playing && sc == kSceneSong) hd_frame(rd(0x020CD390));
    if (g_playing && sc != kSceneSong && g_scene_prev == kSceneSong) hd_stop();
    if (g_playing && g_scene_prev == kSceneSong && sc != kSceneSong) {
        uint32_t s = session();
        now = CustomScore();
        now.score = s ? rd(s + 0xF8) : 0;
        TimingStats t = timing_stats();
        std::copy(t.grades, t.grades + 4, now.grades);
        pending = true;
    }
    if (pending && sc && sc != kSceneSong) {            // the ending's comic after a clear; Regroup? after a fail
        pending = false;
        now.cleared = sc == kSceneEnding;
        std::string key = g_playing->tag + "." + g_difficulty;
        CustomScore& best = g_scores[key];
        nds_note("eba customs: %s %s: score %u (300 x%d, 100 x%d, 50 x%d, miss x%d)%s", g_playing->title.c_str(),
                 difficulty_name(g_difficulty), now.score, now.grades[0], now.grades[1], now.grades[2], now.grades[3],
                 now.cleared ? ", cleared" : "");
        if (now.score > best.score || (now.cleared && !best.cleared)) {
            best = now;
            save_scores();
        }
    }
    if (g_playing && g_scene_prev != kSceneMap && sc == kSceneMap && !g_auto) g_show_after = true;
    if (g_cfg.port.count("clock-log") && sc == kSceneSong && vbl % 30 == 0)   // tools/bake_customs.py's measure
        nds_note("eba clock %llu %u", (unsigned long long)vbl, rd(0x020CD390));
    if (g_playing && sc != g_scene_prev && g_cfg.port.count("timing-log"))
        nds_note("eba customs: scene %08X at VBlank %llu", sc, (unsigned long long)vbl);
    g_scene_prev = sc;
    if (!g_auto) return false;

    // the autopilot: the route on the known save, each step checked against the scene it needs
    const Step route[] = {
        {950, 128, 150, kSceneTitle},
        {1300, 128, 66, kSceneMenu},
        {2400, 128, difficulty_row(g_difficulty), kSceneDifficulty},
        {2450, 236, 178, kSceneDifficulty},
        {3650, 126, 103, kSceneMap},
        {4300, 236, 178, kSceneMap},
    };
    in = NdsInput();
    for (const Step& st : route) {
        if (vbl < st.at || vbl >= st.at + 3) continue;
        if (vbl == st.at && sc != st.scene) {
            nds_note("eba customs: the route found scene %08X at VBlank %llu, not %08X: autopilot stopped", sc,
                     (unsigned long long)vbl, st.scene);
            g_auto = false;
            g_failed = true;
            display_set_turbo(false);
            return false;
        }
        in.pen = true;
        in.x = st.x;
        in.y = st.y;
    }
    if (vbl == 3662) {                                  // the card opening: the custom's stage instead
        uint32_t s = session();
        if (s) {
            g_main[(s + 8) & 0x3FFFFF] = (uint8_t)g_playing->stage_index;
            uint32_t d = difficulty_value(g_difficulty);
            if (rd(s) != d) nds_note("eba customs: difficulty %u on the card, set to %u", rd(s), d);
            std::memcpy(g_main + (s & 0x3FFFFF), &d, 4);
        }
    }
    if (vbl >= 4310) {
        g_auto = false;
        display_set_turbo(false);
        nds_note("eba customs: on its way into %s", g_playing->stage.c_str());
    }
    return true;
}

}  // namespace eba
