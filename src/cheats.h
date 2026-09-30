#pragma once
#include <atomic>
#include <set>
#include "game.h"

namespace cheats {

using game::g;
using game::rd;
using game::rdv;
using game::wr;

const int LOCAL_PLAYER = 0x780, PARAM_CONTAINER = 0xe58, PARAM_TABLE = 0xd0, PARAM_VALUE = 8;
const int HEALTH_OBJECT = 0x8f8, STAMINA_OBJECTS[] = {0x1340, 0x1348};
const int STAMINA_CURRENT = 0x10, STAMINA_FULL = 0x14;
const int MODULE_OWNER = 0x40, MODULE_HEALTH = 0x78;
const int SLOT_IS_IMMORTAL = 3, SLOT_REFILL = 5, SLOT_HEALTH = 199;
const int COPIED_SLOTS = 64;
const float ONE_HIT_HEALTH = 1.0f;

struct Cheat {
    const char* key;
    const char* label;
    const char* hint;
    std::vector<std::pair<const char*, float>> numbers;
    std::vector<const char*> switches;
    std::atomic<bool> on{false};
};

inline const char* SPIT_REGEN[] = {"ZombieSpitControlTheHordeAmmoRegenTime", "ZombieSpitLightDisableAmmoRegenTime",
                                   "ZombieSpitDefensiveSmokeAmmoRegenTime"};

inline Cheat CHEATS[] = {
    {"god", "God mode", "You take no damage from anything.", {}, {}},
    {"stamina", "Infinite stamina", "Sprint, climb and fight without getting tired.", {}, {"InfiniteStamina"}},
    {"hook", "Infinite grappling hook", "No cooldown between grapples, usable in the air.", {{"GrapplingHookCooldown", 0}},
     {"CanUseHook", "ThrowHookCanUseInAir"}},
    {"uv", "Infinite UV flashlight", "The UV light never drains.", {{"FlashlightDrainMul", 0}, {"FlashlightRechargeSpeed", 1000}}, {}},
    {"one_hit", "One hit kill", "Zombies and humans drop to 1 health. Works when you are the host.", {}, {}},
    {"ammo", "Infinite ammo", "Magazines never empty and reserve ammo stays full.", {}, {}},
    {"supplies", "Infinite consumables", "Medkits, throwables and crafting materials never run out.", {}, {}},
    {"z_energy", "Infinite hunter energy", "Fitness and stamina never drain, UV light cannot exhaust you.", {},
     {"InfiniteFitness", "InfiniteStamina"}},
    {"z_cooldowns", "No ability cooldowns", "Tendril, camouflage, ground pound and grab breaks are always ready.",
     {{"TDCooldown", 0}, {"CamouflageCooldown", 0}, {"ZombieGroundPoundCooldown", 0}, {"ChargeLightCooldown", 0},
      {"FastGrabBreakCooldown", 0}},
     {}},
    {"z_spits", "Infinite spits", "Every spit type recharges instantly.", {}, {}},
    {"z_unlock", "Unlock all hunter abilities", "Pounce slam, every spit type, camouflage and UV heal without leveling up.", {},
     {"ZombiePounceEnabled", "ZombiePounceSlamEnabled", "ZombieSpitLightDisableEnabled", "ZombieSpitLightDisableUpgraded",
      "ZombieSpitGroundPoundEnabled", "ZombieSpitControlTheHordeEnabled", "ZombieSpitControlTheHordeUpgraded",
      "ZombieSpitChargingEnabled", "ZombieSpitCamoEnabled", "ZombieSpitToxicEnabled", "UVHealEnabled"}},
    {"z_camo", "Long camouflage", "Camouflage lasts ten minutes and you can run and attack while hidden.",
     {{"CamouflageDuration", 600}}, {"CamouflageEnabled", "CamouflageCanRun", "CamouflageCanAttack"}},
};

inline Cheat* find(const std::string& key) {
    for (auto& c : CHEATS)
        if (key == c.key) return &c;
    return nullptr;
}
inline bool is_on(const char* key) { return find(key) && find(key)->on; }

inline uintptr_t player = 0;
inline uintptr_t local_player_root = 0, params_root = 0, unlimited_ammo_flag = 0;
inline std::map<std::string, int> param_ids;
inline uintptr_t immortal_vtable[COPIED_SLOTS + 1];
inline uintptr_t original_vtable = 0;
inline float best_stamina[2] = {};
inline std::mutex modules_mx;
inline std::vector<uintptr_t> enemy_modules;
inline std::map<uintptr_t, int> stack_floor;
inline std::map<uintptr_t, uint32_t> saved_values;
inline std::vector<uintptr_t> saved_containers;

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

inline void read_param_names() {
    auto secs = game::sections(g.base);
    auto* text = game::section_named(secs, ".text");
    auto* rdata = game::section_named(secs, ".rdata");
    if (!text || !rdata) return;
    const char needle[] = "\0MaxStamina";
    auto hit = std::search((const char*)rdata->start, (const char*)rdata->end, needle, needle + sizeof needle);
    if (hit == (const char*)rdata->end) return;
    uintptr_t name = (uintptr_t)hit + 1;
    auto jump_target = [&](uintptr_t c) -> uintptr_t {
        const uint8_t* b = (const uint8_t*)c;
        if (b[7] == 0xE9) return game::rip_target(c + 7, 1, 5);
        if (b[7] == 0xEB) return c + 9 + (int8_t)b[8];
        return 0;
    };
    auto is_case = [&](uintptr_t c, uintptr_t common) {
        const uint8_t* b = (const uint8_t*)c;
        return c >= text->start && c + 12 <= text->end && b[0] == 0x48 && b[1] == 0x8D && b[2] == 0x15 && jump_target(c) &&
               (!common || jump_target(c) == common);
    };
    uintptr_t case_at = 0, common = 0;
    for (uintptr_t p = text->start; p + 12 <= text->end && !case_at; p++)
        if (is_case(p, 0) && game::rip_target(p, 3, 7) == name) case_at = p, common = jump_target(p);
    if (!case_at) return;
    uint32_t rva = (uint32_t)(case_at - g.base);
    uintptr_t at = 0;
    for (uintptr_t p = text->start & ~(uintptr_t)3; p + 4 <= text->end && !at; p += 4)
        if (*(const uint32_t*)p == rva) at = p;
    if (!at) return;
    auto entry = [&](uintptr_t p) { return g.base + *(const uint32_t*)p; };
    auto in_table = [&](uintptr_t p) { return p >= text->start && p + 4 <= text->end && entry(p) + 0x10000 > common && entry(p) < common + 0x10000; };
    uintptr_t start = at;
    while (in_table(start - 4)) start -= 4;
    for (uintptr_t p = start; in_table(p); p += 4)
        if (is_case(entry(p), common)) param_ids[(const char*)game::rip_target(entry(p), 3, 7)] = (int)((p - start) / 4);
}

inline void locate(uintptr_t base) {
    if (auto hits = game::find_code(base, "48 8B 05 ? ? ? ? 48 85 C0 74 ? 48 8B 88 80 07 00 00"); !hits.empty())
        local_player_root = game::rip_target(hits[0], 3, 7);
    if (auto hits = game::find_code(base, "48 8B 05 ? ? ? ? 74 07 48 8B ? 58 0E 00 00"); !hits.empty())
        params_root = game::rip_target(hits[0], 3, 7);
    if (auto hits = game::find_code(base, "E8 ? ? ? ? 40 38 B8 B1 15 00 00"); !hits.empty()) {
        uintptr_t getter = game::rip_target(hits[0], 1, 5);
        for (uintptr_t p = getter; p < getter + 0x60; p++)
            if (!memcmp((const void*)p, "\x48\x8D\x05", 3) && !memcmp((const void*)(p + 7), "\x48\x83\xC4\x28\xC3", 5)) {
                unlimited_ammo_flag = game::rip_target(p, 3, 7) + rdv<int32_t>(hits[0] + 8);
                break;
            }
    }
    read_param_names();
}

inline bool alive(uintptr_t p) { return p && g.vt_player && rdv<uintptr_t>(p) == g.vt_player; }

inline uintptr_t find_player() {
    uintptr_t local = local_player_root ? rdv<uintptr_t>(rdv<uintptr_t>(local_player_root) + LOCAL_PLAYER) : 0;
    if (alive(local)) return local;
    if (alive(player)) return player;
    for (uintptr_t p : g.players)
        if (alive(p) && in_game_module(rdv<uintptr_t>(p + HEALTH_OBJECT))) return p;
    return 0;
}

inline float health() {
    int off = float_getter_offset(slot(player, SLOT_HEALTH));
    return alive(player) && off > 0 ? rdv<float>(player + off, NAN) : NAN;
}

inline uintptr_t stamina_object(int i) { return alive(player) ? rdv<uintptr_t>(player + STAMINA_OBJECTS[i]) : 0; }
inline float stamina() { return rdv<float>(stamina_object(0) + STAMINA_CURRENT, NAN); }
inline float stamina_full() { return rdv<float>(stamina_object(0) + STAMINA_FULL, NAN); }

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

inline std::vector<uintptr_t> param_containers() {
    std::vector<uintptr_t> out;
    for (uintptr_t c : {alive(player) ? rdv<uintptr_t>(player + PARAM_CONTAINER) : 0, rdv<uintptr_t>(params_root)})
        if (c && std::find(out.begin(), out.end(), c) == out.end()) out.push_back(c);
    return out;
}

inline uintptr_t param_value(uintptr_t container, const std::string& name) {
    auto it = param_ids.find(name);
    if (it == param_ids.end()) return 0;
    uintptr_t table = rdv<uintptr_t>(rdv<uintptr_t>(container + PARAM_TABLE));
    uintptr_t param = rdv<uintptr_t>(table + (it->second + 1) * 8);
    return param ? param + PARAM_VALUE : 0;
}

inline void override_value(uintptr_t at, const void* v, size_t n, std::set<uintptr_t>& touched) {
    if (!at) return;
    touched.insert(at);
    if (!saved_values.count(at)) saved_values[at] = rdv<uint32_t>(at);
    if (!IsBadWritePtr((void*)at, n)) memcpy((void*)at, v, n);
}

inline void apply_overrides() {
    auto containers = param_containers();
    if (containers != saved_containers) saved_values.clear(), saved_containers = containers;
    std::set<uintptr_t> touched;
    for (auto& c : CHEATS) {
        if (!c.on) continue;
        std::vector<std::pair<std::string, float>> numbers(c.numbers.begin(), c.numbers.end());
        if (!strcmp(c.key, "z_spits"))
            for (const char* base : SPIT_REGEN)
                for (int players = 1; players <= 4; players++) numbers.push_back({base + std::to_string(players) + "v1", 0.05f});
        for (uintptr_t container : containers) {
            for (auto& [name, v] : numbers) override_value(param_value(container, name), &v, 4, touched);
            for (const char* name : c.switches) {
                uint8_t yes = 1;
                override_value(param_value(container, name), &yes, 1, touched);
            }
        }
    }
    if (is_on("ammo") && unlimited_ammo_flag) {
        uint8_t yes = 1;
        override_value(unlimited_ammo_flag, &yes, 1, touched);
    }
    for (auto it = saved_values.begin(); it != saved_values.end();) {
        if (touched.count(it->first)) { ++it; continue; }
        wr<uint32_t>(it->first, it->second);
        it = saved_values.erase(it);
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

inline bool is_ammo(const game::Item& it) { return it.info && strstr(it.info->id, "Ammo"); }

inline void keep_stacks(bool ammo, bool supplies) {
    if (!ammo && !supplies) return stack_floor.clear();
    for (auto& inv : g.invs) {
        if (inv.kind == game::K_STASH) continue;
        uintptr_t arr = rdv<uintptr_t>(inv.obj + 0x40);
        uint32_t n = rdv<uint32_t>(inv.obj + 0x48);
        std::set<uintptr_t> present;
        for (uint32_t i = 0; i < n && i < 4000; i++) present.insert(rdv<uintptr_t>(arr + i * 8));
        for (auto& it : inv.items) {
            bool wanted = is_ammo(it) ? ammo : supplies;
            if (!wanted || !it.info || !(it.info->st[ST_MaxStackCount] > 1) || !present.count(it.addr)) continue;
            int cur = game::count(it);
            auto f = stack_floor.find(it.addr);
            if (f == stack_floor.end()) f = stack_floor.emplace(it.addr, std::max(cur, 2)).first;
            else if (cur > f->second) f->second = cur;
            if (cur < f->second) game::set_count(it, f->second);
        }
    }
}

inline void tick() {
    std::lock_guard<std::mutex> l(game::mx);
    game::reapply_stats();
    keep_stacks(is_on("ammo"), is_on("supplies"));
    if (is_on("one_hit")) weaken_enemies();
    player = find_player();
    apply_overrides();
    if (!player) return;
    set_immortal(is_on("god"));
    if (is_on("stamina") || is_on("z_energy")) keep_stamina();
    else best_stamina[0] = best_stamina[1] = 0;
}

inline std::string describe() {
    char b[256];
    snprintf(b, sizeof b,
             "player %s (%zu scanned, local root %s), health %.0f, stamina %.0f, params %zu, ammo flag %s, enemies %zu",
             player ? "ok" : "missing", g.players.size(), local_player_root ? "ok" : "missing", health(), stamina(),
             param_ids.size(), unlimited_ammo_flag ? "ok" : "missing", enemy_modules.size());
    return b;
}

}
