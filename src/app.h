#pragma once
#include <windows.h>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "cheats.h"
#include "config.h"
#include "game.h"

inline const char* TITLE = "FaTrainer | Dying Light";
inline const char* VERSION = "1.7";

inline void logf(const char* fmt, ...) {
    static std::string path;
    if (path.empty()) {
        char p[MAX_PATH];
        GetModuleFileNameA(nullptr, p, MAX_PATH);
        path = p;
        path = path.substr(0, path.find_last_of("\\/") + 1) + "fatrainer.log";
    }
    if (FILE* f = fopen(path.c_str(), "a")) {
        va_list a;
        va_start(a, fmt);
        vfprintf(f, fmt, a);
        va_end(a);
        fputc('\n', f);
        fclose(f);
    }
}

inline const bool logf_wired = (cheats::logf_hook = logf, true);

inline std::atomic<bool> g_open{false};
inline DWORD g_last_scan = 0;
inline HWND g_hwnd;
inline const UINT WM_FATRAINER = WM_APP + 0x4F;
inline std::mutex g_qmx;
inline std::vector<std::function<void()>> g_queue;
inline std::string g_toast;
inline DWORD g_toast_at = 0;

inline void toast(const std::string& s) {
    logf("toast: %s", s.c_str());
    std::lock_guard<std::mutex> l(g_qmx);
    g_toast = s;
    g_toast_at = GetTickCount();
}

inline void on_game_thread(std::function<void()> f) {
    {
        std::lock_guard<std::mutex> l(g_qmx);
        g_queue.push_back(std::move(f));
    }
    PostMessageW(g_hwnd, WM_FATRAINER, 0, 0);
}

inline void drain_queue() {
    std::vector<std::function<void()>> q;
    {
        std::lock_guard<std::mutex> l(g_qmx);
        q.swap(g_queue);
    }
    for (auto& f : q) f();
}

inline void request_refresh() {
    g_last_scan = GetTickCount();
    std::thread([] {
        game::refresh();
        bool find_locks = cheats::is_on("lockpick") && cheats::lock_records.empty();
        auto locks = find_locks ? game::find_float_records(cheats::lock_patterns(), game::g.base) : std::vector<uintptr_t>();
        std::lock_guard<std::mutex> l(game::mx);
        if (find_locks) cheats::lock_records = locks;
        cheats::player = cheats::find_player();
        logf("refresh: %s", game::g.status.c_str());
        logf("inventories:%s", game::inventory_report().c_str());
        on_game_thread([] {
            std::lock_guard<std::mutex> l(game::mx);
            cheats::read_sections();
        });
        logf("stats: %s", game::stat_report().c_str());
        logf("cheats: %s", cheats::describe().c_str());
    }).detach();
}

inline void save_config() {
    auto& c = config::cfg;
    c.cheats_on.clear();
    for (auto& ch : cheats::CHEATS)
        if (ch.on) c.cheats_on.push_back(ch.key);
    c.tweaks.clear();
    for (auto& t : cheats::TWEAKS)
        if (t.factor != 1.0f) c.tweaks.push_back({t.key, t.factor});
    config::save(config::default_path());
}
