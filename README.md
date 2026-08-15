# RebirthProjectMU

MU Online client + server source (Season 5.2~6.3), `Source_Client` (C++/OpenGL) and `Source_Server` (DataServer, JoinServer, ConnectServer, GameServer).

## Changelog

### 2026-08-15
- **Feature**: Lua scripting engine embedded in the GameServer (`ScriptLoader`/`LuaFunction`, ~170 native bindings covering objects, items, monsters, guilds, commands, effects, messages), ported from a compatible SSeMU-lineage pack and adapted to this project's real API surface. Vendors Lua 5.2 (headers + a prebuilt `lua52.lib`). Scripts live under `MuServer/Data/Script/` (`ScriptMain.lua` → `System/ScriptCore.lua` core dispatcher + individual scripts); a new `Script\WindowTitle.lua` sets the client's window title to `Player: <nick> || Level: <level> || Reset: <reset>` on character entry, using a new `SetObjectWindowTitle` Lua binding and a matching new client↔server packet (`0xC1:0x74`).
- **Fix**: the window title (above) only refreshed at login, so leveling up mid-session left it showing a stale level. Added a proper `OnUserLevelUp` script hook (fires the instant `CharacterLevelUp` succeeds) instead of relying on login-only data. Confirmed fixed in-game.
- **Feature**: MU Helper auto-play engine ported in full (attack, heal, buff, drain life, item pickup/filtering, equipment repair, party support, combo skills, self-defense), with a gear/play icon pair on the position HUD and a `Z` hotkey to open it.
- **Feature**: MU Helper config (skills, range, repair, pickup filters, thresholds) now persists across logins, server-side — reused this pack's existing but previously client-unused `HelperData` DB pipe (`0xC1:0xAE`/`0xC1:0x17`) instead of a bespoke mechanism.
- **Fix**: MU Helper's repair-item check flooded a request every 250ms tick during combat — throttled to once every 5s.
- **Fix**: MU Helper's "Pick All Near Items" / "Pick Selected Items" were only visually mutually exclusive — the underlying flags never actually cleared, so it kept picking up everything regardless of the UI.
- **Fix**: closing MU Helper left its skill-picker sub-window open (`CNewUIObj::Show()` isn't virtual, so the generic window-close path couldn't reach the override that closes it).
- **Fix**: MU Helper's "Add Extra Item" truncated names to 14 characters (e.g. "Scroll of Hellfire" → "Scroll of Hell") — two separate leftover buffers, both from a dropped older protocol format.
- **Fix**: MU Helper's saved-item list couldn't select any line but the first, and allowed duplicate entries.
- **Fix**: MU Helper's Auto Potion/Heal/Party-Heal threshold bars reset unless closed via the exact right button.
- **Fix**: Dark Spirit pet never actually attacked in any mode — was sending the raw command enum value instead of the small index the server expects.
- **Fix**: several skills (Twisting Slash, Evil Spirit/Storm, and ~30 others across every class) dealt damage via MU Helper but never played their animation — ported each one's real client-authoritative packet sequence from its own source location.
- **Fix**: MU Helper's basic attack ignored the character's actual attack speed, attacking every 250ms tick regardless of Agility — now paced using the same animation-duration data the game itself uses to play the swing.
- **Fix**: skill names (character skill list tooltip) were in Portuguese in *both* the Portuguese and English clients — translated all 365 to English in both.
- **Fix/Translation**: Command Window (`D` key), several item-stat tooltip lines (e.g. "Reflect Damage"), a few special-item descriptions, all 158 buff names/descriptions, and the Guild creation prompt were in English in the Portuguese client — translated to Portuguese.
- **Feature**: screenshots (`PrintScreen`) now save into a `Screenshots\` folder (created automatically if missing) instead of the client's root, and the filename now includes the character's nickname.
- **Chore**: stopped tracking `Source_Client/Global Debug/Main` — it was a stale 19MB `.ilk` (incremental linker database) file mistakenly committed instead of the real `Main.exe` (it dodged the `.gitignore`'s `*.exe`/`*.ilk` rules by having no extension). The real `Main.exe` stays untracked, as it already was.

### 2026-08-14
- **Feature**: skill use locked out for 1.5s right after the SM's Teleport skill, to stop it being used to chain into other skills faster than normal.
- **Fix**: mailbox letter-read crash (`MemBlock==0xFD` assert) — `ReceiveLetterText` wrote a null terminator past the received packet data when the letter's memo length exactly matched the packet size.
- **Feature**: F9 — CTRL Lock, keeps CTRL reporting as held (see names/attack without holding the key), persists across sessions.
- **Feature**: F12 — minimize to system tray with a tray icon; restoring is double-click-the-icon only (safe with multiple client instances running at once).
- **Fix**: `/addpoint` (`/v /a /s /e /c`) requiring a relog to show added stat points — now confirms immediately via the same packet the native "+" button uses, and no longer floods/disconnects on large amounts (e.g. `/a 1000`).
- **Fix**: Life/Mana/Shield/BP/LevelUpPoint no longer wrap or cap at 65000/65535 (widened `WORD`→`DWORD` across the relevant packets, server and client).
- **Fix**: widescreen "black triangle" corners on the terrain at wider-than-4:3 resolutions (e.g. 1600x900) — the terrain visibility test was always sized for a 4:3 view regardless of the real screen aspect ratio.
- **Feature**: Camera3D scroll-wheel zoom clamped to `[25, 40]` FOV in Release builds (ported from the sibling `project_mu_main52_63`).
- **Fix**: "Now Loading..." screen only filling the original 800x600 area instead of the whole window on higher resolutions.
- **Fix**: `/post` command producing garbled text (e.g. `"ÁN"`) — a `wsprintf` call was missing one of its two format arguments, reading uninitialized memory.
- **Chore**: stopped tracking `PacketList.txt` and `Source_Client/Global Release/` (runtime debug log and a 1GB+ build output, neither is source).
