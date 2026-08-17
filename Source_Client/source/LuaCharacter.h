#pragma once

// Exposes other players' CHARACTER entries (the CharactersClient[] array -
// nearby players the client currently has spawned) to client Lua scripts:
// name/level/class/guild, equipped-item model ids, position/angle, and a
// few outgoing requests (trade/party/guild/shop). Ported from the
// "RoxGaming Main 5.2 - 60 FPS UPDATE" source pack (source/LuaCharacter.cpp/
// .h there) - see LuaBMD.h for the general porting notes (luaaa binding
// helper, why this project's classes matched closely enough to port
// near-verbatim).
//
// Two real fixes made during the port, not present upstream:
//   1. SendRequestTrade(p_Key) is a 1-argument macro in this project's
//      wsclientinline.h (upstream calls it with a second "0" argument that
//      doesn't exist here).
//   2. SendRequestOpenPersonalShop's second argument (the shop title) was
//      upstream reading the same Lua stack slot as the first (arg 1 twice) -
//      corrected to read arg 2.
//
// Not yet build-verified in-game.

#include "_define.h"
#include "w_CharacterInfo.h"
#include "ZzzCharacter.h"
#include "ZzzOpenglUtil.h"
#include "ZzzAI.h"
#include "ZzzInterface.h"
#include "CharacterManager.h"

void InitLuaCharacter(lua_State* L);
