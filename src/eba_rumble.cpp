// Rumble: the gamepads' motors in a song, in button mode (SDL's rumble: a
// DualSense's haptics, a Switch Pro Controller's or Joy-Con's HD rumble, Xbox
// motors), after rhythm games like Fortnite Festival:
// - a strong pulse for the note that ends a combo (a tap, a slider's tail, a
//   spinner's end): the game's next note is number 1;
// - a medium pulse for every other note hit and each slider tick;
// - a slight hum while a slider is followed or a spinner spun, rising with
//   the spinner's speed.
// Misses are felt as nothing. Taps and slider heads come from the judge's hook
// (eba_timing.cpp); slider ticks and tails are the combo going up without one
// (the game's combo counts them); the song as button mode sees it each frame
// (eba_buttons.cpp's SongView).
//
// Options (the options bar, saved; or ndsrun --port): rumble=off;
// rumble-strength=N (10-100 percent); rumble-log (each pulse; a headless run
// logs them with no gamepad).
#include "eba.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>

namespace eba {
namespace {

constexpr int kStrongFrames = 9, kMediumFrames = 5;   // a pulse's length (it fades over it)
constexpr double kStrong = 1.0, kMedium = 0.6, kSlight = 0.2;

bool g_log;
int g_strong, g_medium;                 // frames left of each pulse
int g_hits;                             // taps the judge counted since the last frame
int g_combo;
// the slider being played: held at its last frame seen; its tail felt
struct { uint32_t note = 0; int end = 0; bool last = false, held = false, tail = false; } g_slider;
struct { uint32_t note = 0; int end = 0; bool last = false, spun = false; } g_spinner;
uint16_t g_sent_lo, g_sent_hi;
uint64_t g_sent_at;

void pulse(bool strong, const char* what, int now) {
    if (strong) g_strong = kStrongFrames;
    else g_medium = kMediumFrames;
    if (g_log) nds_note("eba rumble: %s %s at song frame %d", strong ? "strong" : "medium", what, now);
}

void send(uint16_t lo, uint16_t hi, uint64_t vbl) {
    if (!g_cfg.window && !g_log) return;
    if (lo == g_sent_lo && hi == g_sent_hi && (vbl - g_sent_at < 6 || (!lo && !hi))) return;
    g_sent_lo = lo;
    g_sent_hi = hi;
    g_sent_at = vbl;
    if (!g_cfg.window) return;
    int n = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&n);
    for (int i = 0; i < n; ++i)
        if (SDL_Gamepad* gp = SDL_GetGamepadFromID(ids[i])) SDL_RumbleGamepad(gp, lo, hi, 120);
    SDL_free(ids);
}

}  // namespace

Rumble& rumble() {
    static Rumble r;
    return r;
}

void rumble_options() {
    const auto& o = g_cfg.port;
    if (o.count("rumble")) rumble().on = o.at("rumble") != "off";
    if (o.count("rumble-strength")) rumble().strength = std::clamp(std::atoi(o.at("rumble-strength").c_str()), 10, 100);
    g_log = o.count("rumble-log") > 0;
}

void rumble_judged(uint32_t note, int grade) {
    if (grade > 2) return;                          // a miss: nothing
    ++g_hits;
    int kind = rd(note + 4) & 0xFF;
    if (kind == 1) pulse(false, "slider head", (int)rd(game().clock));   // its tail ends a combo, if any
    else pulse(note_ends_combo(note), "hit", (int)rd(game().clock));
}

void rumble_frame(const SongView& v, uint64_t vbl) {
    int c = combo();
    if (!v.live) {                                  // no note left (a song's finale spinner just ended), or none
        if (g_spinner.spun) pulse(g_spinner.last, "spinner end", g_spinner.end);
        g_hits = 0;
        g_slider = {};
        g_spinner = {};
        g_combo = c;
        double s = kStrong * g_strong / kStrongFrames, k = rumble().on ? rumble().strength / 100.0 : 0;
        if (g_strong) --g_strong;
        g_medium = 0;
        send((uint16_t)std::lround(s * k * 0xFFFF), (uint16_t)std::lround(s * 0.8 * k * 0xFFFF), vbl);
        return;
    }
    // the combo up by more than the judge's taps: slider ticks (a tail, too, mostly: Ouendan 1
    // leaves out some, so a tail is felt as the ball ends, held)
    int ticks = c > g_combo ? c - g_combo - g_hits : 0;
    g_combo = c;
    g_hits = 0;
    if (v.kind == 1) {
        if (g_slider.note != v.note) {
            if (g_log) nds_note("eba rumble: slider %08X, frames %d to %d%s", v.note, v.start, v.end, v.last ? ", ends its combo" : "");
            g_slider = {};
        }
        g_slider.note = v.note;
        g_slider.end = v.end;
        g_slider.last = v.last;
        if (v.now <= v.end) g_slider.held = v.pen;
    }
    if (g_slider.note && !g_slider.tail && v.now > g_slider.end) {   // the ball at its end
        g_slider.tail = true;
        if (g_slider.held) pulse(g_slider.last, "slider tail", v.now);
    } else if (g_slider.note && ticks > 0 && v.now < g_slider.end - 1) {
        pulse(false, "slider tick", v.now);
    }
    if (g_slider.note && v.now > g_slider.end + 4) g_slider = {};

    double body = 0;
    if (v.kind == 1 && v.pen && v.now >= v.start && v.now <= v.end) body = kSlight;
    if (v.kind == 2) {
        if (g_spinner.note != v.note) g_spinner = {};
        g_spinner.note = v.note;
        g_spinner.end = v.end;
        g_spinner.last = v.last;
        if (v.pen && v.spin > 0.02 && v.now >= v.start && v.now <= v.end) {
            g_spinner.spun = true;
            body = kSlight * (0.75 + std::min(1.0, v.spin / 0.12));   // 0.15 .. 0.35 with its speed
        }
    }
    if (g_spinner.note && (v.note != g_spinner.note || v.now > g_spinner.end)) {
        if (g_spinner.spun) pulse(g_spinner.last, "spinner end", v.now);
        g_spinner = {};
    }

    double s = kStrong * g_strong / kStrongFrames, m = kMedium * g_medium / kMediumFrames;
    if (g_strong) --g_strong;
    if (g_medium) --g_medium;
    double k = rumble().on ? rumble().strength / 100.0 : 0;
    double lo = std::max(s, m * 0.5) * k, hi = std::max({s * 0.8, m, body}) * k;
    send((uint16_t)std::lround(std::min(1.0, lo) * 0xFFFF), (uint16_t)std::lround(std::min(1.0, hi) * 0xFFFF), vbl);
}

}  // namespace eba
