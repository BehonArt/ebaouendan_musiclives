// The settings: the window's (display.h), the sound's latency, button mode's,
// the timing offset, the assists, the effects and the skin, in a file of
// KEY=VALUE lines in the game's data folder (eba_paths.cpp: on Windows
// %APPDATA%\eba-recomp\EBA\settings.ini, ...\OE2\, ...\OE1\; portable, beside
// the executable). A window's
// run reads it at its first frame and the options bar writes it at each
// change; what ndsrun's own options set (--scale 3, --port offset=2) wins over
// it for that run. A headless run neither reads nor writes it: its results
// depend on its options alone.
#include "eba.h"
#include "display.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>

namespace eba {
namespace {

std::string path() {
    std::string dir = data_dir();
    return dir.empty() ? "" : dir + "settings.ini";
}

const char* kLayouts[] = {"stacked", "side", "top", "bottom"};

}  // namespace

void settings_load() {
    std::string p = path();
    std::ifstream f(p);
    if (!f) return;
    std::map<std::string, std::string> v;
    std::string line;
    while (std::getline(f, line)) {
        size_t e = line.find('=');
        if (e != std::string::npos && line[0] != '#') v[line.substr(0, e)] = line.substr(e + 1);
    }
    auto num = [&](const char* k, double d) { return v.count(k) ? std::atof(v[k].c_str()) : d; };
    DisplaySettings s = display_settings();
    if (!g_cfg.scale) s.scale = (int)num("scale", s.scale);
    if (!g_cfg.fps) s.fps = (int)num("fps", s.fps);
    if (!g_cfg.fullscreen) s.fullscreen = num("fullscreen", s.fullscreen) != 0;
    if (!g_cfg.low_latency) s.low_latency = num("low_latency", s.low_latency) != 0;
    if (!g_cfg.filter) s.filter = std::clamp((int)num("filter", s.filter), 0, 3);
    if (!g_cfg.vsync) s.vsync = std::clamp((int)num("vsync", s.vsync), 0, 2);
    if (!g_cfg.layout && v.count("layout"))
        for (int l = 0; l < 4; ++l)
            if (v["layout"] == kLayouts[l]) s.layout = (NdsLayout)l;
    display_change(s);
    if (g_cfg.audio_latency == 3) audio_set_latency(num("audio_latency", 3));
    Buttons& b = buttons();
    b.on = num("buttons", 1) != 0;
    b.trigger_feel = num("trigger_feel", 1) != 0;
    for (int i = 0; i < 4; ++i) {
        std::string k = "button_key" + std::to_string(i + 1);
        if (v.count(k)) b.keys[i] = v[k].empty() ? 0 : SDL_GetScancodeFromName(v[k].c_str());
    }
    set_offset((int)num("offset", 0));
    assists().approach_pct = (int)num("approach_pct", 0);
    assists().hit_extra = (int)num("hit_extra", 0);
    effects().on = num("fx", 0) != 0;
    effects().intensity = std::clamp((int)num("fx_intensity", 70), 10, 100);
    effects().flashes = num("fx_flashes", 1) != 0;
    assists().no_fail = num("no_fail", 0) != 0;
    rumble().on = num("rumble", 1) != 0;
    rumble().strength = std::clamp((int)num("rumble_strength", 100), 10, 100);
    assists().hide_life = num("hide_life", 1) != 0;
    skin().name = v.count("skin") ? v["skin"] : "";
    skin().hd_rings = std::clamp((int)num("hd_rings", 0), 0, 3);
    achievements().on = num("achievements", 1) != 0;
    nds_note("eba: settings from %s", p.c_str());
}

void settings_save() {
    if (!g_cfg.window) return;
    std::string p = path();
    std::FILE* f = p.empty() ? nullptr : std::fopen(p.c_str(), "w");
    if (!f) { nds_note("eba: cannot write %s", p.c_str()); return; }
    const DisplaySettings& s = display_settings();
    std::fprintf(f, "# %s (eba-recomp): the options bar writes this file\n", game().id);
    std::fprintf(f, "scale=%d\nlayout=%s\nfps=%d\nfullscreen=%d\nlow_latency=%d\naudio_latency=%g\n", s.scale,
                 kLayouts[(int)s.layout], s.fps, s.fullscreen ? 1 : 0, s.low_latency ? 1 : 0, audio_latency());
    const Buttons& b = buttons();
    std::fprintf(f, "buttons=%d\ntrigger_feel=%d\n", b.on ? 1 : 0, b.trigger_feel ? 1 : 0);
    for (int i = 0; i < 4; ++i)
        std::fprintf(f, "button_key%d=%s\n", i + 1, b.keys[i] ? SDL_GetScancodeName((SDL_Scancode)b.keys[i]) : "");
    std::fprintf(f, "offset=%d\n", offset());
    std::fprintf(f, "filter=%d\nvsync=%d\n", s.filter, s.vsync);
    std::fprintf(f, "fx=%d\nfx_intensity=%d\nfx_flashes=%d\n", effects().on ? 1 : 0, effects().intensity,
                 effects().flashes ? 1 : 0);
    std::fprintf(f, "approach_pct=%d\nhit_extra=%d\n", assists().approach_pct, assists().hit_extra);
    std::fprintf(f, "rumble=%d\nrumble_strength=%d\n", rumble().on ? 1 : 0, rumble().strength);
    std::fprintf(f, "no_fail=%d\nhide_life=%d\n", assists().no_fail ? 1 : 0, assists().hide_life ? 1 : 0);
    std::fprintf(f, "achievements=%d\n", achievements().on ? 1 : 0);
    std::fprintf(f, "skin=%s\nhd_rings=%d\n", skin().name.c_str(), skin().hd_rings);
    std::fclose(f);
}

void init() {
    if (g_cfg.window) settings_load();
    buttons_options();
    timing_options();
    play_options();
    fx_options();
    skins_options();
    rumble_options();
    customs_init();
    ra_init();
}

}  // namespace eba
