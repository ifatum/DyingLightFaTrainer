#pragma once
#include <atomic>
#include <set>
#include "game.h"

namespace cheats {

using game::g;
using game::rd;
using game::rdv;
using game::wr;

const int HEALTH_OBJECT = 0x8f8, STAMINA_OBJECTS[] = {0x1340, 0x1348};
const int STAMINA_CURRENT = 0x10, STAMINA_FULL = 0x14;
const int MODULE_OWNER = 0x40, MODULE_HEALTH = 0x78;
const int SLOT_IS_IMMORTAL = 3, SLOT_REFILL = 5, SLOT_HEALTH = 199;
const int COPIED_SLOTS = 64;
const float ONE_HIT_HEALTH = 1.0f;

struct Toggles {
    std::atomic<bool> god{false}, stamina{false}, one_hit{false}, supplies{false};
};
inline Toggles on;

inline uintptr_t player = 0;
inline uintptr_t immortal_vtable[COPIED_SLOTS + 1];
inline uintptr_t original_vtable = 0;
inline float best_stamina[2] = {};
inline std::mutex modules_mx;
inline std::vector<uintptr_t> enemy_modules;
inline std::map<uintptr_t, int> supply_floor;

inline bool in_game_module(uintptr_t p) {
    for (auto& s : game::sections(g.base))
        if (p >= s.start && p < s.end) return true;
    return false;
}

inline uintptr_t slot(uintptr_t object, int index) { return rdv<uintptr_t>(rdv<uintptr_t>(object) + index * 8); }

inline int float_getter_offset(uintptr_t fn) {
    uint8_t b[9];
    if (!rd(fn, b, sizeof b) || b[0] != 0xF3 || b[1] != 0x0F || b[2] != 0x10 || b[3] != 0x81 || b[8] != 0xC3) return -1;
    int32_t off;
    memcpy(&off, b + 4, 4);
    return off;
}

inline bool alive(uintptr_t p) { return p && g.vt_player && rdv<uintptr_t>(p) == g.vt_player; }

inline uintptr_t find_player() {
    if (alive(player)) return player;
    for (uintptr_t p : g.players)
        if (alive(p) && in_game_module(rdv<uintptr_t>(p + HEALTH_OBJECT))) return p;
    return 0;
}

inline float health() {
    int off = float_getter_offset(slot(player, SLOT_HEALTH));
    return alive(player) && off > 0 ? rdv<float>(player + off, NAN) : NAN;
}

inline float stamina() {
    uintptr_t s = alive(player) ? rdv<uintptr_t>(player + STAMINA_OBJECTS[0]) : 0;
    return s ? rdv<float>(s + STAMINA_CURRENT, NAN) : NAN;
}

inline bool __fastcall always_immortal(uintptr_t) { return true; }

inline void set_immortal(bool enable) {
    uintptr_t health_object = player + HEALTH_OBJECT;
    uintptr_t current = rdv<uintptr_t>(health_object);
    uintptr_t ours = (uintptr_t)&immortal_vtable[1];
    if (enable && current != ours && in_game_module(current)) {
        if (!rd(current - 8, immortal_vtable, sizeof immortal_vtable)) return;
        immortal_vtable[1 + SLOT_IS_IMMORTAL] = (uintptr_t)&always_immortal;
        original_vtable = current;
        wr<uintptr_t>(health_object, ours);
    } else if (!enable && current == ours && original_vtable) {
        wr<uintptr_t>(health_object, original_vtable);
    }
}

inline void refill() {
    if (!alive(player)) return;
    uintptr_t health_object = player + HEALTH_OBJECT;
    auto fn = (void(__fastcall*)(uintptr_t))slot(health_object, SLOT_REFILL);
    if (in_game_module((uintptr_t)fn)) fn(health_object);
}

inline void keep_stamina() {
    for (int i = 0; i < 2; i++) {
        uintptr_t s = rdv<uintptr_t>(player + STAMINA_OBJECTS[i]);
        float cur = rdv<float>(s + STAMINA_CURRENT, NAN), full = rdv<float>(s + STAMINA_FULL, NAN);
        if (!s || !(cur >= 0 && cur < 100000) || !(full >= 0 && full < 100000)) continue;
        best_stamina[i] = std::max({best_stamina[i], cur, full});
        if (cur < best_stamina[i]) wr<float>(s + STAMINA_CURRENT, best_stamina[i]);
    }
}

inline bool is_health_module(uintptr_t m) {
    uintptr_t vt = rdv<uintptr_t>(m);
    return vt && (vt == g.vt_health[0] || vt == g.vt_health[1] || vt == g.vt_health[2]);
}

inline bool is_enemy_module(uintptr_t m) {
    uintptr_t owner = rdv<uintptr_t>(m + MODULE_OWNER);
    return is_health_module(m) && owner && owner != player && g.vt_human &&
           slot(owner, SLOT_HEALTH) == rdv<uintptr_t>(g.vt_human + SLOT_HEALTH * 8);
}

inline void scan_enemies() {
    std::vector<uintptr_t> vts(std::begin(g.vt_health), std::end(g.vt_health)), found;
    for (auto& list : game::scan(vts))
        for (uintptr_t m : list)
            if (is_enemy_module(m)) found.push_back(m);
    std::lock_guard<std::mutex> l(modules_mx);
    enemy_modules.swap(found);
}

inline void weaken_enemies() {
    std::lock_guard<std::mutex> l(modules_mx);
    for (uintptr_t m : enemy_modules) {
        float cur = rdv<float>(m + MODULE_HEALTH, NAN);
        if (is_enemy_module(m) && cur > ONE_HIT_HEALTH && cur < 1e7f) wr<float>(m + MODULE_HEALTH, ONE_HIT_HEALTH);
    }
}

inline bool stackable(const game::Item& it) { return it.info && it.info->st[ST_MaxStackCount] > 1; }

inline void keep_supplies() {
    for (auto& inv : g.invs) {
        if (inv.kind == game::K_STASH) continue;
        uintptr_t arr = rdv<uintptr_t>(inv.obj + 0x40);
        uint32_t n = rdv<uint32_t>(inv.obj + 0x48);
        std::set<uintptr_t> present;
        for (uint32_t i = 0; i < n && i < 4000; i++) present.insert(rdv<uintptr_t>(arr + i * 8));
        for (auto& it : inv.items) {
            if (!stackable(it) || !present.count(it.addr)) continue;
            int cur = game::count(it);
            auto f = supply_floor.find(it.addr);
            if (f == supply_floor.end()) supply_floor[it.addr] = std::max(cur, 2);
            else if (cur > f->second) f->second = cur;
            if (cur < supply_floor[it.addr]) game::set_count(it, supply_floor[it.addr]);
        }
    }
}

inline void tick() {
    std::lock_guard<std::mutex> l(game::mx);
    game::reapply_stats();
    if (!on.supplies) supply_floor.clear();
    else keep_supplies();
    if (on.one_hit) weaken_enemies();
    player = find_player();
    if (!player) return;
    set_immortal(on.god);
    if (on.stamina) keep_stamina();
    else best_stamina[0] = best_stamina[1] = 0;
}

inline std::string describe() {
    char b[160];
    snprintf(b, sizeof b, "player %s (%zu found), health %.0f, stamina %.0f, enemies tracked %zu", player ? "ok" : "missing",
             g.players.size(), health(), stamina(), enemy_modules.size());
    return b;
}

}
