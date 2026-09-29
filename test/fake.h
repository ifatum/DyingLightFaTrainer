#pragma once
#include <cstring>
#include "game.h"

namespace fake {

const int DAMAGE_OFF = 0x120, CONDITION_OFF = 0x140, COLOR_OFF = 0x150;

struct World {
    uint8_t* mem;
    size_t used = 0;
    uintptr_t wallet, backpack, stash, materials, manager;
    std::map<std::string, uintptr_t> desc;

    uintptr_t alloc(size_t n) {
        uintptr_t p = (uintptr_t)mem + used;
        used += (n + 15) & ~(size_t)15;
        return p;
    }
    template <class T> void put(uintptr_t a, T v) { memcpy((void*)a, &v, sizeof v); }

    uintptr_t item(const char* id, int count) {
        uintptr_t it = alloc(0x100);
        put<int>(it + 0x40, count);
        put<uintptr_t>(it + 0x60, desc[id]);
        return it;
    }
    uintptr_t inventory(uintptr_t vt, int cap, std::vector<std::pair<const char*, int>> items) {
        uintptr_t inv = alloc(0x80), arr = alloc(8 * 64);
        put<uintptr_t>(inv, vt);
        put<uintptr_t>(inv + 0x40, arr);
        put<uint32_t>(inv + 0x48, (uint32_t)items.size());
        put<int>(inv + 0x58, cap);
        for (size_t i = 0; i < items.size(); i++) put<uintptr_t>(arr + i * 8, item(items[i].first, items[i].second));
        return inv;
    }

    void build() {
        mem = (uint8_t*)VirtualAlloc(nullptr, 64 << 20, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        uintptr_t names = alloc(ITEM_COUNT * 72), arr = alloc(ITEM_COUNT * 8);
        for (int i = 0; i < ITEM_COUNT; i++) {
            uintptr_t d = alloc(0x600), s = names + i * 72;
            strcpy((char*)s, ITEMS[i].id);
            put<uintptr_t>(d + 0x18, s);
            if (!std::isnan(ITEMS[i].st[ST_Damage])) put<float>(d + DAMAGE_OFF, ITEMS[i].st[ST_Damage]);
            if (!std::isnan(ITEMS[i].st[ST_Condition])) put<int>(d + CONDITION_OFF, (int)ITEMS[i].st[ST_Condition]);
            if (!std::isnan(ITEMS[i].st[ST_Color])) put<int>(d + COLOR_OFF, (int)ITEMS[i].st[ST_Color]);
            put<uintptr_t>(arr + i * 8, d);
            desc[ITEMS[i].id] = d;
        }
        manager = alloc(0x80);
        put<uintptr_t>(manager, game::g.vt_manager);
        put<uintptr_t>(manager + 0x50, arr);
        put<uint32_t>(manager + 0x58, ITEM_COUNT);
        wallet = alloc(0x80);
        put<uintptr_t>(wallet, game::g.vt_money);
        put<int>(wallet + 0x40, 15855);
        backpack = inventory(game::g.vt_inv[0], 14, {{"Throwable_ZaidFlare", 9}, {"Firearm_PistolAGen", 1},
                                                     {"Throwable_ThrowingAxeAGen", 30}, {"Throwable_Molotov", 7},
                                                     {"Melee_MacheteAGen", 1}, {"Special_Hook", 1}});
        stash = inventory(game::g.vt_inv[0], -1, {{"Melee_WrenchARusty", 1}, {"Melee_KnifeDGen", 1}, {"Throwable_Btz_Flare", 8}});
        materials = inventory(game::g.vt_inv[1], -1, {{"Craft_Gauze", 48}, {"Craft_Alcohol", 21}, {"Craft_MetalScrap", 81},
                                                      {"Medkit_HealthPackLarge", 20}, {"LockpickItem", 28}});
    }
};

}
