// Recomp Achievements: RecompLauncher's own achievements for the three games,
// made for it (not RetroAchievements' set). The game fires named events; the
// list RecompLauncher hosts (docs/recomp-achievements/, made by
// tools/make_rla_lists.py) maps each to an achievement, its title and points.
// The SDK is third_party/rl_achievements: started without RecompLauncher (no
// RL_SESSION), its calls do nothing, so every build has it.
//
// The events come from the game's own progress, as its save keeps it in RAM
// (games.cpp's Progress: each stage's best rank and score per difficulty, the
// stage's state). What the progress meets (rla_events) is taken as each song
// starts; after the song, each event it meets that it did not is fired. So a
// save loaded at the start fires nothing by itself, and what a song played
// with autoplay or a replay brought is never fired (the next song starts from
// it). Re-clears count: a song cleared live fires its clear (and on the finale
// its credits) even if the save had it cleared already, and an S got live fires
// every S milestone the save's S ranks reach; the map's tiers, the overall
// rank and the bonus songs fire as they first happen. Any input counts: the
// stylus, Tap mode, a gamepad, the assists. Customs (a run of their own, on
// their own save) never connect.
//
// The events (P the game's: eba, oe1, oe2; D e n h x; STAGE the chart's stem, lower case):
//   P_clear_STAGE    the stage cleared, any difficulty
//   P_tier_D_N       the map's tier N (1..7) reached on D, the game's own table: each is a
//                    "New songs" (EBA and Ouendan 2's 7: the finale's second part; Ouendan 1's: all cleared)
//   P_credits_D      the finale cleared on D (the credits; after n and h the next difficulty opens)
//   P_rank_N         the overall rank N (every record's best score summed, the game's own table)
//   P_bonus_STAGE    a bonus song unlocked (EBA, Ouendan 2)
//   P_s_N, P_s_all   N S ranks (1 5 10 20, a stage on a difficulty each), an S on every one
//
// Options (ndsrun --port): rla-print=VBLANK (the events the progress meets then, in the log:
// tests); rla-quiet (no summary at each song's start); rla-stage=N (a test: the story's stage N
// played whatever card is picked, set in the session during the opening comic, as Customs do).
#include "eba.h"
#include "rl_achievements.h"
#include <cctype>
#include <climits>
#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace eba {
namespace {

bool g_on;                      // rla_init ran (not Customs)
bool g_was_song, g_counts, g_armed, g_loaded;
const char* g_why = "";         // why this song fires nothing
std::set<std::string> g_seen;   // what the progress met as the song started (and what was fired since)
std::set<std::string> g_sent;   // what was fired since the song started
uint64_t g_print = ~0ull;
int g_test_stage = -1;          // rla-stage=N
// the song played, as the session has it: its stage, difficulty and score
int g_stage = -1, g_diff = -1;
int32_t g_score = 0;
bool g_after;                   // the scene after the song is known
bool g_cleared;                 // it was cleared: the ending comic or the results came next (not Regroup)

const char* launcher_id() {     // the game's id in RecompLauncher (recomplauncher.json's games)
    std::string e = game().progress.event;
    return e == "oe1" ? "ouendan1" : e == "oe2" ? "ouendan2" : "elitebeatagents";
}

uint8_t byte(uint32_t a) { return g_main[a & 0x3FFFFFu]; }

uint32_t base() {
    const Game::Progress& p = game().progress;
    if (!p.ptr) return 0;
    uint32_t b = rd(p.ptr);
    return in_main(b) ? b : 0;
}

struct Record { bool cleared = false, s = false; int64_t score = 0; int state = -1; };

Record record(uint32_t b, int stage, int d) {
    const Game::Progress& p = game().progress;
    uint32_t r = b + 0x10 + (uint32_t)stage * p.stage_stride + (uint32_t)d * p.record;
    Record x;
    if (p.three) {                                  // three best scores, each with its rank
        int32_t best = (int32_t)rd(r);
        x.cleared = best >= 0;
        x.score = best > 0 ? best : 0;
        for (int i = 0; i < 3; ++i)
            if ((int32_t)rd(r + 4 * i) >= 0 && byte(r + 0x1F + i) == 0) x.s = true;
    } else {                                        // the best rank, the best score
        int32_t rank = (int32_t)rd(r), score = (int32_t)rd(r + 4);
        x.cleared = rank != INT_MAX;
        x.s = x.cleared && rank == 0;
        x.score = score > 0 ? score : 0;
    }
    if (p.state >= 0) x.state = byte(r + (uint32_t)p.state);
    return x;
}

// the song just played got an S: its record holds this run's score with an S
bool live_s(uint32_t b, int stage, int d, int32_t score) {
    const Game::Progress& p = game().progress;
    uint32_t r = b + 0x10 + (uint32_t)stage * p.stage_stride + (uint32_t)d * p.record;
    if (score <= 0) return false;
    if (p.three) {
        for (int i = 0; i < 3; ++i)
            if ((int32_t)rd(r + 4 * i) == score && byte(r + 0x1F + i) == 0) return true;
        return false;
    }
    return (int32_t)rd(r) == 0 && (int32_t)rd(r + 4) == score;
}

void fire(const std::string& e) {
    if (!g_sent.insert(e).second) return;
    g_seen.insert(e);
    rl_ach_event(e.c_str());
    nds_note("eba rla: event %s", e.c_str());
}

std::string lower(const char* s) {
    std::string r(s);
    for (char& c : r) c = (char)std::tolower((unsigned char)c);
    return r;
}

// the progress in a line, for the log
std::string summary() {
    uint32_t b = base();
    const Game::Progress& p = game().progress;
    if (!b) return "no progress yet";
    int n = (int)game().stages.size(), s = 0;
    int64_t total = 0;
    std::string r;
    for (int d = 0; d < 4; ++d) {
        int cleared = 0, tier = 7;
        for (int st = 0; st < n; ++st) {
            Record x = record(b, st, d);
            cleared += x.cleared;
            s += x.s;
        }
        for (int st = 0; st < p.story; ++st)
            if (!record(b, st, d).cleared) { tier = p.tiers[st]; break; }
        char buf[64];
        std::snprintf(buf, sizeof buf, "%c %d/%d tier %d, ", "enhx"[d], cleared, n, tier);
        r += buf;
    }
    for (int st = 0; st < n; ++st)
        for (int d = 0; d < p.records; ++d) total += record(b, st, d).score;
    int rank = 0;
    while (rank < (int)p.ranks.size() && total >= p.ranks[rank]) ++rank;
    char buf[96];
    std::snprintf(buf, sizeof buf, "S %d/%d, total %lld, overall rank %d/%d", s, n * 4, (long long)total, rank,
                  (int)p.ranks.size());
    return r + buf;
}

}  // namespace

std::vector<std::string> rla_events() {
    std::vector<std::string> ev;
    uint32_t b = base();
    const Game& g = game();
    const Game::Progress& p = g.progress;
    if (!b || !p.story) return ev;
    const std::string P = p.event;
    const int n = (int)g.stages.size();
    int s = 0;
    int64_t total = 0;
    for (int st = 0; st < n; ++st) {
        bool cleared = false, open = false;
        for (int d = 0; d < p.records; ++d) {
            Record x = record(b, st, d);
            total += x.score;                       // (the game sums Training's records too: none kept)
            if (d >= 4) continue;
            cleared |= x.cleared;
            open |= x.state > 0;
            s += x.s;
        }
        if (cleared) ev.push_back(P + "_clear_" + lower(g.stages[st]));
        if (st >= p.story && open) ev.push_back(P + "_bonus_" + lower(g.stages[st]));
    }
    for (int d = 0; d < 4; ++d) {
        int tier = 7;                               // the map's tier: the game's, by the first stage not cleared
        for (int st = 0; st < p.story; ++st)
            if (!record(b, st, d).cleared) { tier = p.tiers[st]; break; }
        for (int k = 1; k <= tier; ++k) ev.push_back(P + "_tier_" + "enhx"[d] + "_" + std::to_string(k));
        if (record(b, p.finale, d).cleared) ev.push_back(P + "_credits_" + "enhx"[d]);
    }
    for (size_t k = 0; k < p.ranks.size(); ++k)
        if (total >= p.ranks[k]) ev.push_back(P + "_rank_" + std::to_string(k + 1));
    for (int t : {1, 5, 10, 20})
        if (s >= t) ev.push_back(P + "_s_" + std::to_string(t));
    if (s == n * 4) ev.push_back(P + "_s_all");
    return ev;
}

void rla_init() {
    const auto& o = g_cfg.port;
    if (o.count("rla-print")) g_print = std::strtoull(o.at("rla-print").c_str(), nullptr, 10);
    if (o.count("rla-stage")) g_test_stage = std::atoi(o.at("rla-stage").c_str());
    // Customs are a mode of their own: their runs (and the start save's first boot) never connect
    if (custom_playing() || o.count("custom") || o.count("customs-first-boot") || o.count("customs-make-start-save"))
        return;
    if (!game().progress.ptr) return;
    g_on = true;
    rl_ach_set_info(launcher_id(), EBA_VERSION);
    int connected = rl_ach_init("", nullptr, nullptr);
    nds_note("eba rla: Recomp Achievements as %s: %s", launcher_id(),
             connected ? "connected to RecompLauncher" : "no RecompLauncher here (the events are only logged)");
}

void rla_frame(uint64_t vbl) {
    if (vbl == g_print) {
        std::vector<std::string> ev = rla_events();
        std::string all;
        for (auto& e : ev) all += " " + e;
        nds_note("eba rla: %s", summary().c_str());
        nds_note("eba rla: %zu events met:%s", ev.size(), all.c_str());
    }
    if (!g_on) return;
    rl_ach_frame();
    if (!g_loaded && base()) { g_loaded = true; rl_ach_save_loaded(); }
    const Game& g = game();
    bool song = scene_is_song();
    uint32_t s = g.session ? rd(g.session) : 0;
    if (!in_main(s)) s = 0;
    if (g_test_stage >= 0 && s && g.scenes.intro && scene() == g.scenes.intro) {   // a test's stage, as Customs set one
        g_main[(s + g.stage) & 0x3FFFFFu] = (uint8_t)g_test_stage;
        if (g.team_table) {
            uint32_t team = rd(g.team_table + 4 * (uint32_t)g_test_stage);
            std::memcpy(g_main + ((s + g.team) & 0x3FFFFFu), &team, 4);
        }
    }
    if (song && !g_was_song) {                      // a song starts: what the progress meets now is the base
        std::vector<std::string> ev = rla_events();
        g_seen = std::set<std::string>(ev.begin(), ev.end());
        g_sent.clear();
        g_counts = true;
        g_why = "";
        g_stage = s ? (int)(rd(s + g.stage) & 0xFF) : -1;
        g_diff = s ? (int)rd(s) : -1;
        g_score = 0;
        g_after = g_cleared = false;
        if (!g_cfg.port.count("rla-quiet"))
            nds_note("eba rla: a song starts (stage %d, difficulty %d); %s", g_stage, g_diff, summary().c_str());
    }
    if (song) {
        const char* why = autoplay() ? "autoplay"
                        : replay_playing() ? "a replay"
                        : s && g.autoplay && rd(s + g.autoplay) ? "the game's autoplay (its demo)"
                        : s && g.replaying && rd(s + g.replaying) ? "the game's replay"
                        : nullptr;
        if (why && g_counts) {
            g_counts = false;
            g_why = why;
            nds_note("eba rla: this song is played with %s: what it brings fires nothing", why);
        }
        g_armed = g_counts;
        if (s && g.score) g_score = (int32_t)rd(s + g.score);
    }
    g_was_song = song;
    if (!song && g_armed && !g_after) {             // the scene after the song: cleared or not
        uint32_t sc = scene();
        if (sc) {
            g_after = true;
            g_cleared = sc == g.scenes.ending || sc == g.scenes.results;
            nds_note("eba rla: the song %s (score %d)", g_cleared ? "is cleared" : "is not cleared", g_score);
        }
    }
    if (!song && g_armed && vbl % 15 == 0) {        // after it: what it brought, as the game saves it
        const Game::Progress& p = g.progress;
        uint32_t b = base();
        int n = (int)g.stages.size();
        if (g_cleared && b && g_stage >= 0 && g_stage < n && g_diff >= 0 && g_diff < 4 &&
            record(b, g_stage, g_diff).cleared) {   // re-clears count: the song, the finale's credits
            fire(std::string(p.event) + "_clear_" + lower(g.stages[g_stage]));
            if (g_stage == p.finale) fire(std::string(p.event) + "_credits_" + "enhx"[g_diff]);
            if (live_s(b, g_stage, g_diff, g_score))  // an S got live: every S milestone the save reaches
                for (const std::string& e : rla_events())
                    if (e.rfind(std::string(p.event) + "_s_", 0) == 0) fire(e);
        }
        for (const std::string& e : rla_events())
            if (!g_seen.count(e)) fire(e);
    }
}

}  // namespace eba
