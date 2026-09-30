# Changelog

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
