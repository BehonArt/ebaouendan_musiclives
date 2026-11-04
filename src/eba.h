// The port's own code in ndsrun (EBA, Ouendan 2, Ouendan 1): what its parts
// share (button mode, timing, the options bar, the settings file, Customs).
#pragma once
#include "nds.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

struct rc_client_t;

namespace eba {

// The first frame (after ndsrun's options are read): the saved settings, then --port's over them.
void init();

// ---- the release (cmake -DEBA_RELEASE=ON, -DEBA_VERSION=x.y.z) -------------------------
// A release build leaves out what is not in it yet (Customs, RetroAchievements, replays
// saved to disk; docs/ROADMAP.md): the code stays, its menus and calls do not run.
#ifdef EBA_RELEASE
constexpr bool kRelease = true;
#else
constexpr bool kRelease = false;
#endif
#ifndef EBA_VERSION
#define EBA_VERSION "0.0.0-dev"
#endif

// ---- where the player's files go (eba_paths.cpp) ----------------------------------------
// A window's run with no --save: the save, the settings, screenshots and the log in
// %APPDATA%\eba-recomp\<game>\ (SDL's folder for the user's settings), or, with a file
// portable.txt beside the executable, in userdata\<game>\ beside it.
std::string data_dir();                 // that folder (ends with a separator)
std::string exe_dir();                  // the executable's folder (ends with a separator)
bool portable();

inline uint32_t rd(uint32_t a) { uint32_t v; std::memcpy(&v, g_main + (a & 0x3FFFFFu), 4); return v; }
inline int16_t rh(uint32_t a) { int16_t v; std::memcpy(&v, g_main + (a & 0x3FFFFFu), 2); return v; }
inline bool in_main(uint32_t a) { return a >= 0x02000000u && a < 0x02400000u; }
// a note's field, at EBA's offset (from +0x14 on, Ouendan 1's are 4 later): see note_field below

// ---- the game (games.cpp) ----------------------------------------------------------------
// What the port reads and writes of one game; 0 where it is not known (that
// part does nothing for the game).
struct Scenes { uint32_t title, menu, difficulty, map, intro, song, ending, results, regroup; };
struct RouteStep { uint64_t at; int x, y; uint32_t scene; };   // y 0: the difficulty's row
struct Game {
    const char* id = "";                // EBA, OE2, OE1: the settings' folder
    const char* title = "";
    std::string code;                   // the cartridge's game code
    uint32_t clock = 0;                 // the song's clock, frames
    uint32_t manager = 0, tap = 0, slider = 0, spinner = 0;   // the notes' manager's and notes' vtables
    uint32_t heap = 0;                  // where to look for the manager from
    uint32_t notes = 0x70;              // the manager's Array of notes (+4 its capacity, +8 its count)
    uint32_t shift = 0;                 // a note's fields from +0x14 on are this much later (Ouendan 1: 4)
    uint32_t number = 7;                // the byte of a note's combo number
    uint32_t stage = 8;                 // the session's stage index
    uint32_t judge = 0;                 // the judge (recompiled with --hook)
    uint32_t scene_root = 0;            // the scene's vtable, scene_depth reads from here
    int scene_depth = 0;
    Scenes scenes{};
    uint32_t session = 0, score = 0;    // the session's pointer; its score's offset
    uint32_t combo = 0;                 // the session's combo count (s16), as the game shows it
    uint32_t song_channels = 0;         // the song's stream (muted for HD audio)
    int skip_x = 0, skip_y = 0;         // a comic's SKIP
    std::vector<RouteStep> route;       // Customs' autopilot on the known save
    int route_tap = 3;                  // the frames each of its taps holds
    std::array<int, 4> difficulty_rows{};
    uint64_t card_from = 0, card_to = 0, hand_over = 0;   // the stage and difficulty set; the player's
    std::array<const char*, 4> difficulties{};            // e n h x
    // the map again, Customs played once more without a new run: the card to tap, the
    // frames to its stage being set and to OK, OK's place
    struct { int x = 0, y = 0, card = 0, ok = 0, ok_x = 0, ok_y = 0; } again;
    // the session's fields of the game's own play (eba_play.cpp; the decomp's Note::Judge)
    uint32_t autoplay = 0;              // its autoplay: 1/2/3 always 300/100/50, 4 random
    uint32_t replaying = 0;             // nonzero: the judge plays the replay array back
    uint32_t replay = 0;                // the replay's Array {data, capacity, count}, +0xC its cursor
    uint32_t approach = 0, hit_radius = 0;   // .data the game sets per stage: frames a note shows; the hit test's extra px
    uint32_t gauge_on = 0;              // s32: 0, the life gauge neither fills nor drains (the song setup sets 1)
    std::vector<uint64_t> life_bar;     // the life bar's textures (texpack hashes): hidden with No-Fail
    const char* skins_dir = "";         // the skins' folder (texture packs made for this game), "": none
    bool intro_skip = true;             // a song's intro has the game's own SKIP (EBA, Ouendan 2; not Ouendan 1)
    const char* customs_dir = "customs";
    std::vector<const char*> stages;    // the story's stages by index (the charts' stems: SS05, OS03, s03)
    float menu_button_x = 0.5f;         // the CUSTOMS button across the main menu's lower screen (0 left .. 1 right)
};
const Game& game();
Game game_of(const std::string& rom);               // the profile for a ROM file (before g_cfg is set)
inline uint32_t note_field(uint32_t note, uint32_t eba_offset) {   // a note's field, given at EBA's offset
    return note + eba_offset + (eba_offset >= 0x14 ? game().shift : 0);
}
const std::vector<uint32_t>& all_judges();          // every game's judge (hooks are set where emitted)
int difficulty_index(char d);                       // e n h x: 0 .. 3, the session's

// ---- button mode (eba_buttons.cpp) -------------------------------------------------------
struct Buttons {
    bool on = true;                     // the face buttons and keys hit notes in a song
    int keys[4] = {44, 0, 0, 0};        // SDL scancodes that hit too (44: Space), 0: none
    bool trigger_feel = true;           // a DualSense's triggers push back on spinners
};
Buttons& buttons();
void buttons_options();                 // --port's buttons options
void debug_pokes(uint64_t vblank);      // --port poke=ADDR:VALUE@VBLANK (tests)
bool note_ends_combo(uint32_t note);    // the next note starts a new combo (its number 1), or none follows

// ---- timing (eba_timing.cpp) -------------------------------------------------------------
// The judge (0x020681D8, taps and slider heads) is given the song's clock minus
// the offset: a tap `offset` frames after its note is judged as on it. Each
// tap the game judges is recorded as the frames between it and its note,
// before the offset (the player's own timing).
struct TimingStats {
    int count = 0;
    double mean = 0, stdev = 0;         // frames, + late
    int hist[17] = {};                  // -8 .. +8 frames
    int grades[4] = {};                 // 300, 100, 50, miss, as judged (with the offset)
};
void set_offset(int frames);
int offset();
TimingStats timing_stats();             // the taps since the last reset
void timing_reset();
void timing_options();                  // --port's timing options

// ---- the game's own play (eba_play.cpp): autoplay, replays, assists ---------------------
// over what the game sets (per stage and difficulty): the frames a note shows, in percent
// (0: the game's own); the hit circle's extra pixels (0: none)
// No-Fail: the life gauge stays as it starts (the game's own switch); hide_life: its bar is not drawn
struct Assists { int approach_pct = 0, hit_extra = 0; bool no_fail = false, hide_life = true; };
Assists& assists();
void play_options();                                // --port autoplay=N replay=PATH replay-save=PATH ...
void play_frame(uint64_t vblank);                   // each frame: the flags, the replay, the assists
void set_autoplay(int n);                           // 0 off, 1 every note a 300 (the game's own)
int autoplay();
bool replay_playing();                              // a recorded run is being played back
std::string last_replay();                          // the file the last song's run was saved to

// ---- light behind the 3D (eba_fx.cpp) ---------------------------------------------------
struct Effects { bool on = false; int intensity = 70; bool flashes = true; };   // fire; off by default
Effects& effects();
void fx_options();                                  // --port fx[=off] fx-intensity=N
void fx_frame(uint64_t vblank);
void fx_judged(int grade, int number);             // each tap the judge judges (eba_timing.cpp)
int combo();                                       // the game's combo, as it shows it (slider ticks counted)

// ---- rumble (eba_rumble.cpp): the gamepads' motors in a song, in button mode -------------
struct Rumble { bool on = true; int strength = 100; };   // strength: percent
Rumble& rumble();
struct SongView {                                  // what button mode sees of the song each frame
    bool live = false;                             // a song running
    uint32_t note = 0;                             // the note judged now (else the next)
    int kind = -1, now = 0, start = 0, end = 0;    // its kind (0 tap 1 slider 2 spinner, -1 none); its body's frames
    bool last = false, pen = false;                // it ends its combo; the stylus is down
    double spin = 0;                               // a spinner's speed, turns a frame
    int x = -1;                                    // the stylus's x (0-255; -1: up): a slider's ball, held
};
void rumble_frame(const SongView& v, uint64_t vblank);
void rumble_judged(uint32_t note, int grade);      // each tap the judge judges (eba_timing.cpp)
void rumble_options();                             // --port rumble[=off] rumble-strength=N rumble-log

// ---- skins (eba_skins.cpp): texture packs for the notes' pictures ----------------------
// Off for 1.0 (the Look menu shows no Skin or HD rings, saved choices are kept but not
// loaded) until a full HD skin is made; true brings both back.
constexpr bool kSkins = false;
// <skins_dir>/<name>/ (beside the ROM, else here), a pack of pictures by texture hash
// (tools/make_texture_pack.py makes one from an osu! skin); <skins_dir>/_hd-rings/ the
// approach circles redrawn smooth, _hd-rings-thin/ and _hd-rings-thinner/ thinner. The
// skin first, then the rings.
struct Skin { std::string name; int hd_rings = 0; };   // name "": the game's own; rings 0 off, 1 normal, 2 thin, 3 thinner
Skin& skin();
std::vector<std::string> skins_list();                 // the skins found
bool hd_rings_available(int width);                    // 1 normal, 2 thin, 3 thinner
void skins_apply();                                    // after a change: the packs to the renderer
void skins_options();                                  // --port skin=NAME hd-rings[=off|thin|thinner]

// ---- Customs (eba_customs.cpp) -----------------------------------------------------------
struct Custom {
    std::string tag, dir, title, artist, stage, replaces;
    int stage_index = 0;
    std::vector<std::string> files;     // the cartridge's names of the files it replaces
    std::map<std::string, std::string> extra;   // custom.ini's other keys (the bake's: art, previews, HD audio)
};
struct CustomScore { uint32_t score = 0; int grades[4] = {}; bool cleared = false; };
void customs_init();
const std::vector<Custom>& customs();
const Custom* custom_playing();         // Customs mode: the custom this run plays (nullptr: the campaign)
char custom_difficulty();
const char* customs_difficulty_name(char d);
bool customs_autopilot();               // the route to the stage is being driven (turbo)
bool customs_autopilot_failed();
CustomScore custom_best(const std::string& tag, char difficulty);
void customs_launch(const Custom& c, char difficulty, bool autoplay);   // a Customs run (or the map again)
void customs_back_to_campaign();
std::string customs_import(const std::string& path);   // a custom ROM (.nds) or a custom's folder: what happened
bool customs_frame(NdsInput& in, uint64_t vblank);       // true: the autopilot drives the input
bool customs_take_show_request();       // a custom's song is over: the Customs screen, once
uint32_t scene();                       // the scene's vtable (eba_customs.cpp's table)
bool scene_is_main_menu();
bool scene_is_song();
bool scene_has_skip();                  // a stage's opening or ending comic: SKIP at the lower right

// ---- RetroAchievements (eba_achievements.cpp) ---------------------------------------------
// The game's own set, casual only, paused in Customs, button mode, with an assist, autoplay,
// a replay or a test's options: the game as it was made.
// The sign-in is shared by the three games (%APPDATA%/eba-recomp/RetroAchievements/login.ini).
struct Achievements { bool on = true; std::string user, token; };   // token: RA's, from a sign-in
Achievements& achievements();
void ra_init();                                     // after the settings: signs in with the saved token
void ra_frame(uint64_t vblank);                     // each frame: the network's answers, the achievements
void ra_login(const std::string& user, std::string& password);   // the password is wiped
void ra_logout();
void ra_set_on(bool on);
bool ra_signed_in();
std::string ra_status();                            // signed in, the game's set and its progress
const char* ra_paused_by();                         // why they are paused now (nullptr: they count)
struct RaToast { std::string title, text; };
std::vector<RaToast> ra_toasts();                   // the pop-ups to show now
rc_client_t* ra_client();                           // for the list (rc_client.h)

// ---- the options bar (eba_ui.cpp) --------------------------------------------------------
bool ui_wants_mouse();                  // the mouse is over the bar or a menu: not the stylus
void customs_ui_draw();                 // eba_customs_ui.cpp: its button, screen, loading screen
void customs_ui_open();

// ---- settings (eba_settings.cpp) ---------------------------------------------------------
void settings_load();                   // at the start: the saved display, input and timing
void settings_save();                   // after each change in the options bar

}  // namespace eba
