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
const int STAMINA_CURRENT = 0x10, STAMINA_FULL = 0x14, HEALTH_VALUE = 0x964, SLOT_SET_HEALTH = 4;
const int ROPE_ENERGY = 0x40, ROPE_DEPLETED = 0x44;
const int REPL_OWNED = 0x28;
const int PARAM_SLOTS = 1100, CACHED_VALUE = 8, CACHED_FLAGS = 0x18, CACHED_VERSION = -8;
const int SKILL_TREES = 0x40, TREE_RECORD = 0x20, TREE_XP = 8, TREE_LEVEL_START = 0xc, TREE_SPAN = 0x10, TREE_LEVEL = 0x14, TREE_MAX = 0x16;
const int MODULE_OWNER = 0x40, MODULE_HEALTH = 0x78;
const int SLOT_IS_IMMORTAL = 3, SLOT_REFILL = 5, SLOT_HEALTH = 199, SLOT_MAX_HEALTH = 41, SLOT_MODULE_UPDATE = 245;
const int UV_CHARGE = 0x50, UV_EXHAUSTED = 0x55;
const int PRISON_START_TIME = 0x44, PRISON_END_TIME = 0x48, PRISON_REWARD_TIER = 0x4c, PRISON_STATE = 0x54;
const int SLOT_FLOAT_FIELD_EDITOR = 34;
const int COPIED_SLOTS = 64, SLOT_VAR_FLOAT = 114, VAR_SLOTS = 1024;
const float ONE_HIT_HEALTH = 1.0f;
inline const uint8_t SPOT_DISTANCE_CLAMP[4] = {0xF3, 0x0F, 0x5F, 0xD3}, SPOT_DISTANCE_ZERO[4] = {0x0F, 0x57, 0xD2, 0x90};

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
    {"hook", "Infinite grappling hook", "The hook recharges instantly, so you can grapple again right away, also in the air.",
     {{"RopeEnergyRegenTime", 0.01f}}, {"CanUseHook", "ThrowHookCanUseInAir"}},
    {"uv", "Infinite UV flashlight", "The UV light never drains.", {{"FlashlightDrainMul", 0}, {"FlashlightRechargeSpeed", 1000}}, {}},
    {"one_hit", "One hit kill", "Zombies and humans drop to 1 health. Works when you are the host.", {}, {}},
    {"ammo", "Infinite ammo", "Magazines never empty and reserve ammo stays full.", {}, {}},
    {"supplies", "Infinite consumables", "Medkits, throwables and crafting materials never run out.", {}, {}},
    {"lockpick", "Instant lockpicking", "Every pick position opens the lock and lockpicks never break.", {}, {}},
    {"durability", "Unbreakable weapons", "Melee weapons stop losing durability when you hit things.",
     {{"BluntWpnDurabilityLoss", 0}, {"CutWpnDurabilityLoss", 0}}, {}},
    {"no_fall", "No fall damage", "Land safely from any height.", {{"FallDamageReduction", 1}, {"FallHeightMedium", 9999}, {"FallHeightHigh", 9999}},
     {"FallDampingEnabled", "AutomaticFallDamping"}},
    {"no_reload", "No reload", "999 round magazines and instant reloads. Best together with infinite ammo.",
     {{"FirearmsPistolReloadTimeMul", 0.05f}, {"FirearmsRevolverReloadTimeMul", 0.05f}, {"FirearmsRifleReloadTimeMul", 0.05f},
      {"FirearmsShotgunReloadTimeMul", 0.05f}, {"FirearmsHeavyReloadTimeMul", 0.05f}},
     {"FastRevolverReload"}},
    {"z_energy", "Infinite hunter energy", "Fitness and stamina never drain, UV light cannot exhaust you.", {},
     {"InfiniteFitness", "InfiniteStamina"}},
    {"z_cooldowns", "No ability cooldowns", "Tendril, camouflage, ground pound and grab breaks are always ready.",
     {{"TDCooldown", 0}, {"CamouflageCooldown", 0}, {"ZombieGroundPoundCooldown", 0}, {"ChargeLightCooldown", 0},
      {"FastGrabBreakCooldown", 0}},
     {}},
    {"z_spits", "Infinite spits", "Every spit type recharges instantly.", {}, {}},
    {"prison_pause", "Pause prison timers", "The run timer and the reward room countdown stand still. Works when you are the host.", {}, {}},
    {"z_camo", "Long camouflage", "Camouflage lasts ten minutes and you can run and attack while hidden.",
     {{"CamouflageDuration", 600}}, {"CamouflageEnabled", "CamouflageCanRun", "CamouflageCanAttack"}},
};

enum Group { G_MOVEMENT, G_PROGRESS, G_ZOMBIE, G_HUMAN };

struct ScriptVar { const char* name; float at_max; };

struct Tweak {
    const char* key;
    const char* label;
    const char* hint;
    Group group;
    std::vector<const char*> params;
    std::vector<ScriptVar> vars = {};
    float max = 10.0f;
    std::atomic<float> factor{1.0f};
};

inline Tweak TWEAKS[] = {
    {"speed", "Movement speed", "Walk, sprint and wall run faster.", G_MOVEMENT,
     {"MoveSprintSpeed", "MoveForwardMaxSpeed", "MoveStrafeMaxSpeed", "MoveBackwardMaxSpeed", "WallrunSpeed"}, {}, 3.0f},
    {"jump", "Jump height", "Jump higher.", G_MOVEMENT, {"JumpMaxHeight", "JumpMinHeight"}, {}, 4.0f},
    {"xp", "XP gain", "Agility, Power and Driver experience.", G_PROGRESS, {"RunnerXPFactor", "FighterXPFactor", "DriverXPFactor"}},
    {"z_pounce", "Pounce", "At Max you pounce survivors 40 m away, even when they are not in front of you. Also grows the pounce slam blast.",
     G_ZOMBIE, {"ZombiePounceHighRageExplosionRange"},
     {{"f_btz_zombie_grab_range", 40}, {"f_btz_zombie_grab_range_velocity_factor", 1}, {"f_btz_zombie_grab_angle_max", 180},
      {"f_btz_pvp_grab_above_angle_threshold", 90}, {"f_btz_pvp_grab_below_angle_threshold", -90}}},
    {"z_pound", "Ground pound", "Reach of the ground pound and the aerial ground pound.", G_ZOMBIE, {"ZombieGroundPoundRange", "GroundPoundRangeMul"}},
    {"z_tackle", "Tackle", "How far away the charge tackle still connects.", G_ZOMBIE, {"ZombieChargeAttackRange"}},
    {"z_claws", "Claws", "Reach of your claw swipes.", G_ZOMBIE, {"RangeMeleeMul", "BestTargetMeleeRange"}},
    {"z_spit", "Spit", "Spits fly faster and further.", G_ZOMBIE, {"ZombieSpitControlTheHordeVelocityMul", "ZombieSpitLightDisableVelocityMul"}},
    {"h_dfa", "Death from above range", "How far away the hunter can be when you start it. At Max, 12 m. Also grows the landing shockwave.",
     G_HUMAN, {"JumpAttackRange", "JumpAttackShockwaveRadius"}, {{"f_btz_jump_attack_range", 12}, {"f_btz_jump_attack_range_velocity_factor", 0.5f}}},
    {"h_dfa_pull", "Death from above pull", "How far off target you can start it; the attack pulls you onto the hunter. At Max he can be beside or behind you.",
     G_HUMAN, {}, {{"f_btz_jump_attack_angle_max", 180}, {"f_btz_pvp_grab_above_angle_threshold", 90}, {"f_btz_pvp_grab_below_angle_threshold", -90}}},
    {"h_dropkick", "Dropkick", "At Max you dropkick the hunter from 12 m away, even when he is not in front of you.", G_HUMAN,
     {"AirKickRangeMul"}, {{"f_btz_wrestling_kick_range", 12}, {"f_btz_wrestling_kick_range_velocity_factor", 0.5f}, {"f_btz_wrestling_kick_angle_max", 180}}},
    {"h_kicks", "Other kicks & ground pound", "Wrestling kick and ground pound reach.", G_HUMAN, {"WrestlingKickRangeMul", "GroundPoundRangeMul"}},
    {"h_melee", "Melee", "Reach of melee attacks, how far the game looks for a target and how far above or below your aim it still locks on.",
     G_HUMAN, {"RangeMeleeMul", "BestTargetMeleeRange", "MaxVerticalAngleForRangeMeleeCorrection"}},
};

inline Tweak* find_tweak(const std::string& key) {
    for (auto& t : TWEAKS)
        if (key == t.key) return &t;
    return nullptr;
}

struct Tree { int type; const char* name; };
inline const Tree TREES[] = {{3, "Survivor"}, {1, "Agility"}, {2, "Power"}, {5, "Legend"}, {6, "Driver"}, {7, "Hellraid"}, {4, "Reputation"}, {0, "Other"}};

inline Cheat* find(const std::string& key) {
    for (auto& c : CHEATS)
        if (key == c.key) return &c;
    return nullptr;
}
inline bool is_on(const char* key) { return find(key) && find(key)->on; }

inline uintptr_t player = 0;
inline uintptr_t local_player_root = 0, params_root = 0, unlimited_ammo_flag = 0, set_level_fn = 0, level_from_xp_fn = 0, cache_get_fn = 0, lockpick_patch = 0;
inline uintptr_t vt_param_float = 0, vt_param_bool = 0, var_root = 0;
using VarFloatFn = float (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);
inline VarFloatFn original_var_float = nullptr;
inline uintptr_t var_vtable[VAR_SLOTS + 1];
inline std::map<std::string, int> param_ids;
inline uintptr_t immortal_vtable[COPIED_SLOTS + 1];
inline uintptr_t original_vtable = 0;
inline float best_stamina[2] = {};
inline std::mutex modules_mx;
inline std::vector<uintptr_t> enemy_modules;
inline std::map<uintptr_t, int> stack_floor;
struct Saved { std::vector<uint8_t> original, written; };
inline std::map<uintptr_t, Saved> saved_bytes;
using CacheGetFn = uintptr_t (*)(uintptr_t, int);
inline CacheGetFn cache_get_original = nullptr;
struct CachedParam { std::atomic<uint8_t> kind{0}; std::atomic<bool> stale{false}; std::atomic<float> value{0}; std::atomic<uint32_t> reads{0}; };
enum { CACHED_NONE, CACHED_FLOAT, CACHED_SWITCH };
inline CachedParam cached[PARAM_SLOTS];
inline std::mutex cached_mx;
inline std::map<int, std::set<uintptr_t>> overridden_providers;
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
    if (auto hits = game::find_code(base, "48 89 5C 24 18 48 89 74 24 20 57 48 83 EC 20 49 63 F0 48 8B D9 48 8B 49 40 48 8B FE 48 C1 E7 05 44 0F BF C2");
        !hits.empty())
        set_level_fn = hits[0];
    if (auto hits = game::find_code(base, "40 55 56 41 57 48 83 EC 50 48 8B 41 40 48 8B F1 4C 63 FA 49 8B EF 48 C1 E5 05 66 83 7C 28 14 00");
        !hits.empty())
        level_from_xp_fn = hits[0];
    if (auto hits = game::find_code(base, "8B ? C0 09 00 00 BA ? ? ? ? E8"); !hits.empty()) cache_get_fn = game::rip_target(hits[0] + 11, 1, 5);
    const int CLAMP_AT = 0x2e;
    auto spot = game::find_code(base, "F3 0F 10 56 50 B1 01 F3 0F 5C 90 18 01 00 00 F3 0F 10 4E 54 F3 0F 59 0D ? ? ? ? 0F 54 15 ? ? ? ? 0F 54 0D ? ? ? ? F3 0F 5C D1");
    if (!spot.empty() && !memcmp((const void*)(spot[0] + CLAMP_AT), SPOT_DISTANCE_CLAMP, 4)) lockpick_patch = spot[0] + CLAMP_AT;
    if (auto hits = game::find_code(base, "48 8B 05 ? ? ? ? 48 8B 0D ? ? ? ? 48 8B 18 48 8B 01 FF 90 90 01 00 00 48 8B 0D ? ? ? ? 48 8D 55 ? 4C 8B C0 FF 93 90 03 00 00");
        !hits.empty())
        var_root = game::rip_target(hits[0], 3, 7);
    vt_param_float = game::find_vtable(base, "?$Param@M");
    vt_param_bool = game::find_vtable(base, "?$Param@_N");
    read_param_names();
}

inline bool alive(uintptr_t p) { return p && g.vt_player && rdv<uintptr_t>(p) == g.vt_player; }

inline void no_log(const char*, ...) {}
inline void (*logf_hook)(const char*, ...) = no_log;

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
using SetHealthFn = void(__fastcall*)(uintptr_t, float, bool);
inline SetHealthFn original_set_health = nullptr;
inline void __fastcall set_health_without_damage(uintptr_t health_object, float value, bool notify) {
    if (value < rdv<float>(health_object + HEALTH_VALUE, NAN)) return;
    original_set_health(health_object, value, notify);
}

inline void set_immortal(bool enable) {
    uintptr_t health_object = player + HEALTH_OBJECT;
    uintptr_t current = rdv<uintptr_t>(health_object);
    uintptr_t ours = (uintptr_t)&immortal_vtable[1];
    if (enable && current != ours && in_game_module(current)) {
        if (!rd(current - 8, immortal_vtable, sizeof immortal_vtable)) return;
        immortal_vtable[1 + SLOT_IS_IMMORTAL] = (uintptr_t)&always_immortal;
        original_set_health = (SetHealthFn)immortal_vtable[1 + SLOT_SET_HEALTH];
        immortal_vtable[1 + SLOT_SET_HEALTH] = (uintptr_t)&set_health_without_damage;
        original_vtable = current;
        wr<uintptr_t>(health_object, ours);
    } else if (!enable && current == ours && original_vtable) {
        wr<uintptr_t>(health_object, original_vtable);
    }
}

inline float max_health() {
    auto fn = (float(__fastcall*)(uintptr_t, int))slot(player, SLOT_MAX_HEALTH);
    return alive(player) && in_game_module((uintptr_t)fn) ? fn(player, -1) : NAN;
}

inline void refill() {
    if (!alive(player)) return;
    float before = health();
    uintptr_t health_object = player + HEALTH_OBJECT;
    auto fn = (void(__fastcall*)(uintptr_t))slot(health_object, SLOT_REFILL);
    if (in_game_module((uintptr_t)fn)) fn(health_object);
    int off = float_getter_offset(slot(player, SLOT_HEALTH));
    float full = max_health();
    if (off > 0 && full > 0 && full < 100000 && !(health() >= full)) wr<float>(player + off, full);
    for (int i = 0; i < 2; i++) {
        uintptr_t s = rdv<uintptr_t>(player + STAMINA_OBJECTS[i]);
        float top = rdv<float>(s + STAMINA_FULL, NAN);
        if (s && top > 0 && top < 100000) wr<float>(s + STAMINA_CURRENT, top);
    }
    logf_hook("refill: health %.0f -> %.0f (max %.0f), stamina %.0f", before, health(), full, stamina());
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

inline void put_bytes(uintptr_t at, const void* v, size_t n, std::set<uintptr_t>& touched) {
    if (!at || IsBadWritePtr((void*)at, n)) return;
    auto it = saved_bytes.find(at);
    if (it == saved_bytes.end()) {
        std::vector<uint8_t> b(n);
        if (!rd(at, b.data(), n)) return;
        it = saved_bytes.emplace(at, Saved{b, {}}).first;
    }
    it->second.written.assign((const uint8_t*)v, (const uint8_t*)v + n);
    touched.insert(at);
    memcpy((void*)at, v, n);
}

inline float original_float(uintptr_t at) {
    auto it = saved_bytes.find(at);
    float f;
    if (it == saved_bytes.end() || it->second.original.size() < 4) return rdv<float>(at, NAN);
    memcpy(&f, it->second.original.data(), 4);
    return f;
}

inline void set_param(const std::string& name, float value, bool is_switch, const std::vector<uintptr_t>& containers,
                      std::set<uintptr_t>& touched) {
    uint8_t yes = value != 0;
    for (uintptr_t c : containers) {
        uintptr_t at = param_value(c, name);
        if (is_switch) put_bytes(at, &yes, 1, touched);
        else put_bytes(at, &value, 4, touched);
    }
}

inline void set_desc_stat(uintptr_t desc, int stat, float v, std::set<uintptr_t>& touched) {
    auto& f = g.stats[stat];
    if (f.off < 0 || !desc) return;
    int n = (int)std::lround(v);
    put_bytes(desc + f.off, f.is_float ? (const void*)&v : (const void*)&n, 4, touched);
}

inline void item_overrides(std::set<uintptr_t>& touched) {
    bool uv = is_on("uv"), no_reload = is_on("no_reload");
    if (!uv && !no_reload) return;
    for (int i = 0; i < ITEM_COUNT; i++) {
        const ItemInfo& info = ITEMS[i];
        auto d = g.descs.find(info.id);
        if (d == g.descs.end()) continue;
        if (uv && info.st[ST_DepletionTime] < 1e6f) set_desc_stat(d->second, ST_DepletionTime, 1e7f, touched);
        if (no_reload && info.st[ST_AmmoCount] > 0) {
            set_desc_stat(d->second, ST_AmmoCount, 999, touched);
            if (info.st[ST_ReloadTime] > 0) set_desc_stat(d->second, ST_ReloadTime, 0.05f, touched);
        }
    }
}

inline void apply_cached(const std::map<std::string, std::pair<float, bool>>& want) {
    uint8_t kinds[PARAM_SLOTS] = {};
    for (auto& [name, w] : want) {
        auto it = param_ids.find(name);
        int id = it == param_ids.end() ? -1 : it->second + 1;
        if (id < 0 || id >= PARAM_SLOTS) continue;
        cached[id].value = w.first;
        kinds[id] = w.second ? CACHED_SWITCH : CACHED_FLOAT;
    }
    std::lock_guard<std::mutex> l(cached_mx);
    for (int id = 0; id < PARAM_SLOTS; id++) {
        if (cached[id].kind == kinds[id]) continue;
        cached[id].kind = kinds[id];
        if (!kinds[id]) cached[id].stale = !overridden_providers[id].empty();
    }
}

inline uintptr_t cache_get_hook(uintptr_t provider, int id) {
    uintptr_t param = cache_get_original(provider, id);
    if (id < 0 || id >= PARAM_SLOTS || !param) return param;
    auto& c = cached[id];
    c.reads++;
    uint8_t kind = c.kind;
    if (!kind && !c.stale) return param;
    if (!kind) {
        {
            std::lock_guard<std::mutex> l(cached_mx);
            auto& providers = overridden_providers[id];
            if (!providers.erase(provider)) return param;
            if (providers.empty()) c.stale = false;
        }
        *(uint64_t*)(param + CACHED_VERSION) = ~0ull;
        return cache_get_original(provider, id);
    }
    uint8_t& flags = *(uint8_t*)(param + CACHED_FLAGS);
    if (!(flags & 1) || !*(uintptr_t*)param) {
        uintptr_t vt = kind == CACHED_SWITCH ? vt_param_bool : vt_param_float;
        if (!vt) return param;
        *(uintptr_t*)param = vt;
        flags |= 1;
    }
    if (kind == CACHED_SWITCH) *(uint8_t*)(param + CACHED_VALUE) = c.value != 0;
    else *(float*)(param + CACHED_VALUE) = c.value;
    std::lock_guard<std::mutex> l(cached_mx);
    overridden_providers[id].insert(provider);
    return param;
}

inline void absolute_jump(uint8_t* at, uintptr_t target) {
    const uint8_t jmp[6] = {0xFF, 0x25, 0, 0, 0, 0};
    memcpy(at, jmp, 6);
    memcpy(at + 6, &target, 8);
}

inline bool install_cache_hook() {
    const uint8_t prologue[15] = {0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18};
    if (cache_get_original) return true;
    if (!cache_get_fn || memcmp((const void*)cache_get_fn, prologue, sizeof prologue)) return false;
    auto* trampoline = (uint8_t*)VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!trampoline) return false;
    memcpy(trampoline, prologue, sizeof prologue);
    absolute_jump(trampoline + sizeof prologue, cache_get_fn + sizeof prologue);
    cache_get_original = (CacheGetFn)trampoline;
    uint8_t patch[sizeof prologue];
    absolute_jump(patch, (uintptr_t)&cache_get_hook);
    patch[14] = 0x90;
    DWORD old;
    VirtualProtect((void*)cache_get_fn, sizeof patch, PAGE_EXECUTE_READWRITE, &old);
    memcpy((void*)cache_get_fn, patch, sizeof patch);
    VirtualProtect((void*)cache_get_fn, sizeof patch, old, &old);
    FlushInstructionCache(GetCurrentProcess(), (void*)cache_get_fn, sizeof patch);
    return true;
}

inline bool lockpick_patched() { return lockpick_patch && !memcmp((const void*)lockpick_patch, SPOT_DISTANCE_ZERO, 4); }

inline void patch_lockpick(bool on) {
    if (!lockpick_patch || lockpick_patched() == on) return;
    DWORD old;
    VirtualProtect((void*)lockpick_patch, 4, PAGE_EXECUTE_READWRITE, &old);
    memcpy((void*)lockpick_patch, on ? SPOT_DISTANCE_ZERO : SPOT_DISTANCE_CLAMP, 4);
    VirtualProtect((void*)lockpick_patch, 4, old, &old);
    FlushInstructionCache(GetCurrentProcess(), (void*)lockpick_patch, 4);
}

inline void apply_overrides() {
    auto containers = param_containers();
    if (containers != saved_containers) saved_bytes.clear(), saved_containers = containers;
    std::map<std::string, std::pair<float, bool>> want;
    for (auto& c : CHEATS) {
        if (!c.on) continue;
        for (auto& [name, v] : c.numbers) want[name] = {v, false};
        for (const char* name : c.switches) want[name] = {1, true};
        if (!strcmp(c.key, "z_spits"))
            for (const char* base : SPIT_REGEN)
                for (int players = 1; players <= 4; players++) want[base + std::to_string(players) + "v1"] = {0.05f, false};
    }
    for (auto& t : TWEAKS) {
        float factor = t.factor;
        if (factor == 1.0f || containers.empty()) continue;
        for (const char* name : t.params) {
            float base = original_float(param_value(containers[0], name));
            if (std::isnan(base)) continue;
            auto it = want.find(name);
            float v = base * factor;
            if (it == want.end() || v > it->second.first) want[name] = {v, false};
        }
    }
    std::set<uintptr_t> touched;
    for (auto& [name, w] : want) set_param(name, w.first, w.second, containers, touched);
    apply_cached(want);
    if (is_on("ammo") && unlimited_ammo_flag) {
        uint8_t yes = 1;
        put_bytes(unlimited_ammo_flag, &yes, 1, touched);
    }
    item_overrides(touched);
    for (auto it = saved_bytes.begin(); it != saved_bytes.end();) {
        if (touched.count(it->first)) { ++it; continue; }
        auto& saved = it->second;
        std::vector<uint8_t> now(saved.written.size());
        bool still_ours = rd(it->first, now.data(), now.size()) && now == saved.written;
        if (still_ours && !IsBadWritePtr((void*)it->first, saved.original.size())) memcpy((void*)it->first, saved.original.data(), saved.original.size());
        it = saved_bytes.erase(it);
    }
}

inline float scaled_var_float(uintptr_t self, uintptr_t name, uintptr_t scope, uintptr_t extra) {
    float v = original_var_float(self, name, scope, extra);
    const char* s = name ? *(const char**)name : nullptr;
    if (!s || strncmp(s, "f_btz_", 6)) return v;
    float best = v;
    for (auto& t : TWEAKS)
        for (auto& var : t.vars)
            if (!strcmp(s, var.name) && t.factor != 1.0f) {
                float strength = std::clamp((t.factor - 1.0f) / (t.max - 1.0f), 0.0f, 1.0f);
                float moved = v + (var.at_max - v) * strength;
                if (std::fabs(moved) > std::fabs(best)) best = moved;
            }
    return best;
}

inline bool var_hook_needed() {
    for (auto& t : TWEAKS)
        if (!t.vars.empty() && t.factor != 1.0f) return true;
    return false;
}

inline void install_var_hook() {
    uintptr_t object = rdv<uintptr_t>(var_root), current = rdv<uintptr_t>(object);
    uintptr_t ours = (uintptr_t)&var_vtable[1];
    if (!current || current == ours || !var_hook_needed()) return;
    for (size_t n = VAR_SLOTS + 1; n > SLOT_VAR_FLOAT + 1; n /= 2)
        if (rd(current - 8, var_vtable, n * 8)) {
            original_var_float = (VarFloatFn)var_vtable[1 + SLOT_VAR_FLOAT];
            var_vtable[1 + SLOT_VAR_FLOAT] = (uintptr_t)&scaled_var_float;
            wr<uintptr_t>(object, ours);
            return;
        }
}

inline void all_off() {
    for (auto& c : CHEATS) c.on = false;
    for (auto& t : TWEAKS) t.factor = 1.0f;
}

inline int active_count() {
    int n = 0;
    for (auto& c : CHEATS) n += c.on;
    for (auto& t : TWEAKS) n += t.factor != 1.0f;
    return n;
}

inline uintptr_t tree_record(int type) {
    uintptr_t trees = alive(player) ? rdv<uintptr_t>(rdv<uintptr_t>(player + PARAM_CONTAINER) + SKILL_TREES) : 0;
    return trees ? trees + type * TREE_RECORD : 0;
}
inline int tree_level(int type) { return tree_record(type) ? rdv<uint16_t>(tree_record(type) + TREE_LEVEL) : -1; }
inline int tree_max(int type) {
    int m = tree_record(type) ? rdv<uint16_t>(tree_record(type) + TREE_MAX) : 0;
    return m > 0 && m < 1000 ? m : 0;
}

inline void level_up_with_xp(int type) {
    uintptr_t container = alive(player) ? rdv<uintptr_t>(player + PARAM_CONTAINER) : 0, r = tree_record(type);
    if (!container || !r || !level_from_xp_fn || tree_level(type) >= tree_max(type)) return;
    uint32_t next = rdv<uint32_t>(r + TREE_LEVEL_START) + rdv<uint32_t>(r + TREE_SPAN);
    if (rdv<uint32_t>(r + TREE_XP) < next) wr<uint32_t>(r + TREE_XP, next);
    ((void(__fastcall*)(uintptr_t, int))level_from_xp_fn)(container, type);
}

inline void set_tree_level(int type, int level) {
    uintptr_t container = alive(player) ? rdv<uintptr_t>(player + PARAM_CONTAINER) : 0;
    if (!container || !set_level_fn || !tree_max(type)) return;
    level = std::clamp(level, 0, tree_max(type));
    ((void(__fastcall*)(uintptr_t, int16_t, int))set_level_fn)(container, (int16_t)level, type);
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

extern "C" {
uintptr_t fatrainer_update_original[3] = {};
void fatrainer_update_hook0();
void fatrainer_update_hook1();
void fatrainer_update_hook2();
}

inline void install_update_hooks() {
    void (*hooks[3])() = {fatrainer_update_hook0, fatrainer_update_hook1, fatrainer_update_hook2};
    for (int i = 0; i < 3; i++) {
        if (!g.vt_health[i]) continue;
        auto at = (uintptr_t*)(g.vt_health[i] + SLOT_MODULE_UPDATE * 8);
        if (*at == (uintptr_t)hooks[i] || !in_game_module(*at)) continue;
        fatrainer_update_original[i] = *at;
        DWORD old;
        VirtualProtect(at, 8, PAGE_READWRITE, &old);
        *at = (uintptr_t)hooks[i];
        VirtualProtect(at, 8, old, &old);
    }
}

struct Vec3 { float x, y, z; };
using GetPositionFn = Vec3* (*)(uintptr_t, Vec3*);
using SetPositionFn = void (*)(uintptr_t, const Vec3*);
using FieldIntFn = uintptr_t (*)(uintptr_t, const char*, int*);
using FieldBoolFn = uintptr_t (*)(uintptr_t, const char*, bool*);
using RequestOwnershipFn = void (*)(uintptr_t);
inline RequestOwnershipFn request_ownership = nullptr;
inline GetPositionFn get_position = nullptr;
inline SetPositionFn set_position = nullptr;
inline FieldIntFn field_int = nullptr, field_enum = nullptr;
inline FieldBoolFn field_bool = nullptr;
inline uintptr_t float_field_editor = 0;

inline void locate_engine(HMODULE engine) {
    if (!engine) return;
    get_position = (GetPositionFn)GetProcAddress(engine, "?GetWorldPosition@IControlObject@@QEBA?AVvec3@@XZ");
    set_position = (SetPositionFn)GetProcAddress(engine, "?SetWorldPosition@IControlObject@@QEAAXAEBVvec3@@@Z");
    field_int = (FieldIntFn)GetProcAddress(engine, "?GetFieldInt@CRTTIObject@@QEBAPEBVCRTTIFieldInt@@PEBDAEAH@Z");
    field_enum = (FieldIntFn)GetProcAddress(engine, "?GetFieldEnum@CRTTIObject@@QEBAPEBVCRTTIFieldEnum@@PEBDAEAH@Z");
    field_bool = (FieldBoolFn)GetProcAddress(engine, "?GetFieldBool@CRTTIObject@@QEBAPEBVCRTTIFieldBool@@PEBDAEA_N@Z");
    request_ownership = (RequestOwnershipFn)GetProcAddress(engine, "?RequestOwnership@IGSObject@@QEAAXXZ");
    float_field_editor = (uintptr_t)GetProcAddress(engine, "?GetFieldFloatEditor@CRTTIObject@@UEBAPEBVCRTTIFieldFloat@@PEBDAEAM@Z");
}

struct Section { uintptr_t sensor; int type; int stage; bool last; Vec3 pos; };
enum { SENSOR_START = 1, SENSOR_STAGE = 2, SENSOR_REWARD = 3, SENSOR_EVAC = 4 };
inline std::vector<Section> prison_sections;
inline Vec3 saved_position{};
inline bool has_saved_position = false;
inline DWORD last_prison_tick = 0;

inline bool reflected(uintptr_t object) { return float_field_editor && slot(object, SLOT_FLOAT_FIELD_EDITOR) == float_field_editor; }

inline std::atomic<bool> sections_wanted{false};

inline void read_sections() {
    if (!alive(player)) return;
    std::vector<Section> out;
    for (uintptr_t s : g.prison_sensors) {
        if (!get_position || rdv<uintptr_t>(s) != g.vt_prison_sensor) continue;
        Section sec{s, 0, -1, false, {NAN, NAN, NAN}};
        if (g.sensor_control < 0) continue;
        get_position(s + g.sensor_control, &sec.pos);
        uintptr_t r = g.sensor_rtti < 0 ? 0 : s + g.sensor_rtti;
        if (r && reflected(r) && field_int && field_bool && field_enum) {
            int v = 0;
            bool b = false;
            if (field_int(r, "m_Stage", &v)) sec.stage = v;
            if (field_bool(r, "m_IsLastStage", &b)) sec.last = b;
            for (const char* name : {"m_Type", "m_SensorType", "m_PrisonSensorType"})
                if (field_enum(r, name, &v)) { sec.type = v; break; }
        }
        if (std::isfinite(sec.pos.x)) out.push_back(sec);
    }
    std::sort(out.begin(), out.end(), [](auto& a, auto& b) { return a.type != b.type ? a.type < b.type : a.stage < b.stage; });
    prison_sections = out;
}

inline Vec3 player_position() {
    Vec3 p{NAN, NAN, NAN};
    if (alive(player) && get_position && g.player_control >= 0) get_position(player + g.player_control, &p);
    return p;
}

inline void teleport(Vec3 to) {
    if (!alive(player) || !set_position || g.player_control < 0 || !std::isfinite(to.x)) return;
    Vec3 from = player_position();
    set_position(player + g.player_control, &to);
    Vec3 now = player_position();
    logf_hook("teleport: from %.1f %.1f %.1f to %.1f %.1f %.1f, now %.1f %.1f %.1f", from.x, from.y, from.z, to.x, to.y, to.z, now.x, now.y,
              now.z);
}

inline int pause_prison() {
    DWORD now = GetTickCount();
    float dt = last_prison_tick ? (now - last_prison_tick) / 1000.0f : 0;
    last_prison_tick = is_on("prison_pause") ? now : 0;
    int live = 0;
    for (uintptr_t d : g.prison_data) {
        if (rdv<uintptr_t>(d) != g.vt_prison_data) continue;
        live++;
        if (!last_prison_tick || dt <= 0 || dt > 1) continue;
        if (!rdv<uint8_t>(d + REPL_OWNED) && request_ownership) request_ownership(d);
        float start = rdv<float>(d + PRISON_START_TIME, NAN), end = rdv<float>(d + PRISON_END_TIME, NAN);
        if (start > 0) wr<float>(d + PRISON_START_TIME, start + dt);
        if (end > 0) wr<float>(d + PRISON_END_TIME, end + dt);
    }
    return live;
}

inline int keep_uv_charge() {
    int kept = 0;
    for (uintptr_t e : g.equipment) {
        if (rdv<uintptr_t>(e) != g.vt_equipment) continue;
        kept++;
        float charge = rdv<float>(e + UV_CHARGE, NAN);
        if (!(charge >= 0 && charge <= 1)) continue;
        if (charge < 1) wr<float>(e + UV_CHARGE, 1.0f);
        if (rdv<uint8_t>(e + UV_EXHAUSTED)) wr<uint8_t>(e + UV_EXHAUSTED, 0);
    }
    return kept;
}

inline std::atomic<bool> objects_missing{false};

inline int keep_rope_energy() {
    int live = 0;
    auto containers = param_containers();
    float full = containers.empty() ? NAN : rdv<float>(param_value(containers[0], "RopeMaxEnergy"), NAN);
    for (uintptr_t r : g.ropes) {
        if (rdv<uintptr_t>(r) != g.vt_rope) continue;
        live++;
        float energy = rdv<float>(r + ROPE_ENERGY, NAN);
        if (!(full > 0 && full < 1000) || !(energy >= 0 && energy <= full)) continue;
        if (energy < full) wr<float>(r + ROPE_ENERGY, full);
        if (rdv<uint8_t>(r + ROPE_DEPLETED)) wr<uint8_t>(r + ROPE_DEPLETED, 0);
    }
    return live;
}

inline void tick() {
    std::lock_guard<std::mutex> l(game::mx);
    bool prison_missing = !pause_prison() && is_on("prison_pause");
    bool uv_missing = is_on("uv") && !keep_uv_charge();
    bool rope_missing = is_on("hook") && !keep_rope_energy();
    patch_lockpick(is_on("lockpick"));
    objects_missing = prison_missing || uv_missing || rope_missing;
    game::reapply_stats();
    keep_stacks(is_on("ammo"), is_on("supplies"));
    if (is_on("one_hit")) weaken_enemies();
    player = find_player();
    install_var_hook();
    apply_overrides();
    if (!player) return;
    set_immortal(is_on("god"));
    if (is_on("stamina") || is_on("z_energy")) keep_stamina();
    else best_stamina[0] = best_stamina[1] = 0;
}

inline uint32_t reads_of(const char* name) {
    auto it = param_ids.find(name);
    return it == param_ids.end() || it->second + 1 >= PARAM_SLOTS ? 0 : cached[it->second + 1].reads.load();
}

inline std::string describe() {
    char b[800];
    snprintf(b, sizeof b,
             "player %s (%zu scanned, local root %s), health %.0f, stamina %.0f, params %zu, cache hook %s (hook reads %u, uv reads %u), "
             "ammo flag %s, set level %s, xp level %s, script vars %s, lockpick %s, enemies %zu, overrides %zu, equipment %zu, ropes %zu, "
             "prison data %zu (start %.1f end %.1f), prison sensors %zu, positions %s (player +%x, sensor +%x/+%x)",
             player ? "ok" : "missing", g.players.size(), local_player_root ? "ok" : "missing", health(), stamina(),
             param_ids.size(), cache_get_original ? "ok" : cache_get_fn ? "not installed" : "missing", reads_of("GrapplingHookCooldown"),
             reads_of("FlashlightDrainMul"), unlimited_ammo_flag ? "ok" : "missing", set_level_fn ? "ok" : "missing",
             level_from_xp_fn ? "ok" : "missing", var_root ? "ok" : "missing", !lockpick_patch ? "missing" : lockpick_patched() ? "on" : "ready", enemy_modules.size(), saved_bytes.size(),
             g.equipment.size(), g.ropes.size(), g.prison_data.size(), g.prison_data.empty() ? NAN : rdv<float>(g.prison_data[0] + PRISON_START_TIME, NAN),
             g.prison_data.empty() ? NAN : rdv<float>(g.prison_data[0] + PRISON_END_TIME, NAN), g.prison_sensors.size(),
             get_position && set_position ? "ok" : "missing", game::g.player_control, game::g.sensor_control, game::g.sensor_rtti);
    return b;
}

}

#define FATRAINER_UPDATE_HOOK(n)                                                                                    \
    ".globl fatrainer_update_hook" #n "\n"                                                                          \
    "fatrainer_update_hook" #n ":\n"                                                                                \
    "push %rcx\npush %rdx\npush %r8\npush %r9\nsub $0x68, %rsp\n"                                                  \
    "movdqu %xmm0, 0x20(%rsp)\nmovdqu %xmm1, 0x30(%rsp)\nmovdqu %xmm2, 0x40(%rsp)\nmovdqu %xmm3, 0x50(%rsp)\n"     \
    "call fatrainer_module_update\n"                                                                                \
    "movdqu 0x20(%rsp), %xmm0\nmovdqu 0x30(%rsp), %xmm1\nmovdqu 0x40(%rsp), %xmm2\nmovdqu 0x50(%rsp), %xmm3\n"     \
    "add $0x68, %rsp\npop %r9\npop %r8\npop %rdx\npop %rcx\n"                                                       \
    "jmp *fatrainer_update_original+" #n "*8(%rip)\n"

asm(".text\n" FATRAINER_UPDATE_HOOK(0) FATRAINER_UPDATE_HOOK(1) FATRAINER_UPDATE_HOOK(2));

extern "C" void fatrainer_module_update(uintptr_t module) {
    static cheats::Cheat* one_hit = cheats::find("one_hit");
    if (!one_hit->on || !cheats::is_enemy_module(module)) return;
    float cur = cheats::rdv<float>(module + cheats::MODULE_HEALTH, NAN);
    if (cur > cheats::ONE_HIT_HEALTH && cur < 1e7f) cheats::wr<float>(module + cheats::MODULE_HEALTH, cheats::ONE_HIT_HEALTH);
}
