#pragma once
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>
#include "items.h"

namespace game {

inline bool rd(uintptr_t a, void* out, size_t n) {
    if (a < 0x10000 || IsBadReadPtr((const void*)a, n)) return false;
    memcpy(out, (const void*)a, n);
    return true;
}
template <class T> inline T rdv(uintptr_t a, T def = T{}) {
    T v;
    return rd(a, &v, sizeof v) ? v : def;
}
template <class T> inline bool wr(uintptr_t a, T v) {
    if (a < 0x10000 || IsBadWritePtr((void*)a, sizeof v)) return false;
    memcpy((void*)a, &v, sizeof v);
    return true;
}

struct Section { uintptr_t start, end; std::string name; };

inline std::vector<Section> sections(uintptr_t base) {
    auto dos = (IMAGE_DOS_HEADER*)base;
    auto nt = (IMAGE_NT_HEADERS64*)(base + dos->e_lfanew);
    auto s = IMAGE_FIRST_SECTION(nt);
    std::vector<Section> out;
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, s++)
        out.push_back({base + s->VirtualAddress, base + s->VirtualAddress + s->Misc.VirtualSize,
                       std::string((char*)s->Name, strnlen((char*)s->Name, 8))});
    return out;
}

inline uintptr_t find_vtable(uintptr_t base, const char* cls) {
    std::string needle = std::string(".?AV") + cls + "@@";
    needle.push_back('\0');
    auto secs = sections(base);
    uintptr_t td = 0;
    for (auto& s : secs) {
        if (s.name != ".data" && s.name != ".rdata") continue;
        auto hit = std::search((const char*)s.start, (const char*)s.end, needle.begin(), needle.end());
        if (hit != (const char*)s.end) { td = (uintptr_t)hit - 0x10; break; }
    }
    if (!td) return 0;
    uint32_t td_rva = (uint32_t)(td - base);
    for (auto& s : secs) {
        if (s.name != ".rdata") continue;
        for (uintptr_t p = s.start; p + 24 <= s.end; p += 4) {
            auto col = (const uint32_t*)p;
            if (col[3] != td_rva || col[0] != 1 || col[1] != 0 || col[5] != (uint32_t)(p - base)) continue;
            for (uintptr_t q = s.start; q + 8 <= s.end; q += 8)
                if (*(const uint64_t*)q == p) return q + 8;
        }
    }
    return 0;
}

inline const Section* section_named(const std::vector<Section>& secs, const char* name) {
    for (auto& s : secs)
        if (s.name == name) return &s;
    return nullptr;
}

inline std::vector<uintptr_t> find_code(uintptr_t base, const char* pattern, size_t limit = 1) {
    std::vector<int> bytes;
    for (const char* p = pattern; *p;) {
        if (*p == ' ') { p++; continue; }
        if (*p == '?') { bytes.push_back(-1); p++; continue; }
        bytes.push_back((int)strtol(p, (char**)&p, 16));
    }
    std::vector<uintptr_t> out;
    auto secs = sections(base);
    auto* text = section_named(secs, ".text");
    if (!text || bytes.empty()) return out;
    const uint8_t* b = (const uint8_t*)text->start;
    size_t n = text->end - text->start;
    for (size_t i = 0; i + bytes.size() <= n && out.size() < limit; i++) {
        size_t k = 0;
        while (k < bytes.size() && (bytes[k] < 0 || b[i + k] == bytes[k])) k++;
        if (k == bytes.size()) out.push_back(text->start + i);
    }
    return out;
}

inline uintptr_t rip_target(uintptr_t at, int disp_at, int length) { return at + length + rdv<int32_t>(at + disp_at); }

template <class F> inline void each_heap_chunk(F visit) {
    const size_t CH = 1 << 20;
    MEMORY_BASIC_INFORMATION mbi;
    const uintptr_t STACK_AREA = 0x400000;
    uintptr_t stack_hi = (uintptr_t)((NT_TIB*)NtCurrentTeb())->StackBase, stack_lo = stack_hi - STACK_AREA;
    for (uintptr_t a = 0x10000; a < 0x7FFFFFFF0000ULL && VirtualQuery((LPCVOID)a, &mbi, sizeof mbi); ) {
        uintptr_t s = (uintptr_t)mbi.BaseAddress, e = s + mbi.RegionSize;
        bool ok = mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE && mbi.Protect == PAGE_READWRITE &&
                  mbi.RegionSize <= (1ULL << 30) && !(s < stack_hi && e > stack_lo);
        for (uintptr_t c = s; ok && c < e; c += CH) {
            size_t n = std::min<uintptr_t>(CH, e - c);
            if (!IsBadReadPtr((const void*)c, n)) visit(c, n);
        }
        a = e > a ? e : a + 0x1000;
    }
}

inline std::vector<std::vector<uintptr_t>> scan(const std::vector<uintptr_t>& vts) {
    std::vector<std::vector<uintptr_t>> out(vts.size());
    uintptr_t vts_at = (uintptr_t)vts.data();
    each_heap_chunk([&](uintptr_t c, size_t n) {
        const uint64_t* q = (const uint64_t*)c;
        for (size_t i = 0; i < n / 8; i++)
            for (size_t j = 0; j < vts.size(); j++)
                if (q[i] == vts[j] && vts[j] && (c + i * 8 < vts_at || c + i * 8 >= vts_at + vts.size() * 8))
                    out[j].push_back(c + i * 8);
    });
    return out;
}

inline std::vector<uintptr_t> find_float_records(const std::vector<std::vector<float>>& patterns) {
    std::vector<uintptr_t> out;
    each_heap_chunk([&](uintptr_t c, size_t n) {
        const uint32_t* w = (const uint32_t*)c;
        for (auto& p : patterns) {
            uint32_t first;
            memcpy(&first, p.data(), 4);
            size_t bytes = p.size() * 4;
            for (size_t i = 0; i + p.size() <= n / 4; i++)
                if (w[i] == first && !memcmp(w + i, p.data(), bytes) && (uintptr_t)(w + i) != (uintptr_t)p.data()) out.push_back(c + i * 4);
        }
    });
    return out;
}

inline const ItemInfo* lookup(const char* s) {
    int lo = 0, hi = ITEM_COUNT - 1;
    while (lo <= hi) {
        int m = (lo + hi) / 2, c = strcmp(ITEMS[m].id, s);
        if (!c) return &ITEMS[m];
        if (c < 0) lo = m + 1; else hi = m - 1;
    }
    return nullptr;
}

inline const ItemInfo* str_at(uintptr_t p) {
    char b[72];
    if (!rd(p, b, sizeof b)) return nullptr;
    size_t n = strnlen(b, sizeof b);
    if (n < 3 || n > 64) return nullptr;
    for (size_t i = 0; i < n; i++)
        if (!(isalnum((unsigned char)b[i]) || b[i] == '_')) return nullptr;
    return lookup(b);
}

struct NameRule { int kind = -1, o1 = 0, o2 = 0; };

inline const ItemInfo* resolve(uintptr_t item, const NameRule& r) {
    switch (r.kind) {
        case 0: return str_at(item + r.o1);
        case 1: return str_at(rdv<uintptr_t>(item + r.o1));
        case 2: return str_at(rdv<uintptr_t>(rdv<uintptr_t>(item + r.o1) + r.o2));
        case 3: return str_at(rdv<uintptr_t>(item + r.o1) + r.o2);
    }
    return nullptr;
}

inline NameRule calibrate_names(const std::vector<uintptr_t>& items, int* score_out = nullptr) {
    std::map<std::tuple<int, int, int>, int> score;
    size_t n = std::min<size_t>(items.size(), 80);
    for (size_t i = 0; i < n; i++) {
        uintptr_t it = items[i];
        for (int o1 = 0; o1 < 0x200; o1 += 8) {
            if (str_at(it + o1)) score[{0, o1, 0}]++;
            uintptr_t q = rdv<uintptr_t>(it + o1);
            if (q < 0x10000) continue;
            if (str_at(q)) score[{1, o1, 0}]++;
            for (int o2 = 0; o2 < 0x100; o2 += 8) {
                if (str_at(rdv<uintptr_t>(q + o2))) score[{2, o1, o2}]++;
                if (str_at(q + o2)) score[{3, o1, o2}]++;
            }
        }
    }
    NameRule best;
    int bs = 0;
    for (auto& [k, v] : score)
        if (v > bs) { bs = v; best = {std::get<0>(k), std::get<1>(k), std::get<2>(k)}; }
    if (score_out) *score_out = bs;
    return best;
}

inline std::string readable(std::string id) {
    for (const char* p : {"zzz_", "ZZZZZ_", "ZZZZ3_", "ZZZZ_", "Craft_", "Melee_", "Firearm_", "Throwable_", "Special_", "Medkit_", "Craftplan_"})
        if (id.rfind(p, 0) == 0) id = id.substr(strlen(p));
    if (id.size() > 3 && id.compare(id.size() - 3, 3, "Gen") == 0) id.resize(id.size() - 3);
    std::string out;
    for (size_t i = 0; i < id.size(); i++) {
        char c = id[i], prev = i ? id[i - 1] : ' ';
        if (c == '_') { out += ' '; continue; }
        if (i && prev != '_' && ((isupper((unsigned char)c) && islower((unsigned char)prev)) ||
                                 (isdigit((unsigned char)c) && !isdigit((unsigned char)prev))))
            out += ' ';
        out += c;
    }
    return out;
}

inline std::string display_name(const ItemInfo* info) {
    std::string n = info->name[0] ? info->name : readable(info->id);
    if (!strncmp(info->id, "Craftplan_", 10)) n = "Blueprint: " + n;
    return n;
}

enum Kind { K_BACKPACK, K_STASH, K_MATERIALS, K_TOOLS, K_AMMO, K_COLLECTABLES, K_COUNT };
inline const char* KIND_NAMES[K_COUNT] = {"Backpack", "Stash", "Materials", "Tools", "Ammo", "Collectables"};
inline const char* INV_CLASSES[] = {"InventoryMain", "InventoryLooseItems", "InventorySpecial", "InventoryAmmo", "InventoryCollectable"};
inline const Kind INV_KINDS[] = {K_BACKPACK, K_MATERIALS, K_TOOLS, K_AMMO, K_COLLECTABLES};
const int N_INV_CLASSES = 5;

struct Item { uintptr_t addr; const ItemInfo* info; std::string name; };
struct Inventory { uintptr_t obj; Kind kind; int capacity; std::vector<Item> items; };
struct StatField { int off = -1; bool is_float = true; int votes = 0, samples = 0; };

struct State {
    uintptr_t base = 0, vt_money = 0, vt_manager = 0, vt_inv[N_INV_CLASSES] = {};
    uintptr_t vt_player = 0, vt_human = 0, vt_health[3] = {};
    std::vector<uintptr_t> wallets, players;
    std::map<std::pair<uintptr_t, int>, float> stat_overrides;
    std::vector<Inventory> invs;
    std::map<std::string, uintptr_t> descs;
    StatField stats[ST_COUNT];
    NameRule rule;
    std::string status = "starting";
    bool scanning = false;
};
inline State g;
inline std::mutex mx;

inline int money(uintptr_t w) { return rdv<int>(w + 0x40); }
inline bool set_money(uintptr_t w, int v) { return wr<int>(w + 0x40, v); }
inline int count(const Item& it) { return rdv<int>(it.addr + 0x40); }
inline bool set_count(const Item& it, int v) { return wr<int>(it.addr + 0x40, v); }
inline uintptr_t item_desc(const Item& it) { return rdv<uintptr_t>(it.addr + 0x60); }

inline bool has_stat(int s) { return g.stats[s].off >= 0; }
inline float get_stat(uintptr_t desc, int s) {
    auto& f = g.stats[s];
    if (f.off < 0) return NAN;
    return f.is_float ? rdv<float>(desc + f.off, NAN) : (float)rdv<int>(desc + f.off);
}
inline bool write_stat(uintptr_t desc, int s, float v) {
    auto& f = g.stats[s];
    if (f.off < 0 || !desc) return false;
    return f.is_float ? wr<float>(desc + f.off, v) : wr<int>(desc + f.off, (int)std::lround(v));
}
inline bool set_stat(uintptr_t desc, int s, float v) {
    if (!write_stat(desc, s, v)) return false;
    g.stat_overrides[{desc, s}] = v;
    return true;
}
inline void reapply_stats() {
    for (auto& [key, v] : g.stat_overrides)
        if (get_stat(key.first, key.second) != v) write_stat(key.first, key.second, v);
}

inline void calibrate_stats(const std::vector<std::pair<uintptr_t, const ItemInfo*>>& descs, StatField out[ST_COUNT]) {
    const int SPAN = 0x600;
    std::vector<std::vector<uint8_t>> bufs;
    std::vector<const ItemInfo*> infos;
    for (auto& [d, info] : descs) {
        std::vector<uint8_t> b(SPAN);
        if (!rd(d, b.data(), SPAN)) continue;
        bufs.push_back(std::move(b));
        infos.push_back(info);
    }
    for (int s = 0; s < ST_COUNT; s++) {
        std::map<std::pair<int, bool>, std::pair<int, int>> votes;
        int samples = 0;
        for (size_t k = 0; k < bufs.size(); k++) {
            float v = infos[k]->st[s];
            if (std::isnan(v)) continue;
            samples++;
            bool integral = v == std::floor(v) && std::fabs(v) < 1e9f;
            for (int off = 0; off + 4 <= SPAN; off += 4) {
                float f;
                int n;
                memcpy(&f, &bufs[k][off], 4);
                memcpy(&n, &bufs[k][off], 4);
                if (std::fabs(f - v) <= 1e-4f * std::max(1.0f, std::fabs(v))) {
                    auto& e = votes[{off, true}];
                    e.first++;
                    if (v != 0) e.second++;
                }
                if (integral && n == (int)v) {
                    auto& e = votes[{off, false}];
                    e.first++;
                    if (v != 0) e.second++;
                }
            }
        }
        StatField best;
        best.samples = samples;
        for (auto& [k, e] : votes)
            if (e.first > best.votes && e.second >= std::min(5, std::max(2, samples))) best = {k.first, k.second, e.first, samples};
        bool few = samples > 0 && samples < 8;
        if (few ? best.votes < samples || best.votes < 2 : best.votes < std::max(8, (int)(samples * 0.7))) best.off = -1;
        out[s] = best;
    }
}

inline bool resolve_classes(uintptr_t base) {
    std::lock_guard<std::mutex> l(mx);
    g.base = base;
    g.vt_money = find_vtable(base, "InventoryMoney");
    g.vt_manager = find_vtable(base, "ItemManager");
    for (int i = 0; i < N_INV_CLASSES; i++) g.vt_inv[i] = find_vtable(base, INV_CLASSES[i]);
    g.vt_player = find_vtable(base, "PlayerDI");
    g.vt_human = find_vtable(base, "HumanAI");
    const char* health_classes[] = {"HealthModule", "ArmorHealthModule", "BodyPartsHealthModule"};
    for (int i = 0; i < 3; i++) g.vt_health[i] = find_vtable(base, health_classes[i]);
    bool ok = g.vt_money && g.vt_inv[0];
    g.status = ok ? "ready" : "game classes not found (game updated?)";
    return ok;
}

inline void refresh() {
    std::vector<uintptr_t> vts;
    {
        std::lock_guard<std::mutex> l(mx);
        if (g.scanning || !g.vt_money) return;
        g.scanning = true;
        vts = {g.vt_money, g.vt_manager};
        for (auto v : g.vt_inv) vts.push_back(v);
        vts.push_back(g.vt_player);
    }
    DWORD t0 = GetTickCount();
    auto found = scan(vts);
    for (auto& s : sections(g.base))
        if (s.name == ".data" && g.vt_manager && !IsBadReadPtr((const void*)s.start, s.end - s.start))
            for (uintptr_t p = s.start; p + 8 <= s.end; p += 8)
                if (*(const uintptr_t*)p == g.vt_manager) found[1].push_back(p);
    DWORD t_scan = GetTickCount() - t0;

    std::vector<uintptr_t> wallets;
    for (auto w : found[0]) {
        int m = rdv<int>(w + 0x40, -1);
        if (m >= 0 && m < 1000000000) wallets.push_back(w);
    }

    std::vector<Inventory> invs;
    std::vector<uintptr_t> all;
    for (int c = 0; c < N_INV_CLASSES; c++)
        for (auto o : found[2 + c]) {
            uintptr_t arr = rdv<uintptr_t>(o + 0x40);
            uint32_t n = rdv<uint32_t>(o + 0x48);
            int cap = rdv<int>(o + 0x58);
            Kind k = INV_KINDS[c];
            if (k == K_BACKPACK && cap < 0) k = K_STASH;
            if (n > 4000 || ((!arr || n == 0) && k != K_STASH)) continue;
            Inventory inv{o, k, cap, {}};
            for (uint32_t i = 0; i < n; i++) {
                uintptr_t it = rdv<uintptr_t>(arr + i * 8);
                if (it < 0x10000 || rdv<int>(it + 0x40, INT32_MIN) == INT32_MIN) continue;
                inv.items.push_back({it, nullptr, ""});
                all.push_back(it);
            }
            if (!inv.items.empty() || k == K_STASH) invs.push_back(std::move(inv));
        }
    int sc = 0;
    NameRule rule = calibrate_names(all, &sc);
    for (auto& inv : invs)
        for (auto& it : inv.items) {
            it.info = resolve(it.addr, rule);
            if (it.info) it.name = display_name(it.info);
            else { char b[40]; snprintf(b, sizeof b, "Unknown item %llx", (unsigned long long)it.addr); it.name = b; }
        }
    std::stable_sort(invs.begin(), invs.end(), [](auto& a, auto& b) {
        return a.kind != b.kind ? a.kind < b.kind : a.items.size() > b.items.size();
    });

    int name_off = rule.kind == 2 ? rule.o2 : 0x18;
    std::map<std::string, uintptr_t> descs;
    std::vector<std::pair<uintptr_t, const ItemInfo*>> desc_list;
    for (auto m : found[1]) {
        uintptr_t arr = rdv<uintptr_t>(m + 0x50);
        uint32_t n = rdv<uint32_t>(m + 0x58);
        if (!arr || n < 100 || n > 20000) continue;
        for (uint32_t i = 0; i < n; i++) {
            uintptr_t d = rdv<uintptr_t>(arr + i * 8);
            if (auto info = str_at(rdv<uintptr_t>(d + name_off))) {
                if (descs.emplace(info->id, d).second) desc_list.push_back({d, info});
            }
        }
    }
    StatField stats[ST_COUNT];
    calibrate_stats(desc_list, stats);
    int calibrated = 0;
    for (auto& s : stats) calibrated += s.off >= 0;

    std::lock_guard<std::mutex> l(mx);
    g.players = found[2 + N_INV_CLASSES];
    g.wallets = wallets;
    g.invs = std::move(invs);
    g.descs = std::move(descs);
    for (int s = 0; s < ST_COUNT; s++) g.stats[s] = stats[s];
    g.rule = rule;
    char s[260];
    snprintf(s, sizeof s,
             "%zu/%zu wallets, %zu inventories, %zu items, names %d/%zu (rule %d +%x +%x), %zu item types, %d/%d stats, scan %lu ms, total %lu ms",
             wallets.size(), found[0].size(), g.invs.size(), all.size(), sc, std::min<size_t>(all.size(), 80), rule.kind,
             rule.o1, rule.o2, g.descs.size(), calibrated, (int)ST_COUNT, (unsigned long)t_scan,
             (unsigned long)(GetTickCount() - t0));
    g.status = s;
    g.scanning = false;
}

inline std::string inventory_report() {
    std::string out;
    for (auto& inv : g.invs) out += std::string(" ") + KIND_NAMES[inv.kind] + "(" + std::to_string(inv.items.size()) + "/" + std::to_string(inv.capacity) + ")";
    return out;
}

inline std::string stat_report() {
    std::string out;
    for (int s = 0; s < ST_COUNT; s++) {
        char b[96];
        snprintf(b, sizeof b, "%s=%s+%x(%d/%d) ", STAT_KEYS[s], g.stats[s].off < 0 ? "none" : g.stats[s].is_float ? "f" : "i",
                 g.stats[s].off < 0 ? 0 : g.stats[s].off, g.stats[s].votes, g.stats[s].samples);
        out += b;
    }
    return out;
}

inline Kind default_target(const char* id) {
    if (!strncmp(id, "Ammo_", 5)) return K_AMMO;
    for (const char* p : {"Craft_", "Medkit_", "LockpickItem", "misc_"})
        if (!strncmp(id, p, strlen(p))) return K_MATERIALS;
    return K_BACKPACK;
}

inline const Inventory* find_inventory(Kind k) {
    for (auto& inv : g.invs)
        if (inv.kind == k) return &inv;
    return nullptr;
}

inline uintptr_t pick_template(const Inventory* target) {
    auto plain = [](const Inventory& inv) -> uintptr_t {
        for (auto& it : inv.items)
            if (it.info && !rdv<uintptr_t>(it.addr + 0xA0)) return it.addr;
        return 0;
    };
    if (target)
        if (uintptr_t t = plain(*target)) return t;
    for (auto& inv : g.invs)
        if (uintptr_t t = plain(inv)) return t;
    return 0;
}

using AddItemFn =void(__fastcall*)(uintptr_t inv, void* data, int reason, bool flag);

inline int count_of(uintptr_t inv, uintptr_t desc) {
    uintptr_t arr = rdv<uintptr_t>(inv + 0x40);
    uint32_t n = rdv<uint32_t>(inv + 0x48);
    int total = 0;
    for (uint32_t i = 0; i < n && i < 4000; i++) {
        uintptr_t it = rdv<uintptr_t>(arr + i * 8);
        if (rdv<uintptr_t>(it + 0x60) == desc) total += std::max(1, rdv<int>(it + 0x40));
    }
    return total + (int)n * 1000000;
}

inline bool give(uintptr_t inv, uintptr_t tmpl_item, uintptr_t desc, int amount) {
    uint8_t data[0xC0];
    if (!rd(tmpl_item + 0x40, data, sizeof data) || !desc) return false;
    memcpy(data + 0x00, &amount, 4);
    memcpy(data + 0x20, &desc, 8);
    uintptr_t none = 0;
    memcpy(data + 0x60, &none, 8);
    auto fn = (AddItemFn)rdv<uintptr_t>(rdv<uintptr_t>(inv) + 6 * 8);
    if (!fn) return false;
    int before = count_of(inv, desc);
    fn(inv, data, 0, false);
    if (count_of(inv, desc) != before) return true;
    fn(inv, data, 0, true);
    return count_of(inv, desc) != before;
}

inline std::vector<Kind> give_order(Kind first) {
    std::vector<Kind> order = {first};
    for (Kind k : {K_MATERIALS, K_BACKPACK, K_AMMO, K_TOOLS, K_COLLECTABLES, K_STASH})
        if (k != first) order.push_back(k);
    return order;
}

inline int give_anywhere(Kind first, uintptr_t desc, int amount) {
    for (Kind k : give_order(first)) {
        uintptr_t inv, tmpl;
        {
            std::lock_guard<std::mutex> l(mx);
            auto* i = find_inventory(k);
            if (!i) continue;
            inv = i->obj;
            tmpl = pick_template(i);
        }
        if (tmpl && give(inv, tmpl, desc, amount)) return k;
    }
    return -1;
}

}
