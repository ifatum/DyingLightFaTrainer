<div align="center">

# FaTrainer | Dying Light

**In-game trainer for Dying Light 1 on Windows and Linux**

![platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-orange)
![game](https://img.shields.io/badge/game-Dying%20Light%201-red)
![license](https://img.shields.io/badge/license-MIT-blue)

</div>

---

### Features

- **Player**: god mode, infinite stamina, infinite grappling hook, infinite UV flashlight, instant lockpicking, no fall damage, movement speed, jump height, instant refill
- **Combat**: one hit kill, infinite ammo, no reload, infinite consumables, unbreakable weapons
- **Skills**: XP multiplier, Level up with XP, or set any skill tree level (+1, +10, max, -1, -10)
- **Prison**: pause the Harran Prison timers, teleport to every section, save and load your position
- **Night Hunter**: hunter god mode, infinite energy, no ability cooldowns, infinite spits, spit aimbot, spit keys, long camouflage
- **PvP**: one reach slider per attack (pounce, ground pound, tackle, claws, spit, death from above, dropkick, kicks, melee). At Max the pounce, dropkick and death from above also hit targets that are not in front of you
- **Cash / Backpack / Stash / Materials**: set your money and any stack count
- **Give items**: spawn any weapon, upgrade, consumable, material or blueprint; each item goes into the inventory the game accepts it in
- **Weapon editor**: damage, durability, crits, knockback, stamina, reach, magazine, reload time, upgrades, repairs, rarity
- **Settings**: accent color, interface size, background dim, menu key, sidebar order and visibility, saved to `fatrainer.ini`

### Install

Build `xinput1_3.dll` with `./build.sh` (needs mingw-w64 through nix) and copy it next to `DyingLightGame.exe`.

- **Windows**: nothing else to do.
- **Linux (Proton)**: run `./install.sh` and add this to the Steam launch options:

```
WINEDLLOVERRIDES="xinput1_3=n,b" %command%
```

Press **Insert** or **F8** in game to open the menu. While it is open the game ignores your mouse and keyboard. A dot in the sidebar marks pages with something turned on, and **Turn all off** resets every cheat and slider at once.

See [CHANGELOG.md](CHANGELOG.md) for what changed in each version.

### Notes

- Made for single player. Back up your saves first.
- Log file: `fatrainer.log` next to `DyingLightGame.exe`.

<div align="center"><sub>MIT License</sub></div>
