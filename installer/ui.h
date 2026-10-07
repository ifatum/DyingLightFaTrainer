#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "imgui.h"
#include "platform.h"

#ifdef _WIN32
#define EMBED_SECTION ".section .rdata,\"dr\"\n"
#else
#define EMBED_SECTION ".section .rodata\n"
#endif
#define EMBED(name, file)                                     \
    extern "C" const unsigned char name[], name##_end[];      \
    asm(EMBED_SECTION ".global " #name "\n.balign 16\n" #name \
                      ":\n.incbin \"" file "\"\n.global " #name "_end\n" #name "_end:\n.byte 0\n.text\n");

EMBED(font_display, "installer/fonts/display-latin.ttf")
EMBED(font_display_ext, "installer/fonts/display-latin-ext.ttf")
EMBED(font_heading, "installer/fonts/heading-latin.ttf")
EMBED(font_heading_ext, "installer/fonts/heading-latin-ext.ttf")
EMBED(font_body, "installer/fonts/body-latin.ttf")
EMBED(font_body_ext, "installer/fonts/body-latin-ext.ttf")
EMBED(font_strong, "installer/fonts/strong-latin.ttf")
EMBED(font_strong_ext, "installer/fonts/strong-latin-ext.ttf")
EMBED(font_mono, "installer/fonts/mono-latin.ttf")
EMBED(font_mono_ext, "installer/fonts/mono-latin-ext.ttf")
EMBED(changelog_md, "CHANGELOG.md")
#ifdef FATRAINER_OFFLINE
EMBED(trainer_dll, "dist/nexus/xinput1_3.dll")
const bool OFFLINE = true;
#else
const bool OFFLINE = false;
#endif

namespace app {

const ImU32 GROUND = IM_COL32(17, 17, 19, 255), RAISED = IM_COL32(24, 24, 27, 255), LINE = IM_COL32(42, 42, 46, 255),
            LINE_SOFT = IM_COL32(30, 30, 34, 255), TEXT = IM_COL32(236, 230, 218, 255), SOFT = IM_COL32(185, 178, 165, 255),
            MUTED = IM_COL32(132, 125, 114, 255), ACCENT = IM_COL32(232, 151, 58, 255), ACCENT_INK = IM_COL32(27, 17, 5, 255),
            GOOD = IM_COL32(126, 201, 140, 255), BAD = IM_COL32(236, 112, 96, 255);
const float PI = 3.14159265f;
const char* LAUNCH_OPTION = "WINEDLLOVERRIDES=\"xinput1_3=n,b\" %command%";

inline float scale = 1;
inline float S(float v) { return v * scale; }
inline float clamp01(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }
inline float ease_out(float t) { t = clamp01(t); return 1 - powf(1 - t, 3); }
inline float ease_expo(float t) { t = clamp01(t); return t >= 1 ? 1 : 1 - powf(2, -10 * t); }
inline float ease_in_out(float t) { t = clamp01(t); return t < 0.5f ? 4 * t * t * t : 1 - powf(-2 * t + 2, 3) / 2; }
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
inline ImVec2 lerp(ImVec2 a, ImVec2 b, float t) { return {lerp(a.x, b.x, t), lerp(a.y, b.y, t)}; }
inline ImU32 C(ImU32 c, float alpha = 1) { return ImGui::GetColorU32(c, alpha); }
inline ImU32 mix(ImU32 a, ImU32 b, float t) {
    auto ch = [&](int shift) { return (ImU32)lerp((float)((a >> shift) & 0xff), (float)((b >> shift) & 0xff), clamp01(t)) << shift; };
    return ch(0) | ch(8) | ch(16) | ch(24);
}

struct Fonts { ImFont *intro, *brand, *hero, *heading, *body, *strong, *small, *label, *mono; };
inline Fonts F;

inline ImFont* load_font(const unsigned char* latin, const unsigned char* latin_end, const unsigned char* ext, const unsigned char* ext_end, float size) {
    static const ImWchar LATIN[] = {0x0020, 0x00FF, 0x2010, 0x2027, 0x2030, 0x205E, 0x20AC, 0x20AC, 0x2122, 0x2122, 0};
    static const ImWchar LATIN_EXT[] = {0x0100, 0x02FF, 0x1E00, 0x1EFF, 0x20A0, 0x20C0, 0};
    ImFontConfig c;
    c.FontDataOwnedByAtlas = false;
    c.OversampleH = 2;
    ImFont* f = ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)latin, (int)(latin_end - latin), size, &c, LATIN);
    c.MergeMode = true;
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)ext, (int)(ext_end - ext), size, &c, LATIN_EXT);
    return f;
}

inline float text_w(ImFont* f, const char* s, const char* e = nullptr, float size = 0) {
    return f->CalcTextSizeA(size ? size : f->FontSize, FLT_MAX, 0, s, e).x;
}

inline std::unordered_map<ImGuiID, float> anims;
inline float& anim(const std::string& key, float initial = 0) {
    auto it = anims.try_emplace(ImGui::GetID(key.c_str()), initial).first;
    return it->second;
}
inline float follow(float& v, float target, float rate) {
    v += (target - v) * (1 - expf(-rate * ImGui::GetIO().DeltaTime));
    return v;
}

inline void soft_glow(ImDrawList* dl, ImVec2 c, float radius, ImU32 color, float strength) {
    const int RINGS = 48;
    for (int i = 0; i < RINGS; i++) {
        float f = 1 - i / (float)RINGS;
        dl->AddCircleFilled(c, radius * f, C(color, strength / RINGS * 2.2f * (0.4f + 0.6f * f)), 72);
    }
}

inline void round_corners(ImDrawList* dl, ImVec2 a, ImVec2 b, float r, ImU32 color) {
    struct Corner { ImVec2 corner, center; float from; };
    const Corner corners[4] = {{a, {a.x + r, a.y + r}, PI}, {{b.x, a.y}, {b.x - r, a.y + r}, 1.5f * PI}, {b, {b.x - r, b.y - r}, 0}, {{a.x, b.y}, {a.x + r, b.y - r}, 0.5f * PI}};
    const int STEPS = 16;
    for (const Corner& c : corners)
        for (int i = 0; i < STEPS; i++) {
            float a0 = c.from + PI / 2 * i / STEPS, a1 = c.from + PI / 2 * (i + 1) / STEPS;
            dl->AddTriangleFilled(c.corner, {c.center.x + cosf(a0) * r, c.center.y + sinf(a0) * r}, {c.center.x + cosf(a1) * r, c.center.y + sinf(a1) * r}, color);
        }
}

struct Window { float x, y, phase; };
struct Building { float x, w, h, roof_w, roof_h, antenna; std::vector<Window> windows; };
struct City { std::vector<Building> back, front; float crane_x = 0, crane_h = 0; };

inline uint32_t rng_state;
inline float rnd() {
    rng_state = rng_state * 1664525u + 1013904223u;
    return (rng_state >> 8) / 16777216.0f;
}

inline std::vector<Building> make_layer(uint32_t seed, float min_h, float max_h, float min_w, float max_w, float window_chance) {
    rng_state = seed;
    std::vector<Building> out;
    for (float x = -60; x < 1660;) {
        Building b{x, min_w + rnd() * (max_w - min_w), min_h + rnd() * (max_h - min_h), 0, 0, 0, {}};
        if (rnd() < 0.45f) b.roof_w = b.w * (0.2f + rnd() * 0.3f), b.roof_h = 8 + rnd() * 14;
        if (rnd() < 0.3f) b.antenna = 20 + rnd() * 40;
        for (float wy = 16; wy < b.h - 12; wy += 21)
            for (float wx = 9; wx < b.w - 14; wx += 15)
                if (rnd() < window_chance) b.windows.push_back({wx, wy, rnd() * 100});
        out.push_back(b);
        x += b.w + 2 + rnd() * 6;
    }
    return out;
}

inline City make_city() {
    City c;
    c.back = make_layer(7, 120, 270, 60, 125, 0.16f);
    c.front = make_layer(19, 40, 165, 70, 150, 0.13f);
    rng_state = 41;
    Building tower{1150, 58, 340, 0, 0, 0, {}};
    for (float wy = 20; wy < 330; wy += 21)
        for (float wx = 10; wx < 46; wx += 15)
            if (rnd() < 0.2f) tower.windows.push_back({wx, wy, rnd() * 100});
    c.back.push_back(tower);
    c.crane_x = 1460, c.crane_h = 330;
    return c;
}

inline const City& city() {
    static City c = make_city();
    return c;
}

inline void draw_city(ImDrawList* dl, ImVec2 a, ImVec2 b, ImVec2 light, ImVec2 parallax, float t, float rise_t) {
    const float VISIBLE_UNITS = 1180;
    float k = std::max((b.x - a.x) / VISIBLE_UNITS, (b.y - a.y) / 380);
    float ox = (a.x + b.x) / 2 - 800 * k;
    float radius = S(190);
    auto light_on = [&](float x0, float y0, float x1, float y1) {
        float cx = std::clamp(light.x, x0, x1), cy = std::clamp(light.y, y0, y1);
        float f = 1 - sqrtf((light.x - cx) * (light.x - cx) + (light.y - cy) * (light.y - cy)) / radius;
        return f > 0 ? f * f : 0.0f;
    };
    auto layer = [&](const std::vector<Building>& buildings, ImU32 wall, ImU32 wall_lit, float shift, float delay) {
        for (const Building& bd : buildings) {
            float x0 = ox + bd.x * k + shift, x1 = x0 + bd.w * k;
            float rise = ease_expo((rise_t - delay - (bd.x + 60) / 1700 * 0.55f) / 0.7f);
            if (x1 < a.x - S(40) || x0 > b.x + S(40) || rise <= 0) continue;
            float y1 = b.y, y0 = y1 - bd.h * k * rise;
            float lit = light_on(x0, y0, x1, y1);
            ImU32 color = mix(wall, wall_lit, lit);
            dl->AddRectFilled({x0, y0}, {x1, y1}, C(color));
            if (bd.roof_w > 0) {
                float rx = x0 + (bd.w - bd.roof_w) * 0.35f * k;
                dl->AddRectFilled({rx, y0 - bd.roof_h * k}, {rx + bd.roof_w * k, y0 + 1}, C(color));
            }
            if (bd.antenna > 0) dl->AddLine({x0 + bd.w * 0.7f * k, y0}, {x0 + bd.w * 0.7f * k, y0 - bd.antenna * k}, C(color), std::max(1.0f, 2 * k));
            float wake = clamp01((rise_t - delay - 0.5f) / 1.2f);
            for (const Window& w : bd.windows) {
                float show = clamp01(wake * 3 - w.phase / 50);
                if (show <= 0) continue;
                float cycle = fmodf(t * 0.37f + w.phase, 17.0f);
                if ((int)w.phase % 6 == 0 && cycle < 0.35f) continue;
                float wx = x0 + w.x * k, wy = y1 - (w.y + 10) * k * rise;
                if (wy < y0) continue;
                float wl = light_on(wx, wy, wx + 6 * k, wy + 9 * k);
                ImU32 window = mix(IM_COL32(168, 104, 42, 150), IM_COL32(255, 216, 160, 255), wl * 1.6f);
                dl->AddRectFilled({wx, wy}, {wx + std::max(2.0f, 6 * k), wy + std::max(3.0f, 9 * k)}, C(window, show));
            }
        }
    };
    soft_glow(dl, light, radius * 1.5f, ACCENT, 0.28f);
    layer(city().back, IM_COL32(31, 31, 38, 255), IM_COL32(76, 63, 50, 255), parallax.x * 0.4f, 0);
    float crane_rise = ease_expo((rise_t - 0.55f) / 0.8f);
    if (crane_rise > 0) {
        float cx = ox + city().crane_x * k + parallax.x * 0.4f, top = b.y - city().crane_h * k * crane_rise;
        ImU32 steel = C(IM_COL32(52, 50, 54, 255));
        dl->AddLine({cx, b.y}, {cx, top}, steel, std::max(1.5f, 3 * k));
        dl->AddLine({cx - 70 * k, top + 8 * k}, {cx + 150 * k, top + 8 * k}, steel, std::max(1.0f, 2.5f * k));
        float sway = sinf(t * 0.6f) * 6 * k;
        dl->AddLine({cx + 120 * k, top + 8 * k}, {cx + 120 * k + sway, top + 90 * k}, steel, 1);
        float blink = fmodf(t, 2.4f) < 0.5f ? 1.0f : 0.25f;
        dl->AddCircleFilled({cx + 150 * k, top + 8 * k}, std::max(1.5f, 2.5f * k), C(IM_COL32(230, 70, 50, 255), blink * crane_rise));
    }
    layer(city().front, IM_COL32(13, 13, 16, 255), IM_COL32(56, 46, 36, 255), parallax.x, 0.18f);
    float fog = (b.y - a.y) * 0.32f;
    dl->AddRectFilledMultiColor({a.x, b.y - fog}, b, C(GROUND, 0), C(GROUND, 0), C(GROUND, 0.92f), C(GROUND, 0.92f));
}

inline void draw_letters(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, const char* word, float t, ImU32 color) {
    float x = pos.x;
    dl->PushClipRect({pos.x - size, pos.y - size * 0.1f}, {pos.x + text_w(font, word, nullptr, size) + size, pos.y + size * 1.12f}, true);
    for (int i = 0; word[i]; i++) {
        float a = ease_expo((t - i * 0.055f) / 0.7f);
        dl->AddText(font, size, {x, pos.y + (1 - a) * size}, C(color, a), word + i, word + i + 1);
        x += text_w(font, word + i, word + i + 1, size);
    }
    dl->PopClipRect();
}

inline void label_caps(ImDrawList* dl, ImVec2 pos, const char* text, ImU32 color) {
    float x = pos.x;
    for (const char* c = text; *c; c++) {
        dl->AddText(F.label, F.label->FontSize, {x, pos.y}, C(color), c, c + 1);
        x += text_w(F.label, c, c + 1) + S(1.4f);
    }
}

enum class Kind { primary, ghost, quiet };

inline bool button(const char* id, const std::string& text, ImVec2 size, Kind kind, bool enabled = true, float progress = -1) {
    ImGui::PushID(id);
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool pressed = ImGui::InvisibleButton("button", size) && enabled;
    bool hovered = enabled && ImGui::IsItemHovered(), held = enabled && ImGui::IsItemActive();
    float h = follow(anim("hover"), hovered ? 1.0f : 0.0f, 16), d = follow(anim("down"), held ? 1.0f : 0.0f, 30);
    float e = follow(anim("enabled", enabled ? 1.0f : 0.0f), enabled ? 1.0f : 0.0f, 10);
    ImGui::PopID();
    if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float lift = (h - d) * S(1.5f);
    ImVec2 a{p.x, p.y - lift}, b{p.x + size.x, p.y + size.y - lift};
    float r = S(10);
    ImU32 ink = TEXT;
    if (kind == Kind::primary) {
        if (progress >= 0) {
            dl->AddRectFilled(a, b, C(RAISED), r);
            float fill = a.x + (b.x - a.x) * clamp01(progress);
            if (fill > a.x + 1) {
                dl->PushClipRect(a, {fill, b.y}, true);
                dl->AddRectFilled(a, b, C(ACCENT), r);
                float sheen = a.x + fmodf((float)ImGui::GetTime() * S(320), (b.x - a.x) + S(160)) - S(80);
                dl->AddRectFilledMultiColor({sheen - S(60), a.y}, {sheen, b.y}, C(IM_COL32(255, 255, 255, 0)), C(IM_COL32(255, 255, 255, 60)),
                                            C(IM_COL32(255, 255, 255, 60)), C(IM_COL32(255, 255, 255, 0)));
                dl->AddRectFilledMultiColor({sheen, a.y}, {sheen + S(60), b.y}, C(IM_COL32(255, 255, 255, 60)), C(IM_COL32(255, 255, 255, 0)),
                                            C(IM_COL32(255, 255, 255, 0)), C(IM_COL32(255, 255, 255, 60)));
                dl->PopClipRect();
            }
            dl->AddRect(a, b, C(LINE), r);
            ink = progress > 0.55f ? ACCENT_INK : TEXT;
        } else {
            for (int i = 1; i <= 4; i++) dl->AddRect({a.x - i * S(2), a.y - i * S(2)}, {b.x + i * S(2), b.y + i * S(2)}, C(ACCENT, h * 0.09f / i), r + i * S(2), 0, S(2));
            ImU32 fill = mix(mix(IM_COL32(70, 62, 52, 255), ACCENT, e), IM_COL32(246, 181, 104, 255), h * 0.6f);
            dl->AddRectFilled(a, b, C(fill), r);
            ink = mix(MUTED, ACCENT_INK, e);
        }
    } else if (kind == Kind::ghost) {
        dl->AddRectFilled(a, b, C(RAISED, h), r);
        dl->AddRect(a, b, C(mix(LINE, ACCENT, h * 0.8f)), r);
        ink = enabled ? TEXT : MUTED;
    } else {
        ink = mix(MUTED, TEXT, h);
        float tw = text_w(F.strong, text.c_str());
        float ux = (a.x + b.x - tw) / 2;
        dl->AddLine({ux, b.y - S(6)}, {ux + tw * h, b.y - S(6)}, C(ACCENT), S(1.5f));
    }
    float tw = text_w(F.strong, text.c_str());
    dl->AddText(F.strong, F.strong->FontSize, {(a.x + b.x - tw) / 2, (a.y + b.y - F.strong->FontSize) / 2}, C(ink), text.c_str());
    return pressed;
}

enum class Fetch { loading, ready, failed };
enum class Job { idle, working, done, failed };

struct Model {
    std::mutex mx;
    Fetch fetch = Fetch::loading;
    std::string fetch_error;
    ReleaseInfo latest;
    std::vector<logic::ChangelogEntry> changelog;
    fs::path game;
    std::string installed;
    Job job = Job::idle;
    std::string step;
    float progress = 0;
    double finished_at = -10;
    std::string message;
    bool message_bad = false;
    bool picking = false, picked_ready = false, picker_missing = false;
    std::string picked;
};
inline Model M;

inline void refresh_installed() {
    std::error_code ec;
    fs::path dll = M.game / platform::DLL_NAME;
    M.installed = !M.game.empty() && fs::exists(dll, ec) ? logic::trainer_version_in(platform::read_file(dll)) : "";
}

inline void fetch_release() {
#ifdef FATRAINER_OFFLINE
    std::string dll((const char*)trainer_dll, (size_t)(trainer_dll_end - trainer_dll));
    M.latest = {logic::trainer_version_in(dll), logic::sha256_hex(dll), dll.size()};
    M.fetch = Fetch::ready;
#else
    {
        std::lock_guard<std::mutex> l(M.mx);
        M.fetch = Fetch::loading;
    }
    std::thread([] {
        std::string text, error, changes, ignored;
        bool ok = platform::net::get_text(std::string(RELEASE_DOWNLOADS) + "version.txt", text, error);
        ReleaseInfo info = parse_release_info(text);
        if (ok && (info.version.empty() || info.sha256.size() != 64)) ok = false, error = "the release has no valid version.txt";
        bool have_changes = ok && platform::net::get_text(std::string(RELEASE_DOWNLOADS) + "CHANGELOG.md", changes, ignored);
        std::lock_guard<std::mutex> l(M.mx);
        M.fetch = ok ? Fetch::ready : Fetch::failed;
        M.fetch_error = error;
        if (ok) M.latest = info;
        if (have_changes) {
            auto parsed = logic::parse_changelog(changes);
            if (!parsed.empty()) M.changelog = parsed;
        }
    }).detach();
#endif
}

inline void finish(Job job, const std::string& message) {
    std::lock_guard<std::mutex> l(M.mx);
    M.job = job;
    M.message = message;
    M.message_bad = job == Job::failed;
    M.finished_at = ImGui::GetTime();
    if (job == Job::done) M.progress = 1;
}

inline std::string replace_hint() {
    return platform::LINUX ? "Check that you can write to the game folder."
                           : "Close Dying Light if it is running. If it is not, right-click the installer and choose Run as administrator.";
}

inline void start_install() {
    fs::path game;
    ReleaseInfo latest;
    {
        std::lock_guard<std::mutex> l(M.mx);
        if (M.job == Job::working) return;
        game = M.game, latest = M.latest;
        M.job = Job::working, M.step = OFFLINE ? "Installing" : "Downloading", M.progress = 0, M.message.clear();
    }
    std::thread([game, latest] {
#ifdef FATRAINER_OFFLINE
        std::string data((const char*)trainer_dll, (size_t)(trainer_dll_end - trainer_dll));
#else
        const unsigned long long LIMIT = 64ull << 20;
        std::string data, error;
        bool ok = platform::net::get(std::string(RELEASE_DOWNLOADS) + platform::DLL_NAME, [&](const char* d, size_t n, unsigned long long total) {
            data.append(d, n);
            unsigned long long expected = latest.size ? latest.size : total;
            std::lock_guard<std::mutex> l(M.mx);
            M.progress = expected ? 0.88f * std::min(1.0f, (float)data.size() / expected) : 0.5f;
            return data.size() <= LIMIT;
        }, error);
        if (!ok) return finish(Job::failed, "Download failed: " + error + ". Nothing was changed.");
#endif
        {
            std::lock_guard<std::mutex> l(M.mx);
            M.step = "Checking", M.progress = 0.92f;
        }
        if ((latest.size && data.size() != latest.size) || logic::sha256_hex(data) != latest.sha256 || logic::trainer_version_in(data).empty())
            return finish(Job::failed, "The download did not match the release checksum, so nothing was installed. Try again in a minute.");
        {
            std::lock_guard<std::mutex> l(M.mx);
            M.step = "Installing", M.progress = 0.96f;
        }
        std::error_code ec;
        fs::path target = game / platform::DLL_NAME, temp = game / platform::DOWNLOAD_NAME, backup = game / platform::BACKUP_NAME;
        if (fs::exists(target, ec) && logic::trainer_version_in(platform::read_file(target)).empty() && !fs::exists(backup, ec)) {
            fs::copy_file(target, backup, ec);
            if (ec) return finish(Job::failed, "Could not keep a copy of the xinput1_3.dll that was already there. " + replace_hint());
        }
        {
            std::ofstream out(temp, std::ios::binary | std::ios::trunc);
            out.write(data.data(), (std::streamsize)data.size());
            if (!out) {
                out.close();
                fs::remove(temp, ec);
                return finish(Job::failed, "Could not write to the game folder. " + replace_hint());
            }
        }
        fs::rename(temp, target, ec);
        if (ec) {
            fs::remove(temp, ec);
            return finish(Job::failed, "Could not replace xinput1_3.dll. " + replace_hint());
        }
        {
            std::lock_guard<std::mutex> l(M.mx);
            if (M.game == game) M.installed = latest.version;
        }
        finish(Job::done, platform::LINUX ? "Installed FaTrainer " + latest.version + ". Add the launch option below once, then start the game and press Insert."
                                          : "Installed FaTrainer " + latest.version + ". Start Dying Light and press Insert or F8 in game.");
    }).detach();
}

inline void uninstall() {
    std::lock_guard<std::mutex> l(M.mx);
    std::error_code ec;
    fs::path target = M.game / platform::DLL_NAME, backup = M.game / platform::BACKUP_NAME;
    fs::remove(target, ec);
    if (ec) {
        M.message = "Could not remove xinput1_3.dll. " + replace_hint(), M.message_bad = true;
        return;
    }
    bool restored = fs::exists(backup, ec) && (fs::rename(backup, target, ec), !ec);
    refresh_installed();
    M.job = Job::idle;
    M.message = restored ? "FaTrainer was removed and the xinput1_3.dll that was there before is back." : "FaTrainer was removed. Your fatrainer.ini settings stay in the game folder.";
    M.message_bad = false;
}

inline void use_folder(const std::string& utf8) {
    fs::path chosen = fs::u8path(utf8);
    if (!platform::is_game_dir(chosen) && platform::is_game_dir(chosen.parent_path())) chosen = chosen.parent_path();
    if (!platform::is_game_dir(chosen) && platform::is_game_dir(chosen / "Dying Light")) chosen = chosen / "Dying Light";
    if (!platform::is_game_dir(chosen)) {
        M.message = "That folder has no DyingLightGame.exe. Choose the Dying Light folder itself.", M.message_bad = true;
        return;
    }
    M.game = chosen;
    M.job = Job::idle;
    M.message.clear();
    refresh_installed();
}

inline void choose_folder() {
    {
        std::lock_guard<std::mutex> l(M.mx);
        if (M.picking) return;
        M.picking = true;
    }
    platform::pick_folder([](std::string picked) {
        std::lock_guard<std::mutex> l(M.mx);
        M.picking = false;
        M.picked_ready = true;
        M.picked = picked;
    });
}

inline void init(float dpi_scale) {
    scale = dpi_scale;
    ImGui::GetIO().IniFilename = nullptr;
    F.intro = load_font(font_display, font_display_end, font_display_ext, font_display_ext_end, S(92));
    F.brand = load_font(font_display, font_display_end, font_display_ext, font_display_ext_end, S(26));
    F.hero = load_font(font_display, font_display_end, font_display_ext, font_display_ext_end, S(46));
    F.heading = load_font(font_heading, font_heading_end, font_heading_ext, font_heading_ext_end, S(21));
    F.body = load_font(font_body, font_body_end, font_body_ext, font_body_ext_end, S(16));
    F.strong = load_font(font_strong, font_strong_end, font_strong_ext, font_strong_ext_end, S(16));
    F.small = load_font(font_body, font_body_end, font_body_ext, font_body_ext_end, S(13.5f));
    F.label = load_font(font_strong, font_strong_end, font_strong_ext, font_strong_ext_end, S(11.5f));
    F.mono = load_font(font_mono, font_mono_end, font_mono_ext, font_mono_ext_end, S(12.5f));
    ImGui::GetIO().FontDefault = F.body;
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowPadding = {0, 0}, s.WindowBorderSize = 0, s.ChildBorderSize = 0, s.FrameRounding = S(8), s.FramePadding = {S(12), S(9)};
    s.ItemSpacing = {S(10), S(10)}, s.ScrollbarSize = S(6), s.ScrollbarRounding = S(3);
    s.Colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(TEXT);
    s.Colors[ImGuiCol_WindowBg] = s.Colors[ImGuiCol_ChildBg] = {0, 0, 0, 0};
    s.Colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(RAISED);
    s.Colors[ImGuiCol_FrameBgHovered] = s.Colors[ImGuiCol_FrameBgActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(30, 30, 34, 255));
    s.Colors[ImGuiCol_TextSelectedBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(232, 151, 58, 90));
    s.Colors[ImGuiCol_ScrollbarBg] = {0, 0, 0, 0};
    s.Colors[ImGuiCol_ScrollbarGrab] = ImGui::ColorConvertU32ToFloat4(LINE);
    s.Colors[ImGuiCol_ScrollbarGrabHovered] = s.Colors[ImGuiCol_ScrollbarGrabActive] = ImGui::ColorConvertU32ToFloat4(MUTED);
    s.Colors[ImGuiCol_NavHighlight] = ImGui::ColorConvertU32ToFloat4(ACCENT);
    M.changelog = logic::parse_changelog(std::string((const char*)changelog_md, (size_t)(changelog_md_end - changelog_md)));
    M.game = platform::find_game();
    refresh_installed();
    fetch_release();
}

inline int tab = 0;
inline double tab_changed_at = 0;
const float CONTENT_AT = 1.45f;

inline float reveal(int index) {
    double base = std::max((double)CONTENT_AT, tab_changed_at);
    return ease_out((float)(ImGui::GetTime() - base - index * 0.07) / 0.6f);
}

struct Reveal {
    float offset;
    explicit Reveal(int index) {
        float a = reveal(index);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, a);
        offset = (1 - a) * S(18);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset);
    }
    ~Reveal() {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - offset);
        ImGui::PopStyleVar();
    }
};

inline void tabs(ImVec2 pos) {
    const char* names[] = {"Games", "Changelog"};
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float x = pos.x;
    static float line_x = -1, line_w = 0;
    for (int i = 0; i < 2; i++) {
        float w = text_w(F.strong, names[i]);
        ImGui::SetCursorScreenPos({x - S(10), pos.y - S(10)});
        ImGui::PushID(i);
        if (ImGui::InvisibleButton("tab", {w + S(20), F.strong->FontSize + S(20)}) && tab != i) tab = i, tab_changed_at = ImGui::GetTime();
        bool hovered = ImGui::IsItemHovered();
        ImGui::PopID();
        if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        float h = follow(anim(std::string("tab") + names[i]), tab == i ? 1.0f : hovered ? 0.6f : 0.0f, 14);
        dl->AddText(F.strong, F.strong->FontSize, {x, pos.y}, C(mix(MUTED, TEXT, h)), names[i]);
        if (tab == i) {
            if (line_x < 0) line_x = x, line_w = w;
            follow(line_x, x, 16), follow(line_w, w, 16);
        }
        x += w + S(30);
    }
    dl->AddRectFilled({line_x, pos.y + F.strong->FontSize + S(8)}, {line_x + line_w, pos.y + F.strong->FontSize + S(10)}, C(ACCENT), S(1));
}

inline void status_pill(ImVec2 right_top) {
    std::string text;
    ImU32 dot = MUTED;
    Fetch fetch;
    {
        std::lock_guard<std::mutex> l(M.mx);
        fetch = M.fetch;
        if (fetch == Fetch::ready) text = OFFLINE ? FATRAINER_EDITION " " + M.latest.version : FATRAINER_EDITION ", latest " + M.latest.version, dot = GOOD;
        else if (fetch == Fetch::failed) text = "GitHub not reachable", dot = BAD;
        else text = "Checking GitHub";
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float retry_w = fetch == Fetch::failed ? text_w(F.strong, "Retry") + S(24) : 0;
    float w = text_w(F.small, text.c_str()) + S(40) + retry_w, h = S(32);
    ImVec2 a{right_top.x - w, right_top.y}, b{right_top.x, right_top.y + h};
    dl->AddRectFilled(a, b, C(RAISED), h / 2);
    dl->AddRect(a, b, C(LINE_SOFT), h / 2);
    float pulse = fetch == Fetch::loading ? 0.45f + 0.55f * (0.5f + 0.5f * sinf((float)ImGui::GetTime() * 5)) : 1;
    dl->AddCircleFilled({a.x + S(17), a.y + h / 2}, S(3.5f), C(dot, pulse));
    if (fetch == Fetch::ready) dl->AddCircle({a.x + S(17), a.y + h / 2}, S(3.5f) + S(6) * fmodf((float)ImGui::GetTime(), 2.2f) / 2.2f, C(dot, 0.5f * (1 - fmodf((float)ImGui::GetTime(), 2.2f) / 2.2f)));
    dl->AddText(F.small, F.small->FontSize, {a.x + S(30), a.y + (h - F.small->FontSize) / 2}, C(SOFT), text.c_str());
    if (fetch == Fetch::failed) {
        ImGui::SetCursorScreenPos({b.x - retry_w - S(6), a.y});
        if (button("retry", "Retry", {retry_w, h}, Kind::quiet)) fetch_release();
    }
}

inline void game_list(ImVec2 a, float width, const std::string& state) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float r0 = reveal(0), r1 = reveal(1), r2 = reveal(2);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, r0);
    label_caps(dl, {a.x, a.y + (1 - r0) * S(12)}, "GAMES", MUTED);
    ImGui::PopStyleVar();
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, r1);
    ImVec2 c0{a.x, a.y + S(30) + (1 - r1) * S(16)}, c1{a.x + width, c0.y + S(92)};
    ImGui::SetCursorScreenPos(c0);
    ImGui::InvisibleButton("game_dying_light", {width, S(92)});
    float h = follow(anim("game_card"), ImGui::IsItemHovered() ? 1.0f : 0.0f, 12);
    dl->AddRectFilled(c0, c1, C(mix(RAISED, IM_COL32(30, 29, 31, 255), h)), S(12));
    dl->AddRect(c0, c1, C(mix(LINE_SOFT, LINE, h)), S(12));
    dl->AddRectFilled({c0.x, c0.y + S(18)}, {c0.x + S(3), c1.y - S(18)}, C(ACCENT), S(2));
    dl->PushClipRect({c0.x + S(12), c0.y}, {c1.x - S(12), c1.y}, true);
    float sky_k = width / 260;
    for (const Building& bd : city().front) {
        float x0 = c0.x + (bd.x - 300) * sky_k * 0.42f, x1 = x0 + bd.w * sky_k * 0.42f;
        if (x1 < c0.x || x0 > c1.x) continue;
        dl->AddRectFilled({x0, c1.y - bd.h * 0.16f * sky_k}, {x1, c1.y}, C(IM_COL32(34, 30, 28, 255), 0.55f + 0.45f * h));
    }
    dl->PopClipRect();
    dl->AddText(F.heading, F.heading->FontSize, {c0.x + S(20), c0.y + S(18)}, C(TEXT), "Dying Light");
    dl->AddText(F.small, F.small->FontSize, {c0.x + S(20), c0.y + S(50)}, C(state == "update" ? ACCENT : state == "installed" ? GOOD : MUTED),
                state == "update" ? "Update available" : state == "installed" ? "Installed" : "Not installed");
    ImGui::PopStyleVar();
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, r2);
    ImVec2 d0{a.x, c1.y + S(14) + (1 - r2) * S(16)}, d1{a.x + width, d0.y + S(64)};
    for (float x = d0.x + S(12); x < d1.x - S(12); x += S(10)) {
        dl->AddLine({x, d0.y}, {std::min(x + S(5), d1.x - S(12)), d0.y}, C(LINE));
        dl->AddLine({x, d1.y}, {std::min(x + S(5), d1.x - S(12)), d1.y}, C(LINE));
    }
    for (float y = d0.y + S(12); y < d1.y - S(12); y += S(10)) {
        dl->AddLine({d0.x, y}, {d0.x, std::min(y + S(5), d1.y - S(12))}, C(LINE));
        dl->AddLine({d1.x, y}, {d1.x, std::min(y + S(5), d1.y - S(12))}, C(LINE));
    }
    dl->AddText(F.small, F.small->FontSize, {d0.x + S(20), d0.y + (S(64) - F.small->FontSize) / 2}, C(MUTED), "More games are on the way");
    ImGui::PopStyleVar();
}

inline void check_mark(ImDrawList* dl, ImVec2 c, float size, float t, ImU32 color) {
    ImVec2 p0{c.x - size * 0.5f, c.y}, p1{c.x - size * 0.12f, c.y + size * 0.38f}, p2{c.x + size * 0.55f, c.y - size * 0.42f};
    float first = clamp01(t / 0.4f), second = clamp01((t - 0.4f) / 0.6f);
    if (first > 0) dl->AddLine(p0, lerp(p0, p1, first), color, S(2.5f));
    if (second > 0) dl->AddLine(p1, lerp(p1, p2, second), color, S(2.5f));
}

inline void detail(float width) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    Model view;
    {
        std::lock_guard<std::mutex> l(M.mx);
        if (M.picked_ready) {
            M.picked_ready = false;
            if (M.picked == platform::FOLDER_PICKER_MISSING) M.picker_missing = true;
            else if (!M.picked.empty()) use_folder(M.picked);
        }
        view.fetch = M.fetch, view.fetch_error = M.fetch_error, view.latest = M.latest, view.game = M.game, view.installed = M.installed;
        view.job = M.job, view.step = M.step, view.progress = M.progress, view.finished_at = M.finished_at, view.message = M.message;
        view.message_bad = M.message_bad, view.picking = M.picking, view.picker_missing = M.picker_missing;
    }
    bool found = platform::is_game_dir(view.game);
    bool ready = view.fetch == Fetch::ready;
    bool outdated = !view.installed.empty() && ready && (view.installed == logic::LEGACY_VERSION || newer_version(view.latest.version, view.installed));
    bool newer_installed = !view.installed.empty() && view.installed != logic::LEGACY_VERSION && ready && newer_version(view.installed, view.latest.version);
    bool current = !view.installed.empty() && ready && !outdated && !newer_installed;
    float t = (float)ImGui::GetTime();

    {
        Reveal r(0);
        ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + width, a.y + S(212)};
        ImGui::InvisibleButton("hero", {width, S(212)});
        static ImVec2 light{-1, -1};
        ImVec2 mouse = ImGui::GetIO().MousePos;
        bool inside = mouse.x >= a.x && mouse.x <= b.x && mouse.y >= a.y && mouse.y <= b.y;
        ImVec2 target = inside ? mouse : ImVec2{a.x + width * (0.62f + 0.22f * sinf(t * 0.33f)), a.y + S(212) * (0.55f + 0.18f * sinf(t * 0.47f))};
        if (light.x < 0) light = target;
        follow(light.x, target.x, inside ? 9.0f : 2.0f), follow(light.y, target.y, inside ? 9.0f : 2.0f);
        ImVec2 parallax{((light.x - a.x) / width - 0.5f) * -S(26), 0};
        dl->PushClipRect(a, b, true);
        dl->AddRectFilledMultiColor(a, b, C(IM_COL32(20, 20, 24, 255)), C(IM_COL32(20, 20, 24, 255)), C(IM_COL32(28, 24, 22, 255)), C(IM_COL32(28, 24, 22, 255)));
        draw_city(dl, a, b, light, parallax, t, t - CONTENT_AT + 0.1f);
        dl->AddRectFilledMultiColor({a.x, a.y}, {a.x + width * 0.6f, b.y}, C(GROUND, 0.55f), C(GROUND, 0), C(GROUND, 0), C(GROUND, 0.55f));
        dl->PopClipRect();
        round_corners(dl, a, b, S(14), C(GROUND));
        dl->AddRect(a, b, C(LINE_SOFT), S(14));
        draw_letters(dl, F.hero, F.hero->FontSize, {a.x + S(26), b.y - S(26) - F.hero->FontSize * 1.05f}, "Dying Light", t - CONTENT_AT - 0.25f, TEXT);
        label_caps(dl, {a.x + S(28), a.y + S(24)}, "STEAM  /  THE FOLLOWING  /  HARRAN PRISON", SOFT);
    }
    ImGui::Dummy({0, S(6)});

    {
        Reveal r(1);
        ImVec2 a = ImGui::GetCursorScreenPos();
        float cell = (width - S(24)) / 3;
        struct Stat { const char* label; std::string value; ImU32 color; };
        Stat stats[3] = {
            {"INSTALLED", view.installed.empty() ? "Not yet" : view.installed == logic::LEGACY_VERSION ? "Older than 2.0" : view.installed,
             outdated ? ACCENT : current ? GOOD : SOFT},
            {OFFLINE ? "IN THIS INSTALLER" : "LATEST", ready ? view.latest.version : view.fetch == Fetch::loading ? "Checking" : "Unknown", TEXT},
            {"RUNS ON", platform::LINUX ? "Linux, Proton" : "Windows", TEXT}};
        for (int i = 0; i < 3; i++) {
            ImVec2 c0{a.x + i * (cell + S(12)), a.y}, c1{c0.x + cell, c0.y + S(78)};
            dl->AddRectFilled(c0, c1, C(RAISED), S(12));
            label_caps(dl, {c0.x + S(18), c0.y + S(16)}, stats[i].label, MUTED);
            dl->AddText(F.heading, F.heading->FontSize, {c0.x + S(18), c0.y + S(38)}, C(stats[i].color), stats[i].value.c_str());
        }
        ImGui::Dummy({width, S(78)});
    }
    ImGui::Dummy({0, S(4)});

    {
        Reveal r(2);
        ImVec2 a = ImGui::GetCursorScreenPos();
        label_caps(dl, a, "GAME FOLDER", MUTED);
        ImGui::Dummy({0, S(14)});
        ImVec2 f0 = ImGui::GetCursorScreenPos(), f1{f0.x + width - S(140), f0.y + S(46)};
        dl->AddRectFilled(f0, f1, C(RAISED), S(10));
        dl->AddRect(f0, f1, C(found ? LINE_SOFT : mix(LINE, BAD, 0.6f)), S(10));
        dl->AddCircleFilled({f0.x + S(18), (f0.y + f1.y) / 2}, S(4), C(found ? GOOD : BAD));
        std::string shown = found ? view.game.u8string() : "Dying Light was not found in your Steam libraries";
        dl->PushClipRect(f0, {f1.x - S(14), f1.y}, true);
        dl->AddText(found ? F.mono : F.small, (found ? F.mono : F.small)->FontSize, {f0.x + S(34), (f0.y + f1.y - (found ? F.mono : F.small)->FontSize) / 2}, C(found ? SOFT : MUTED),
                    shown.c_str());
        dl->PopClipRect();
        ImGui::SetCursorScreenPos({f1.x + S(12), f0.y});
        if (button("choose", view.picking ? "Choosing" : found ? "Change" : "Choose", {S(128), S(46)}, Kind::ghost, !view.picking && view.job != Job::working))
            choose_folder();
        ImGui::SetCursorScreenPos({f0.x, f1.y + S(10)});
        static char typed[1024] = "";
        static bool typing = false;
        if (view.picker_missing || typing) {
            typing = true;
            ImGui::PushFont(F.mono);
            ImGui::SetNextItemWidth(width - S(140));
            bool enter = ImGui::InputTextWithHint("##typed", "Paste the folder path here", typed, sizeof typed, ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopFont();
            ImGui::SameLine(0, S(12));
            if ((button("use", "Use folder", {S(128), S(36)}, Kind::ghost, typed[0] != 0) || enter) && typed[0]) {
                std::lock_guard<std::mutex> l(M.mx);
                use_folder(typed);
            }
            if (view.picker_missing) {
                ImGui::PushFont(F.small);
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(MUTED), "No folder picker (zenity or kdialog) was found, so type or paste the folder instead.");
                ImGui::PopFont();
            }
        } else {
            ImGui::PushFont(F.small);
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(MUTED), found ? "Found automatically. Change it if you have more than one copy of the game."
                                                                             : "Choose the folder that contains DyingLightGame.exe.");
            ImGui::PopFont();
            ImGui::SameLine(0, S(8));
            ImGui::PushFont(F.small);
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(SOFT), "Type it instead");
            ImGui::PopFont();
            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (ImGui::IsItemClicked()) typing = true, snprintf(typed, sizeof typed, "%s", found ? view.game.u8string().c_str() : "");
        }
    }
    ImGui::Dummy({0, S(8)});

    {
        Reveal r(3);
        std::string text;
        float progress = -1;
        bool enabled = false;
        bool celebrate = view.job == Job::done && ImGui::GetTime() - view.finished_at < 2.6;
        if (view.job == Job::working) text = view.step + "  " + std::to_string((int)(view.progress * 100)) + "%", progress = view.progress;
        else if (celebrate) text = "Installed", progress = 1;
        else if (view.fetch == Fetch::loading) text = "Checking for the latest version";
        else if (view.fetch == Fetch::failed) text = "Cannot reach GitHub";
        else if (!found) text = "Choose the game folder first";
        else if (outdated) text = "Update to " + view.latest.version, enabled = true;
        else if (newer_installed) text = "Replace " + view.installed + " with " + view.latest.version, enabled = true;
        else if (current) text = "Reinstall " + view.latest.version, enabled = true;
        else text = "Install FaTrainer " + view.latest.version, enabled = true;
        ImVec2 at = ImGui::GetCursorScreenPos();
        float shown_progress = progress >= 0 ? follow(anim("progress"), progress, 10) : (anim("progress") = 0);
        if (button("install", text, {S(300), S(52)}, Kind::primary, enabled, progress >= 0 ? shown_progress : -1)) start_install();
        if (celebrate) check_mark(dl, {at.x + S(300) / 2 - text_w(F.strong, "Installed") / 2 - S(22), at.y + S(26)}, S(14), (float)(ImGui::GetTime() - view.finished_at) * 2.2f, C(ACCENT_INK));
        if (!view.installed.empty() && view.job != Job::working) {
            ImGui::SameLine(0, S(18));
            if (button("uninstall", "Uninstall", {S(110), S(52)}, Kind::quiet)) uninstall();
        }
        std::string message = view.message;
        if (message.empty() && view.fetch == Fetch::failed) message = "Could not check for the latest release: " + view.fetch_error + ".", view.message_bad = true;
        if (!message.empty()) {
            float shown = ease_out((float)(ImGui::GetTime() - view.finished_at) / 0.5f);
            if (view.finished_at < 0) shown = 1;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * shown);
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + width);
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(view.message_bad ? BAD : SOFT), "%s", message.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleVar();
        }
    }

    if (platform::LINUX) {
        ImGui::Dummy({0, S(10)});
        Reveal r(4);
        ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + width, a.y + S(132)};
        dl->AddRectFilled(a, b, C(RAISED), S(12));
        label_caps(dl, {a.x + S(20), a.y + S(18)}, "STEAM LAUNCH OPTION", MUTED);
        dl->AddText(F.small, F.small->FontSize, {a.x + S(20), a.y + S(40)}, C(SOFT),
                    "Proton needs this once: Steam, right-click Dying Light, Properties, Launch options.");
        ImVec2 c0{a.x + S(20), a.y + S(68)}, c1{b.x - S(130), a.y + S(112)};
        dl->AddRectFilled(c0, c1, C(GROUND), S(8));
        dl->AddText(F.mono, F.mono->FontSize, {c0.x + S(14), (c0.y + c1.y - F.mono->FontSize) / 2}, C(ACCENT), LAUNCH_OPTION);
        static double copied_at = -10;
        ImGui::SetCursorScreenPos({c1.x + S(12), c0.y});
        if (button("copy", ImGui::GetTime() - copied_at < 1.8 ? "Copied" : "Copy", {S(98), S(44)}, Kind::ghost)) {
            ImGui::SetClipboardText(LAUNCH_OPTION);
            copied_at = ImGui::GetTime();
        }
        ImGui::SetCursorScreenPos({a.x, b.y});
        ImGui::Dummy({width, 0});
    }
    ImGui::Dummy({0, S(24)});
}

inline void changelog(float width) {
    std::vector<logic::ChangelogEntry> entries;
    {
        std::lock_guard<std::mutex> l(M.mx);
        entries = M.changelog;
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (size_t i = 0; i < entries.size(); i++) {
        Reveal r((int)std::min<size_t>(i, 6));
        ImVec2 a = ImGui::GetCursorScreenPos();
        float version_w = S(150);
        dl->AddText(F.hero, F.hero->FontSize * (i == 0 ? 1.0f : 0.7f), {a.x, a.y - S(4)}, C(i == 0 ? ACCENT : TEXT), entries[i].version.c_str());
        if (i == 0) label_caps(dl, {a.x + S(2), a.y + F.hero->FontSize + S(4)}, "NEWEST", MUTED);
        ImGui::SetCursorScreenPos({a.x + version_w, a.y + S(4)});
        ImGui::BeginGroup();
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + width - version_w - S(10));
        for (const std::string& line : entries[i].lines) {
            ImVec2 p = ImGui::GetCursorScreenPos();
            dl->AddCircleFilled({p.x + S(3), p.y + F.body->FontSize * 0.55f}, S(2.5f), C(line.rfind("New", 0) == 0 ? ACCENT : line.rfind("Fixed", 0) == 0 ? GOOD : MUTED));
            ImGui::SetCursorScreenPos({p.x + S(16), p.y});
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(i == 0 ? TEXT : SOFT));
            ImGui::TextWrapped("%s", line.c_str());
            ImGui::PopStyleColor();
            ImGui::Dummy({0, S(2)});
        }
        ImGui::PopTextWrapPos();
        ImGui::EndGroup();
        ImVec2 end = ImGui::GetCursorScreenPos();
        float bottom = std::max(end.y, a.y + F.hero->FontSize + S(30));
        dl->AddLine({a.x, bottom + S(14)}, {a.x + width, bottom + S(14)}, C(LINE_SOFT));
        ImGui::SetCursorScreenPos({a.x, bottom + S(34)});
        ImGui::Dummy({width, 0});
    }
}

inline void frame() {
    ImGuiIO& io = ImGui::GetIO();
    float t = (float)ImGui::GetTime();
    ImVec2 size = io.DisplaySize;
    ImGui::GetBackgroundDrawList()->AddRectFilled({0, 0}, size, GROUND);

    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(size);
    ImGui::Begin("FaTrainer Installer", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float MARGIN = S(36), BAR = S(84);
    const char* WORD = "FaTrainer";
    float move = ease_in_out((t - 0.95f) / 0.8f);
    float font_size = lerp(F.intro->FontSize, F.brand->FontSize, move);
    ImFont* font = move >= 1 ? F.brand : F.intro;
    ImVec2 start{(size.x - text_w(F.intro, WORD)) / 2, size.y / 2 - F.intro->FontSize * 0.62f}, end{MARGIN, S(28)};
    draw_letters(dl, font, font_size, lerp(start, end, move), WORD, t - 0.15f, TEXT);
    float intro_line = ease_expo((t - 0.55f) / 0.6f) * (1 - move);
    if (intro_line > 0) {
        float w = text_w(F.intro, WORD) * intro_line;
        dl->AddRectFilled({size.x / 2 - w / 2, start.y + F.intro->FontSize * 1.18f}, {size.x / 2 + w / 2, start.y + F.intro->FontSize * 1.18f + S(3)}, C(ACCENT));
    }
    float shown = ease_out((t - CONTENT_AT) / 0.7f);
    if (shown > 0) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, shown);
        float brand_w = text_w(F.brand, WORD);
        const char* SUBTITLE = "Installer  \xc2\xb7  " FATRAINER_EDITION;
        dl->AddText(F.small, F.small->FontSize, {MARGIN + brand_w + S(12), S(28) + F.brand->FontSize - F.small->FontSize - S(2)}, C(MUTED), SUBTITLE);
        tabs({MARGIN + brand_w + S(12) + text_w(F.small, SUBTITLE) + S(44), S(32)});
        status_pill({size.x - MARGIN, S(24)});
        dl->AddLine({MARGIN, BAR}, {MARGIN + (size.x - 2 * MARGIN) * shown, BAR}, C(LINE_SOFT));
        ImGui::PopStyleVar();

        float top = BAR + S(28), height = size.y - top;
        if (tab == 0) {
            float list_w = std::clamp(size.x * 0.24f, S(220), S(290));
            std::string state;
            {
                std::lock_guard<std::mutex> l(M.mx);
                bool ready = M.fetch == Fetch::ready;
                if (M.installed == logic::LEGACY_VERSION || (!M.installed.empty() && ready && newer_version(M.latest.version, M.installed))) state = "update";
                else if (!M.installed.empty()) state = "installed";
            }
            game_list({MARGIN, top}, list_w, state);
            float x = MARGIN + list_w + S(32), w = size.x - x - MARGIN;
            ImGui::SetCursorScreenPos({x, top});
            ImGui::BeginChild("detail", {w + S(12), height}, 0, ImGuiWindowFlags_NoBackground);
            detail(w);
            ImGui::EndChild();
        } else {
            float w = std::min(size.x - 2 * MARGIN, S(860));
            ImGui::SetCursorScreenPos({(size.x - w) / 2, top});
            ImGui::BeginChild("changelog", {w + S(12), height}, 0, ImGuiWindowFlags_NoBackground);
            changelog(w);
            ImGui::EndChild();
        }
    }
    ImGui::End();
    soft_glow(ImGui::GetForegroundDrawList(), {size.x * 0.82f + sinf(t * 0.21f) * S(60), -S(120)}, S(520), ACCENT, 0.08f);
}

}
