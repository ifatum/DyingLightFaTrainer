#pragma once
#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace config {

struct PageEntry { std::string id; bool visible = true; };

struct Config {
    float accent[3] = {0.953f, 0.604f, 0.118f};
    float dim = 0.6f;
    float scale = 1.0f;
    int menu_key = VK_INSERT;
    bool remember_cheats = true;
    std::vector<PageEntry> pages;
    std::vector<std::string> cheats_on;
};

inline Config cfg;

inline std::string default_path() {
    char p[MAX_PATH];
    GetModuleFileNameA(nullptr, p, MAX_PATH);
    std::string s = p;
    return s.substr(0, s.find_last_of("\\/") + 1) + "fatrainer.ini";
}

inline void normalize(const std::vector<std::string>& known) {
    std::vector<PageEntry> out;
    for (auto& p : cfg.pages)
        if (std::find(known.begin(), known.end(), p.id) != known.end() &&
            std::none_of(out.begin(), out.end(), [&](auto& o) { return o.id == p.id; }))
            out.push_back(p);
    for (auto& id : known)
        if (std::none_of(out.begin(), out.end(), [&](auto& o) { return o.id == id; })) out.push_back({id, true});
    for (auto& p : out)
        if (p.id == "settings") p.visible = true;
    cfg.pages = out;
    cfg.scale = std::clamp(cfg.scale, 0.75f, 1.5f);
    cfg.dim = std::clamp(cfg.dim, 0.0f, 0.95f);
}

inline void load(const std::string& path) {
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return;
    cfg.pages.clear();
    cfg.cheats_on.clear();
    char line[512];
    while (fgets(line, sizeof line, f)) {
        std::string s = line;
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
        size_t eq = s.find('=');
        if (eq == std::string::npos) continue;
        std::string k = s.substr(0, eq), v = s.substr(eq + 1);
        if (k == "accent") sscanf(v.c_str(), "%f,%f,%f", &cfg.accent[0], &cfg.accent[1], &cfg.accent[2]);
        else if (k == "dim") cfg.dim = (float)atof(v.c_str());
        else if (k == "scale") cfg.scale = (float)atof(v.c_str());
        else if (k == "menu_key") cfg.menu_key = atoi(v.c_str());
        else if (k == "remember_cheats") cfg.remember_cheats = v == "1";
        else if (k == "cheat") cfg.cheats_on.push_back(v);
        else if (k == "page") {
            size_t c = v.find(',');
            cfg.pages.push_back({v.substr(0, c), c == std::string::npos || v.substr(c + 1) != "0"});
        }
    }
    fclose(f);
}

inline bool save(const std::string& path) {
    FILE* f = fopen(path.c_str(), "w");
    if (!f) return false;
    fprintf(f, "accent=%.3f,%.3f,%.3f\n", cfg.accent[0], cfg.accent[1], cfg.accent[2]);
    fprintf(f, "dim=%.2f\nscale=%.2f\nmenu_key=%d\nremember_cheats=%d\n", cfg.dim, cfg.scale, cfg.menu_key, (int)cfg.remember_cheats);
    for (auto& p : cfg.pages) fprintf(f, "page=%s,%d\n", p.id.c_str(), (int)p.visible);
    if (cfg.remember_cheats)
        for (auto& c : cfg.cheats_on) fprintf(f, "cheat=%s\n", c.c_str());
    fclose(f);
    return true;
}

}
