#pragma once

// Phase 1: a minimal native binding surface for the client-side Lua engine,
// just enough to prove the whole path (script -> render + click) works.
// Expand this alongside ScriptCore.lua as real features need more.

void InitClientLuaFunction(lua_State* L);

int LuaMousePosX(lua_State* L);
int LuaMousePosY(lua_State* L);
int LuaMouseClicked(lua_State* L);
int LuaDrawText(lua_State* L);
int LuaChatMessage(lua_State* L);
int LuaSendCommand(lua_State* L);
int LuaIsWarehouseOpen(lua_State* L);
int LuaIsInventoryOpen(lua_State* L);
int LuaGetInventoryPos(lua_State* L);
int LuaDrawPanel(lua_State* L);
int LuaConsumeClick(lua_State* L);
int LuaGetTickCount(lua_State* L);
int LuaGetChatTime(lua_State* L);
int LuaLoadImage(lua_State* L);
int LuaRenderImage(lua_State* L);
