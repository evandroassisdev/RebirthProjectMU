#include "stdafx.h"
#include "ClientLuaFunction.h"
#include "NewUISystem.h"
#include "UIControls.h"
#include "Winmain.h"
#include "ZzzOpenglUtil.h"

void InitClientLuaFunction(lua_State* L) // OK
{
	lua_register(L, "MousePosX", LuaMousePosX);
	lua_register(L, "MousePosY", LuaMousePosY);
	lua_register(L, "MouseClicked", LuaMouseClicked);
	lua_register(L, "DrawText", LuaDrawText);
	lua_register(L, "ChatMessage", LuaChatMessage);
}

int LuaMousePosX(lua_State* L) // OK
{
	lua_pushinteger(L, MouseX);
	return 1;
}

int LuaMousePosY(lua_State* L) // OK
{
	lua_pushinteger(L, MouseY);
	return 1;
}

int LuaMouseClicked(lua_State* L) // OK
{
	// True only on the frame the left button went down - not held.
	lua_pushboolean(L, MouseLButtonPush ? 1 : 0);
	return 1;
}

int LuaDrawText(lua_State* L) // OK
{
	if (lua_gettop(L) != 7)
	{
		return luaL_error(L, "[7 arguments expected: x, y, text, r, g, b, a]");
	}

	int x = lua_tointeger(L, 1);
	int y = lua_tointeger(L, 2);
	const char* text = lua_tostring(L, 3);
	int r = lua_tointeger(L, 4);
	int g = lua_tointeger(L, 5);
	int b = lua_tointeger(L, 6);
	int a = lua_tointeger(L, 7);

	g_pRenderText->SetFont(g_hFont);
	g_pRenderText->SetTextColor((BYTE)r, (BYTE)g, (BYTE)b, (BYTE)a);
	g_pRenderText->SetBgColor(0, 0, 0, 0);
	g_pRenderText->RenderText(x, y, text);

	return 0;
}

int LuaChatMessage(lua_State* L) // OK
{
	if (lua_gettop(L) != 1)
	{
		return luaL_error(L, "[1 argument expected: text]");
	}

	const char* text = lua_tostring(L, 1);

	g_pChatListBox->AddText("", (char*)text, SEASON3B::TYPE_SYSTEM_MESSAGE);

	return 0;
}
