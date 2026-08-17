#pragma once

// Exposes the local player (Hero) to client Lua: name/level/class/guild,
// equipped-item model ids, position, current map. Mirrors LuaCharacter.h
// (which does the same for other nearby players, CharactersClient[]) but
// reads the single global Hero pointer instead of indexing an array.
// Ported from the "RoxGaming Main 5.2 - 60 FPS UPDATE" source pack
// (source/LuaUser.cpp/.h there) - see LuaBMD.h and LuaCharacter.h for the
// general porting notes; every symbol here was already verified while
// porting LuaCharacter.
//
// Not yet build-verified.

#include "_define.h"
#include "w_CharacterInfo.h"
#include "ZzzCharacter.h"
#include "MapManager.h"
#include "GuildCache.h"

void InitLuaUser(lua_State* L);
