#include "stdafx.h"
#include "ClientLuaFunction.h"
#include "NewUISystem.h"
#include "UIControls.h"
#include "Winmain.h"
#include "ZzzOpenglUtil.h"
#include "wsclientinline.h"

void InitClientLuaFunction(lua_State* L) // OK
{
	lua_register(L, "MousePosX", LuaMousePosX);
	lua_register(L, "MousePosY", LuaMousePosY);
	lua_register(L, "MouseClicked", LuaMouseClicked);
	lua_register(L, "DrawText", LuaDrawText);
	lua_register(L, "ChatMessage", LuaChatMessage);
	lua_register(L, "SendCommand", LuaSendCommand);
	lua_register(L, "IsWarehouseOpen", LuaIsWarehouseOpen);
	lua_register(L, "DrawPanel", LuaDrawPanel);
	lua_register(L, "ConsumeClick", LuaConsumeClick);
	lua_register(L, "GetTickCount", LuaGetTickCount);
	lua_register(L, "GetChatTime", LuaGetChatTime);
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

int LuaSendCommand(lua_State* L) // OK
{
	if (lua_gettop(L) != 1)
	{
		return luaL_error(L, "[1 argument expected: text]");
	}

	const char* text = lua_tostring(L, 1);

	// Same path the chat input box uses when the player presses Enter -
	// handles plain chat and slash commands (e.g. "/bau 2") the same way,
	// including the normal spam-cooldown.
	SendChat(text);

	return 0;
}

int LuaIsWarehouseOpen(lua_State* L) // OK
{
	lua_pushboolean(L, g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_STORAGE) ? 1 : 0);
	return 1;
}

int LuaDrawPanel(lua_State* L) // OK
{
	if (lua_gettop(L) != 8)
	{
		return luaL_error(L, "[8 arguments expected: x, y, w, h, r, g, b, a]");
	}

	float x = (float)lua_tonumber(L, 1);
	float y = (float)lua_tonumber(L, 2);
	float w = (float)lua_tonumber(L, 3);
	float h = (float)lua_tonumber(L, 4);
	float r = (float)lua_tonumber(L, 5) / 255.f;
	float g = (float)lua_tonumber(L, 6) / 255.f;
	float b = (float)lua_tonumber(L, 7) / 255.f;
	float a = (float)lua_tonumber(L, 8) / 255.f;

	// Flat solid rectangle, no texture needed - same primitive
	// NewUIMuHelper's own panel background uses.
	glColor4f(r, g, b, a);
	RenderColor(x, y, w, h, 0.0f, 0);
	EndRenderColor();

	return 0;
}

int LuaConsumeClick(lua_State* L) // OK
{
	// Same trick every native window in this client uses to stop a click
	// from falling through to the world underneath it - see e.g.
	// NewUIMyInventory.cpp. Both flags matter: Attack()/movement in
	// ZzzInterface.cpp checks "MouseLButtonPush || MouseLButton", and
	// MouseLButtonPush stays true for the whole time the button is held
	// (only WM_LBUTTONUP clears it), so clearing just MouseLButton isn't
	// enough on its own.
	MouseLButton = false;
	MouseLButtonPush = false;

	return 0;
}

int LuaGetTickCount(lua_State* L) // OK
{
	lua_pushnumber(L, (double)GetTickCount());
	return 1;
}

int LuaGetChatTime(lua_State* L) // OK
{
	// SendChat() drops anything sent while this is above 50 (see
	// wsclientinline.h). Polling it directly is more precise than guessing
	// a fixed wait - fires the instant it's actually safe to send again.
	lua_pushinteger(L, ChatTime);
	return 1;
}
