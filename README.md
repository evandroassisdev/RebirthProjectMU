# RebirthProjectMU

MU Online client + server source (Season 5.2~6.3), `Source_Client` (C++/OpenGL) and `Source_Server` (DataServer, JoinServer, ConnectServer, GameServer).

## Changelog

### 2026-08-14
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
