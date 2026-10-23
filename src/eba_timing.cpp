// The timing offset and the record of each tap's timing (eba.h).
//
// The judge, judge(note, now) (EBA's 0x020681D8, Ouendan 2's 0x020BB2B4;
// recompiled with a hook: the emit asks for --hook ADDR), judges the note the game is judging at the
// song's frame `now`: a fresh touch on it within 8 frames of its time is a
// 300, 100 or 50 (2, 5, 8 frames), one earlier a miss, none by 8 frames after
// a late miss. Taps and slider heads come through it (method 10 of both
// classes); spinners do not. The hook gives it now - offset: a tap made
// `offset` frames late, by a player whose sound or picture reaches them that
// late, is judged as on time. The picture and the sound are untouched.
//
// Options (ndsrun --port): offset=N (frames, -8 to 8); timing-log (each
// judged tap).
#include "eba.h"
#include "nds.h"
#include <algorithm>
#include <cmath>
#include <deque>

namespace eba {
namespace {

constexpr size_t kKeep = 200;                       // the taps the readout is made of

int g_offset;
bool g_log;
std::deque<int> g_taps;                             // frames after their note, before the offset
int g_grades[4];

// It returns the grade: 0 300, 1 100, 2 50, 3 a miss, 4 nothing yet.
void judge(ArmContext& c, ArmFunc original) {
    uint32_t note = c.r[0];
    int now = (int)c.r[1];
    if (!in_main(note)) { original(c); return; }
    int time = (int)(rd(note_field(note, 0x14)) >> 16);
    c.r[1] = (uint32_t)(now - g_offset);
    original(c);
    int grade = (int)c.r[0];
    if (grade < 0 || grade > 3) return;
    ++g_grades[grade];
    bool tapped = grade < 3 || now - g_offset < time;   // a miss after the note is no tap at all
    if (tapped) {
        g_taps.push_back(now - time);
        if (g_taps.size() > kKeep) g_taps.pop_front();
    }
    if (g_log)
        nds_note("eba timing: #%d judged %s at song frame %d (%+d, judged as %+d)%s", g_main[(note + game().number) & 0x3FFFFF],
                 grade == 0 ? "300" : grade == 1 ? "100" : grade == 2 ? "50" : "miss", now, now - time,
                 now - g_offset - time, tapped ? "" : ", no tap");
}

bool g_hooked;

struct Register {
    Register() {                                    // whichever game's judge this build hooks
        for (uint32_t j : all_judges()) g_hooked |= arm_set_hook(9, j, judge);
    }
} registered;

}  // namespace

void set_offset(int frames) { g_offset = std::clamp(frames, -8, 8); }
int offset() { return g_offset; }

TimingStats timing_stats() {
    TimingStats s;
    s.count = (int)g_taps.size();
    for (int t : g_taps) {
        s.mean += t;
        ++s.hist[std::clamp(t, -8, 8) + 8];
    }
    if (s.count) s.mean /= s.count;
    for (int t : g_taps) s.stdev += (t - s.mean) * (t - s.mean);
    if (s.count) s.stdev = std::sqrt(s.stdev / s.count);
    std::copy(g_grades, g_grades + 4, s.grades);
    return s;
}

void timing_reset() {
    g_taps.clear();
    std::fill(g_grades, g_grades + 4, 0);
}

void timing_options() {
    const auto& o = g_cfg.port;
    if (o.count("offset")) set_offset(std::atoi(o.at("offset").c_str()));
    g_log = o.count("timing-log") > 0;
    if (!g_hooked)
        nds_note("eba timing: the judge has no hook (recompile with --hook %08X): no offset", game().judge);
}

}  // namespace eba
