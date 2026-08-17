#pragma once

// Game-data/settings queries for client Lua: window size, font size,
// resolution, item lookup by index (name/width/height/inventory slot),
// what item is under the mouse in the inventory, current map name, party
// size, monster name by class id. Ported from the "RoxGaming Main 5.2 -
// 60 FPS UPDATE" source pack (source/LuaGlobal.cpp/.h there) - see
// LuaBMD.h and LuaInterface.h for the general porting notes; see
// LuaGlobal.cpp for what got left out and why (most notably: never port
// setLang() - it calls two hardcoded addresses from the RoxGaming binary
// itself).
//
// Not yet build-verified.

void InitLuaGlobal(lua_State* L);
