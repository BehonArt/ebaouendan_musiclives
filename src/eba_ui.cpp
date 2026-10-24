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
#include "rc_client.h"
#include <SDL3/SDL.h>
#include <cfloat>
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

// The menus have no submenus: in a small window one cannot sit beside its menu and
// covers it. Each choice is a list under a grey heading instead.
void heading(const char* t) {
    ImGui::Spacing();
    ImGui::TextDisabled("%s", t);
}

void display_menu() {
    DisplaySettings s = display_settings();
    heading("Resolution");
    if (ImGui::MenuItem("Auto (follows the window)", nullptr, s.scale == 0)) { s.scale = 0; change(s); }
    for (int n = 1; n <= kMaxScale; ++n) {
        char t[64];
        std::snprintf(t, sizeof t, "%dx  (%dx%d a screen)%s", n, 256 * n, 192 * n, n == kMaxScale ? "  4K" : "");
        if (ImGui::MenuItem(t, nullptr, s.scale == n)) { s.scale = n; change(s); }
    }
    ImGui::Separator();
    heading("Layout (F9 cycles)");
    const char* names[] = {"Vertical (one above the other)", "Side by side", "Top screen only", "Bottom screen only"};
    for (int l = 0; l < 4; ++l)
        if (ImGui::MenuItem(names[l], nullptr, (int)s.layout == l)) {
            s.layout = (NdsLayout)l;
            change(s);
        }
    ImGui::Separator();
    heading("Frame rate");
    struct { const char* name; int fps; } r[] = {
        {"DS (59.83, accurate)", 0}, {"60", 60}, {"120", 120}, {"240", 240}, {"Uncapped", -1}};
    for (auto& x : r) {
        if (ImGui::MenuItem(x.name, nullptr, s.fps == x.fps)) { s.fps = x.fps; change(s); }
        if (x.fps > 0 && ImGui::IsItemHovered())
            ImGui::SetTooltip("Runs the game at 60 (0.29%% fast): smooth on a display at this rate.");
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Fullscreen", "F11 / Alt+Enter", s.fullscreen)) { s.fullscreen = !s.fullscreen; change(s); }
    if (ImGui::MenuItem("Low latency", nullptr, s.low_latency)) { s.low_latency = !s.low_latency; change(s); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Wait for each picture to be drawn: no frames queued in the driver.");
}

// The picture's filter; the skins (eba_skins.cpp): texture packs over the notes' pictures.
void look_menu() {
    DisplaySettings s = display_settings();
    heading("Filter");
    struct { const char* name; const char* tip; } f[] = {
        {"Sharp", "The DS's pixels, crisp (the default)."},
        {"Bilinear", "Pixels blended with their neighbours: soft."},
        {"Bicubic", "Smooth curves between the pixels, still fairly crisp."},
        {"Soft", "The most blended (a B-spline): the least pixelated."}};
    for (int i = 0; i < 4; ++i) {
        if (ImGui::MenuItem(f[i].name, nullptr, s.filter == i)) { s.filter = i; change(s); }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", f[i].tip);
    }
    if (!*game().skins_dir) return;
    Skin& k = skin();
    ImGui::Separator();
    heading("Skin (pictures only: the game's sounds stay)");
    auto skins = skins_list();
    if (ImGui::MenuItem("The game's own", nullptr, k.name.empty())) { k.name.clear(); skins_apply(); settings_save(); }
    for (auto& n : skins)
        if (ImGui::MenuItem(n.c_str(), nullptr, k.name == n)) { k.name = n; skins_apply(); settings_save(); }
    if (skins.empty()) ImGui::TextDisabled("(no skins in the %s folder)", game().skins_dir);
    if (!hd_rings_available(1)) return;
    ImGui::Separator();
    heading("HD approach circles");
    const char* widths[] = {"Off (the game's own)", "Normal", "Thin", "Thinner"};
    for (int w = 0; w < 4; ++w) {
        if ((w == 0 || hd_rings_available(w)) && ImGui::MenuItem(widths[w], nullptr, k.hd_rings == w)) {
            k.hd_rings = w;
            skins_apply();
            settings_save();
        }
        if (w && ImGui::IsItemHovered())
            ImGui::SetTooltip("The shrinking rings drawn smooth at any resolution (a skin's own come first).");
    }
}

void input_menu() {
    Buttons& b = buttons();
    if (ImGui::MenuItem("Button mode", nullptr, b.on)) { b.on = !b.on; settings_save(); }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("In a song, a gamepad's face buttons, its d-pad or the keys below hit the notes:\n"
                          "tap on the beat, hold through sliders, mash or turn the left stick round for spinners.");
    Rumble& r = rumble();
    ImGui::BeginDisabled(!b.on);
    if (ImGui::MenuItem("Rumble", nullptr, r.on)) { r.on = !r.on; settings_save(); }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("In button mode, the gamepad's motors (a DualSense's haptics, a Switch controller's HD rumble):\n"
                          "strong for the note that ends a combo, medium for each hit and slider tick,\n"
                          "a slight hum through sliders and spinners.");
    ImGui::BeginDisabled(!r.on);
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10);
    if (ImGui::SliderInt("Rumble strength", &r.strength, 10, 100, "%d%%")) settings_save();
    ImGui::EndDisabled();
    ImGui::EndDisabled();
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
        ImGui::SetTooltip("The life gauge stays where it starts: misses never end the song.");
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

// RetroAchievements (eba_achievements.cpp): the sign-in, the game's set, why it is paused
bool g_ra;
char g_ra_user[64], g_ra_pass[128];

void ra_window() {
    if (!g_ra) return;
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 30, ImGui::GetFontSize() * 26), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("RetroAchievements", &g_ra)) { ImGui::End(); return; }
    Achievements& a = achievements();
    ImGui::TextWrapped("%s", ra_status().c_str());
    if (const char* why = ra_paused_by(); why && ra_signed_in() && a.on)
        ImGui::TextColored(ImVec4(1, 0.75f, 0.3f, 1), "Paused: %s.", why);
    ImGui::TextDisabled("Casual mode. Achievements count only for the game as made: not in Customs,");
    ImGui::TextDisabled("button mode, with an assist (approach, hit circle, No-Fail), autoplay or a replay.");
    ImGui::Separator();
    if (!ra_signed_in()) {
        if (a.user[0] && !g_ra_user[0]) std::snprintf(g_ra_user, sizeof g_ra_user, "%s", a.user.c_str());
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14);
        ImGui::InputText("Username", g_ra_user, sizeof g_ra_user);
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14);
        bool go = ImGui::InputText("Password", g_ra_pass, sizeof g_ra_pass,
                                   ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);
        if ((ImGui::Button("Sign in") || go) && g_ra_user[0] && g_ra_pass[0]) {
            if (!a.on) ra_set_on(true);
            std::string pass = g_ra_pass;
            std::fill(std::begin(g_ra_pass), std::end(g_ra_pass), '\0');
            ra_login(g_ra_user, pass);
        }
        ImGui::TextDisabled("Your retroachievements.org account. Only RA's sign-in token is kept,");
        ImGui::TextDisabled("shared by the three games; never the password.");
        ImGui::End();
        return;
    }
    bool on = a.on;
    if (ImGui::Checkbox("Achievements", &on)) ra_set_on(on);
    ImGui::SameLine();
    if (ImGui::Button("Sign out")) ra_logout();
    ImGui::Separator();
    rc_client_t* c = ra_client();
    if (c && a.on && rc_client_get_game_info(c)) {
        rc_client_achievement_list_t* list =
            rc_client_create_achievement_list(c, RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE, RC_CLIENT_ACHIEVEMENT_LIST_GROUPING_PROGRESS);
        if (list) {
            for (uint32_t b = 0; b < list->num_buckets; ++b) {
                const rc_client_achievement_bucket_t& bk = list->buckets[b];
                ImGui::SeparatorText(bk.label);
                for (uint32_t i = 0; i < bk.num_achievements; ++i) {
                    const rc_client_achievement_t* x = bk.achievements[i];
                    bool got = x->unlocked != RC_CLIENT_ACHIEVEMENT_UNLOCKED_NONE;
                    ImGui::TextColored(got ? ImVec4(1, 0.85f, 0.35f, 1) : ImVec4(0.85f, 0.85f, 0.85f, 1), "%s  (%u)", x->title,
                                       x->points);
                    ImGui::Indent();
                    ImGui::TextWrapped("%s", x->description);
                    if (x->measured_progress[0]) ImGui::TextDisabled("%s", x->measured_progress);
                    ImGui::Unindent();
                }
            }
            rc_client_destroy_achievement_list(list);
        }
    }
    ImGui::End();
}

// the pop-ups, stacked at the lower right
void ra_toasts_draw() {
    auto toasts = ra_toasts();
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    float y = vp->WorkPos.y + vp->WorkSize.y - ImGui::GetFontSize();
    for (size_t i = toasts.size(); i-- > 0;) {
        char id[32];
        std::snprintf(id, sizeof id, "##ra_toast%zu", i);
        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x - ImGui::GetFontSize(), y), ImGuiCond_Always, ImVec2(1, 1));
        ImGui::SetNextWindowBgAlpha(0.85f);
        ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(ImGui::GetFontSize() * 26, FLT_MAX));
        ImGuiWindowFlags f = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs |
                             ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings;
        if (ImGui::Begin(id, nullptr, f)) {
            ImGui::TextColored(ImVec4(1, 0.85f, 0.35f, 1), "%s", toasts[i].title.c_str());
            if (!toasts[i].text.empty()) {
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 25);
                ImGui::TextUnformatted(toasts[i].text.c_str());
                ImGui::PopTextWrapPos();
            }
            y -= ImGui::GetWindowHeight() + ImGui::GetStyle().ItemSpacing.y;
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
            if (ImGui::MenuItem("RetroAchievements...")) g_ra = true;
            ImGui::Separator();
            if (ImGui::MenuItem("Screenshot", "F12")) push_key(SDL_SCANCODE_F12);
            if (ImGui::MenuItem("Quit", "Esc")) { SDL_Event e{}; e.type = SDL_EVENT_QUIT; SDL_PushEvent(&e); }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Display")) { display_menu(); ImGui::EndMenu(); }
        if (ImGui::BeginMenu("Look")) { look_menu(); ImGui::EndMenu(); }
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
    ra_window();
    ra_toasts_draw();
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
