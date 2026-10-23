// The options bar: a menu bar in the window (Dear ImGui) for the display,
// the input and the timing, with a window that calibrates the timing from
// the taps the game judges. Each change is made at once and saved
// (eba_settings.cpp). In a window the bar sits above the screens (the window
// keeps room for it); on the whole display it shows while the mouse is near
// the top. F1 hides it, or shows it, anywhere. While the mouse is over it or
// one of its menus, the mouse is not the stylus.
#include "eba.h"
#include "display.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace eba {
namespace {

constexpr double kFrameMs = 1000.0 * 263 * 355 * 6 / 33513982.0;   // 16.7154 ms

bool g_ready, g_shown = true, g_calibration, g_shortcuts, g_about;
int g_rebind = -1;                                  // the hit key waiting for a press
float g_bar_h;

void push_key(SDL_Scancode sc) {
    SDL_Event e{};
    e.type = SDL_EVENT_KEY_DOWN;
    e.key.scancode = sc;
    e.key.down = true;
    e.key.windowID = SDL_GetWindowID(display_window());
    SDL_PushEvent(&e);
}

void change(const DisplaySettings& s) {
    display_change(s);
    settings_save();
}

// The skins (eba_skins.cpp): texture packs over the notes' pictures.
void skin_menu() {
    Skin& k = skin();
    if (ImGui::BeginMenu("Skin")) {
        auto names = skins_list();
        if (ImGui::MenuItem("The game's own", nullptr, k.name.empty())) { k.name.clear(); skins_apply(); settings_save(); }
        for (auto& n : names)
            if (ImGui::MenuItem(n.c_str(), nullptr, k.name == n)) { k.name = n; skins_apply(); settings_save(); }
        if (names.empty()) ImGui::TextDisabled("(no skins in the %s folder)", game().skins_dir);
        ImGui::Separator();
        ImGui::TextDisabled("Pictures only: the game's sounds stay.");
        ImGui::EndMenu();
    }
    if (hd_rings_available()) {
        if (ImGui::MenuItem("HD approach circles", nullptr, k.hd_rings)) { k.hd_rings = !k.hd_rings; skins_apply(); settings_save(); }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("The shrinking rings drawn smooth at any resolution (a skin's own come first).");
    }
}

void display_menu() {
    DisplaySettings s = display_settings();
    if (ImGui::BeginMenu("Resolution")) {
        if (ImGui::MenuItem("Auto (follows the window)", nullptr, s.scale == 0)) { s.scale = 0; change(s); }
        for (int n = 1; n <= kMaxScale; ++n) {
            char t[64];
            std::snprintf(t, sizeof t, "%dx  (%dx%d a screen)%s", n, 256 * n, 192 * n, n == kMaxScale ? "  4K" : "");
            if (ImGui::MenuItem(t, nullptr, s.scale == n)) { s.scale = n; change(s); }
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Layout")) {
        const char* names[] = {"Vertical (one above the other)", "Side by side", "Top screen only", "Bottom screen only"};
        for (int l = 0; l < 4; ++l)
            if (ImGui::MenuItem(names[l], l == 0 ? "F9 cycles" : nullptr, (int)s.layout == l)) {
                s.layout = (NdsLayout)l;
                change(s);
            }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Filter")) {
        struct { const char* name; const char* tip; } f[] = {
            {"Sharp", "The DS's pixels, crisp (the default)."},
            {"Bilinear", "Pixels blended with their neighbours: soft."},
            {"Bicubic", "Smooth curves between the pixels, still fairly crisp."},
            {"Soft", "The most blended (a B-spline): the least pixelated."}};
        for (int i = 0; i < 4; ++i) {
            if (ImGui::MenuItem(f[i].name, nullptr, s.filter == i)) { s.filter = i; change(s); }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", f[i].tip);
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Frame rate")) {
        struct { const char* name; int fps; } r[] = {
            {"DS (59.83, accurate)", 0}, {"60", 60}, {"120", 120}, {"240", 240}, {"Uncapped", -1}};
        for (auto& x : r)
            if (ImGui::MenuItem(x.name, nullptr, s.fps == x.fps)) { s.fps = x.fps; change(s); }
        ImGui::Separator();
        ImGui::TextDisabled("60/120/240 run the game at 60 (0.29%% fast),");
        ImGui::TextDisabled("smooth on a display at that rate.");
        ImGui::EndMenu();
    }
    if (*game().skins_dir) {
        ImGui::Separator();
        skin_menu();
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Fullscreen", "F11 / Alt+Enter", s.fullscreen)) { s.fullscreen = !s.fullscreen; change(s); }
    if (ImGui::MenuItem("Low latency", nullptr, s.low_latency)) { s.low_latency = !s.low_latency; change(s); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Wait for each picture to be drawn: no frames queued in the driver.");
}

void input_menu() {
    Buttons& b = buttons();
    if (ImGui::MenuItem("Button mode", nullptr, b.on)) { b.on = !b.on; settings_save(); }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("In a song, a gamepad's face buttons, its d-pad or the keys below hit the notes:\n"
                          "tap on the beat, hold through sliders, mash for spinners.");
    ImGui::Separator();
    ImGui::TextDisabled("Hit keys (keyboard)");
    for (int i = 0; i < 4; ++i) {
        ImGui::PushID(i);
        const char* name = b.keys[i] ? SDL_GetScancodeName((SDL_Scancode)b.keys[i]) : "-";
        char t[96];
        std::snprintf(t, sizeof t, "Key %d: %s", i + 1, g_rebind == i ? "press a key... (Esc: cancel)" : name);
        if (ImGui::MenuItem(t, nullptr, false)) g_rebind = i;
        if (b.keys[i] && ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Clear")) { b.keys[i] = 0; settings_save(); }
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
    ImGui::TextDisabled("Click a key to change it; right-click to clear.");
    ImGui::Separator();
    ImGui::TextDisabled("Gamepads");
    int n = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&n);
    for (int i = 0; i < n; ++i) ImGui::BulletText("%s", SDL_GetGamepadNameForID(ids[i]));
    if (!n) ImGui::BulletText("none connected");
    SDL_free(ids);
    ImGui::TextDisabled("Outside songs: bottom B, right A, left Y, top X,");
    ImGui::TextDisabled("d-pad, Start, Back = Select, shoulders L/R.");
    ImGui::Separator();
    ImGui::TextDisabled("Keyboard as the DS: X A, Z B, S X, A Y, Q L, W R,");
    ImGui::TextDisabled("Enter Start, Backspace Select, arrows. Mouse: stylus.");
}

void timing_menu() {
    int off = offset();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12);
    char fmt[64];
    std::snprintf(fmt, sizeof fmt, "%%+d frames (%+.0f ms)", off * kFrameMs);
    if (ImGui::SliderInt("Offset", &off, -8, 8, fmt)) { set_offset(off); settings_save(); }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Taps this many frames after their note are judged as on it.\n"
                          "Raise it if you hit late (your sound or picture reaches you late).");
    float lat = (float)audio_latency();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12);
    std::snprintf(fmt, sizeof fmt, "%%.1f frames (%.0f ms)", lat * kFrameMs);
    if (ImGui::SliderFloat("Sound latency", &lat, 1.0f, 6.0f, fmt)) {
        audio_set_latency(std::round(lat * 2) / 2);
        settings_save();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("The sound queued ahead of the device: less is sooner, too little crackles.");
    ImGui::Separator();
    if (ImGui::MenuItem("Calibrate...", nullptr, g_calibration)) g_calibration = !g_calibration;
}

// Light behind the 3D in a song (eba_fx.cpp), off by default.
void effects_menu() {
    Effects& e = effects();
    bool eba = game().id == std::string("EBA");
    if (ImGui::MenuItem(eba ? "Spotlights behind the Agents" : "Fire behind the squad", nullptr, e.on)) {
        e.on = !e.on;
        settings_save();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(eba ? "Spotlights rise behind the Agents in a song, swinging with each hit,\n"
                                "brighter as the combo grows."
                              : "Fire rises behind the squad in a song, taller as the combo grows.");
    if (ImGui::MenuItem(eba ? "Elite Beat flashes" : "Bursts on perfect groups", nullptr, e.flashes)) {
        e.flashes = !e.flashes;
        settings_save();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("A flash when a group of notes ends all 300.");
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10);
    if (ImGui::SliderInt("Intensity", &e.intensity, 10, 100, "%d%%")) settings_save();
}

// The game's own tuning (eba_play.cpp), over what it sets for each stage: how long notes
// show, how wide the hit circle is.
void assist_menu() {
    Assists& a = assists();
    ImGui::TextDisabled("Approach (how long a note shows before its beat)");
    struct { const char* name; int pct; } app[] = {
        {"Normal (the game's own)", 0}, {"Slow  (+30%)", 130}, {"Slower  (+65%)", 165}, {"Fast  (-20%)", 80}};
    for (auto& x : app)
        if (ImGui::MenuItem(x.name, nullptr, a.approach_pct == x.pct)) { a.approach_pct = x.pct; settings_save(); }
    ImGui::TextDisabled("(from the next song)");
    ImGui::Separator();
    ImGui::TextDisabled("Hit circle leniency (Customs keep no score above Normal)");
    struct { const char* name; int px; } rad[] = {{"Normal", 0}, {"Generous  (+5 px)", 5}, {"Very generous  (+11 px)", 11}};
    for (auto& x : rad)
        if (ImGui::MenuItem(x.name, nullptr, a.hit_extra == x.px)) { a.hit_extra = x.px; settings_save(); }
    if (!game().gauge_on) return;
    ImGui::Separator();
    if (ImGui::MenuItem("No-Fail", nullptr, a.no_fail)) { a.no_fail = !a.no_fail; settings_save(); }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("The life gauge stays where it starts: misses never end the song.\n"
                          "Customs keep no score with it.");
    ImGui::BeginDisabled(!a.no_fail);
    if (ImGui::MenuItem("   Hide the life bar", nullptr, a.hide_life)) { a.hide_life = !a.hide_life; settings_save(); }
    ImGui::EndDisabled();
}

void calibration_window() {
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 26, 0), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Timing calibration", &g_calibration)) { ImGui::End(); return; }
    TimingStats s = timing_stats();
    ImGui::TextWrapped("Play a song and hit as you normally do. Each tap the game judges is counted "
                       "by how many frames after its note you hit it, before the offset.");
    ImGui::Separator();
    float hist[17];
    float top = 1;
    for (int i = 0; i < 17; ++i) top = std::max(top, (float)(hist[i] = (float)s.hist[i]));
    ImGui::PlotHistogram("##taps", hist, 17, 0, "-8                 early | late                 +8", 0, top,
                         ImVec2(-1, ImGui::GetFontSize() * 6));
    if (s.count) {
        ImGui::Text("%d taps: on average %+.2f frames (%+.0f ms) %s, spread %.2f frames", s.count, s.mean,
                    s.mean * kFrameMs, s.mean >= 0 ? "late" : "early", s.stdev);
        int want = (int)std::lround(s.mean);
        ImGui::Text("Offset now %+d; suggested %+d", offset(), std::clamp(want, -8, 8));
        ImGui::SameLine();
        ImGui::BeginDisabled(want == offset() || s.count < 20);
        if (ImGui::Button("Apply")) { set_offset(want); settings_save(); }
        ImGui::EndDisabled();
        if (s.count < 20) ImGui::TextDisabled("(at least 20 taps before suggesting)");
    } else {
        ImGui::TextDisabled("No taps yet.");
    }
    ImGui::Text("Judged: 300 x%d   100 x%d   50 x%d   miss x%d", s.grades[0], s.grades[1], s.grades[2], s.grades[3]);
    if (ImGui::Button("Reset")) timing_reset();
    ImGui::End();
}

void help_windows() {
    if (g_shortcuts) {
        if (ImGui::Begin("Shortcuts", &g_shortcuts, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("F1   the options bar");
            ImGui::Text("F9   the next screen layout");
            ImGui::Text("F11  fullscreen (or Alt+Enter)");
            ImGui::Text("F12  a screenshot into the run's folder");
            ImGui::Text("Esc  quit");
        }
        ImGui::End();
    }
    if (g_about) {
        if (ImGui::Begin("About", &g_about, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s, recompiled to native code (eba-recomp)", game().title);
            ImGui::Text("on ndskit, a static recompiler and runtime for the Nintendo DS.");
            ImGui::TextDisabled("Dear ImGui %s", IMGUI_VERSION);
        }
        ImGui::End();
    }
}

void setup() {
    SDL_Window* win = display_window();
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    float dpi = SDL_GetWindowDisplayScale(win);
    if (dpi <= 0) dpi = 1;
    style.ScaleAllSizes(dpi);
    style.FontScaleDpi = dpi;
    ImGui_ImplSDL3_InitForOpenGL(win, SDL_GL_GetCurrentContext());
    ImGui_ImplOpenGL3_Init("#version 450");
    g_ready = true;
}

// before each present: the bar over the picture
void draw(int, int) {
    if (!g_ready) setup();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    bool full = display_settings().fullscreen;
    float mx, my;
    SDL_GetMouseState(&mx, &my);
    bool near_top = my < ImGui::GetFrameHeight() * 1.5f;
    bool menu_open = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
    bool bar = g_shown && (!full || near_top || menu_open);
    if (bar && ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Game")) {
            if (ImGui::MenuItem("Customs...")) customs_ui_open();
            if (custom_playing() && ImGui::MenuItem("Back to campaign")) customs_back_to_campaign();
            ImGui::Separator();
            if (ImGui::MenuItem("Screenshot", "F12")) push_key(SDL_SCANCODE_F12);
            if (ImGui::MenuItem("Quit", "Esc")) { SDL_Event e{}; e.type = SDL_EVENT_QUIT; SDL_PushEvent(&e); }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Display")) { display_menu(); ImGui::EndMenu(); }
        if (ImGui::BeginMenu("Input")) { input_menu(); ImGui::EndMenu(); }
        if (ImGui::BeginMenu("Timing")) { timing_menu(); ImGui::EndMenu(); }
        if (game().approach && ImGui::BeginMenu("Assist")) { assist_menu(); ImGui::EndMenu(); }
        if (ImGui::BeginMenu("Effects")) { effects_menu(); ImGui::EndMenu(); }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Shortcuts")) g_shortcuts = true;
            if (ImGui::MenuItem("About")) g_about = true;
            ImGui::EndMenu();
        }
        g_bar_h = ImGui::GetWindowHeight();
        ImGui::EndMainMenuBar();
    }
    display_set_top_margin(g_shown && !full ? (int)std::ceil(g_bar_h) : 0);
    if (g_calibration) calibration_window();
    help_windows();
    customs_ui_draw();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// each host event, before the runtime: the UI's own, a key being bound, F1
bool event(const void* p) {
    const SDL_Event& e = *static_cast<const SDL_Event*>(p);
    if (!g_ready) return false;
    ImGui_ImplSDL3_ProcessEvent(&e);
    ImGuiIO& io = ImGui::GetIO();
    if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
        if (g_rebind >= 0) {
            if (e.key.scancode != SDL_SCANCODE_ESCAPE) {
                buttons().keys[g_rebind] = e.key.scancode;
                settings_save();
            }
            g_rebind = -1;
            return true;
        }
        if (e.key.scancode == SDL_SCANCODE_F1) { g_shown = !g_shown; return true; }
        if (e.key.scancode == SDL_SCANCODE_ESCAPE &&
            ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
            return true;                            // Esc closes the menu, not the game
    }
    bool mouse = e.type == SDL_EVENT_MOUSE_BUTTON_DOWN || e.type == SDL_EVENT_MOUSE_BUTTON_UP ||
                 e.type == SDL_EVENT_MOUSE_MOTION || e.type == SDL_EVENT_MOUSE_WHEEL;
    bool keys = e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP || e.type == SDL_EVENT_TEXT_INPUT;
    return (mouse && io.WantCaptureMouse) || (keys && io.WantCaptureKeyboard);
}

struct Register {
    Register() {
        DisplayHooks h;
        h.event = event;
        h.draw = draw;
        display_set_hooks(h);
    }
} registered;

}  // namespace

bool ui_wants_mouse() { return g_ready && ImGui::GetIO().WantCaptureMouse; }

}  // namespace eba
