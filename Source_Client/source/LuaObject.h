#pragma once

// Exposes a generic OBJECT's render state (alpha, blend mesh, animation
// action/timer) to client Lua, plus a couple of standalone helpers
// (worldTime, a sine-based pulsing-alpha helper). Ported from the
// "RoxGaming Main 5.2 - 60 FPS UPDATE" source pack (source/LuaObject.cpp/.h
// there) - see LuaBMD.h for the general porting notes.
//
// Not yet build-verified.

void InitLuaObject(lua_State* L);

// GetDoubleRender(period, amplitude): 0..1 pulsing value driven by the
// engine's global WorldTime clock - upstream's own name, kept as-is.
float GetDoubleRender(float a1, float a2);
