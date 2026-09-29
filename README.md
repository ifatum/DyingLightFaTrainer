<div align="center">

# FaTrainer | Dying Light

**In-game trainer for Dying Light 1 on Linux (Steam + Proton)**

![platform](https://img.shields.io/badge/platform-Linux%20%7C%20Proton-orange)
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

```sh
./build.sh
./install.sh
```

Add this to the Steam launch options:

```
WINEDLLOVERRIDES="xinput1_3=n,b" %command%
```

Press **Insert** or **F8** in game to open the menu.

### Notes

- Made for single player. Back up your saves first.
- Log file: `fatrainer.log` next to `DyingLightGame.exe`.

<div align="center"><sub>MIT License</sub></div>
