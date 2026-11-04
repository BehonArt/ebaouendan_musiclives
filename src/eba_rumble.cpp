// Rumble: the gamepads' motors in a song, in button mode (SDL's rumble: a
// DualSense's haptics, a Switch Pro Controller's or Joy-Con's HD rumble, Xbox
// motors), after rhythm games like Fortnite Festival. Three strengths:
// - high: the note that ends a combo (a tap, a slider's tail: the game's next
//   note is number 1) and every spinner's end;
// - medium: every other note hit, a slider's head, and a miss (with its X);
// - low: a slider's ticks, panned across the pad (left to right) by where the
//   ball is on the screen.
// Nothing while a spinner turns (a DualSense's triggers push back instead:
// eba_buttons.cpp). Taps, slider heads and misses come from the judge's hook
// (eba_timing.cpp); slider ticks are the combo going up without one (the
// game's combo counts them); a tail is felt as the ball reaches its end while
// held (Ouendan 1 leaves some tails out of its combo); the song as button mode
// sees it each frame (eba_buttons.cpp's SongView).
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

enum Level { kLow, kMedium, kHigh };
constexpr int kFrames[3] = {4, 6, 9};               // a pulse's length (it fades over it)
constexpr double kAmp[3] = {0.3, 0.6, 1.0};
constexpr const char* kName[3] = {"low", "medium", "high"};

bool g_log;
int g_left[3];                          // frames left of each pulse
int g_hits;                             // taps the judge counted since the last frame
int g_combo;
// the slider being played: held at its last frame seen; its tail felt
struct { uint32_t note = 0; int end = 0; bool last = false, held = false, tail = false; } g_slider;
struct { uint32_t note = 0; int end = 0; bool spun = false; } g_spinner;
double g_tick_pan = 0.5;                // the last tick's place: 0 the screen's left, 1 its right
uint16_t g_sent_lo, g_sent_hi;
uint64_t g_sent_at;

void pulse(Level l, const char* what, int now) {
    g_left[l] = kFrames[l];
    if (g_log) nds_note("eba rumble: %s %s at song frame %d", kName[l], what, now);
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

// the motors from the pulses left: the low (heavy, left) motor for the high pulses, the high (light,
// right) one for the high and medium; the ticks on both, panned by the ball's x (constant power)
void motors(uint64_t vbl) {
    double a[3];
    for (int l = 0; l < 3; ++l) {
        a[l] = kAmp[l] * g_left[l] / kFrames[l];
        if (g_left[l]) --g_left[l];
    }
    double k = rumble().on ? rumble().strength / 100.0 : 0;
    double pan = g_tick_pan * M_PI / 2, tl = a[kLow] * std::cos(pan), tr = a[kLow] * std::sin(pan);
    double lo = std::max({a[kHigh], a[kMedium] * 0.5, tl}) * k, hi = std::max({a[kHigh] * 0.8, a[kMedium], tr}) * k;
    send((uint16_t)std::lround(std::min(1.0, lo) * 0xFFFF), (uint16_t)std::lround(std::min(1.0, hi) * 0xFFFF), vbl);
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
    int now = (int)rd(game().clock);
    if (grade > 2) { pulse(kMedium, "miss", now); return; }
    ++g_hits;
    int kind = rd(note + 4) & 0xFF;
    if (kind == 1) pulse(kMedium, "slider head", now);   // its tail ends a combo, if any
    else if (note_ends_combo(note)) pulse(kHigh, "combo end", now);
    else pulse(kMedium, "hit", now);
}

void rumble_frame(const SongView& v, uint64_t vbl) {
    int c = combo();
    if (!v.live) {                                  // no song: a last spinner's end, then the motors run down
        if (g_spinner.spun) pulse(kHigh, "spinner end", g_spinner.end);
        g_hits = 0;
        g_slider = {};
        g_spinner = {};
        g_combo = c;
        motors(vbl);
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
        if (g_slider.held) pulse(g_slider.last ? kHigh : kMedium, g_slider.last ? "combo end (slider tail)" : "slider tail", v.now);
    } else if (g_slider.note && ticks > 0 && v.now < g_slider.end - 1) {
        g_tick_pan = v.x >= 0 ? std::clamp(v.x / 255.0, 0.0, 1.0) : 0.5;
        pulse(kLow, "slider tick", v.now);
        if (g_log) nds_note("eba rumble: the tick at x %d", v.x);
    }
    if (g_slider.note && v.now > g_slider.end + 4) g_slider = {};

    if (v.kind == 2) {                              // a spinner: nothing while it turns, high at its end
        if (g_spinner.note != v.note) g_spinner = {};
        g_spinner.note = v.note;
        g_spinner.end = v.end;
        if (v.pen && v.spin > 0.02 && v.now >= v.start && v.now <= v.end) g_spinner.spun = true;
    }
    if (g_spinner.note && (v.note != g_spinner.note || v.now > g_spinner.end)) {
        if (g_spinner.spun) pulse(kHigh, "spinner end", v.now);
        g_spinner = {};
    }
    motors(vbl);
}

}  // namespace eba
