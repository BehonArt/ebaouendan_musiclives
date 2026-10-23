// Skins: texture packs (ndskit's texpack.cpp) over the notes' pictures. A
// skin is a folder of PNGs named by the texture they replace
// (tools/make_texture_pack.py makes one from an osu! skin: the circles, their
// numbers, the approach circles, the slider ball, the judgements, the spinner;
// pictures only, the game's sounds stay). The skins' folder is the game's
// skins_dir beside the ROM (a play folder's), else here; its _hd-rings folder
// holds the approach circles redrawn smooth, the HD approach circles option.
//
// Options (ndsrun --port): skin=NAME; hd-rings[=off].
#include "eba.h"
#include "display.h"
#include <algorithm>
#include <filesystem>

namespace eba {
namespace {

namespace fs = std::filesystem;

constexpr const char* kHdRings = "_hd-rings";

fs::path root() {
    if (!*game().skins_dir) return {};
    fs::path beside = fs::path(g_cfg.rom).parent_path() / game().skins_dir;
    return fs::exists(beside) ? beside : fs::current_path() / game().skins_dir;
}

}  // namespace

Skin& skin() {
    static Skin s;
    return s;
}

std::vector<std::string> skins_list() {
    std::vector<std::string> v;
    fs::path r = root();
    std::error_code ec;
    if (!r.empty())
        for (const auto& e : fs::directory_iterator(r, ec))
            if (e.is_directory() && e.path().filename().string()[0] != '_') v.push_back(e.path().filename().string());
    std::sort(v.begin(), v.end());
    return v;
}

bool hd_rings_available() {
    fs::path r = root();
    std::error_code ec;
    return !r.empty() && fs::is_directory(r / kHdRings, ec);
}

void skins_apply() {
    if (g_cfg.port.count("tex-pack")) return;       // ndsrun's own packs win for that run
    fs::path r = root();
    std::vector<std::string> packs;
    std::error_code ec;
    if (!r.empty() && !skin().name.empty()) {
        if (fs::is_directory(r / skin().name, ec)) packs.push_back((r / skin().name).string());
        else nds_note("eba skins: no skin %s in %s", skin().name.c_str(), r.string().c_str());
    }
    if (skin().hd_rings && hd_rings_available()) packs.push_back((r / kHdRings).string());
    static bool applied;
    if (packs.empty() && !applied) return;          // the game's own: nothing to change
    display_set_texture_packs(packs);
    applied = true;
}

void skins_options() {
    const auto& o = g_cfg.port;
    if (o.count("skin")) skin().name = o.at("skin");
    if (o.count("hd-rings")) skin().hd_rings = o.at("hd-rings") != "off";
    skins_apply();
}

}  // namespace eba
