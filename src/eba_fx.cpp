// Optional light behind the 3D in a song (display.h's DisplayFx), off by default.
// Nothing shows before a 30x combo (the first combo burst); a broken combo fades
// it out until the next 30x.
// - EBA: spotlights rising from below the stage behind the Agents, swung a step by
//   each note hit (so on the song's beats): 2 from 30x, 3 from 60x, 4 from 100x
//   (EBA's combo bursts: 30, 60, 100, 150); a gold flash when a group of notes
//   ends all 300 (an Elite Beat).
// - Ouendan 1 and 2: fire rising behind the squad, taller as the combo grows,
//   a burst on an all-300 group (the games' own fire, heightened).
// It is drawn in the 3D frame just in front of its farthest opaque polygon
// (render_gl.cpp), and under the 3D where a 2D background shows through, so the
// Agents, the notes and the HUD stay as they are. The combo is the game's own
// (Game::combo, slider ticks counted, as shown); each judged tap comes from the
// judge's hook (eba_timing.cpp).
//
// Options (ndsrun --port): fx[=off]; fx-intensity=N (10-100); combo-log.
#include "eba.h"
#include "display.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace eba {
namespace {

int g_group_n, g_group_300;             // the group being played: its taps judged, its 300s
float g_pulse, g_flash, g_heat, g_phase, g_target, g_fade, g_beams;
bool g_hit;
int g_last_combo = -1;

}  // namespace

int combo() {
    const Game& g = game();
    if (!g.session || !g.combo) return 0;
    uint32_t s = rd(g.session);
    return in_main(s) ? std::max(0, (int)rh(s + g.combo)) : 0;
}

Effects& effects() {
    static Effects e;
    return e;
}

void fx_judged(int grade, int number) {
    if (number == 1 && g_group_n >= 2 && g_group_300 == g_group_n && effects().flashes) g_flash = 1;   // an Elite Beat
    if (number == 1) g_group_n = g_group_300 = 0;
    ++g_group_n;
    if (grade == 0) ++g_group_300;
    if (grade != 3) g_hit = true;
}

void fx_frame(uint64_t vbl) {
    const Effects& e = effects();
    int c = combo();
    if (g_cfg.port.count("combo-log") && c != g_last_combo && (c % 10 == 0 || c < g_last_combo))
        nds_note("eba fx: combo %d at VBlank %llu", c, (unsigned long long)vbl);
    g_last_combo = c;
    bool song = scene_is_song();
    DisplayFx fx;
    if (!e.on || !game().scenes.song) { display_set_fx(fx); return; }
    bool lit = song && c >= 30;                         // from the first combo burst
    g_fade = std::clamp(g_fade + (lit ? 1.0f / 30 : -1.0f / 45), 0.0f, 1.0f);   // in over 1/2 s, out over 3/4 s
    if (!song) g_group_n = g_group_300 = 0;
    if (g_hit) { g_pulse = 1; g_target += 1; g_hit = false; }
    g_pulse *= 0.88f;
    g_flash *= 0.93f;
    g_phase += (g_target - g_phase) * 0.18f;
    float heat = lit ? std::clamp((c - 30) / 120.0f, 0.0f, 1.0f) : g_heat;   // full by 150x
    g_heat += (heat - g_heat) * 0.03f;
    if (lit) g_beams = c >= 100 ? 4.0f : c >= 60 ? 3.0f : 2.0f;
    if (g_fade <= 0) { display_set_fx(fx); return; }
    fx.kind = game().id == std::string("EBA") ? 1 : 2;
    fx.time = (float)(vbl / 60.0);
    fx.level = g_fade * e.intensity / 100.0f;
    fx.pulse = g_pulse;
    fx.flash = g_flash;
    fx.heat = g_heat;
    fx.phase = g_phase;
    fx.beams = (int)g_beams;
    fx.debug = g_cfg.port.count("fx-debug") > 0;
    static bool told;
    if (!told) { told = true; nds_note("eba fx: lit at combo %d (%s)", c, fx.kind == 1 ? "spotlights" : "fire"); }
    display_set_fx(fx);
}

void fx_options() {
    const auto& o = g_cfg.port;
    if (o.count("fx")) effects().on = o.at("fx") != "off";
    if (o.count("fx-intensity")) effects().intensity = std::clamp(std::atoi(o.at("fx-intensity").c_str()), 10, 100);
}

}  // namespace eba
