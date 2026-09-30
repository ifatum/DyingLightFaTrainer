#include <cstdio>
#include "cheats.h"
#include "config.h"
#include "fake.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static fake::World w;

static void __fastcall fake_add(uintptr_t inv, void* data, int, bool) {
    int count;
    uintptr_t desc;
    memcpy(&count, (uint8_t*)data, 4);
    memcpy(&desc, (uint8_t*)data + 0x20, 8);
    uintptr_t arr = game::rdv<uintptr_t>(inv + 0x40);
    uint32_t n = game::rdv<uint32_t>(inv + 0x48);
    uintptr_t it = w.alloc(0x100);
    w.put<int>(it + 0x40, count);
    w.put<uintptr_t>(it + 0x60, desc);
    w.put<uintptr_t>(arr + n * 8, it);
    w.put<uint32_t>(inv + 0x48, n + 1);
}

static float fake_var_float(uintptr_t, uintptr_t name, uintptr_t, uintptr_t) {
    const char* s = *(const char**)name;
    if (!strcmp(s, "f_btz_zombie_grab_range")) return 10;
    if (!strcmp(s, "f_btz_wrestling_kick_angle_max")) return 22;
    if (!strcmp(s, "f_btz_pvp_grab_below_angle_threshold")) return -50;
    return 7;
}
static uintptr_t fake_var_vtable[1100];
static uintptr_t fake_var_object = (uintptr_t)&fake_var_vtable[1];
static uintptr_t fake_var_holder = (uintptr_t)&fake_var_object;

static float read_var(const char* name) {
    auto fn = (cheats::VarFloatFn)cheats::slot((uintptr_t)&fake_var_object, cheats::SLOT_VAR_FLOAT);
    return fn((uintptr_t)&fake_var_object, (uintptr_t)&name, 0, 0);
}

static void __fastcall refuse_add(uintptr_t, void*, int, bool) {}

int main(int argc, char** argv) {
    HMODULE m = LoadLibraryExA(argv[1], nullptr, DONT_RESOLVE_DLL_REFERENCES);
    CHECK(m);
    if (!m) return 1;
    CHECK(game::resolve_classes((uintptr_t)m));
    CHECK(game::g.vt_manager && game::g.vt_inv[1] && game::g.vt_inv[2]);
    const uint8_t* f16 = *(const uint8_t**)(game::g.vt_money + 16 * 8);
    CHECK(memcmp(f16 + 7, "\x8b\x41\x40", 3) == 0);

    w.build();
    game::refresh();
    printf("status: %s\nstats: %s\n", game::g.status.c_str(), game::stat_report().c_str());
    CHECK(game::g.wallets.size() == 1 && game::money(game::g.wallets[0]) == 15855);
    auto* bp = game::find_inventory(game::K_BACKPACK);
    auto* st = game::find_inventory(game::K_STASH);
    auto* mat = game::find_inventory(game::K_MATERIALS);
    CHECK(bp && bp->items.size() == 6 && bp->capacity == 14);
    CHECK(st && st->items.size() == 3);
    CHECK(mat && mat->items.size() == 5 && mat->items[0].info && !strcmp(mat->items[0].info->id, "Craft_Gauze"));
    CHECK(game::g.descs.size() == (size_t)ITEM_COUNT);
    CHECK(game::g.stats[ST_Damage].off == fake::DAMAGE_OFF && game::g.stats[ST_Damage].is_float);
    CHECK(game::g.stats[ST_Condition].off == fake::CONDITION_OFF && !game::g.stats[ST_Condition].is_float);
    CHECK(game::g.stats[ST_Color].off == fake::COLOR_OFF);
    CHECK(game::g.stats[ST_Price].off < 0);

    uintptr_t machete = w.desc["Melee_MacheteAGen"];
    CHECK(game::get_stat(machete, ST_Damage) == 184.0f);
    CHECK(game::set_stat(machete, ST_Damage, 999.5f) && game::get_stat(machete, ST_Damage) == 999.5f);
    CHECK(game::set_stat(machete, ST_Condition, 500) && game::rdv<int>(machete + fake::CONDITION_OFF) == 500);

    CHECK(game::money(game::g.wallets[0]) == 15855 && game::set_money(game::g.wallets[0], 30000) &&
          game::money(game::g.wallets[0]) == 30000);
    CHECK(game::set_count(mat->items[0], 999) && game::count(mat->items[0]) == 999);

    uintptr_t vt[8] = {};
    vt[6] = (uintptr_t)&fake_add;
    uintptr_t inv = w.alloc(0x80), arr = w.alloc(8 * 16);
    w.put<uintptr_t>(inv, (uintptr_t)vt);
    w.put<uintptr_t>(inv + 0x40, arr);
    uintptr_t tmpl = mat->items[1].addr;
    uintptr_t medkit = game::g.descs["Medkit_HealthPackLarge"];
    CHECK(game::pick_template(mat) == mat->items[0].addr);
    CHECK(game::give(inv, tmpl, medkit, 5));
    uintptr_t added = game::rdv<uintptr_t>(arr);
    CHECK(game::rdv<int>(added + 0x40) == 5 && game::rdv<uintptr_t>(added + 0x60) == medkit);
    CHECK(game::default_target("Craft_Gauze") == game::K_MATERIALS && game::default_target("Melee_MacheteAGen") == game::K_BACKPACK);
    CHECK(game::default_target("Ammo_PistolBig") == game::K_AMMO);

    CHECK(game::g.vt_player && game::g.vt_human && game::g.vt_health[0]);
    CHECK(game::g.players.size() == 1);
    game::set_stat(machete, ST_Damage, 1234);
    game::write_stat(machete, ST_Damage, 5);
    cheats::locate((uintptr_t)m);
    printf("params: %zu names\n", cheats::param_ids.size());
    CHECK(cheats::param_ids["MaxStamina"] == 495 && cheats::param_ids["InfiniteStamina"] == 969);
    CHECK(cheats::param_ids["GrapplingHookCooldown"] == 541 && cheats::param_ids["TDCooldown"] == 20);
    CHECK(cheats::param_ids.count("FlashlightDrainMul") && cheats::param_ids.count("ZombieSpitToxicEnabled"));
    for (auto& c : cheats::CHEATS) {
        for (auto& n : c.numbers) CHECK(cheats::param_ids.count(n.first));
        for (auto* n : c.switches) CHECK(cheats::param_ids.count(n));
    }
    CHECK(cheats::local_player_root && cheats::params_root && cheats::unlimited_ammo_flag && cheats::var_root);
    CHECK(cheats::var_root > (uintptr_t)m && cheats::var_root < (uintptr_t)m + 0x4000000);
    fake_var_vtable[1 + cheats::SLOT_VAR_FLOAT] = (uintptr_t)&fake_var_float;
    cheats::var_root = (uintptr_t)&fake_var_holder;
    cheats::local_player_root = cheats::params_root = 0;
    uintptr_t flag = cheats::unlimited_ammo_flag;
    static uint8_t fake_rules[4];
    cheats::unlimited_ammo_flag = (uintptr_t)fake_rules;
    for (auto* k : {"god", "stamina", "supplies", "one_hit", "hook", "uv", "ammo", "z_spits", "no_fall", "durability"}) cheats::find(k)->on = true;
    for (auto& t : cheats::TWEAKS)
        for (auto* n : t.params) CHECK(cheats::param_ids.count(n));
    game::wr<float>(w.params + cheats::param_ids["GrapplingHookCooldown"] * 16 + 8, 12.5f);
    game::wr<float>(w.params + cheats::param_ids["AirKickRangeMul"] * 16 + 8, 1.0f);
    cheats::find("no_reload")->on = true;
    cheats::find_tweak("h_dropkick")->factor = 3.0f;
    cheats::scan_enemies();
    game::set_count(mat->items[0], 999);
    cheats::tick();
    game::set_count(mat->items[0], 3);
    cheats::tick();
    CHECK(game::get_stat(machete, ST_Damage) == 1234);
    CHECK(cheats::player == w.player && cheats::health() == 87);
    CHECK(game::rdv<uintptr_t>(w.player + 0x8f8) == (uintptr_t)&cheats::immortal_vtable[1]);
    CHECK(((bool(__fastcall*)(uintptr_t))cheats::slot(w.player + 0x8f8, cheats::SLOT_IS_IMMORTAL))(0));
    CHECK(game::rdv<float>(w.stamina + 0x10) == 100);
    CHECK(game::rdv<float>(w.enemy_health + 0x78) == 1);
    CHECK(game::count(mat->items[0]) == 999);
    auto param = [&](const char* n) { return w.params + cheats::param_ids[n] * 16 + 8; };
    auto cached = [&](const char* n) { return w.cache + (cheats::param_ids[n] + 1) * 40; };
    CHECK(game::rdv<float>(cached("GrapplingHookCooldown") + 0x10) == 0 && (game::rdv<uint8_t>(cached("GrapplingHookCooldown") + 0x20) & 1));
    CHECK(game::rdv<uintptr_t>(cached("GrapplingHookCooldown") + 8) == cheats::vt_param_float && cheats::vt_param_float);
    CHECK(game::rdv<uint8_t>(cached("CanUseHook") + 0x10) == 1 && game::rdv<uintptr_t>(cached("CanUseHook") + 8) == cheats::vt_param_bool);
    CHECK(game::rdv<float>(param("AirKickRangeMul")) == 3.0f && game::rdv<float>(cached("AirKickRangeMul") + 0x10) == 3.0f);
    CHECK(game::g.stats[ST_AmmoCount].off == fake::AMMO_OFF && game::g.stats[ST_DepletionTime].off == fake::DEPLETION_OFF);
    CHECK(game::rdv<int>(w.desc["Firearm_PistolAGen"] + fake::AMMO_OFF) == 999 && game::rdv<float>(w.desc["Firearm_PistolAGen"] + fake::RELOAD_OFF) == 0.05f);
    CHECK(game::rdv<float>(w.desc["Flashlight_Superlight"] + fake::DEPLETION_OFF) == 1e7f);
    CHECK(cheats::tree_level(2) == 7 && cheats::tree_max(2) == 25 && cheats::tree_max(5) == 0);
    game::wr<float>(w.enemy_health + 0x78, 300);
    fatrainer_module_update(w.enemy_health);
    CHECK(game::rdv<float>(w.enemy_health + 0x78) == 1);
    CHECK(game::rdv<float>(param("GrapplingHookCooldown")) == 0 && game::rdv<uint8_t>(param("CanUseHook")) == 1);
    CHECK(game::rdv<uint8_t>(param("InfiniteStamina")) == 1 && game::rdv<float>(param("FlashlightRechargeSpeed")) == 1000);
    CHECK(game::rdv<uint8_t>(param("CanUseHook")) == 1 && game::rdv<float>(param("ZombieSpitLightDisableAmmoRegenTime3v1")) == 0.05f);
    CHECK(fake_rules[0] == 1);
    for (auto& c : cheats::CHEATS) c.on = false;
    cheats::find_tweak("h_dropkick")->factor = 1.0f;
    cheats::tick();
    CHECK(game::rdv<float>(param("AirKickRangeMul")) == 1.0f && game::rdv<uint64_t>(cached("AirKickRangeMul")) == ~0ull);
    CHECK(game::rdv<uintptr_t>(cached("GrapplingHookCooldown") + 8) == 0 && game::rdv<uint8_t>(cached("GrapplingHookCooldown") + 0x20) == 0);
    CHECK(game::rdv<int>(w.desc["Firearm_PistolAGen"] + fake::AMMO_OFF) == 8 && game::rdv<float>(w.desc["Flashlight_Superlight"] + fake::DEPLETION_OFF) == 10.0f);
    CHECK(game::rdv<float>(param("GrapplingHookCooldown")) == 12.5f && game::rdv<uint8_t>(param("CanUseHook")) == 0 && fake_rules[0] == 0);
    (void)flag;
    CHECK(game::rdv<uintptr_t>(w.player + 0x8f8) == game::g.vt_human);
    printf("cheats: %s\n", cheats::describe().c_str());

    CHECK(read_var("f_btz_zombie_grab_range") == 10);
    cheats::find_tweak("z_pounce")->factor = 5.5f;
    cheats::find_tweak("h_dropkick")->factor = 10.0f;
    cheats::tick();
    CHECK(fake_var_object == (uintptr_t)&cheats::var_vtable[1]);
    CHECK(read_var("f_btz_zombie_grab_range") == 25 && read_var("f_btz_wrestling_kick_angle_max") == 180);
    CHECK(read_var("f_btz_pvp_grab_below_angle_threshold") == -70 && read_var("f_btz_other") == 7 && read_var("i_other") == 7);
    CHECK(cheats::active_count() == 2);
    cheats::find_tweak("h_dfa")->factor = 10.0f;
    CHECK(read_var("f_btz_jump_attack_range") == 12 && read_var("f_btz_jump_attack_angle_max") == 180);
    CHECK(read_var("f_btz_pvp_grab_below_angle_threshold") == -90 && read_var("f_btz_zombie_grab_range") == 25);
    cheats::all_off();
    CHECK(read_var("f_btz_jump_attack_range") == 7);
    CHECK(read_var("f_btz_zombie_grab_range") == 10 && read_var("f_btz_wrestling_kick_angle_max") == 22 && cheats::active_count() == 0);

    {
        static uintptr_t refusing_vt[8] = {}, accepting_vt[8] = {};
        refusing_vt[6] = (uintptr_t)&refuse_add;
        accepting_vt[6] = (uintptr_t)&fake_add;
        uintptr_t refusing = w.alloc(0x80), accepting = w.alloc(0x80);
        w.put<uintptr_t>(refusing, (uintptr_t)refusing_vt);
        w.put<uintptr_t>(accepting, (uintptr_t)accepting_vt);
        w.put<uintptr_t>(refusing + 0x40, w.alloc(8 * 16));
        w.put<uintptr_t>(accepting + 0x40, w.alloc(8 * 16));
        auto saved = game::g.invs;
        game::Item gauze = mat->items[0], alcohol = mat->items[1];
        game::g.invs = {{refusing, game::K_MATERIALS, -1, {gauze}}, {accepting, game::K_TOOLS, -1, {alcohol}}};
        uintptr_t king = game::g.descs["Craft_Upgrade_DamL2DurL2BalL2"];
        CHECK(king && game::give_anywhere(game::K_MATERIALS, king, 3) == game::K_TOOLS);
        uintptr_t given = game::rdv<uintptr_t>(game::rdv<uintptr_t>(accepting + 0x40));
        CHECK(game::rdv<uintptr_t>(given + 0x60) == king && game::rdv<int>(given + 0x40) == 3);
        game::g.invs = {{refusing, game::K_MATERIALS, -1, {gauze}}};
        CHECK(game::give_anywhere(game::K_MATERIALS, king, 3) == -1);
        game::g.invs = saved;
    }

    config::cfg.accent[0] = 0.25f;
    config::cfg.pages = {{"combat", false}, {"player", true}};
    config::cfg.cheats_on = {"god", "uv"};
    config::cfg.tweaks = {{"h_dfa", 4.5f}};
    char tmp[MAX_PATH];
    GetTempPathA(MAX_PATH, tmp);
    std::string path = std::string(tmp) + "fatrainer_selftest.ini";
    CHECK(config::save(path));
    config::cfg = config::Config();
    config::load(path);
    config::normalize({"player", "combat", "settings"});
    CHECK(config::cfg.accent[0] == 0.25f && config::cfg.cheats_on.size() == 2 && config::cfg.cheats_on[1] == "uv");
    CHECK(config::cfg.pages.size() == 3 && config::cfg.pages[0].id == "combat" && !config::cfg.pages[0].visible);
    CHECK(config::cfg.pages[2].id == "settings" && config::cfg.pages[2].visible);
    CHECK(config::cfg.tweaks.size() == 1 && config::cfg.tweaks[0].second == 4.5f);
    config::normalize({"player", "combat", "skills", "settings"});
    CHECK(config::cfg.pages[2].id == "skills" && config::cfg.pages[3].id == "settings");
    DeleteFileA(path.c_str());

    printf(fails ? "SELFTEST FAILED (%d)\n" : "SELFTEST OK\n", fails);
    return fails != 0;
}
