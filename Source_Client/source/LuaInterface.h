#pragma once

// General-purpose UI toolkit for client Lua scripts: text rendering (fonts/
// colors), image/bitmap drawing, native window show/hide, keyboard/mouse
// state, a full 3D item-icon preview, sound, and chat/interface locking.
// Ported from the "RoxGaming Main 5.2 - 60 FPS UPDATE" source pack
// (source/LuaInterface.cpp/.h there) - see LuaBMD.h for the general porting
// notes. Every symbol below was verified against this project's actual
// headers (UIControls.h, ZzzOpenglUtil.h, NewUISystem.h, ZzzInterface.h,
// DSPlaySound.h, GlobalBitmap.h) before porting - see LuaInterface.cpp for
// the handful of real bugs found and fixed along the way, and what got
// left out and why.
//
// Not yet build-verified.

void InitLuaInterface(lua_State* L);
