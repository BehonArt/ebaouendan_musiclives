// RetroAchievements (rcheevos's rc_client, third_party/rcheevos): the game's own
// achievement set, read from the game's own memory. RA's Nintendo DS addresses
// are the recomp's: 0 to 0x3FFFFF main RAM (0x02000000 on the DS), 0x1000000 to
// 0x1003FFF the ARM9's data TCM. The game is known by RA's hash of the ROM (its
// header, ARM9 and ARM7 code, icon), the stock dump's.
//
// Casual (softcore) only: RA counts hardcore unlocks from the emulators it has
// approved. And only for the game as it was made: achievements are paused
// (nothing is checked, and they start over from their own start conditions when
// the pause ends) in Customs, in button mode, with an assist (approach, hit
// circle, No-Fail), the game's autoplay, a replay played back, or a test's
// options. The timing offset and the sound's latency, the picture's options
// and skins, the effects and rumble change nothing the game judges.
//
// The sign-in keeps RA's token in the settings, never the password. The
// network is WinHTTP's (Windows'), on a thread of its own; rc_client hears
// back on the game's, each frame.
//
// Options (ndsrun --port): ra-log (rc_client's log); ra-spectator (achievements
// checked, nothing sent: tests; a headless run then signs in with the saved
// token).
#include "eba.h"
#include "rc_client.h"
#include "rc_consoles.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#endif

extern uint8_t g_main[];
extern uint8_t g_dtcm[];
extern uint32_t g_dtcm_size;

namespace eba {
namespace {

constexpr const char* kVersion = "0.1.0";

rc_client_t* g_client;
bool g_spectator, g_paused = true;
const char* g_why = "not signed in";
std::string g_status = "Not signed in";

// ---- the network: a thread of its own, the answers handed back on the game's ------------
struct Request { std::string url, post, type; rc_client_server_callback_t done; void* data; };
struct Answer { std::string body; int status; rc_client_server_callback_t done; void* data; };
std::mutex g_mu;
std::condition_variable g_cv;
std::deque<Request> g_requests;
std::deque<Answer> g_answers;
std::string g_agent;

#ifdef _WIN32
std::wstring wide(const std::string& s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(n ? n - 1 : 0, L'\0');
    if (n > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), n);
    return w;
}

Answer fetch(HINTERNET session, const Request& r) {
    Answer a{"", RC_API_SERVER_RESPONSE_RETRYABLE_CLIENT_ERROR, r.done, r.data};
    std::wstring url = wide(r.url);
    URL_COMPONENTS u{};
    u.dwStructSize = sizeof u;
    wchar_t host[256] = {}, path[4096] = {};
    u.lpszHostName = host;
    u.dwHostNameLength = 256;
    u.lpszUrlPath = path;
    u.dwUrlPathLength = 4096;
    u.dwExtraInfoLength = (DWORD)-1;
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &u)) return a;
    std::wstring full = std::wstring(path, u.dwUrlPathLength) + (u.lpszExtraInfo ? std::wstring(u.lpszExtraInfo, u.dwExtraInfoLength) : L"");
    HINTERNET c = WinHttpConnect(session, host, u.nPort, 0);
    if (!c) return a;
    bool post = !r.post.empty();
    HINTERNET q = WinHttpOpenRequest(c, post ? L"POST" : L"GET", full.c_str(), nullptr, WINHTTP_NO_REFERER,
                                     WINHTTP_DEFAULT_ACCEPT_TYPES, u.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    if (q) {
        std::wstring hdr = post ? L"Content-Type: " + wide(r.type.empty() ? "application/x-www-form-urlencoded" : r.type) : L"";
        if (WinHttpSendRequest(q, post ? hdr.c_str() : WINHTTP_NO_ADDITIONAL_HEADERS, post ? (DWORD)-1L : 0,
                               post ? (LPVOID)r.post.data() : WINHTTP_NO_REQUEST_DATA, (DWORD)r.post.size(),
                               (DWORD)r.post.size(), 0) &&
            WinHttpReceiveResponse(q, nullptr)) {
            DWORD code = 0, len = sizeof code;
            WinHttpQueryHeaders(q, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &code,
                                &len, WINHTTP_NO_HEADER_INDEX);
            DWORD avail = 0;
            while (WinHttpQueryDataAvailable(q, &avail) && avail) {
                size_t at = a.body.size();
                a.body.resize(at + avail);
                DWORD got = 0;
                if (!WinHttpReadData(q, a.body.data() + at, avail, &got)) break;
                a.body.resize(at + got);
            }
            a.status = (int)code;
        }
        WinHttpCloseHandle(q);
    }
    WinHttpCloseHandle(c);
    return a;
}

void network() {
    HINTERNET session = WinHttpOpen(wide(g_agent).c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                                    WINHTTP_NO_PROXY_BYPASS, 0);
    for (;;) {
        Request r;
        {
            std::unique_lock<std::mutex> l(g_mu);
            g_cv.wait(l, [] { return !g_requests.empty(); });
            r = std::move(g_requests.front());
            g_requests.pop_front();
        }
        Answer a = session ? fetch(session, r) : Answer{"", RC_API_SERVER_RESPONSE_RETRYABLE_CLIENT_ERROR, r.done, r.data};
        std::lock_guard<std::mutex> l(g_mu);
        g_answers.push_back(std::move(a));
    }
}
#endif

void RC_CCONV server_call(const rc_api_request_t* req, rc_client_server_callback_t done, void* data, rc_client_t*) {
#ifdef _WIN32
    static bool started;
    if (!started) { started = true; std::thread(network).detach(); }
    std::lock_guard<std::mutex> l(g_mu);
    g_requests.push_back({req->url, req->post_data ? req->post_data : "", req->content_type ? req->content_type : "", done, data});
    g_cv.notify_one();
#else
    rc_api_server_response_t r{nullptr, 0, RC_API_SERVER_RESPONSE_CLIENT_ERROR};
    done(&r, data);
#endif
}

void answers() {
    std::deque<Answer> got;
    {
        std::lock_guard<std::mutex> l(g_mu);
        got.swap(g_answers);
    }
    for (auto& a : got) {
        rc_api_server_response_t r{a.body.c_str(), a.body.size(), a.status};
        a.done(&r, a.data);
    }
}

// ---- the game's memory at RA's addresses ----------------------------------------------
uint32_t RC_CCONV read_memory(uint32_t address, uint8_t* buffer, uint32_t n, rc_client_t*) {
    uint32_t done = 0;
    while (done < n) {
        uint32_t a = address + done;
        if (a < 0x400000u) buffer[done] = g_main[a];
        else if (a >= 0x1000000u && a < 0x1004000u && g_dtcm_size) buffer[done] = g_dtcm[a - 0x1000000u];
        else break;
        ++done;
    }
    return done;
}

// ---- what the player sees -------------------------------------------------------------
struct Toast { std::string title, text; uint64_t until; };
std::vector<Toast> g_toasts;
uint64_t g_frame;

void toast(const std::string& title, const std::string& text = "", int seconds = 5) {
    g_toasts.push_back({title, text, SDL_GetTicks() + (uint64_t)seconds * 1000});
    nds_note("eba ra: %s%s%s", title.c_str(), text.empty() ? "" : ": ", text.c_str());
}

void RC_CCONV log_message(const char* message, const rc_client_t*) { nds_note("eba ra: %s", message); }

void RC_CCONV event(const rc_client_event_t* e, rc_client_t*) {
    switch (e->type) {
    case RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED:
        toast("Achievement unlocked" + std::string(g_spectator ? " (spectating: not sent)" : ""),
              std::string(e->achievement->title) + " (" + std::to_string(e->achievement->points) + ")  " + e->achievement->description, 7);
        break;
    case RC_CLIENT_EVENT_GAME_COMPLETED:
        toast("Set mastered (casual)", "Every achievement of this game earned.", 8);
        break;
    case RC_CLIENT_EVENT_SERVER_ERROR:
        toast("RetroAchievements error", e->server_error->error_message ? e->server_error->error_message : "");
        break;
    case RC_CLIENT_EVENT_DISCONNECTED: toast("RetroAchievements offline", "Unlocks are kept and sent when it is back."); break;
    case RC_CLIENT_EVENT_RECONNECTED: toast("RetroAchievements back online", "The kept unlocks were sent."); break;
    default: break;
    }
}

// the sign-in, shared by the three games: user=, token= in SDL's folder for the user's settings
std::string login_path() {
    char* dir = SDL_GetPrefPath("eba-recomp", "RetroAchievements");
    if (!dir) return "";
    std::string p = std::string(dir) + "login.ini";
    SDL_free(dir);
    return p;
}

void load_login() {
    std::FILE* f = std::fopen(login_path().c_str(), "r");
    if (!f) return;
    char line[512];
    while (std::fgets(line, sizeof line, f)) {
        std::string l(line);
        while (!l.empty() && (l.back() == '\n' || l.back() == '\r')) l.pop_back();
        if (l.rfind("user=", 0) == 0) achievements().user = l.substr(5);
        if (l.rfind("token=", 0) == 0) achievements().token = l.substr(6);
    }
    std::fclose(f);
}

void save_login() {
    if (g_spectator && !g_cfg.window) return;      // a test never writes it
    std::FILE* f = std::fopen(login_path().c_str(), "w");
    if (!f) return;
    std::fprintf(f, "# RetroAchievements sign-in for eba-recomp (EBA, Ouendan 1 and 2): RA's token, not the password\n");
    std::fprintf(f, "user=%s\ntoken=%s\n", achievements().user.c_str(), achievements().token.c_str());
    std::fclose(f);
}

void RC_CCONV game_loaded(int result, const char* error, rc_client_t* c, void*) {
    const rc_client_game_t* g = rc_client_get_game_info(c);
    if (result != RC_OK || !g) {
        g_status = std::string("No achievement set: ") + (error ? error : "the game is not known");
        toast("RetroAchievements", g_status);
        return;
    }
    rc_client_user_game_summary_t s;
    rc_client_get_user_game_summary(c, &s);
    g_status = std::string(g->title) + ": " + std::to_string(s.num_unlocked_achievements) + " of " +
               std::to_string(s.num_core_achievements) + " earned (casual)";
    toast(g->title, std::to_string(s.num_unlocked_achievements) + " of " + std::to_string(s.num_core_achievements) +
                        " achievements earned. Casual mode.");
}

void RC_CCONV logged_in(int result, const char* error, rc_client_t* c, void*) {
    const rc_client_user_t* u = rc_client_get_user_info(c);
    if (result != RC_OK || !u) {
        g_status = std::string("Sign-in failed: ") + (error ? error : "?");
        toast("RetroAchievements", g_status);
        if (result == RC_INVALID_CREDENTIALS || result == RC_EXPIRED_TOKEN) {
            achievements().token.clear();
            save_login();
        }
        return;
    }
    achievements().user = u->username;
    achievements().token = u->token;
    save_login();
    g_status = std::string("Signed in as ") + u->display_name;
    rc_client_begin_identify_and_load_game(c, RC_CONSOLE_NINTENDO_DS, g_cfg.rom.c_str(), nullptr, 0, game_loaded, nullptr);
}

// why the achievements are paused now (nullptr: they count)
const char* blocked() {
    const auto& o = g_cfg.port;
    if (custom_playing()) return "Customs";
    if (buttons().on) return "button mode";
    const Assists& a = assists();
    if (game().approach && a.approach_pct > 0 && a.approach_pct != 100) return "the approach assist";
    if (game().hit_radius && a.hit_extra > 0) return "the hit circle assist";
    if (game().gauge_on && a.no_fail) return "No-Fail";
    if (autoplay()) return "autoplay";
    if (replay_playing()) return "a replay";
    for (const char* k : {"buttons-auto", "poke", "autoplay", "replay", "stick-auto", "fx-debug"})
        if (o.count(k)) return "a test option";
    return nullptr;
}

}  // namespace

Achievements& achievements() {
    static Achievements a;
    return a;
}

const char* ra_paused_by() { return g_paused ? g_why : nullptr; }
std::string ra_status() { return g_status; }
bool ra_signed_in() { return g_client && rc_client_get_user_info(g_client); }

void ra_init() {
    g_spectator = g_cfg.port.count("ra-spectator") > 0;
    if (!g_cfg.window && !g_spectator) return;      // headless: only a test's spectator
    load_login();                                   // (a headless test signs in as the window does)
    if (!g_cfg.window) achievements().on = true;
    g_client = rc_client_create(read_memory, server_call);
    char clause[128];
    rc_client_get_user_agent_clause(g_client, clause, sizeof clause);
    std::string id = game().id;
    std::transform(id.begin(), id.end(), id.begin(), ::tolower);
    g_agent = std::string("eba-recomp/") + kVersion + " (Windows NT 10.0) " + id + "/" + kVersion + " " + clause;
    if (g_cfg.port.count("ra-log")) rc_client_enable_logging(g_client, RC_CLIENT_LOG_LEVEL_INFO, log_message);
    rc_client_set_event_handler(g_client, event);
    rc_client_set_hardcore_enabled(g_client, 0);    // casual: RA's hardcore is for the emulators it approves
    rc_client_set_spectator_mode_enabled(g_client, g_spectator);
    Achievements& a = achievements();
    if (a.on && !a.user.empty() && !a.token.empty()) {
        g_status = "Signing in...";
        rc_client_begin_login_with_token(g_client, a.user.c_str(), a.token.c_str(), logged_in, nullptr);
    }
}

void ra_login(const std::string& user, std::string& password) {
    if (!g_client) return;
    g_status = "Signing in...";
    rc_client_begin_login_with_password(g_client, user.c_str(), password.c_str(), logged_in, nullptr);
    std::fill(password.begin(), password.end(), '\0');
    password.clear();
}

void ra_logout() {
    if (!g_client) return;
    rc_client_unload_game(g_client);
    rc_client_logout(g_client);
    achievements().token.clear();
    save_login();
    g_status = "Not signed in";
}

void ra_set_on(bool on) {
    Achievements& a = achievements();
    a.on = on;
    settings_save();
    if (!g_client) return;
    if (!on) {
        rc_client_unload_game(g_client);
        g_status = "Achievements off";
    } else if (!a.token.empty()) {
        g_status = "Signing in...";
        rc_client_begin_login_with_token(g_client, a.user.c_str(), a.token.c_str(), logged_in, nullptr);
    }
}

void ra_frame(uint64_t) {
    if (!g_client) return;
    answers();
    const char* why = rc_client_get_game_info(g_client) ? blocked() : "no achievement set loaded";
    bool paused = why != nullptr;
    if (paused != g_paused || (paused && why != g_why)) {
        if (paused && rc_client_get_game_info(g_client)) toast("Achievements paused", std::string("Off while playing with ") + why + ".");
        if (!paused) {
            rc_client_reset(g_client);              // they start over from their own start conditions
            if (g_frame) toast("Achievements on", "The game as made: they count again (casual).");
        }
        g_paused = paused;
        g_why = why;
    }
    ++g_frame;
    if (paused) rc_client_idle(g_client);
    else rc_client_do_frame(g_client);
}

rc_client_t* ra_client() { return g_client; }

std::vector<RaToast> ra_toasts() {
    uint64_t now = SDL_GetTicks();
    g_toasts.erase(std::remove_if(g_toasts.begin(), g_toasts.end(), [&](const Toast& t) { return t.until < now; }),
                   g_toasts.end());
    std::vector<RaToast> v;
    for (auto& t : g_toasts) v.push_back({t.title, t.text});
    return v;
}

}  // namespace eba
