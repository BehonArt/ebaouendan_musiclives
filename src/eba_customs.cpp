// Customs: songs charted over the story's stages, played as a mode of
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
// customs-dir=PATH (where the customs are:
// by default customs/ beside the cartridge, else the game's folder in the working
// directory: customs/ for EBA, customs_oe2/ for Ouendan 2).
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
    SDL_SetAudioStreamGain(g_hd.stream, (float)g_hd.gain);
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
        g_dir = fs::exists(beside) ? beside.string() : (fs::current_path() / game().customs_dir).string();
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

// the four difficulties, as the charts' letters (SSxx_e/n/h/x.seq_)
const char* difficulty_name(char d) { return game().difficulties[difficulty_index(d)]; }

}  // namespace

const std::vector<Custom>& customs() { return g_catalog; }
const Custom* custom_playing() { return g_playing; }
char custom_difficulty() { return g_difficulty; }
bool customs_autopilot() { return g_auto || g_again_at || g_again_asked; }
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
    load_scores();
    const auto& o = g_cfg.port;
    if (o.count("custom")) {
        for (const Custom& c : g_catalog)
            if (c.tag == o.at("custom")) g_playing = &c;
        if (!g_playing) nds_note("eba customs: no custom %s in %s", o.at("custom").c_str(), g_dir.c_str());
        if (o.count("difficulty") && !o.at("difficulty").empty()) g_difficulty = o.at("difficulty")[0];
    }
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
    set_autoplay(autoplay ? 1 : 0);
    const Game& g = game();
    if (g_playing && g_playing->tag == c.tag && g.again.ok && g.scenes.map && scene() == g.scenes.map && !g_auto) {
        g_difficulty = d;                               // this run has the custom's files: the map again, no new run
        g_again_asked = true;
        nds_note("eba customs: %s again (%s%s), from the map", c.title.c_str(), difficulty_name(d),
                 autoplay ? ", autoplay" : "");
        return;
    }
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
    bool song = k.song && sc == k.song, was_song = k.song && g_scene_prev == k.song;
    // a song of Customs mode ending: its score to the scores file, the Customs screen after the results
    static bool pending;                                // a song over, its score kept until the next scene says how
    static CustomScore now;
    static const char* unranked;                        // why its score is not kept (autoplay, a replay, an assist)
    if (g_playing && song && !was_song) { timing_reset(); hd_start(); }
    if (g_playing && song) hd_frame(rd(g.clock));
    if (g_playing && !song && was_song) hd_stop();
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
                g_main[(s + 8) & 0x3FFFFF] = (uint8_t)g_playing->stage_index;
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

    // the autopilot: the route on the known save, each step checked against the scene it needs
    in = NdsInput();
    for (const RouteStep& st : g.route) {
        if (vbl < st.at || vbl >= st.at + g.route_tap) continue;
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
        in.y = st.y ? st.y : g.difficulty_rows[difficulty_index(g_difficulty)];
    }
    if (vbl >= g.card_from && vbl <= g.card_to) {       // the card open: the custom's stage instead, and its difficulty
        uint32_t s = session();
        if (s) {
            uint32_t d = (uint32_t)difficulty_index(g_difficulty);
            if (vbl == g.card_from && rd(s) != d) nds_note("eba customs: difficulty %u on the card, set to %u", rd(s), d);
            g_main[(s + 8) & 0x3FFFFF] = (uint8_t)g_playing->stage_index;
            std::memcpy(g_main + (s & 0x3FFFFF), &d, 4);
        }
    }
    if (vbl >= g.hand_over) {
        g_auto = false;
        display_set_turbo(false);
        nds_note("eba customs: on its way into %s", g_playing->stage.c_str());
    }
    return true;
}

}  // namespace eba
