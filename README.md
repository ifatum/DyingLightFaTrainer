<div align="center">

# FaTrainer | Dying Light

**In-game trainer for Dying Light 1 on Windows and Linux**

![platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-orange)
![game](https://img.shields.io/badge/game-Dying%20Light%201-red)
![license](https://img.shields.io/badge/license-MIT-blue)

</div>

---

### Features

- **Cash**: set your money live
- **Backpack / Stash / Materials**: change any stack count
- **Give items**: spawn any weapon, consumable, material or blueprint
- **Weapon editor**: damage, durability, crits, knockback, stamina, reach, upgrades, repairs, rarity

### Install

Build `xinput1_3.dll` with `./build.sh` (needs mingw-w64 through nix) and copy it next to `DyingLightGame.exe`.

- **Windows**: nothing else to do.
- **Linux (Proton)**: run `./install.sh` and add this to the Steam launch options:

```
WINEDLLOVERRIDES="xinput1_3=n,b" %command%
```

Press **Insert** or **F8** in game to open the menu.

### Notes

- Made for single player. Back up your saves first.
- Log file: `fatrainer.log` next to `DyingLightGame.exe`.

<div align="center"><sub>MIT License</sub></div>
