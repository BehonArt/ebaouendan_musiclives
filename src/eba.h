// EBA's own code in ndsrun: what its parts share (button mode, timing, the
// options bar, the settings file).
#pragma once
#include "nds.h"
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace eba {

// The first frame (after ndsrun's options are read): the saved settings, then --port's over them.
void init();

inline uint32_t rd(uint32_t a) { uint32_t v; std::memcpy(&v, g_main + (a & 0x3FFFFFu), 4); return v; }
inline int16_t rh(uint32_t a) { int16_t v; std::memcpy(&v, g_main + (a & 0x3FFFFFu), 2); return v; }
inline bool in_main(uint32_t a) { return a >= 0x02000000u && a < 0x02400000u; }

// ---- button mode (eba_buttons.cpp) -------------------------------------------------------
struct Buttons {
    bool on = true;                     // the face buttons and keys hit notes in a song
    int keys[4] = {44, 0, 0, 0};        // SDL scancodes that hit too (44: Space), 0: none
};
Buttons& buttons();
void buttons_options();                 // --port's buttons options
void debug_pokes(uint64_t vblank);      // --port poke=ADDR:VALUE@VBLANK (tests)

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
void customs_launch(const Custom& c, char difficulty);   // a Customs run instead of this one
void customs_back_to_campaign();
bool customs_frame(NdsInput& in, uint64_t vblank);       // true: the autopilot drives the input
bool customs_take_show_request();       // a custom's song is over: the Customs screen, once
uint32_t scene();                       // the scene's vtable (eba_customs.cpp's table)
bool scene_is_main_menu();
bool scene_is_song();
bool scene_has_skip();                  // a stage's opening or ending comic: SKIP at the lower right

// ---- the options bar (eba_ui.cpp) --------------------------------------------------------
bool ui_wants_mouse();                  // the mouse is over the bar or a menu: not the stylus
void customs_ui_draw();                 // eba_customs_ui.cpp: its button, screen, loading screen
void customs_ui_open();

// ---- settings (eba_settings.cpp) ---------------------------------------------------------
void settings_load();                   // at the start: the saved display, input and timing
void settings_save();                   // after each change in the options bar

}  // namespace eba
