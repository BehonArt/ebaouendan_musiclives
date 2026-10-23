// The game's own play, from the decomp (eba-decomp, docs/DECOMP_FINDINGS.md):
//
// - Autoplay: the judge (Note::Judge) asks the session's autoplay field after
//   its touch test; nonzero, a note is judged on its own frame as 300, 100 or
//   50 (1, 2, 3) or at random (4). No input is played at all.
// - Replays: every judged tap is recorded into the session's replay Array
//   ({data, capacity, count}, 8-byte events: u16 frame judged, u16 the note's
//   time, u8 kind, s8 result, u8 a new group); with the replaying field set,
//   the judge returns each event's result once its frame comes. A song's run
//   is saved to a file as it ends (a window's: <out>/replays/); --port
//   replay=FILE plays one back into the buffer the game makes for its own.
// - No-Fail (EBA): the life gauge's switch (0x020B40C8, set by the song's setup) held
//   at 0 in a song: no hit fills it, no miss drains it; its bar can be hidden.
// - Assists (EBA): the frames a note shows before its beat (0x020B4704) and the
//   hit test's extra radius (0x020B22E0), .data the game sets for each stage
//   (75 to 100 frames seen): scaled and widened from the game's own value.
//
// Options (ndsrun --port): autoplay=N; replay=FILE; replay-save=FILE (a
// headless run's save); approach=PERCENT; hit-extra=PX; no-fail[=off];
// hide-life[=off].
#include "eba.h"
#include "display.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <vector>

namespace eba {
namespace {

int g_autoplay;                         // the value written to the session (0: left alone)
bool g_autoplay_set;                    // written once: cleared again when turned off
std::vector<uint8_t> g_replay;          // the events to play back (8 bytes each)
uint32_t g_replay_stage = ~0u, g_replay_diff = ~0u;
bool g_injected, g_was_song;
std::string g_last;                     // the last song's replay file

constexpr char kMagic[8] = {'N', 'D', 'S', 'R', 'P', 'L', '1', 0};

uint32_t session() {
    uint32_t s = game().session ? rd(game().session) : 0;
    return in_main(s) ? s : 0;
}

void wr(uint32_t a, uint32_t v) { std::memcpy(g_main + (a & 0x3FFFFFu), &v, 4); }

bool load_replay(const std::string& path) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) { nds_note("eba replay: cannot open %s", path.c_str()); return false; }
    char magic[8], id[4];
    uint32_t h[4];
    bool ok = std::fread(magic, 8, 1, f) == 1 && std::memcmp(magic, kMagic, 8) == 0 && std::fread(id, 4, 1, f) == 1 &&
              std::fread(h, 4, 4, f) == 4;
    if (ok && std::memcmp(id, game().id, std::strlen(game().id)) != 0) {
        nds_note("eba replay: %s is %.4s's, not %s's", path.c_str(), id, game().id);
        ok = false;
    }
    if (ok) {
        g_replay.resize(h[2] * 8);
        ok = std::fread(g_replay.data(), 8, h[2], f) == h[2];
        g_replay_stage = h[0];
        g_replay_diff = h[1];
        if (ok) nds_note("eba replay: %s, %u events (stage %u, difficulty %u, score %u)", path.c_str(), h[2], h[0], h[1], h[3]);
    }
    std::fclose(f);
    if (!ok) g_replay.clear();
    return ok;
}

void save_replay(uint32_t s) {
    const Game& g = game();
    uint32_t arr = s + g.replay, data = rd(arr), n = rd(arr + 8);
    if (!g.replay || !in_main(data) || !n || n > 8192) return;
    std::string path;
    const auto& o = g_cfg.port;
    if (o.count("replay-save")) path = o.at("replay-save");
    else if (g_cfg.window) {                        // a window's run: <out>/replays/GAME_stage_difficulty_time.rep
        namespace fs = std::filesystem;
        fs::path dir = fs::path(g_cfg.out) / "replays";
        std::error_code ec;
        fs::create_directories(dir, ec);
        char t[32];
        std::time_t now = std::time(nullptr);
        std::strftime(t, sizeof t, "%Y%m%d-%H%M%S", std::localtime(&now));
        const Custom* c = custom_playing();
        char name[160];
        std::snprintf(name, sizeof name, "%s_%s_%u_%s.rep", g.id, c ? c->tag.c_str() : "stage", rd(s + g.stage), t);
        path = (dir / name).string();
    } else return;
    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) { nds_note("eba replay: cannot write %s", path.c_str()); return; }
    char id[4] = {};
    std::memcpy(id, g.id, std::min<size_t>(4, std::strlen(g.id)));
    uint32_t h[4] = {rd(s + g.stage), rd(s), n, g.score ? rd(s + g.score) : 0};
    std::fwrite(kMagic, 8, 1, f);
    std::fwrite(id, 4, 1, f);
    std::fwrite(h, 4, 4, f);
    std::fwrite(g_main + (data & 0x3FFFFFu), 8, n, f);
    std::fclose(f);
    g_last = path;
    nds_note("eba replay: %u events saved to %s", n, path.c_str());
}

}  // namespace

Assists& assists() {
    static Assists a;
    return a;
}
void set_autoplay(int n) { g_autoplay = std::clamp(n, 0, 4); }
int autoplay() { return g_autoplay; }
bool replay_playing() { return g_injected; }
std::string last_replay() { return g_last; }

void play_options() {
    const auto& o = g_cfg.port;
    if (o.count("autoplay")) set_autoplay(o.at("autoplay").empty() ? 1 : std::atoi(o.at("autoplay").c_str()));
    if (o.count("replay")) load_replay(o.at("replay"));
    if (o.count("approach")) assists().approach_pct = std::atoi(o.at("approach").c_str());
    if (o.count("hit-extra")) assists().hit_extra = std::atoi(o.at("hit-extra").c_str());
    if (o.count("no-fail")) assists().no_fail = o.at("no-fail") != "off";
    if (o.count("hide-life")) assists().hide_life = o.at("hide-life") != "off";
    if (g_autoplay) nds_note("eba play: the game's own autoplay (%d)", g_autoplay);
}

void play_frame(uint64_t) {
    const Game& g = game();
    // the assists, over the value the game sets for each stage: a value other than the one
    // written last is the game's new own; off, its own is put back once
    struct Knob { uint32_t base = 0, written = 0; bool on = false; };
    static Knob app, rad;
    auto knob = [](Knob& k, uint32_t addr, bool on, auto scaled) {
        if (!addr) return;
        uint32_t v = rd(addr);
        if (!k.on || v != k.written) k.base = v;
        if (on) { k.written = scaled(k.base); wr(addr, k.written); k.on = true; }
        else if (k.on) { wr(addr, k.base); k.on = false; }
    };
    int pct = assists().approach_pct, extra = assists().hit_extra;
    knob(app, g.approach, pct > 0 && pct != 100, [&](uint32_t b) { return (uint32_t)std::max(1u, b * (uint32_t)pct / 100); });
    knob(rad, g.hit_radius, extra > 0, [&](uint32_t b) { return b + (uint32_t)extra; });

    uint32_t s = session();
    bool song = scene_is_song();
    // No-Fail: in a song, the gauge's switch off (the song's setup turns it on); off again
    // mid-song, on. Its bar hidden meanwhile, if asked.
    static bool gauge_off, life_hidden;
    bool no_fail = g.gauge_on && assists().no_fail && song;
    if (no_fail) { wr(g.gauge_on, 0); gauge_off = true; }
    else if (gauge_off) { if (song) wr(g.gauge_on, 1); gauge_off = false; }
    bool hide = no_fail && assists().hide_life && !g.life_bar.empty();
    if (hide != life_hidden) {
        display_set_texture_hidden(hide ? g.life_bar : std::vector<uint64_t>{});
        life_hidden = hide;
    }
    if (song && !g_was_song) g_injected = false;    // (it says the last song was a replay until the next)
    if (s && g.autoplay) {
        if (song && g_autoplay) { wr(s + g.autoplay, (uint32_t)g_autoplay); g_autoplay_set = true; }
        else if (g_autoplay_set && !g_autoplay) { wr(s + g.autoplay, 0); g_autoplay_set = false; }
    }
    // a replay: into the game's own buffer as it makes it, before the first note
    if (s && song && g.replay && !g_replay.empty() && !g_injected) {
        uint32_t arr = s + g.replay, data = rd(arr), cap = rd(arr + 4), count = rd(arr + 8);
        uint32_t n = (uint32_t)(g_replay.size() / 8);
        if (in_main(data) && cap && count == 0) {
            if (rd(s + g.stage) != g_replay_stage || rd(s) != g_replay_diff)
                nds_note("eba replay: recorded on stage %u difficulty %u, this is %u, %u", g_replay_stage, g_replay_diff,
                         rd(s + g.stage), rd(s));
            if (n > cap) {
                nds_note("eba replay: %u events, the game's buffer holds %u: not played", n, cap);
                g_replay.clear();
            } else {
                std::memcpy(g_main + (data & 0x3FFFFFu), g_replay.data(), g_replay.size());
                wr(arr + 8, n);
                wr(arr + 0xC, 0);
                wr(s + g.replaying, 1);
                g_injected = true;
                nds_note("eba replay: playing back %u events", n);
            }
        }
    }
    if (s && g_was_song && !song) {                 // the song over: its run saved; a replay's flag off
        if (g_injected) {
            wr(s + g.replaying, 0);
            g_replay.clear();
        } else {
            save_replay(s);
        }
    }
    g_was_song = song;
}

}  // namespace eba
