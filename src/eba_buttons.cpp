// Button mode: a gamepad's face buttons or d-pad, or a key, in place of the
// stylus in a song. A press puts the stylus on the note the game is judging
// now, for two frames, and the game judges that tap as its own: the same
// windows, the same score. On a slider, held, the stylus follows its ball
// along the path, a point a frame. On a spinner, each press spins the stylus
// faster around its centre; it slows down when the presses stop. Or the left
// stick, turned round: the stylus turns with it (1.5 times as far, either way), on
// top of the presses; or a gamepad's triggers, mashed (a DualSense's push back,
// hard at first, easing as the spinner speeds up). The mouse, when it is down,
// wins. In a stage's opening or ending comic, and in a song's intro after GO!
// (up to its first note), a hit button presses SKIP; where the game has none
// (Ouendan 1's intro) and after a song's last note (every game's outro), it
// fast-forwards instead (fast_forward below), button mode or not. Elsewhere the buttons are the DS's as before (south B,
// east A, west Y, north X); in a song and a comic, the face buttons and the
// d-pad are hits only (both sets: alternating them makes fast streams easier).
//
// What it reads of the game (EBA's addresses, the stock USA ARM9, found
// 2026-10-22; Ouendan 2's are its own, the fields the same: games.cpp):
//   0x020CD390        the song's clock, in frames (the judge's `now`)
//   class 0x020B4110  the notes' manager, on the heap: +0x70 the array of
//                     the song's notes, +0x74 their count
//   each note         vtable 0x020B4648 tap, 0x020B478C slider, 0x020B48D4
//                     spinner; +4 byte 0 its kind (0 1 2), byte 1 its state
//                     (4: the one being judged), byte 3 its number; +0x14
//                     low half the frame it appears, high half its time (a
//                     spinner's end); +0x18, +0x1C x, y on the lower screen
//                     (a spinner's centre)
//   a slider          +0x40 its path, a point a frame of one pass (int16
//                     x * 16, y * 64 / 3), +0x44 the points, +0x4C the passes
//                     (counted down while it plays: read at the press); its
//                     tail is judged the frame after the last point
//   a spinner         +0x40 the frames it spins (ending at its time), +0x44
//                     the turns it asks for
//
// Options (the options bar, saved; or ndsrun --port): buttons=off;
// button-keys=Space,... (SDL key names, up to 4; Space by default);
// buttons-log (each press and what it hit; buttons-trace adds the note judged
// every 15 frames); buttons-auto[=N] (a test: presses on every note's frame,
// or N frames after it, holds the sliders, mashes the spinners at 7.5 a
// second: The Anthem on Hard Rock, rank S, 258 of 258 at 300); stick-auto (with
// buttons-auto: the spinners spun by a left stick turning N times a second, 5
// by default).
#include "eba.h"
#include "display.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace {

using eba::in_main;
using eba::rd;
using eba::rh;

constexpr uint32_t kFaceKeys = 0xCF3;               // DS A, B, X, Y and the d-pad
constexpr uint32_t kFacePad = 0x780F;               // the gamepad's south, east, west, north; d-pad up, down, left, right
constexpr int kTapFrames = 2;                       // a tap holds the stylus this long
constexpr double kSpinBoost = 2.0 / 60;             // a press adds 2 turns a second ...
constexpr double kSpinDecay = 0.9715;               // ... which halve in 0.4 s without presses
constexpr double kSpinMax = 10.0 / 60;              // at most 10 turns a second
constexpr double kSpinRadius = 40;
constexpr double kStickDead = 0.6;                  // a stick pushed past this turns a spinner
constexpr double kStickGain = 1.5;                  // the stylus turns this much for a turn of the stick
constexpr double kStickMax = 0.3;                   // at most this of a turn a frame (18 a second)

struct Note {
    uint32_t at = 0;
    int kind = 0, state = 0, number = 0, appear = 0, time = 0, x = 0, y = 0;
};

Note note_at(uint32_t a) {
    Note n;
    n.at = a;
    uint32_t w4 = rd(a + 4), t = rd(eba::note_field(a, 0x14));
    n.kind = w4 & 0xFF;
    n.state = (w4 >> 8) & 0xFF;
    n.number = g_main[(a + eba::game().number) & 0x3FFFFF];
    n.appear = t & 0xFFFF;
    n.time = t >> 16;
    n.x = (int)rd(eba::note_field(a, 0x18));
    n.y = (int)rd(eba::note_field(a, 0x1C));
    return n;
}

// A slider as it was before it began: the game counts its passes down in
// the object while it plays, so its shape is taken once, at the press.
struct Slider {
    uint32_t at = 0, path = 0;
    int time = 0, n = 0, passes = 1;
};

struct Mode {
    bool init = false, log = false, autoplay = false;
    int auto_late = 0;                  // buttons-auto=N: each press N frames after its note (a late player)
    uint32_t mgr = 0;
    uint64_t next_scan = 0;
    uint32_t clock = 0;
    uint64_t clock_seen = 0;            // the VBlank the clock last moved
    uint32_t held_prev = 0;             // the hit buttons held at the last call
    int tap_left = 0, tap_x = 0, tap_y = 0;
    int pending = 0;                    // a press that waits for the stylus to lift first
    Slider slider;                      // the slider the held buttons follow
    uint32_t spinner = 0;
    double angle = 0, speed = 0;        // turns, turns a frame
    uint32_t auto_last = 0;             // buttons-auto: the last note pressed
    int auto_mash = 0;
    bool stick_auto = false;            // stick-auto[=N]: buttons-auto turns the stick N times a second on spinners (5)
    double stick_rate = 5;
    bool stick_on = false;              // the left stick past its dead zone at the last frame
    double stick_prev = 0, stick_turn = 0;   // its angle then (turns); this frame's turn of it
    eba::SongView view;                 // for the rumble
} m;
bool g_tap_used;                         // Tap mode played a note in the song (eba::tap_mode_used)

bool manager_ok(uint32_t a) {
    const eba::Game& g = eba::game();
    if (!in_main(a) || rd(a) != g.manager) return false;
    uint32_t arr = rd(a + g.notes), n = rd(a + g.notes + 8);
    if (!in_main(arr) || n == 0 || n > 2048) return false;
    uint32_t first = rd(arr), vt = in_main(first) ? rd(first) : 0;
    return vt == g.tap || vt == g.slider || vt == g.spinner;
}

uint32_t manager(uint64_t vbl) {
    if (manager_ok(m.mgr)) return m.mgr;
    m.mgr = 0;
    if (vbl < m.next_scan) return 0;
    m.next_scan = vbl + 30;                         // not in a song: look again in half a second
    const eba::Game& g = eba::game();
    if (!g.manager) return 0;
    for (uint32_t a = g.heap; a < 0x02400000u; a += 4)
        if (rd(a) == g.manager && manager_ok(a)) return m.mgr = a;
    return 0;
}

// The note the game judges now (state 4), else the next one to come.
bool current(uint32_t mgr, Note& out) {
    uint32_t arr = rd(mgr + eba::game().notes), n = rd(mgr + eba::game().notes + 8);
    bool upcoming = false;
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t a = rd(arr + 4 * i);
        if (!in_main(a)) return false;
        Note t = note_at(a);
        if (t.state == 4) { out = t; return true; }
        if (!upcoming && t.state >= 1 && t.state <= 3) { out = t; upcoming = true; }
    }
    return upcoming;
}

Slider slider_at(uint32_t s) {
    Slider r;
    r.at = s;
    r.time = (int)(rd(eba::note_field(s, 0x14)) >> 16);
    r.path = rd(eba::note_field(s, 0x40));
    r.n = (int)rd(eba::note_field(s, 0x44));
    r.passes = std::max(1, (int)rd(eba::note_field(s, 0x4C)));
    if (!in_main(r.path) || r.n <= 0 || r.n > 4096) r.at = 0;
    return r;
}

// The ball at a frame of the song: a path point a frame, passes alternating.
bool ball(const Slider& s, int now, int& x, int& y) {
    if (!s.at) return false;
    int k = now - s.time;
    if (k > s.n * s.passes) return false;           // over (the frame after the last point still holds)
    if (k < 0) k = 0;
    if (k == s.n * s.passes) --k;
    int pass = k / s.n, i = k - pass * s.n;
    if (pass & 1) i = s.n - 1 - i;
    x = rh(s.path + 4 * i) / 16;
    y = rh(s.path + 4 * i + 2) * 3 / 64;
    return true;
}

// The hit buttons held: the gamepad's face buttons and d-pad, and the keys, one bit each.
uint32_t hit_buttons(const NdsInput& in) {
    uint32_t held = in.pad & kFacePad;
    if (in.keyboard)
        for (int i = 0; i < 4; ++i) {
            int sc = eba::buttons().keys[i];
            if (sc > 0 && sc < SDL_SCANCODE_COUNT && in.keyboard[sc]) held |= 1u << (16 + i);
        }
    return held;
}

// The left sticks: the angle (turns: 0 right, a quarter down) of the one pushed furthest past the dead zone.
bool stick_angle(double& turns) {
    if (!g_cfg.window) return false;
    int n = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&n);
    double best = kStickDead;
    bool ok = false;
    for (int i = 0; i < n; ++i) {
        SDL_Gamepad* gp = SDL_GetGamepadFromID(ids[i]);
        if (!gp) continue;
        double x = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_LEFTX) / 32767.0;
        double y = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_LEFTY) / 32767.0;
        double r = std::hypot(x, y);
        if (r > best) { best = r; turns = std::atan2(y, x) / (2 * M_PI); ok = true; }
    }
    SDL_free(ids);
    return ok;
}

// The triggers (L2, R2) of every gamepad, two hit bits for spinners: down past 60% of the pull, up under 35%.
uint32_t trigger_bits() {
    static uint32_t state;
    if (!g_cfg.window) return 0;
    int n = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&n);
    double l = 0, r = 0;
    for (int i = 0; i < n; ++i)
        if (SDL_Gamepad* gp = SDL_GetGamepadFromID(ids[i])) {
            l = std::max(l, SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) / 32767.0);
            r = std::max(r, SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) / 32767.0);
        }
    SDL_free(ids);
    auto set = [](double v, uint32_t bit) { if (v > 0.6) state |= bit; else if (v < 0.35) state &= ~bit; };
    set(l, 1u << 20);
    set(r, 1u << 21);
    return state;
}

// A DualSense's adaptive triggers on a spinner: a moderate resistance from the start of the
// pull (120 of 255), easing slowly as the spinner speeds up (over about 4 seconds of keeping
// up), none at full speed; off elsewhere. SDL's DualSense effect report (its
// DS5EffectsState_t, 47 bytes): both triggers' effects enabled, mode 1 (continuous
// resistance: start, force) or 5 (off).
void trigger_feel(bool spinning, double speed) {
    static int sent = -1;                           // the force last sent (0: off)
    static double pace;                             // the spinner's speed, smoothed over about 2 seconds
    pace = spinning ? pace + (speed - pace) * 0.02 : 0;
    int force = 0;
    if (spinning && eba::buttons().trigger_feel)
        force = (int)std::lround(120 * std::pow(std::clamp(1.0 - pace / kSpinMax, 0.0, 1.0), 0.8)) / 10 * 10;
    if (force == sent) return;
    sent = force;
    if (m.log) nds_note("eba buttons: trigger resistance %d", force);
    if (!g_cfg.window) return;
    struct {
        uint8_t enable1, enable2, rumble_right, rumble_left, headphones, speaker, mic, audio, mic_light, mute;
        uint8_t right[11], left[11];
        uint8_t rest[15];
    } e{};
    static_assert(sizeof e == 47, "DS5EffectsState_t");
    e.enable1 = 0x04 | 0x08;                        // the right and the left trigger's effect
    uint8_t fx[11] = {};
    if (force) { fx[0] = 0x01; fx[1] = 0; fx[2] = (uint8_t)force; }
    else fx[0] = 0x05;
    std::memcpy(e.right, fx, 11);
    std::memcpy(e.left, fx, 11);
    int n = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&n);
    for (int i = 0; i < n; ++i)
        if (SDL_Gamepad* gp = SDL_GetGamepadFromID(ids[i]); gp && SDL_GetGamepadType(gp) == SDL_GAMEPAD_TYPE_PS5)
            SDL_SendGamepadEffect(gp, &e, sizeof e);
    SDL_free(ids);
}

// Fast-forward (all three games): in a song's intro where the game has no SKIP of its
// own (Ouendan 1), and in every song's outro once its last note is past (EBA and Ouendan
// 1 never let the player skip it; Ouendan 2 does), a face button, the d-pad or a hit key
// runs the game at full speed (the recomp's turbo: frames neither drawn nor paced, the
// sound dropped) up to 1.5 s before the first note appears, or until the song is over
// (the results, or the ending comic). Button mode or not: the buttons do nothing else
// there. A test run never presses, so it never fast-forwards.
struct FastForward {
    bool on = false;
    uint32_t until = 0;                 // the song frame to stop at (0: when the song scene ends)
    uint64_t since = 0;
    uint32_t mgr = 0;                   // the song whose last note is known
    int last_time = 0;
    uint32_t prev = 0;                  // the hit buttons held at the last call
} ff;

bool fast_forward(NdsInput& in, uint32_t mgr, uint64_t vbl) {
    const eba::Game& g = eba::game();
    bool song = eba::scene_is_song();
    if (ff.on) {                                    // running: until the mark, the scene's end, or 3 minutes
        uint32_t clk = mgr ? rd(g.clock) : 0;
        if (!song || !mgr || (ff.until && clk + 1 >= ff.until) || vbl - ff.since > 10800) {
            ff.on = false;
            display_set_turbo(false);
            if (m.log) nds_note("eba buttons: fast-forward over at song frame %u", clk);
        }
        in.pen = false;                             // nothing reaches the game meanwhile
        in.keys = 0;
        return true;
    }
    if (!song || !mgr) { ff.prev = 0; return false; }
    uint32_t held = hit_buttons(in), pressed = held & ~ff.prev;
    ff.prev = held;
    if (!pressed) return false;
    // This song's last note's time (a spinner's is its end), read at the press: the next song's
    // notes are often at the last one's address, so a time kept per address went stale, and a press
    // late in a longer song fast-forwarded it (a player's report, 9 Nov 2026).
    uint32_t arr = rd(mgr + g.notes), n = rd(mgr + g.notes + 8);
    ff.last_time = 0;
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t a = rd(arr + 4 * i);
        if (!in_main(a)) return false;              // (not all there: no fast-forward on a guess)
        ff.last_time = std::max(ff.last_time, note_at(a).time);
    }
    Note first = note_at(rd(arr));
    int now = (int)rd(g.clock);
    bool intro = !g.intro_skip && now < first.appear, outro = now > ff.last_time + 2;   // (a played note may read state 0 again)
    if (!(intro || outro)) return false;
    in.keys &= ~(in.pad_keys & kFaceKeys);
    ff.on = true;
    ff.since = vbl;
    ff.until = intro ? (uint32_t)std::max(0, first.appear - 90) : 0;
    display_set_turbo(true);
    if (m.log) nds_note("eba buttons: fast-forward from song frame %d (%s)", now, intro ? "the intro" : "the outro");
    return true;
}

// A stage's opening or ending comic: a hit button presses its SKIP, at the lower right.
void skip_comic(NdsInput& in) {
    static uint32_t prev;
    static int left;
    uint32_t held = hit_buttons(in);
    in.keys &= ~(in.pad_keys & kFaceKeys);
    if (held & ~prev) left = kTapFrames;
    prev = held;
    if (left > 0 && !in.pen) {
        --left;
        in.pen = true;
        in.x = eba::game().skip_x;
        in.y = eba::game().skip_y;
    }
}

void play_input(NdsInput& in, uint64_t vbl) {
    if (!m.init) { m.init = true; eba::init(); }
    eba::debug_pokes(vbl);
    eba::play_frame(vbl);                           // autoplay, replays, assists
    eba::fx_frame(vbl);                             // the light behind the 3D
    if (eba::customs_frame(in, vbl)) return;        // Customs' autopilot drives the game to its stage
    if (eba::ui_wants_mouse()) in.pen = false;      // the mouse is on the options bar
    uint32_t mgr = manager(vbl);
    if (fast_forward(in, mgr, vbl)) return;        // a song's intro or outro skipped
    if (!eba::buttons().on) return;
    if (eba::scene_has_skip()) { skip_comic(in); return; }
    // a song's intro, after GO! (the Agents' arrival, the episode's card, ...) up to its first
    // note: SKIP at the lower right too, while no note has appeared (they appear in order)
    if (eba::game().intro_skip && eba::scene_is_song() &&
        (!mgr || note_at(rd(rd(mgr + eba::game().notes))).state == 0)) {
        skip_comic(in);
        m.held_prev = 0;
        return;
    }
    uint32_t clock = mgr ? rd(eba::game().clock) : 0;
    if (clock != m.clock) { m.clock = clock; m.clock_seen = vbl; }
    bool live = mgr && vbl - m.clock_seen < 30;     // a song running: its clock moved in the last half second
    if (!live) {
        m.slider = Slider();
        m.spinner = 0;
        m.tap_left = m.pending = 0;
        m.held_prev = 0;
        return;
    }
    uint32_t held = hit_buttons(in);
    Note cur;
    bool have = current(mgr, cur);
    int now = (int)clock + 1;                       // the frame this input is read in
    // the left stick turned round: this frame's turn of it, for a spinner
    double st = 0;
    bool stick = stick_angle(st);
    if (m.autoplay && m.stick_auto && have && cur.kind == 2) { st = m.stick_prev + m.stick_rate / 60; stick = true; }
    m.stick_turn = 0;
    if (stick && m.stick_on) {
        double d = st - m.stick_prev;
        m.stick_turn = std::clamp((d - std::round(d)) * kStickGain, -kStickMax, kStickMax);
    }
    m.stick_on = stick;
    if (stick) m.stick_prev = st;
    if (m.stick_turn != 0 && have && cur.kind == 2) m.spinner = cur.at;
    if (m.autoplay) {                               // the test: a perfect player
        held = 0;
        if (have && cur.kind != 2 && cur.at != m.auto_last && now >= cur.time + m.auto_late) {
            m.auto_last = cur.at;
            held = 1;
        }
        if (m.slider.at) { int bx, by; if (ball(m.slider, now, bx, by)) held |= 1; }   // the same button, held on
        if (!m.stick_auto && have && cur.kind == 2 && now >= cur.time - (int)rd(eba::note_field(cur.at, 0x40)) && now <= cur.time)
            held |= (++m.auto_mash % 8 < 4) ? 4 : 0;    // 4 frames down, 4 up: 7.5 presses a second
    }
    uint32_t triggers = trigger_bits();             // on a spinner the triggers mash too
    if (have && cur.kind == 2) held |= triggers;
    if (m.log && g_cfg.port.count("buttons-trace") && vbl % 15 == 0)
        nds_note("eba buttons: song frame %d: current #%d kind %d state %d time %d at %08X; slider %08X held %X",
                 now, have ? cur.number : -1, cur.kind, cur.state, cur.time, cur.at, m.slider.at, held);
    in.keys &= ~(in.pad_keys & kFaceKeys);          // in a song the face buttons and d-pad are hits, not the DS's
    m.view.live = true;                             // what the rumble sees: the song, its note if any
    m.view.now = now;
    if (have) {
        eba::SongView& v = m.view;
        v.note = cur.at;
        v.kind = cur.kind;
        v.start = v.end = cur.time;
        if (cur.kind == 1) {
            Slider s = m.slider.at == cur.at ? m.slider : slider_at(cur.at);
            if (s.at) v.end = s.time + s.n * s.passes;
        }
        if (cur.kind == 2) v.start = cur.time - (int)rd(eba::note_field(cur.at, 0x40));
        v.last = eba::note_ends_combo(cur.at);
    }
    int presses = __builtin_popcount(held & ~m.held_prev);
    m.held_prev = held;
    if ((presses && have) || (m.stick_turn != 0 && have && cur.kind == 2)) g_tap_used = true;
    bool mouse = in.pen;

    // a press: a tap on the note being judged, the head of a slider, or a spinner's push
    if (presses && have) {
        if (cur.kind == 2) {
            m.spinner = cur.at;
            m.speed = std::min(kSpinMax, m.speed + kSpinBoost * presses);
        } else if (m.tap_left > 0 || m.pending) {
            m.tap_left = 0;                         // still down from the last: lift for a frame first
            m.pending = 1;
        } else {
            m.tap_left = kTapFrames;
            m.tap_x = cur.x;
            m.tap_y = cur.y;
            m.slider = cur.kind == 1 ? slider_at(cur.at) : Slider();
        }
        if (m.log)
            nds_note("eba buttons: press at song frame %d: #%d %s, time %d (%+d), state %d", now, cur.number,
                     cur.kind == 0 ? "tap" : cur.kind == 1 ? "slider" : "spinner", cur.time, now - cur.time, cur.state);
    } else if (m.pending && m.tap_left == 0 && have && cur.kind != 2) {
        m.pending = 0;                              // the lifted frame is past: the waiting press
        m.tap_left = kTapFrames;
        m.tap_x = cur.x;
        m.tap_y = cur.y;
        m.slider = cur.kind == 1 ? slider_at(cur.at) : Slider();
    }
    if (mouse) return;

    // the stylus: a slider held, a tap's frames, a spinner turning
    int bx, by;
    if (m.slider.at && held && ball(m.slider, now, bx, by)) {
        in.pen = true; in.x = bx; in.y = by;
        m.tap_left = 0;
        return;
    }
    if (m.slider.at && !(held && ball(m.slider, now, bx, by))) m.slider = Slider();
    if (m.tap_left > 0) {
        --m.tap_left;
        in.pen = true; in.x = m.tap_x; in.y = m.tap_y;
        return;
    }
    if (m.spinner) {
        Note s = note_at(m.spinner);
        if (now > s.time || s.kind != 2) { m.spinner = 0; m.speed = 0; return; }
        m.angle += m.speed + m.stick_turn;
        m.speed *= kSpinDecay;
        m.view.spin = m.speed + std::abs(m.stick_turn);
        if (m.speed < 0.2 / 60 && !held && !m.stick_on) return;   // stopped: the stylus up
        double r = m.angle * 2 * M_PI;
        in.pen = true;
        in.x = std::clamp((int)std::lround(s.x + kSpinRadius * std::cos(r)), 0, 255);
        in.y = std::clamp((int)std::lround(s.y + kSpinRadius * std::sin(r)), 0, 191);
    }
}

// each frame: the input, then the rumble from what it saw of the song
void hook(NdsInput& in, uint64_t vbl) {
    static bool was_song;                           // a new song: nothing has been played with buttons yet
    bool is_song = eba::scene_is_song();
    if (is_song && !was_song) g_tap_used = false;
    was_song = is_song;
    m.view = eba::SongView();
    play_input(in, vbl);
    m.view.pen = in.pen;
    m.view.x = in.pen ? in.x : -1;
    if (!eba::buttons().on || !m.view.live) m.view = eba::SongView();
    eba::rumble_frame(m.view, vbl);
    trigger_feel(m.view.kind == 2 && m.view.now >= m.view.start && m.view.now <= m.view.end, m.view.spin);
    eba::ra_frame(vbl);                             // the achievements, on the frame's RAM as it is
}

struct Register {
    Register() { nds_set_input_hook(hook); }
} registered;

}  // namespace

namespace eba {

Buttons& buttons() {
    static Buttons b;
    return b;
}

void buttons_options() {
    const auto& o = g_cfg.port;
    Buttons& b = buttons();
    if (o.count("buttons")) b.on = o.at("buttons") != "off";
    m.log = o.count("buttons-log") > 0;
    m.autoplay = o.count("buttons-auto") > 0;
    if (m.autoplay) b.on = true;                    // (the test player plays with buttons, whatever the default)
    m.stick_auto = o.count("stick-auto") > 0;
    if (m.stick_auto && !o.at("stick-auto").empty()) m.stick_rate = std::atof(o.at("stick-auto").c_str());
    if (m.autoplay) m.auto_late = std::atoi(o.at("buttons-auto").c_str());
    if (o.count("button-keys")) {
        std::fill(b.keys, b.keys + 4, 0);
        std::stringstream ss(o.at("button-keys"));
        std::string k;
        for (int i = 0; i < 4 && std::getline(ss, k, ',');) {
            SDL_Scancode sc = SDL_GetScancodeFromName(k.c_str());
            if (sc == SDL_SCANCODE_UNKNOWN) nds_note("eba buttons: no key %s", k.c_str());
            else b.keys[i++] = sc;
        }
    }
    if (b.on) nds_note("eba buttons: on%s", m.autoplay ? " (buttons-auto)" : "");
}

bool note_ends_combo(uint32_t note) {
    if (!manager_ok(m.mgr)) return false;
    uint32_t arr = rd(m.mgr + game().notes), n = rd(m.mgr + game().notes + 8);
    for (uint32_t i = 0; i < n; ++i)
        if (rd(arr + 4 * i) == note) {
            if (i + 1 == n) return true;
            uint32_t next = rd(arr + 4 * (i + 1));
            return in_main(next) && g_main[(next + game().number) & 0x3FFFFF] == 1;
        }
    return false;
}

// --port poke=ADDR:VALUE@VBLANK,... (hex address, decimal byte): a test's writes to main RAM
void debug_pokes(uint64_t vbl) {
    static std::vector<std::tuple<uint32_t, uint8_t, uint64_t>> pokes;
    static bool parsed;
    if (!parsed) {
        parsed = true;
        if (g_cfg.port.count("poke")) {
            std::stringstream ss(g_cfg.port.at("poke"));
            std::string p;
            while (std::getline(ss, p, ',')) {
                unsigned a = 0, v = 0;
                unsigned long long at = 0;
                if (std::sscanf(p.c_str(), "%x:%u@%llu", &a, &v, &at) == 3) pokes.emplace_back(a, (uint8_t)v, at);
            }
        }
    }
    for (auto& [a, v, at] : pokes)
        if (vbl == at && in_main(a)) {
            g_main[a & 0x3FFFFF] = v;
            nds_note("eba: poke %08X = %u", a, v);
        }
    if (g_cfg.port.count("channel-log") && vbl % 300 == 0) {   // the sound channels playing
        std::string s;
        for (int n = 0; n < 16; ++n) {
            SoundChannel c = sound_channel(n);
            if (!c.on) continue;
            char t[96];
            static const char* const kFmt[4] = {"pcm8", "pcm16", "adpcm", "psg"};
            std::snprintf(t, sizeof t, " %d:%s,%s %08X+%X", n, kFmt[c.format & 3],
                          c.repeat == 1 ? "L" : c.repeat == 2 ? "1" : "M", c.src, c.loop + c.len);
            s += t;
        }
        nds_note("eba: channels at song frame %u:%s", game().clock ? rd(game().clock) : 0, s.c_str());
    }
}

}  // namespace eba

bool eba::tap_mode_used() { return g_tap_used; }
