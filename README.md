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
- **Night Hunter**: hunter god mode, infinite energy, no ability cooldowns, infinite spits, spit keys, long camouflage
- **PvP**: one reach slider per attack (pounce, ground pound, tackle, claws, spit hit radius, death from above, dropkick, kicks, melee). At Max the pounce, dropkick and death from above also hit targets that are not in front of you
- **Cash / Backpack / Stash / Materials**: set your money and any stack count
- **Give items**: spawn any weapon, upgrade, consumable, material or blueprint; each item goes into the inventory the game accepts it in
- **Weapon editor**: damage, durability, crits, knockback, stamina, reach, magazine, reload time, upgrades, repairs, rarity
- **Settings**: accent color, interface size, background dim, menu key, sidebar order and visibility, saved to `fatrainer.ini`

### Install

Download the installer from the [latest release](https://github.com/ifatum/DyingLightFaTrainer/releases/latest):

- **Windows**: `FaTrainer-Installer.exe`
- **Linux**: `FaTrainer-Installer-Linux.tar.gz`, unpack it and run `FaTrainer-Installer` (works on any distribution with glibc 2.27 or newer and `curl` or `wget`). On NixOS start it with `steam-run ./FaTrainer-Installer`

On Linux, Dying Light has to run through Proton (Properties → Compatibility → force a Proton version); the trainer does not load in the native Linux version.

The installer finds Dying Light in your Steam libraries (or lets you choose the folder), downloads `xinput1_3.dll` from the release, checks it against the release checksum and puts it next to `DyingLightGame.exe`. Run it again to update or uninstall. On Linux it also shows the Steam launch option Proton needs:

```
WINEDLLOVERRIDES="xinput1_3=n,b" %command%
```

When a newer version is released, the trainer stays off in game and asks you to run the installer again. The game itself keeps working, and without internet the trainer works as usual.

The [Nexus Mods](https://www.nexusmods.com/dyinglight/mods/1724) download is an offline edition: its installer has the trainer inside, and neither connects to the internet, so there is no update check. Nexus Mods tells you about updates when you track the mod.

Press **Insert** or **F8** in game to open the menu. While it is open the game ignores your mouse and keyboard. A dot in the sidebar marks pages with something turned on, and **Turn all off** resets every cheat and slider at once.

See [CHANGELOG.md](CHANGELOG.md) for what changed in each version.

### Building

`./build.sh` builds the trainer, `./build_installer.sh` both installers (plus the offline Nexus Mods edition in `dist/nexus/`, built with `-DFATRAINER_OFFLINE`) and `./release.sh` the release files and the Nexus Mods zip (all through nix).

### Notes

- Made for single player. Back up your saves first.
- Log file: `fatrainer.log` next to `DyingLightGame.exe`.

<div align="center"><sub>MIT License</sub></div>
