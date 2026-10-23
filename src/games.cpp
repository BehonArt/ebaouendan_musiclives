// The three games' facts: what button mode, the timing offset, the options
// bar and Customs read and write of each (eba.h's Game). The code is the same
// for all three, linked into each one's ndsrun; the cartridge's game code
// (its header, +0xC) says which facts apply.
//
// EBA (USA, AOSE): found 2026-10-22, see eba_buttons.cpp and eba_customs.cpp.
// Ouendan 2 (AO2J), found 2026-10-22 from RAM dumps of a song and the menus;
// its engine is EBA's, the notes the same objects with the same fields:
//   0x020A7CA0        the song's clock (read by overlay 5's note code)
//   class 0x02092250  the notes' manager (+0x70 the notes, +0x74 their count);
//                     notes' vtables 0x020C04B8 tap, 0x020C02E8 slider,
//                     0x020C05CC spinner (overlay 5)
//   0x020BB2B4        the judge (overlay 5; method 10 of the tap and the slider
//                     calls it, as EBA's): --hook 020BB2B4
//   **0x020D70F4      the scene's vtable
//   scenes            title 020ADA5C, main menu 020ADC98, difficulty 020ADD50,
//                     map 020ADEC4, opening comic 020B0820, song 020B0888,
//                     ending comic 020B07BC, results 020B0668, fail 020B0978
//   the song's stream channels 6 and 7 (PCM16 looped), as EBA's
//   *0x020A7A88       the session: +0x10C the score, +0 the difficulty (0 気軽に .. 3 華麗に), +8 the
//                     stage's index in the table at 0x02092020 (OS01, OS02, OS04,
//                     OS06, OS08, OS10, OSB02, OS11, OS03, OS07, OS14, OS12, OS13,
//                     OS05, OS15A, OS15B, OSB03, OSB01, OS09)
#include "eba.h"
#include <cstring>
#include <fstream>

namespace eba {
namespace {

Game make_eba() {
    Game g;
    g.id = "EBA";
    g.title = "Elite Beat Agents";
    g.clock = 0x020CD390;
    g.manager = 0x020B4110; g.tap = 0x020B4648; g.slider = 0x020B478C; g.spinner = 0x020B48D4;
    g.heap = 0x020CD000;
    g.judge = 0x020681D8;
    g.scene_root = 0x020CD1FC; g.scene_depth = 3;
    g.scenes = {0x020B3D78, 0x020B7288, 0x020B736C, 0x020B7528, 0x020B64F0, 0x020B65A0, 0x020B6448, 0x020B62C0, 0x020B66C8};
    g.session = 0x020CD248; g.score = 0xF8;
    g.song_channels = 0xC0;
    g.skip_x = 238; g.skip_y = 183;
    // the title, Single Player, the difficulty and OK, The Anthem's card, OK
    g.route = {{950, 128, 150, g.scenes.title}, {1300, 128, 66, g.scenes.menu},
               {2400, 128, 0, g.scenes.difficulty}, {2450, 236, 178, g.scenes.difficulty},
               {3650, 126, 103, g.scenes.map}, {4300, 236, 178, g.scenes.map}};
    g.difficulty_rows = {153, 119, 78, 45};
    g.card_from = 3662; g.card_to = 3662; g.hand_over = 4310;
    g.again = {126, 103, 640, 650, 236, 178};   // the map centres on the stage last played
    g.autoplay = 0x234; g.replaying = 0x138; g.replay = 0xA0;
    g.approach = 0x020B4704; g.hit_radius = 0x020B22E0;
    g.difficulties = {"Breezin'", "Cruisin'", "Sweatin'", "Hard Rock"};
    g.customs_dir = "customs";
    return g;
}

Game make_oe2() {
    Game g;
    g.id = "OE2";
    g.title = "Moero! Nekketsu Rhythm Damashii Osu! Tatakae! Ouendan 2";
    g.clock = 0x020A7CA0;
    g.manager = 0x02092250; g.tap = 0x020C04B8; g.slider = 0x020C02E8; g.spinner = 0x020C05CC;
    g.heap = 0x020D9000;
    g.judge = 0x020BB2B4;
    g.scene_root = 0x020D70F4; g.scene_depth = 2;
    g.scenes = {0x020ADA5C, 0x020ADC98, 0x020ADD50, 0x020ADEC4, 0x020B0820, 0x020B0888, 0x020B07BC, 0x020B0668,
                0x020B0978};
    g.session = 0x020A7A88; g.score = 0x10C;
    g.song_channels = 0xC0;
    g.skip_x = 238; g.skip_y = 183;
    // the title, ひとりで応援 (alone), OK on the difficulty, the map's book (the next
    // stage's card), OK
    g.route = {{1850, 128, 150, g.scenes.title}, {2650, 65, 55, g.scenes.menu},
               {3350, 238, 183, g.scenes.difficulty}, {4350, 238, 178, g.scenes.map},
               {4750, 238, 183, g.scenes.map}};
    g.route_tap = 9;                    // the card's OK misses a 3-frame tap
    g.card_from = 4364; g.card_to = 4364; g.hand_over = 4760;  // the card sets its stage by 4362
    g.again = {160, 88, 440, 450, 238, 183};    // the stage last played: its character, in the middle
    g.autoplay = 0x27C; g.replaying = 0x15C; g.replay = 0xA0;   // from its judge (EBA's +0x234, +0x138)
    g.difficulties = {"Kigaru (easy)", "Kakan (normal)", "Gekiretsu (hard)", "Karei (insane)"};
    g.customs_dir = "customs_oe2";
    g.menu_button_x = 0.0f;             // the strip's right holds 応援スタイルを選ぶべし
    return g;
}

Game make_oe1() {
    Game g;
    g.id = "OE1";
    g.title = "Osu! Tatakae! Ouendan";
    g.difficulties = {"Easy", "Normal", "Hard", "Insane"};
    g.customs_dir = "customs_oe1";
    return g;
}

std::string game_code() {
    std::ifstream f(g_cfg.rom, std::ios::binary);
    char c[4] = {};
    f.seekg(0xC);
    f.read(c, 4);
    return std::string(c, 4);
}

}  // namespace

const Game& game() {
    static const Game g = [] {
        std::string code = game_code();
        Game r = code == "AO2J" ? make_oe2() : code == "AOSJ" ? make_oe1() : make_eba();
        r.code = code;
        return r;
    }();
    return g;
}

const std::vector<uint32_t>& all_judges() {
    static const std::vector<uint32_t> j = {make_eba().judge, make_oe2().judge};
    return j;
}

int difficulty_index(char d) { return d == 'e' ? 0 : d == 'n' ? 1 : d == 'x' ? 3 : 2; }

}  // namespace eba
