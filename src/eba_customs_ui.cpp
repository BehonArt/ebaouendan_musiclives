// The Customs screen and its button (eba_customs.cpp is the mode itself).
//
// On the game's main menu a CUSTOMS button sits over the lower screen,
// wherever the layout puts it. It opens the Customs screen: each custom's art,
// title and artist, the stage it replaces with that stage's own card as the
// game shows it, a preview of each (the custom's from its HD audio, the
// stage's own as the game plays it; the game's sound is muted while one
// plays), the best scores of each difficulty (customs/scores.ini), and Play.
// In Customs mode a loading screen covers the autopilot's way to the stage,
// and the Customs screen comes back once the song's results are left.
#include "eba.h"
#include "display.h"
#include "gl.h"
#include "imgui.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <filesystem>
#include <map>
#include <string>

namespace eba {
namespace {

bool g_open;
char g_diff = 'h';
bool g_diff_set;

// ---- images (BMP, from the bake) -----------------------------------------------------------
struct Tex { GLuint id = 0; int w = 0, h = 0; };
std::map<std::string, Tex> g_tex;

const Tex& texture(const std::string& path) {
    auto it = g_tex.find(path);
    if (it != g_tex.end()) return it->second;
    Tex t;
    if (SDL_Surface* s = SDL_LoadBMP(path.c_str())) {
        if (SDL_Surface* rgba = SDL_ConvertSurface(s, SDL_PIXELFORMAT_ABGR8888)) {
            glCreateTextures(GL_TEXTURE_2D, 1, &t.id);
            glTextureStorage2D(t.id, 1, GL_RGBA8, rgba->w, rgba->h);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            glTextureSubImage2D(t.id, 0, 0, 0, rgba->w, rgba->h, GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);
            glTextureParameteri(t.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(t.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            t.w = rgba->w;
            t.h = rgba->h;
            SDL_DestroySurface(rgba);
        }
        SDL_DestroySurface(s);
    }
    return g_tex[path] = t;
}

std::string asset(const Custom& c, const char* key) {
    auto it = c.extra.find(key);
    return it == c.extra.end() ? "" : (std::filesystem::path(c.dir) / it->second).string();
}

void image(const Custom& c, const char* key, float w) {
    const Tex& t = texture(asset(c, key));
    if (t.id) ImGui::Image(ImTextureRef((ImTextureID)(intptr_t)t.id), ImVec2(w, w * t.h / std::max(t.w, 1)));
    else ImGui::Dummy(ImVec2(w, w * 0.75f));
}

// ---- previews ------------------------------------------------------------------------------
SDL_AudioStream* g_preview;
std::string g_preview_path;
Uint8* g_preview_buf;

void preview_stop() {
    if (g_preview) SDL_DestroyAudioStream(g_preview);
    if (g_preview_buf) SDL_free(g_preview_buf);
    g_preview = nullptr;
    g_preview_buf = nullptr;
    g_preview_path.clear();
    sound_set_muted(0);
}

void preview_play(const std::string& path) {
    bool same = path == g_preview_path;
    preview_stop();
    if (same || path.empty()) return;                   // pressed again: stopped
    SDL_AudioSpec spec;
    Uint32 len = 0;
    if (!SDL_LoadWAV(path.c_str(), &spec, &g_preview_buf, &len)) return;
    g_preview = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!g_preview) return;
    SDL_PutAudioStreamData(g_preview, g_preview_buf, (int)len);
    SDL_FlushAudioStream(g_preview);
    SDL_ResumeAudioStreamDevice(g_preview);
    g_preview_path = path;
    sound_set_muted(0xFFFF);                            // the game's own sound, quiet meanwhile
}

bool preview_playing(const std::string& path) {
    if (!g_preview || path != g_preview_path) return false;
    if (SDL_GetAudioStreamQueued(g_preview) == 0) { preview_stop(); return false; }
    return true;
}

// ---- the screen ----------------------------------------------------------------------------
const char kDiffs[4] = {'e', 'n', 'h', 'x'};

void scores_line(const Custom& c) {
    for (char d : kDiffs) {
        CustomScore s = custom_best(c.tag, d);
        ImGui::TextDisabled("%s", customs_difficulty_name(d));
        ImGui::SameLine(ImGui::GetFontSize() * 5.5f);
        if (!s.score) { ImGui::TextDisabled("-"); continue; }
        ImGui::Text("%u%s", s.score, s.cleared ? "  cleared" : "");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("300 x%d   100 x%d   50 x%d   miss x%d", s.grades[0], s.grades[1], s.grades[2], s.grades[3]);
    }
}

void customs_window() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x * 0.5f, vp->WorkPos.y + vp->WorkSize.y * 0.5f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(std::min(vp->WorkSize.x * 0.96f, ImGui::GetFontSize() * 62),
                                    std::min(vp->WorkSize.y * 0.94f, ImGui::GetFontSize() * 48)), ImGuiCond_Always);
    bool open = true;
    const Custom* now = custom_playing();
    if (!ImGui::Begin("Customs", &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::End();
        return;
    }
    if (!g_diff_set) { g_diff = now ? custom_difficulty() : 'h'; g_diff_set = true; }
    ImGui::TextWrapped("Songs charted over the story's stages. Each plays as its own mode, with its own save and "
                       "scores: your campaign is never touched.");
    ImGui::Text("Difficulty:");
    for (char d : kDiffs) {                             // on the same line while they fit
        float w = ImGui::CalcTextSize(customs_difficulty_name(d)).x + ImGui::GetFrameHeight() * 2;
        if (ImGui::GetItemRectMax().x + w < ImGui::GetWindowPos().x + ImGui::GetContentRegionMax().x) ImGui::SameLine();
        if (ImGui::RadioButton(customs_difficulty_name(d), g_diff == d)) g_diff = d;
    }
    if (now && ImGui::Button("Back to campaign")) { preview_stop(); customs_back_to_campaign(); }
    ImGui::Separator();
    float fs = ImGui::GetFontSize();
    float art_w = std::clamp(ImGui::GetContentRegionAvail().x * 0.3f, fs * 7, fs * 12);
    ImGui::BeginChild("list");
    if (ImGui::BeginTable("customs", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_PadOuterX)) {
        ImGui::TableSetupColumn("art", ImGuiTableColumnFlags_WidthFixed, art_w);
        ImGui::TableSetupColumn("info", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("stage", ImGuiTableColumnFlags_WidthFixed, art_w);
        for (const Custom& c : customs()) {
            ImGui::PushID(c.tag.c_str());
            ImGui::TableNextRow();
            ImGui::TableNextColumn();                   // the custom: its art and HD preview
            image(c, "art", art_w);
            std::string pc = asset(c, "preview_custom");
            if (ImGui::Button(preview_playing(pc) ? "Stop##c" : "Preview (HD)", ImVec2(art_w, 0))) preview_play(pc);
            ImGui::TableNextColumn();                   // its title, scores, Play
            ImGui::PushTextWrapPos(0);
            ImGui::Text("%s", c.title.c_str());
            ImGui::TextDisabled("%s", c.artist.c_str());
            ImGui::PopTextWrapPos();
            ImGui::Spacing();
            scores_line(c);
            ImGui::Spacing();
            bool here = now && now->tag == c.tag;
            if (ImGui::Button(here ? "Play again" : "Play", ImVec2(std::min(fs * 8, ImGui::GetContentRegionAvail().x), 0))) {
                preview_stop();
                customs_launch(c, g_diff);
            }
            ImGui::TableNextColumn();                   // the stage it replaces: its card and its own preview
            ImGui::PushTextWrapPos(0);
            ImGui::TextDisabled("Replaces %s (%s)", c.replaces.c_str(), c.stage.c_str());
            ImGui::PopTextWrapPos();
            image(c, "stage_card", art_w);
            std::string po = asset(c, "preview_original");
            if (ImGui::Button(preview_playing(po) ? "Stop##o" : "Original preview", ImVec2(art_w, 0))) preview_play(po);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    if (customs().empty()) ImGui::TextDisabled("No customs found (customs/ beside the cartridge).");
    ImGui::EndChild();
    ImGui::End();
    if (!open) { g_open = false; preview_stop(); }
}

void loading_screen() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::Begin("##loading", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                                           ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBringToFrontOnFocus);
    const Custom* c = custom_playing();
    float w = std::min(vp->Size.x * 0.6f, vp->Size.y * 0.6f * 4 / 3);
    ImGui::SetCursorPos(ImVec2((vp->Size.x - w) * 0.5f, vp->Size.y * 0.12f));
    image(*c, "art", w);
    char t[256];
    std::snprintf(t, sizeof t, "Loading  %s  (%s)...", c->title.c_str(), customs_difficulty_name(custom_difficulty()));
    ImGui::SetCursorPosX((vp->Size.x - ImGui::CalcTextSize(t).x) * 0.5f);
    ImGui::Text("%s", t);
    ImGui::End();
}

void failed_screen() {
    ImGui::OpenPopup("Customs##failed");
    if (ImGui::BeginPopupModal("Customs##failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("The way to the custom's stage did not go as expected.");
        ImGui::Text("(Customs mode drives the menus on a known save: see the log.)");
        if (ImGui::Button("Back to campaign")) customs_back_to_campaign();
        ImGui::EndPopup();
    }
}

void menu_button() {
    float x, y, w, h;
    if (!display_screen_rect(1, &x, &y, &w, &h)) display_screen_rect(0, &x, &y, &w, &h);
    float k = h / 192.0f;                               // the lower screen's top strip, above "Select Mission Type"
    ImGui::SetNextWindowPos(ImVec2(x + w * 0.5f, y), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::Begin("##customs_button", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                                                  ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
    ImGui::SetWindowFontScale(std::max(0.8f, k * 0.7f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.45f, 0.05f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.6f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.75f, 0.3f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5 * k);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2 * k));
    if (ImGui::Button("  CUSTOMS  ", ImVec2(0, 14 * k))) g_open = true;
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
    ImGui::End();
}

}  // namespace

void customs_ui_open() { g_open = true; }

// Each picture, after the options bar's menus (eba_ui.cpp).
void customs_ui_draw() {
    if (customs_autopilot()) { loading_screen(); return; }
    if (customs_autopilot_failed()) { failed_screen(); return; }
    if (customs_take_show_request()) g_open = true;
    static bool asked;                                  // --port customs-screen: open at the main menu (a test's)
    if (!asked && g_cfg.port.count("customs-screen") && scene_is_main_menu()) { asked = true; g_open = true; }
    if (!custom_playing() && !g_open && scene_is_main_menu()) menu_button();
    if (g_open) customs_window();
    else if (g_preview) preview_stop();
}

}  // namespace eba
