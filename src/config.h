#pragma once
#include <windows.h>
#include <algorithm>
#include <cctype>
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
    std::vector<std::pair<std::string, float>> tweaks;
};

inline Config cfg;

inline std::string default_path() {
    char p[MAX_PATH];
    GetModuleFileNameA(nullptr, p, MAX_PATH);
    std::string s = p;
    return s.substr(0, s.find_last_of("\\/") + 1) + "fatrainer.ini";
}

struct Profile {
    std::vector<std::string> cheats;
    std::vector<std::pair<std::string, float>> tweaks;
};

inline std::string profile_dir() {
    std::string ini = default_path();
    return ini.substr(0, ini.find_last_of("\\/") + 1) + "fatrainer_configs";
}

inline std::string profile_name(const std::string& raw) {
    std::string out;
    for (char ch : raw)
        if (isalnum((unsigned char)ch) || ch == ' ' || ch == '-' || ch == '_') out += ch;
    while (!out.empty() && out.back() == ' ') out.pop_back();
    while (!out.empty() && out.front() == ' ') out.erase(out.begin());
    return out.substr(0, 40);
}

inline std::string profile_path(const std::string& name) { return profile_dir() + "\\" + name + ".ini"; }

inline std::vector<std::string> list_profiles() {
    std::vector<std::string> out;
    WIN32_FIND_DATAA d;
    HANDLE h = FindFirstFileA((profile_dir() + "\\*.ini").c_str(), &d);
    if (h == INVALID_HANDLE_VALUE) return out;
    do {
        std::string n = d.cFileName;
        if (!(d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) out.push_back(n.substr(0, n.size() - 4));
    } while (FindNextFileA(h, &d));
    FindClose(h);
    std::sort(out.begin(), out.end());
    return out;
}

inline bool save_profile(const std::string& name, const Profile& p) {
    if (name.empty()) return false;
    CreateDirectoryA(profile_dir().c_str(), nullptr);
    FILE* f = fopen(profile_path(name).c_str(), "w");
    if (!f) return false;
    for (auto& c : p.cheats) fprintf(f, "cheat=%s\n", c.c_str());
    for (auto& [k, v] : p.tweaks) fprintf(f, "tweak=%s,%.2f\n", k.c_str(), v);
    fclose(f);
    return true;
}

inline bool read_cheat_line(const std::string& k, const std::string& v, Profile& p) {
    if (k == "cheat") p.cheats.push_back(v);
    else if (k == "tweak") {
        size_t c = v.find(',');
        if (c != std::string::npos) p.tweaks.push_back({v.substr(0, c), (float)atof(v.substr(c + 1).c_str())});
    } else return false;
    return true;
}

inline bool load_profile(const std::string& name, Profile& p) {
    FILE* f = fopen(profile_path(name).c_str(), "r");
    if (!f) return false;
    char line[512];
    while (fgets(line, sizeof line, f)) {
        std::string s = line;
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
        size_t eq = s.find('=');
        if (eq != std::string::npos) read_cheat_line(s.substr(0, eq), s.substr(eq + 1), p);
    }
    fclose(f);
    return true;
}

inline bool delete_profile(const std::string& name) { return DeleteFileA(profile_path(name).c_str()); }

inline void normalize(const std::vector<std::string>& known) {
    std::vector<PageEntry> out;
    for (auto& p : cfg.pages)
        if (std::find(known.begin(), known.end(), p.id) != known.end() &&
            std::none_of(out.begin(), out.end(), [&](auto& o) { return o.id == p.id; }))
            out.push_back(p);
    for (size_t k = 0; k < known.size(); k++) {
        auto has = [&](const std::string& id) { return std::find_if(out.begin(), out.end(), [&](auto& o) { return o.id == id; }); };
        if (has(known[k]) != out.end()) continue;
        auto at = out.begin();
        for (size_t j = k; j-- > 0;)
            if (has(known[j]) != out.end()) { at = has(known[j]) + 1; break; }
        out.insert(known[k] == "settings" ? out.end() : at, {known[k], true});
    }
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
    cfg.tweaks.clear();
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
        else if (Profile p; read_cheat_line(k, v, p)) {
            cfg.cheats_on.insert(cfg.cheats_on.end(), p.cheats.begin(), p.cheats.end());
            cfg.tweaks.insert(cfg.tweaks.end(), p.tweaks.begin(), p.tweaks.end());
        }
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
    for (auto& [k, v] : cfg.tweaks) fprintf(f, "tweak=%s,%.2f\n", k.c_str(), v);
    fclose(f);
    return true;
}

}
