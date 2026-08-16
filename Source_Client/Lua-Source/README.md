# Client Lua source

Editable `.lua` source for the client-side scripting engine (see
`Source_Client/source/ClientScriptLoader.cpp`). This folder is **not**
deployed - it's kept here only so the scripts have readable, diffable
source in git.

The deployed client (`Global Debug/Lua/`, mirroring this folder's
structure) ships only the **encrypted** `.enc` build of each script, so
players can't just open them in Notepad. It's the same tier of protection
the rest of this client already uses for its `.bmd` data files - enough to
stop casual copying, not a determined reverse engineer.

## Workflow

1. Edit the `.lua` file(s) here.
2. Encrypt: `python encrypt_lua.py ScriptMain.lua System\ScriptCore.lua TestButton.lua`
   (writes a matching `.enc` next to each input file).
3. Copy the resulting `.enc` file(s) into `Global Debug/Lua/`, preserving
   the same relative path (e.g. `System\ScriptCore.enc` stays under
   `Global Debug/Lua/System/`).

`ClientScriptLoader` looks for `<name>.enc` first and only falls back to
`<name>.lua` if no `.enc` exists - so for quick local iteration you can
also drop a plain `.lua` straight into `Global Debug/Lua/` and it'll load
directly, no encryption step needed. Just don't ship that copy.
