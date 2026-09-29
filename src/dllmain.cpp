#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <functional>
#include <string>
#include <thread>
#include "game.h"
#include "font.h"
#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static const char* TITLE = "FaTrainer | Dying Light";

static void logf(const char* fmt, ...) {
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

void dlt_assert_fail(const char* expr, const char* file, int line) {
    static std::atomic<int> n{0};
    if (n++ < 20) logf("imgui assert: %s (%s:%d)", expr, file, line);
}

static FARPROC real(const char* name) {
    static HMODULE h = [] {
        char p[MAX_PATH];
        GetSystemDirectoryA(p, MAX_PATH);
        strcat(p, "\\xinput1_3.dll");
        return LoadLibraryA(p);
    }();
    return h ? GetProcAddress(h, name) : nullptr;
}
#define FWD(ret, name, fail, params, args)                  \
    extern "C" ret WINAPI name params {                     \
        static auto fn_ = (ret(WINAPI*) params)real(#name); \
        return fn_ ? fn_ args : fail;                       \
    }
FWD(DWORD, XInputGetState, ERROR_DEVICE_NOT_CONNECTED, (DWORD i, void* s), (i, s))
FWD(DWORD, XInputSetState, ERROR_DEVICE_NOT_CONNECTED, (DWORD i, void* s), (i, s))
FWD(DWORD, XInputGetCapabilities, ERROR_DEVICE_NOT_CONNECTED, (DWORD i, DWORD f, void* c), (i, f, c))
FWD(DWORD, XInputGetDSoundAudioDeviceGuids, ERROR_DEVICE_NOT_CONNECTED, (DWORD i, GUID* a, GUID* b), (i, a, b))
FWD(DWORD, XInputGetBatteryInformation, ERROR_DEVICE_NOT_CONNECTED, (DWORD i, BYTE t, void* b), (i, t, b))
FWD(DWORD, XInputGetKeystroke, ERROR_DEVICE_NOT_CONNECTED, (DWORD i, DWORD r, void* k), (i, r, k))
extern "C" void WINAPI XInputEnable(BOOL e) {
    static auto fn_ = (void(WINAPI*)(BOOL))real("XInputEnable");
    if (fn_) fn_(e);
}

static std::atomic<bool> g_open{false};
static DWORD g_last_scan = 0;
static HWND g_hwnd;
static const UINT WM_FATRAINER = WM_APP + 0x4F;
static std::mutex g_qmx;
static std::vector<std::function<void()>> g_queue;
static std::string g_toast;
static DWORD g_toast_at = 0;

static void toast(const std::string& s) {
    std::lock_guard<std::mutex> l(g_qmx);
    g_toast = s;
    g_toast_at = GetTickCount();
}

static void on_game_thread(std::function<void()> f) {
    {
        std::lock_guard<std::mutex> l(g_qmx);
        g_queue.push_back(std::move(f));
    }
    PostMessageW(g_hwnd, WM_FATRAINER, 0, 0);
}

static void drain_queue() {
    std::vector<std::function<void()>> q;
    {
        std::lock_guard<std::mutex> l(g_qmx);
        q.swap(g_queue);
    }
    for (auto& f : q) f();
}

static void request_refresh() {
    g_last_scan = GetTickCount();
    std::thread([] {
        game::refresh();
        std::lock_guard<std::mutex> l(game::mx);
        logf("refresh: %s", game::g.status.c_str());
        logf("stats: %s", game::stat_report().c_str());
    }).detach();
}

static ImFont *f_body, *f_small, *f_head, *f_big;
static const ImU32 ORANGE_DIM = IM_COL32(243, 154, 30, 90);
static const ImVec4 ORANGE_V{0.953f, 0.604f, 0.118f, 1}, DIM_V{0.55f, 0.55f, 0.55f, 1}, GREEN_V{0.45f, 0.78f, 0.35f, 1},
    RED_V{0.85f, 0.3f, 0.25f, 1};

static void style() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 2, s.ChildRounding = 2, s.FrameRounding = 2, s.TabRounding = 0, s.GrabRounding = 2;
    s.PopupRounding = 2, s.ScrollbarRounding = 2, s.ScrollbarSize = 10;
    s.WindowBorderSize = 1, s.ChildBorderSize = 0, s.FrameBorderSize = 0, s.PopupBorderSize = 1;
    s.WindowPadding = {24, 20}, s.FramePadding = {12, 7}, s.ItemSpacing = {10, 10}, s.CellPadding = {10, 7};
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = {0.07f, 0.07f, 0.07f, 0.96f};
    c[ImGuiCol_Border] = {0.953f, 0.604f, 0.118f, 0.35f};
    c[ImGuiCol_ChildBg] = {0.11f, 0.11f, 0.11f, 1};
    c[ImGuiCol_PopupBg] = {0.08f, 0.08f, 0.08f, 0.99f};
    c[ImGuiCol_ModalWindowDimBg] = {0, 0, 0, 0.55f};
    c[ImGuiCol_Text] = {0.90f, 0.89f, 0.86f, 1};
    c[ImGuiCol_TextDisabled] = DIM_V;
    c[ImGuiCol_FrameBg] = {0.15f, 0.15f, 0.15f, 1};
    c[ImGuiCol_FrameBgHovered] = {0.20f, 0.20f, 0.20f, 1};
    c[ImGuiCol_FrameBgActive] = {0.24f, 0.19f, 0.12f, 1};
    c[ImGuiCol_Button] = {0.18f, 0.18f, 0.18f, 1};
    c[ImGuiCol_ButtonHovered] = {0.953f, 0.604f, 0.118f, 0.85f};
    c[ImGuiCol_ButtonActive] = ORANGE_V;
    c[ImGuiCol_Header] = {0.16f, 0.16f, 0.16f, 1};
    c[ImGuiCol_HeaderHovered] = {0.953f, 0.604f, 0.118f, 0.25f};
    c[ImGuiCol_HeaderActive] = {0.953f, 0.604f, 0.118f, 0.4f};
    c[ImGuiCol_Tab] = {0, 0, 0, 0};
    c[ImGuiCol_TabHovered] = {0.953f, 0.604f, 0.118f, 0.25f};
    c[ImGuiCol_TabSelected] = {0.953f, 0.604f, 0.118f, 0.15f};
    c[ImGuiCol_TabSelectedOverline] = ORANGE_V;
    c[ImGuiCol_TabDimmedSelected] = c[ImGuiCol_TabSelected];
    c[ImGuiCol_Separator] = {1, 1, 1, 0.08f};
    c[ImGuiCol_ScrollbarBg] = {0, 0, 0, 0};
    c[ImGuiCol_ScrollbarGrab] = {0.953f, 0.604f, 0.118f, 0.45f};
    c[ImGuiCol_ScrollbarGrabHovered] = {0.953f, 0.604f, 0.118f, 0.7f};
    c[ImGuiCol_CheckMark] = c[ImGuiCol_SliderGrab] = ORANGE_V;
    c[ImGuiCol_TableRowBg] = {0, 0, 0, 0};
    c[ImGuiCol_TableRowBgAlt] = {1, 1, 1, 0.025f};
    c[ImGuiCol_TableBorderLight] = {1, 1, 1, 0.05f};
    c[ImGuiCol_NavCursor] = ORANGE_V;
    c[ImGuiCol_TextSelectedBg] = {0.953f, 0.604f, 0.118f, 0.35f};
    c[ImGuiCol_ResizeGrip] = {0.953f, 0.604f, 0.118f, 0.2f};
    c[ImGuiCol_ResizeGripHovered] = {0.953f, 0.604f, 0.118f, 0.6f};
    c[ImGuiCol_ResizeGripActive] = ORANGE_V;
}

static std::string thousands(long long v) {
    std::string s = std::to_string(v < 0 ? -v : v);
    for (int i = (int)s.size() - 3; i > 0; i -= 3) s.insert(i, ",");
    return (v < 0 ? "-" : "") + s;
}

struct Cat { const char* label; ImU32 color; };
enum { C_ALL, C_WEAPONS, C_THROWABLE, C_HEALING, C_CRAFTING, C_UPGRADES, C_BLUEPRINTS, C_OTHER, C_COUNT };
static const char* CAT_NAMES[C_COUNT] = {"All", "Weapons", "Throwables", "Healing", "Crafting", "Upgrades", "Blueprints", "Other"};

static int cat_of(const char* id) {
    std::string s = id;
    auto has = [&](const char* p) { return s.find(p) != std::string::npos; };
    if (!strncmp(id, "Craftplan_", 10)) return C_BLUEPRINTS;
    if (has("Upgrade") || !strncmp(id, "CWU_", 4)) return C_UPGRADES;
    if (has("Firearm") || has("Bow") || has("Ammo") || has("Melee")) return C_WEAPONS;
    if (has("Throwable")) return C_THROWABLE;
    if (has("Medkit") || has("Potion") || has("Booster")) return C_HEALING;
    if (has("Craft")) return C_CRAFTING;
    return C_OTHER;
}

static Cat category(const char* id) {
    switch (cat_of(id)) {
        case C_WEAPONS: return {strstr(id, "Melee") ? "Melee" : "Firearm", strstr(id, "Melee") ? IM_COL32(243, 154, 30, 255) : IM_COL32(70, 160, 240, 255)};
        case C_THROWABLE: return {"Throwable", IM_COL32(170, 90, 240, 255)};
        case C_HEALING: return {"Healing", IM_COL32(225, 75, 60, 255)};
        case C_CRAFTING: return {"Crafting", IM_COL32(95, 190, 70, 255)};
        case C_UPGRADES: return {"Upgrade", IM_COL32(60, 200, 190, 255)};
        case C_BLUEPRINTS: return {"Blueprint", IM_COL32(230, 210, 90, 255)};
    }
    return {"Gear", IM_COL32(160, 160, 160, 255)};
}

static bool is_test_item(const char* id) {
    for (const char* p : {"AAA", "TEST", "Test_", "_test", "zzz", "ZZZZ", "Dev", "DEBUG", "Debug"})
        if (strstr(id, p)) return true;
    return false;
}

static void section(const char* t) {
    ImGui::PushFont(f_small);
    ImGui::PushStyleColor(ImGuiCol_Text, ORANGE_V);
    ImGui::TextUnformatted(t);
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

static bool accent_button(const char* label, ImVec2 size = {0, 0}) {
    ImGui::PushStyleColor(ImGuiCol_Button, ORANGE_V);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(20, 20, 20, 255));
    bool r = ImGui::Button(label, size);
    ImGui::PopStyleColor(2);
    return r;
}

static void item_label(const char* name, const char* id) {
    Cat c = category(id);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddRectFilled({p.x - 6, p.y + 2}, {p.x - 2, p.y + 44}, c.color);
    ImGui::SetCursorScreenPos({p.x + 8, p.y + 2});
    ImGui::TextUnformatted(name);
    ImGui::SetCursorScreenPos({p.x + 8, p.y + 26});
    ImGui::PushFont(f_small);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(c.color), "%s", c.label);
    ImGui::SameLine();
    ImGui::TextDisabled("%s", id);
    ImGui::PopFont();
}

static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

static void header() {
    ImGui::PushFont(f_head);
    ImGui::PushStyleColor(ImGuiCol_Text, ORANGE_V);
    ImGui::TextUnformatted("FaTrainer");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextDisabled("|  DYING LIGHT");
    ImGui::PopFont();
    const char* st = game::g.scanning ? "Scanning..." : game::g.vt_money ? "Connected" : "Game not ready";
    ImVec4 col = game::g.scanning ? ORANGE_V : game::g.vt_money ? GREEN_V : RED_V;
    ImGui::PushFont(f_small);
    float w = ImGui::CalcTextSize(st).x + 22;
    ImGui::SameLine(ImGui::GetWindowWidth() - w - 24 - 90);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddCircleFilled({p.x + 6, p.y + ImGui::GetTextLineHeight() * 0.5f + 8}, 4.5f, ImGui::GetColorU32(col));
    ImGui::SetCursorScreenPos({p.x + 18, p.y + 8});
    ImGui::TextColored(col, "%s", st);
    ImGui::PopFont();
    ImGui::SameLine(ImGui::GetWindowWidth() - 24 - 84);
    ImGui::BeginDisabled(game::g.scanning);
    if (ImGui::Button("Refresh", {84, 0})) request_refresh();
    ImGui::EndDisabled();
    ImVec2 a = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddLine(a, {a.x + ImGui::GetContentRegionAvail().x, a.y}, ORANGE_DIM, 1.5f);
    ImGui::Dummy({0, 4});
}

static void cash_tab() {
    if (game::g.wallets.empty()) {
        ImGui::Spacing();
        ImGui::TextDisabled("No wallet found yet.\nLoad into your save, then press Refresh.");
        return;
    }
    static int custom = 100000;
    for (size_t i = 0; i < game::g.wallets.size(); i++) {
        uintptr_t w = game::g.wallets[i];
        ImGui::PushID((int)i);
        ImGui::BeginChild("card", {0, 196}, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
        ImGui::SetCursorPos({22, 16});
        section(game::g.wallets.size() > 1 ? ("WALLET " + std::to_string(i + 1)).c_str() : "CASH");
        ImGui::SetCursorPosX(22);
        ImGui::PushFont(f_big);
        ImGui::TextUnformatted(("$" + thousands(game::money(w))).c_str());
        ImGui::PopFont();
        ImGui::SetCursorPosX(22);
        for (int a : {1000, 10000, 100000}) {
            if (ImGui::Button(("+" + thousands(a)).c_str())) game::set_money(w, game::money(w) + a);
            ImGui::SameLine();
        }
        if (ImGui::Button("Max")) game::set_money(w, 9999999);
        ImGui::SameLine(0, 28);
        ImGui::SetNextItemWidth(150);
        ImGui::InputInt("##custom", &custom, 0, 0);
        ImGui::SameLine();
        if (accent_button("Set")) game::set_money(w, std::max(0, custom));
        ImGui::EndChild();
        ImGui::PopID();
    }
    if (game::g.wallets.size() > 1)
        ImGui::TextDisabled("Several wallets found: yours shows the same amount as the game's inventory screen.");
}

static uintptr_t g_edit_desc = 0;
static std::string g_edit_name, g_edit_id;

static bool editable(const game::Item& it) {
    if (!it.info) return false;
    int c = cat_of(it.info->id);
    return c == C_WEAPONS || c == C_THROWABLE;
}

static bool g_edit_request = false;

static void open_editor(uintptr_t desc, const std::string& name, const std::string& id) {
    g_edit_desc = desc, g_edit_name = name, g_edit_id = id;
    g_edit_request = true;
}

static const char* RARITY[] = {"Common (white)", "Uncommon (green)", "Rare (blue)", "Unique (violet)", "Legendary (orange)", "Gold tier (platinum)"};

static void editor_popup() {
    if (g_edit_request) {
        ImGui::OpenPopup("Edit weapon");
        g_edit_request = false;
    }
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f}, ImGuiCond_Appearing, {0.5f, 0.5f});
    if (!ImGui::BeginPopupModal("Edit weapon", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize)) return;
    ImGui::PushFont(f_head);
    ImGui::TextUnformatted(g_edit_name.c_str());
    ImGui::PopFont();
    ImGui::PushFont(f_small);
    ImGui::TextDisabled("%s  -  changes apply to every item of this type", g_edit_id.c_str());
    ImGui::PopFont();
    ImGui::Separator();
    struct Row { int stat; const char* label; const char* fmt; float mul; };
    const Row rows[] = {{ST_Damage, "Damage", "%.1f", 2},
                        {ST_Condition, "Durability", "%.0f", 10},
                        {ST_CriticalProb, "Critical chance", "%.2f", 2},
                        {ST_CriticalDamage, "Critical damage", "%.2f", 2},
                        {ST_Force, "Knockback force", "%.1f", 2},
                        {ST_StaminaUsage, "Stamina per swing", "%.3f", 0.5f},
                        {ST_DamageRange, "Reach", "%.2f", 1.5f},
                        {ST_UpgradeLevel, "Upgrade level", "%.0f", 0},
                        {ST_AllowedRepairs, "Repairs left", "%.0f", 0},
                        {ST_MaxStackCount, "Max stack", "%.0f", 0},
                        {ST_Price, "Price", "%.0f", 0}};
    int shown = 0;
    if (ImGui::BeginTable("stats", 3, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed, 190);
        ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthFixed, 230);
        ImGui::TableSetupColumn("quick", ImGuiTableColumnFlags_WidthFixed, 70);
        for (auto& r : rows) {
            if (!game::has_stat(r.stat)) continue;
            float v = game::get_stat(g_edit_desc, r.stat);
            if (std::isnan(v)) continue;
            shown++;
            ImGui::PushID(r.stat);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(r.label);
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(160);
            float e = v;
            ImGui::InputFloat("##v", &e, 0, 0, r.fmt);
            if (ImGui::IsItemDeactivatedAfterEdit()) game::set_stat(g_edit_desc, r.stat, e);
            ImGui::TableNextColumn();
            if (r.mul > 0) {
                char b[16];
                snprintf(b, sizeof b, "x%g", r.mul);
                if (ImGui::Button(b)) game::set_stat(g_edit_desc, r.stat, v * r.mul);
            } else if (ImGui::Button("+1")) {
                game::set_stat(g_edit_desc, r.stat, v + 1);
            }
            ImGui::PopID();
        }
        if (game::has_stat(ST_Color)) {
            int cur = (int)game::get_stat(g_edit_desc, ST_Color);
            if (cur >= 0 && cur < 6) {
                shown++;
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Rarity");
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(220);
                if (ImGui::Combo("##rarity", &cur, RARITY, 6)) game::set_stat(g_edit_desc, ST_Color, (float)cur);
            }
        }
        ImGui::EndTable();
    }
    if (!shown) ImGui::TextDisabled("No editable stats found for this item.\nPress Refresh after loading your save; see fatrainer.log.");
    ImGui::Spacing();
    if (accent_button("Done", {120, 0})) ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}

static void inventory_tab(game::Kind kind) {
    static char filter[64] = "";
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##f", "Search items...", filter, sizeof filter);
    std::string f = lower(filter);
    int shown_inv = 0;
    ImGui::BeginChild("list", {0, 0}, ImGuiChildFlags_None);
    for (size_t i = 0; i < game::g.invs.size(); i++) {
        auto& inv = game::g.invs[i];
        if (inv.kind != kind) continue;
        shown_inv++;
        std::string title = game::KIND_NAMES[kind];
        for (auto& c : title) c = toupper(c);
        title += "   " + std::to_string(inv.items.size()) + " ITEMS";
        if (inv.capacity > 0) title += "  /  " + std::to_string(inv.capacity) + " SLOTS";
        ImGui::Spacing();
        section(title.c_str());
        if (!ImGui::BeginTable(("t" + std::to_string(i)).c_str(), 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) continue;
        ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Amount", ImGuiTableColumnFlags_WidthFixed, 262);
        for (size_t j = 0; j < inv.items.size(); j++) {
            auto& it = inv.items[j];
            const char* id = it.info ? it.info->id : "";
            if (!f.empty() && lower(it.name + " " + id).find(f) == std::string::npos) continue;
            ImGui::PushID((int)(i * 10000 + j));
            ImGui::TableNextRow(0, 52);
            ImGui::TableNextColumn();
            item_label(it.name.c_str(), id);
            ImGui::TableNextColumn();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
            int v = game::count(it);
            float bw = ImGui::GetFrameHeight();
            if (ImGui::Button("-", {bw, 0})) game::set_count(it, std::max(0, v - 1));
            ImGui::SameLine(0, 4);
            ImGui::SetNextItemWidth(70);
            int e = v;
            ImGui::InputInt("##c", &e, 0, 0);
            if (ImGui::IsItemDeactivatedAfterEdit()) game::set_count(it, e);
            ImGui::SameLine(0, 4);
            if (ImGui::Button("+", {bw, 0})) game::set_count(it, v + 1);
            ImGui::SameLine(0, 4);
            static bool auto_edit = getenv("DLT_EDIT") != nullptr;
            if (auto_edit && editable(it) && it.info && strstr(it.info->id, "Machete")) {
                open_editor(game::item_desc(it), it.name, id);
                auto_edit = false;
            }
            if (editable(it)) {
                if (ImGui::Button("Edit", {64, 0})) open_editor(game::item_desc(it), it.name, id);
            } else if (ImGui::Button("99", {64, 0})) {
                game::set_count(it, 99);
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    if (!shown_inv) ImGui::TextDisabled("Nothing found here yet.\nLoad into your save, then press Refresh.");
    editor_popup();
    ImGui::EndChild();
}

static void give_item(const ItemInfo* info, int amount, int target) {
    auto d = game::g.descs.find(info->id);
    uintptr_t desc = d == game::g.descs.end() ? 0 : d->second;
    std::string name = game::display_name(info);
    if (!desc) return toast("Not available in this game session: " + name);
    game::Kind k = target < 0 ? game::default_target(info->id) : (game::Kind)target;
    on_game_thread([=] {
        uintptr_t inv, tmpl;
        {
            std::lock_guard<std::mutex> l(game::mx);
            auto* i = game::find_inventory(k);
            if (!i) i = game::find_inventory(game::K_BACKPACK);
            inv = i ? i->obj : 0;
            tmpl = game::pick_template(i);
        }
        if (!inv || !tmpl) return toast("No inventory found yet: load your save and press Refresh");
        bool ok = game::give(inv, tmpl, desc, amount);
        logf("give %s x%d -> %s: %s", info->id, amount, game::KIND_NAMES[k], ok ? "ok" : "refused");
        toast(ok ? "Added " + std::to_string(amount) + " x " + name : "The game refused " + name + " (inventory full?)");
        request_refresh();
    });
}

static void give_tab() {
    static char filter[64] = "";
    static int cat = C_ALL, amount = 1, target = -1;
    static bool show_test = false;
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##gf", "Search every item in the game...", filter, sizeof filter);
    for (int c = 0; c < C_COUNT; c++) {
        bool on = cat == c;
        if (on) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.953f, 0.604f, 0.118f, 0.35f});
        if (ImGui::Button(CAT_NAMES[c])) cat = c;
        if (on) ImGui::PopStyleColor();
        ImGui::SameLine(0, 6);
    }
    ImGui::NewLine();
    ImGui::SetNextItemWidth(120);
    ImGui::InputInt("Amount", &amount, 1, 10);
    amount = std::clamp(amount, 1, 9999);
    ImGui::SameLine(0, 24);
    const char* targets[] = {"Auto", "Backpack", "Stash", "Materials"};
    int ti = target < 0 ? 0 : target == game::K_BACKPACK ? 1 : target == game::K_STASH ? 2 : 3;
    ImGui::SetNextItemWidth(150);
    if (ImGui::Combo("Into", &ti, targets, 4)) target = ti == 0 ? -1 : ti == 1 ? game::K_BACKPACK : ti == 2 ? game::K_STASH : game::K_MATERIALS;
    ImGui::SameLine(0, 24);
    ImGui::Checkbox("Show internal items", &show_test);

    std::string f = lower(filter);
    std::vector<const ItemInfo*> list;
    for (int i = 0; i < ITEM_COUNT; i++) {
        const ItemInfo* info = &ITEMS[i];
        if (!show_test && (!info->name[0] || is_test_item(info->id))) continue;
        if (cat != C_ALL && cat_of(info->id) != cat) continue;
        if (!f.empty() && lower(game::display_name(info) + " " + info->id).find(f) == std::string::npos) continue;
        list.push_back(info);
    }
    if (cat == C_BLUEPRINTS && !list.empty()) {
        if (accent_button(("Give all " + std::to_string(list.size()) + " blueprints").c_str()))
            for (auto* info : list) give_item(info, 1, target);
    }
    ImGui::BeginChild("give", {0, 0}, ImGuiChildFlags_None);
    if (ImGui::BeginTable("g", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
        ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Give", ImGuiTableColumnFlags_WidthFixed, 90);
        ImGuiListClipper clip;
        clip.Begin((int)list.size(), 52 + ImGui::GetStyle().CellPadding.y * 2);
        while (clip.Step())
            for (int i = clip.DisplayStart; i < clip.DisplayEnd; i++) {
                auto* info = list[i];
                ImGui::PushID(i);
                ImGui::TableNextRow(0, 52);
                ImGui::TableNextColumn();
                item_label(game::display_name(info).c_str(), info->id);
                ImGui::TableNextColumn();
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
                if (accent_button("Give", {80, 0})) give_item(info, amount, target);
                ImGui::PopID();
            }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}

static void draw_menu() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowSize({820, 760}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos({io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f}, ImGuiCond_FirstUseEver, {0.5f, 0.5f});
    ImGui::Begin(TITLE, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);
    std::lock_guard<std::mutex> l(game::mx);
    header();
    static int first_tab = getenv("DLT_TAB") ? atoi(getenv("DLT_TAB")) : -1;
    auto tab = [&](const char* name, int idx) {
        bool r = ImGui::BeginTabItem(name, nullptr, first_tab == idx ? ImGuiTabItemFlags_SetSelected : 0);
        return r;
    };
    if (ImGui::BeginTabBar("tabs")) {
        if (tab("  CASH  ", 0)) { cash_tab(); ImGui::EndTabItem(); }
        if (tab("  BACKPACK  ", 1)) { inventory_tab(game::K_BACKPACK); ImGui::EndTabItem(); }
        if (tab("  STASH  ", 2)) { inventory_tab(game::K_STASH); ImGui::EndTabItem(); }
        if (tab("  MATERIALS  ", 3)) { inventory_tab(game::K_MATERIALS); ImGui::EndTabItem(); }
        if (tab("  GIVE ITEMS  ", 4)) { give_tab(); ImGui::EndTabItem(); }
        if (game::find_inventory(game::K_TOOLS) && tab("  TOOLS  ", 5)) { inventory_tab(game::K_TOOLS); ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }
    first_tab = -1;
    ImGui::End();

    std::string msg;
    {
        std::lock_guard<std::mutex> q(g_qmx);
        if (GetTickCount() - g_toast_at < 3500) msg = g_toast;
    }
    if (!msg.empty()) {
        ImGui::SetNextWindowPos({io.DisplaySize.x * 0.5f, io.DisplaySize.y - 60}, ImGuiCond_Always, {0.5f, 1});
        ImGui::Begin("##toast", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs);
        ImGui::TextUnformatted(msg.c_str());
        ImGui::End();
    }
}

using PresentFn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);
using ResizeFn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
static PresentFn oPresent;
static ResizeFn oResize;
static ID3D11Device* g_dev;
static ID3D11DeviceContext* g_ctx;
static WNDPROC oWndProc;
static bool g_ready;
static std::atomic<long> g_dx{0}, g_dy{0}, g_wheel{0};

static void on_raw_input(LPARAM l) {
    RAWINPUT ri;
    UINT sz = sizeof ri;
    if (GetRawInputData((HRAWINPUT)l, RID_INPUT, &ri, &sz, sizeof(RAWINPUTHEADER)) == (UINT)-1) return;
    if (ri.header.dwType != RIM_TYPEMOUSE) return;
    auto& m = ri.data.mouse;
    if (!(m.usFlags & MOUSE_MOVE_ABSOLUTE)) { g_dx += m.lLastX; g_dy += m.lLastY; }
    if (m.usButtonFlags & RI_MOUSE_WHEEL) g_wheel += (SHORT)m.usButtonData;
}

static LRESULT CALLBACK hkWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    static bool logged = false;
    if (!logged) { logged = true; logf("window thread %lu", (unsigned long)GetCurrentThreadId()); }
    drain_queue();
    if (m == WM_FATRAINER) return 0;
    if (g_open) {
        switch (m) {
            case WM_INPUT: on_raw_input(l); return DefWindowProcW(h, m, w, l);
            case WM_KEYDOWN: case WM_KEYUP: case WM_SYSKEYDOWN: case WM_SYSKEYUP: case WM_CHAR:
                ImGui_ImplWin32_WndProcHandler(h, m, w, l);
                return 0;
            case WM_MOUSEWHEEL:
                g_wheel += GET_WHEEL_DELTA_WPARAM(w);
                return 0;
            case WM_MOUSEMOVE: case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_RBUTTONDOWN: case WM_RBUTTONUP:
            case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_LBUTTONDBLCLK: case WM_RBUTTONDBLCLK: case WM_MOUSEHWHEEL:
                return 0;
        }
    }
    return CallWindowProcW(oWndProc, h, m, w, l);
}

static void feed_mouse() {
    ImGuiIO& io = ImGui::GetIO();
    static float vx = -1, vy = -1;
    static POINT last_os{-1, -1};
    static bool btn[3];
    RECT cr;
    GetClientRect(g_hwnd, &cr);
    if (vx < 0) { vx = cr.right / 2.0f; vy = cr.bottom / 2.0f; }
    RECT clip;
    GetClipCursor(&clip);
    bool locked = (clip.right - clip.left) <= 8 || (clip.bottom - clip.top) <= 8;
    POINT p;
    GetCursorPos(&p);
    long dx = g_dx.exchange(0), dy = g_dy.exchange(0);
    if (!locked && (p.x != last_os.x || p.y != last_os.y)) {
        POINT c = p;
        ScreenToClient(g_hwnd, &c);
        vx = (float)c.x, vy = (float)c.y;
    } else {
        vx += dx, vy += dy;
    }
    last_os = p;
    vx = std::clamp(vx, 0.0f, (float)cr.right - 1), vy = std::clamp(vy, 0.0f, (float)cr.bottom - 1);
    io.AddMousePosEvent(vx, vy);
    const int vk[3] = {VK_LBUTTON, VK_RBUTTON, VK_MBUTTON};
    for (int i = 0; i < 3; i++) {
        bool down = GetAsyncKeyState(vk[i]) & 0x8000;
        if (down != btn[i]) io.AddMouseButtonEvent(i, btn[i] = down);
    }
    if (long wh = g_wheel.exchange(0)) io.AddMouseWheelEvent(0, wh / (float)WHEEL_DELTA);
}

static void init_imgui(IDXGISwapChain* sc) {
    if (FAILED(sc->GetDevice(__uuidof(ID3D11Device), (void**)&g_dev))) return;
    g_dev->GetImmediateContext(&g_ctx);
    DXGI_SWAP_CHAIN_DESC d;
    sc->GetDesc(&d);
    g_hwnd = d.OutputWindow;
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NoMouseCursorChange;
    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;
    f_body = io.Fonts->AddFontFromMemoryTTF((void*)FONT_TTF, sizeof FONT_TTF, 21.0f, &cfg);
    f_small = io.Fonts->AddFontFromMemoryTTF((void*)FONT_TTF, sizeof FONT_TTF, 16.0f, &cfg);
    f_head = io.Fonts->AddFontFromMemoryTTF((void*)FONT_TTF, sizeof FONT_TTF, 30.0f, &cfg);
    f_big = io.Fonts->AddFontFromMemoryTTF((void*)FONT_TTF, sizeof FONT_TTF, 56.0f, &cfg);
    style();
    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);
    oWndProc = (WNDPROC)SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
    g_ready = true;
    logf("overlay ready (window %p, render thread %lu)", (void*)g_hwnd, (unsigned long)GetCurrentThreadId());
}

static HRESULT WINAPI hkPresent(IDXGISwapChain* sc, UINT sync, UINT flags) {
    static bool prev = false;
    bool key = (GetAsyncKeyState(VK_INSERT) | GetAsyncKeyState(VK_F8)) & 0x8000;
    if (key && !prev) g_open = !g_open;
    prev = key;
    if (!g_ready) init_imgui(sc);
    if (g_open && GetTickCount() - g_last_scan > 3000) {
        std::lock_guard<std::mutex> l(game::mx);
        if (!game::g.scanning && (game::g.wallets.empty() || game::g.invs.empty() || game::g.descs.empty())) request_refresh();
    }
    if (g_ready && g_open) {
        ImGui::GetIO().MouseDrawCursor = true;
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        feed_mouse();
        ImGui::NewFrame();
        draw_menu();
        ImGui::Render();
        ID3D11Texture2D* bb = nullptr;
        if (SUCCEEDED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb))) {
            ID3D11RenderTargetView* rtv = nullptr;
            if (SUCCEEDED(g_dev->CreateRenderTargetView(bb, nullptr, &rtv))) {
                g_ctx->OMSetRenderTargets(1, &rtv, nullptr);
                ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
                rtv->Release();
            }
            bb->Release();
        }
    }
    return oPresent(sc, sync, flags);
}

static HRESULT WINAPI hkResize(IDXGISwapChain* sc, UINT n, UINT w, UINT h, DXGI_FORMAT f, UINT fl) {
    return oResize(sc, n, w, h, f, fl);
}

static bool hook_d3d() {
    WNDCLASSEXA wc{sizeof wc};
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "fatrainer_dummy";
    RegisterClassExA(&wc);
    HWND w = CreateWindowExA(0, wc.lpszClassName, "", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = w;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    IDXGISwapChain* sc = nullptr;
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                                               &sd, &sc, &dev, nullptr, &ctx);
    bool ok = SUCCEEDED(hr);
    if (ok) {
        void** vt = *(void***)sc;
        DWORD old;
        VirtualProtect(&vt[8], sizeof(void*) * 6, PAGE_EXECUTE_READWRITE, &old);
        oPresent = (PresentFn)vt[8];
        oResize = (ResizeFn)vt[13];
        vt[8] = (void*)hkPresent;
        vt[13] = (void*)hkResize;
        VirtualProtect(&vt[8], sizeof(void*) * 6, old, &old);
        sc->Release();
        ctx->Release();
        dev->Release();
    }
    DestroyWindow(w);
    UnregisterClassA(wc.lpszClassName, wc.hInstance);
    logf("d3d11 hook: %s (hr=%08lx)", ok ? "ok" : "FAILED", (unsigned long)hr);
    return ok;
}

static LONG CALLBACK crash_logger(EXCEPTION_POINTERS* e) {
    DWORD code = e->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_ILLEGAL_INSTRUCTION && code != EXCEPTION_STACK_OVERFLOW &&
        code != EXCEPTION_INT_DIVIDE_BY_ZERO && code != 0xC0000409)
        return EXCEPTION_CONTINUE_SEARCH;
    void* at = e->ExceptionRecord->ExceptionAddress;
    HMODULE m = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)at, &m);
    char path[MAX_PATH] = "?";
    if (m) GetModuleFileNameA(m, path, MAX_PATH);
    const char* name = strrchr(path, '\\') ? strrchr(path, '\\') + 1 : path;
    if (!_stricmp(name, "kernelbase.dll") || !_stricmp(name, "kernel32.dll")) return EXCEPTION_CONTINUE_SEARCH;
    static std::atomic<int> n{0};
    if (n++ < 30)
        logf("exception %08lx in %s+%llx (data %llx) thread %lu menu_open=%d  [may be handled by the game]",
             (unsigned long)code, name, (unsigned long long)((uintptr_t)at - (uintptr_t)m),
             (unsigned long long)(e->ExceptionRecord->NumberParameters > 1 ? e->ExceptionRecord->ExceptionInformation[1] : 0),
             (unsigned long)GetCurrentThreadId(), (int)g_open.load());
    return EXCEPTION_CONTINUE_SEARCH;
}

static void main_thread() {
    logf("--- %s loaded", TITLE);
    HMODULE gamedll = nullptr;
    while (!(gamedll = GetModuleHandleA("gamedll_x64_rwdi.dll"))) Sleep(200);
    if (game::resolve_classes((uintptr_t)gamedll))
        logf("classes: money +%llx, inventory +%llx, item manager +%llx", (unsigned long long)(game::g.vt_money - game::g.base),
             (unsigned long long)(game::g.vt_inv[0] - game::g.base), (unsigned long long)(game::g.vt_manager - game::g.base));
    else
        logf("ERROR: %s", game::g.status.c_str());
    if (getenv("DLT_OPEN")) g_open = true;
    Sleep(4000);
    hook_d3d();
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        AddVectoredExceptionHandler(1, crash_logger);
        CloseHandle(CreateThread(nullptr, 0, [](LPVOID) -> DWORD { main_thread(); return 0; }, nullptr, 0, nullptr));
    }
    return TRUE;
}
