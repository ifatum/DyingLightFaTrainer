# Changelog

## 1.9

- New: configs on the Settings page. Save the cheats and sliders you have on under a name, then load or delete them with one click. Each config is a small file in fatrainer_configs next to the game.
- New: Death from above height on the PvP page. The game only starts Death From Above once you are already falling fast (12 m/s, a long drop). At Max a normal jump is enough.
- Fixed: PvP sliders, Slow down UV flashlight and other cheats stopping after you die. After a respawn the trainer now recognises your new character straight away and finds the game objects again on its own.
- New: Visuals page with a player ESP for Be The Zombie and co-op: box, role (Night Hunter or survivor), health bar and number, distance, PvP rank, hunter rage, lines from the screen bottom, allies on or off, a range limit and separate colors for the Night Hunter and survivors. It keeps drawing with the menu closed.
- New: custom UV light color and glow on the Visuals page.
- New: spit keys on the Night Hunter page. Pick your own key for the Horde Summoner, UV Suppressor, Sense Suppressor and Toxic spit; the trainer presses the game's key for it.
- Fixed: one hit kill no longer drops Be The Zombie nests to 1 health, so their health bar is not stuck on red.

## 1.8.1

- Fixed: the game could crash at start on Windows. The trainer's memory scan read game memory that the game was freeing at the same time; on Windows it now copies each block safely first.
- Fixed: a crash when the trainer read the position of a character that was being removed (Kill all enemies, saved positions, prison sections).
- Unbreakable weapons keeps each weapon's own durability full instead of relying on game settings, so it works for every melee weapon, including the Korek Machete.

## 1.8

- Fixed: zombies and NPCs standing still. Cheats that change game settings (for example the grappling hook) also changed a different setting of every zombie and NPC. They now only change your own settings.
- Fixed: giving an item could crash the game when the trainer tried it in the ammo inventory. Only ammo goes there now.
- Fixed a crash that could happen after turning a cheat off. The trainer wrote to game memory that the game had already freed, for example after a level load.
- Infinite UV flashlight: fixed. The trainer was filling the normal flashlight's battery instead of the UV charge.
- Instant lockpicking: fixed. The game now treats every pick position as the sweet spot, so the lock always turns all the way.
- Fixed a crash while loading into a save and when pressing Find sections: the trainer read prison section positions through the wrong part of the object. Positions are now looked up the way the game's own class layout describes, and only when you press Find sections.
- New: weapon rarity in the weapon editor (Gray, Green, Blue, Purple, Orange, Gold). Found and generated weapons keep their own rarity, so the editor changes that one weapon, and the game saves it with the weapon.
- New: Slow down UV flashlight slider next to Infinite UV flashlight. x10 makes the charge last ten times longer.
- New: Night Hunter level on the Skills page (-10, -1, +1, +10, Max), and a Be The Zombie ranks card to change your survivor and Night Hunter PvP rank.
- New: Kill all enemies nearby (Combat and Prison pages). Kills every zombie and human within 80 m through the game's own kill, so quest objectives like killing the bandits or a final wave count them.
- New: route recorder on the Prison page. Record your route once while playing normally; Replay teleports you along it in small steps so every quest trigger fires, and pauses where you stood still (fights, doors) until you press Continue. Saved in fatrainer_route.txt next to the game.
- Prison sections are recognised again (they all showed as unnamed sections before) and listed in run order: Start, every split checkpoint (Split 1, 2, ...), the reward room and the evacuation. A Next section button jumps to them one after another.
- Teleporting moves your physics body too, so the game no longer pulls you back to where you stood.
- God mode also works in Harran Prison: while it is on, the game's forced damage (which the prison uses) is blocked too.
- Infinite grappling hook keeps the rope energy full directly, so the hook is ready every time, also while you are still hanging on the rope.
- Pause prison timers: the trainer now takes ownership of the prison timer data like the game does, so the paused time is no longer overwritten.
- Cheats that need game objects (UV flashlight, grappling hook, prison timers) find them again on their own after a level load, even when the menu stays closed.
- Turning a cheat off restores a value only if the game has not changed it since.
- Ignores stray stash objects with garbage capacity.

## 1.7

- New: Prison page for Harran Prison. Pause the run timer and the reward room countdown, teleport to every prison section, and save and load your position anywhere.
- Infinite grappling hook: the hook's wait comes from rope energy, not from a cooldown, so the rope now recharges instantly.
- Infinite UV flashlight: the UV charge is held full directly on your equipment.
- Refill health and stamina writes full health and stamina itself if the game's refill leaves them low.
- Instant lockpicking also searches the game's own data for the lock difficulty settings.
- Fixed: giving items could crash the game when the item used as a template had just been removed. The trainer now takes it from your live inventory and checks it first.
- Fixed: the stash shows on the Stash page again (its capacity is unlimited, not -1).
- Fixed: memory scans under Proton skip Wine's own thread memory, which showed extra wallets and players.
- New pages are placed next to their group in the sidebar when you update.

## 1.6

- Infinite grappling hook and infinite UV flashlight: the trainer now hooks the function the game uses to read these settings, so the new values reach every place that uses them.
- New: instant lockpicking. Every pick position opens the lock and lockpicks never break.
- New: Level up button on the Skills page. It gives exactly the XP for the next level, so the game levels you up like normal play.
- Giving items tries every inventory until the game accepts the item, so upgrades like King and Clicker work, and ammo goes into the ammo inventory.
- Typing in the menu works under Proton: money amounts, search boxes, item counts and weapon stats.
- PvP: one slider per attack instead of separate range and angle sliders. At Max the pounce and dropkick also hit targets that are not in front of you.
- PvP: death from above has two sliders, range and pull. At Max pull the hunter can be beside or behind you and the attack pulls you onto him.
- Cash is one simple option: one amount, quick buttons and Set.
- The stash shows up even when it is empty, and only one stash is listed.
- Interface size changes only when you press Apply, so the slider no longer jumps while you drag it.
- Clearer labels on the Give items, inventory and PvP pages.
- Fixed: the memory scan could pick up stale copies from its own thread stack under Proton, which showed extra wallets.
- Better logs for bug reports: system, overlays and other mods, every overlay start step, a first frame watchdog, your inventories, failed gives and messages.

## 1.5

- New sidebar menu grouped into Cheats, Be The Zombie and Items. A dot marks pages with something turned on, and Turn all off resets everything.
- Settings saved to fatrainer.ini: accent color, interface size, background dim, menu key, page order and visibility, remembered cheats.
- The game ignores your mouse and keyboard while the menu is open, the background dims, and the mouse wheel scrolls the menu.
- Player: god mode, infinite stamina, infinite grappling hook, infinite UV flashlight, no fall damage, movement speed, jump height, refill health and stamina.
- Combat: one hit kill, infinite ammo, no reload, infinite consumables, unbreakable weapons.
- Skills: XP gain multiplier and skill tree levels with -10, -1, +1, +10 and Max.
- Night Hunter: hunter god mode, infinite energy, no ability cooldowns, infinite spits, long camouflage.
- PvP: range and aim angle sliders for pounce, ground pound, tackle, claws, spit, death from above, dropkick, kicks and melee.
- Weapon editor: magazine size and reload time, and edits stay applied while the game runs.
- Your own character is found correctly in co-op.

## 1.0.0

- First release: cash, backpack, stash, materials, give any item or blueprint, weapon editor.
- Works on Windows and on Linux with Proton.
