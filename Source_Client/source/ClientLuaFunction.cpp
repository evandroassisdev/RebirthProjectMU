#include "stdafx.h"
#include "ClientLuaFunction.h"
#include "NewUISystem.h"
#include "UIControls.h"
#include "Winmain.h"
#include "ZzzOpenglUtil.h"
#include "GlobalBitmap.h"
#include "wsclientinline.h"
#include "./Utilities/Log/muConsoleDebug.h"

void InitClientLuaFunction(lua_State* L) // OK
{
	lua_register(L, "MousePosX", LuaMousePosX);
	lua_register(L, "MousePosY", LuaMousePosY);
	lua_register(L, "MouseClicked", LuaMouseClicked);
	lua_register(L, "DrawText", LuaDrawText);
	lua_register(L, "DrawTextCentered", LuaDrawTextCentered);
	lua_register(L, "DrawTextBigCentered", LuaDrawTextBigCentered);
	lua_register(L, "ChatMessage", LuaChatMessage);
	lua_register(L, "SendCommand", LuaSendCommand);
	lua_register(L, "IsWarehouseOpen", LuaIsWarehouseOpen);
	lua_register(L, "IsInventoryOpen", LuaIsInventoryOpen);
	lua_register(L, "GetInventoryPos", LuaGetInventoryPos);
	lua_register(L, "DrawPanel", LuaDrawPanel);
	lua_register(L, "ConsumeClick", LuaConsumeClick);
	lua_register(L, "GetTickCount", LuaGetTickCount);
	lua_register(L, "GetChatTime", LuaGetChatTime);
	lua_register(L, "LoadImage", LuaLoadImage);
	lua_register(L, "RenderImage", LuaRenderImage);
	lua_register(L, "ScreenWidth", LuaGetScreenWidth);
	lua_register(L, "ScreenHeight", LuaGetScreenHeight);
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

int LuaDrawTextCentered(lua_State* L) // OK
{
	// Same as DrawText(), but centered inside a box of width w starting at
	// x - uses the engine's own RT3_SORT_CENTER layout (RenderText()'s
	// iBoxWidth/iSort params, UIControls.h) instead of a guessed
	// char-width heuristic, so it's exact for any font/string.
	if (lua_gettop(L) != 8)
	{
		return luaL_error(L, "[8 arguments expected: x, y, w, text, r, g, b, a]");
	}

	int x = lua_tointeger(L, 1);
	int y = lua_tointeger(L, 2);
	int w = lua_tointeger(L, 3);
	const char* text = lua_tostring(L, 4);
	int r = lua_tointeger(L, 5);
	int g = lua_tointeger(L, 6);
	int b = lua_tointeger(L, 7);
	int a = lua_tointeger(L, 8);

	g_pRenderText->SetFont(g_hFont);
	g_pRenderText->SetTextColor((BYTE)r, (BYTE)g, (BYTE)b, (BYTE)a);
	g_pRenderText->SetBgColor(0, 0, 0, 0);
	g_pRenderText->RenderText(x, y, text, w, 0, RT3_SORT_CENTER);

	return 0;
}

int LuaDrawTextBigCentered(lua_State* L) // OK
{
	// Same as DrawTextCentered(), but uses g_hFontBig (Winmain.cpp - same
	// Tahoma family, bold, 2x the normal g_hFont point size) instead of the
	// regular UI font - for callers that want a few standout numbers/labels
	// (e.g. a big selectable digit grid) without needing a whole new font
	// resource.
	if (lua_gettop(L) != 8)
	{
		return luaL_error(L, "[8 arguments expected: x, y, w, text, r, g, b, a]");
	}

	int x = lua_tointeger(L, 1);
	int y = lua_tointeger(L, 2);
	int w = lua_tointeger(L, 3);
	const char* text = lua_tostring(L, 4);
	int r = lua_tointeger(L, 5);
	int g = lua_tointeger(L, 6);
	int b = lua_tointeger(L, 7);
	int a = lua_tointeger(L, 8);

	g_pRenderText->SetFont(g_hFontBig);
	g_pRenderText->SetTextColor((BYTE)r, (BYTE)g, (BYTE)b, (BYTE)a);
	g_pRenderText->SetBgColor(0, 0, 0, 0);
	g_pRenderText->RenderText(x, y, text, w, 0, RT3_SORT_CENTER);

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

int LuaIsInventoryOpen(lua_State* L) // OK
{
	lua_pushboolean(L, g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INVENTORY) ? 1 : 0);
	return 1;
}

int LuaGetInventoryPos(lua_State* L) // OK
{
	if (g_pMyInventory == NULL)
	{
		lua_pushinteger(L, 0);
		lua_pushinteger(L, 0);
		return 2;
	}

	// Window can be dragged - has to be read live, not cached.
	const POINT& pos = g_pMyInventory->GetPos();

	lua_pushinteger(L, pos.x);
	lua_pushinteger(L, pos.y);
	return 2;
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

namespace
{
	// LoadImage() fills this in per texture so RenderImage() never needs the
	// caller to know or compute the real-size-vs-padded-size UV crop itself
	// (see the comment on PeekImageSize below for why the crop exists at
	// all). Only textures loaded through our own LoadImage() get an entry;
	// RenderImage() defaults to the uncropped full texture (1, 1) for
	// anything else, same as before this existed.
	struct ImageCrop { float uWidth; float vHeight; };
	std::map<GLuint, ImageCrop> s_ImageCrop;

	int NextPow2(int n)
	{
		int w = 1;
		while (w < n)
		{
			w <<= 1;
		}
		return w;
	}

	// Struct my_error_mgr/my_error_exit in GlobalBitmap.cpp is private to
	// CGlobalBitmap - this is our own copy of the same setjmp-based libjpeg
	// error trap (the default handler calls exit() on a malformed file,
	// which would silently kill the whole client).
	struct JpegErrorMgr
	{
		struct jpeg_error_mgr pub;
		jmp_buf setjmpBuffer;
	};

	void JpegErrorExit(j_common_ptr cinfo)
	{
		JpegErrorMgr* err = (JpegErrorMgr*)cinfo->err;
		longjmp(err->setjmpBuffer, 1);
	}

	// Reads just the real (pre-padding) pixel width/height straight from
	// disk - the same file Bitmaps.LoadImage() itself opens (ExchangeExt
	// swaps the caller's "foo.jpg"/"foo.tga" path to the real "foo.OZJ"/
	// "foo.OZT" on disk), but this only reads the header, not the pixels.
	// CGlobalBitmap doesn't keep the pre-padding size anywhere once a
	// texture is loaded (OpenJpeg/OpenTga only store the padded Width/
	// Height), so this is the only way to recover it after the fact.
	bool PeekImageSize(const char* fullPath, int& nx, int& ny)
	{
		const char* ext = strrchr(fullPath, '.');
		if (ext == NULL)
		{
			return false;
		}

		char realPath[256];
		strncpy(realPath, fullPath, ext - fullPath);
		realPath[ext - fullPath] = '\0';

		if (_stricmp(ext, ".tga") == 0)
		{
			strcat(realPath, ".OZT");

			FILE* fp = fopen(realPath, "rb");
			if (fp == NULL)
			{
				return false;
			}

			BYTE header[22];
			bool ok = fread(header, 1, sizeof(header), fp) == sizeof(header);
			fclose(fp);

			if (!ok)
			{
				return false;
			}

			nx = *(short*)(header + 16);
			ny = *(short*)(header + 18);
			return true;
		}

		if (_stricmp(ext, ".jpg") == 0)
		{
			strcat(realPath, ".OZJ");

			FILE* fp = fopen(realPath, "rb");
			if (fp == NULL)
			{
				return false;
			}

			fseek(fp, 24, SEEK_SET); // same header skip OpenJpeg() uses

			struct jpeg_decompress_struct cinfo;
			JpegErrorMgr jerr;
			cinfo.err = jpeg_std_error(&jerr.pub);
			jerr.pub.error_exit = JpegErrorExit;

			if (setjmp(jerr.setjmpBuffer))
			{
				jpeg_destroy_decompress(&cinfo);
				fclose(fp);
				return false;
			}

			jpeg_create_decompress(&cinfo);
			jpeg_stdio_src(&cinfo, fp);
			jpeg_read_header(&cinfo, TRUE);

			nx = cinfo.image_width;
			ny = cinfo.image_height;

			jpeg_destroy_decompress(&cinfo);
			fclose(fp);
			return true;
		}

		return false;
	}
}

int LuaLoadImage(lua_State* L) // OK
{
	if (lua_gettop(L) != 1)
	{
		return luaL_error(L, "[1 argument expected: path]");
	}

	const char* path = lua_tostring(L, 1);

	char basePath[256] = { 0 };
	strcpy(basePath, "Data\\");
	strcat(basePath, path);

	// Whatever extension the caller passed (if any) is irrelevant - the
	// real file on disk is always .OZJ or .OZT, never literally ".jpg"/
	// ".tga", so there's nothing meaningful to keep from it. Drop it and
	// try both real formats ourselves - the script shouldn't have to know
	// which one a given asset happens to be.
	char* dot = strrchr(basePath, '.');
	if (dot != NULL)
	{
		*dot = '\0';
	}

	char jpgPath[256];
	char tgaPath[256];
	sprintf(jpgPath, "%s.jpg", basePath);
	sprintf(tgaPath, "%s.tga", basePath);

	// Auto-allocating overload (same one LoadData.cpp uses for arbitrary
	// model textures at runtime) - returns BITMAP_UNKNOWN on failure, NOT 0
	// (_TextureIndex.h: BITMAP_UNKNOWN = 30000) - silently, no MessageBox
	// unlike the LoadBitmap() free function, so trying the second format
	// after a failed first attempt is safe.
	GLuint slot = Bitmaps.LoadImage(jpgPath, GL_LINEAR, GL_CLAMP_TO_EDGE);
	const char* loadedPath = jpgPath;

	if (slot == BITMAP_UNKNOWN)
	{
		slot = Bitmaps.LoadImage(tgaPath, GL_LINEAR, GL_CLAMP_TO_EDGE);
		loadedPath = tgaPath;
	}

	float uWidth = 1.f;
	float vHeight = 1.f;

	if (slot == BITMAP_UNKNOWN)
	{
		slot = 0; // 0, not BITMAP_UNKNOWN, is "failed" on the Lua side
	}
	else
	{
		int nx, ny;
		if (PeekImageSize(loadedPath, nx, ny))
		{
			uWidth = (float)nx / (float)NextPow2(nx);
			vHeight = (float)ny / (float)NextPow2(ny);

			ImageCrop crop;
			crop.uWidth = uWidth;
			crop.vHeight = vHeight;
			s_ImageCrop[slot] = crop;
		}
	}

	// uWidth/vHeight are informational (RenderImage() already applies the
	// crop automatically) - handy for a script to log/debug, but most
	// callers only need the id.
	lua_pushinteger(L, slot);
	lua_pushnumber(L, uWidth);
	lua_pushnumber(L, vHeight);
	return 3;
}

int LuaRenderImage(lua_State* L) // OK
{
	int argc = lua_gettop(L);

	if (argc != 5 && argc != 7)
	{
		return luaL_error(L, "[5 or 7 arguments expected: textureId, x, y, w, h [, uWidth, vHeight]]");
	}

	int textureId = lua_tointeger(L, 1);
	float x = (float)lua_tonumber(L, 2);
	float y = (float)lua_tonumber(L, 3);
	float w = (float)lua_tonumber(L, 4);
	float h = (float)lua_tonumber(L, 5);

	// uWidth/vHeight crop the UV rect to the image's real size instead of
	// the full 0..1 texture - same reason ZzzOpenData.cpp's own button
	// icons pass e.g. "24.f/32.f": non-power-of-2 source art gets padded up
	// to the next power of 2 at load time, and that padding is
	// uninitialized memory, not transparent - drawing the full 0..1 UV
	// shows that garbage around/behind the icon. An explicit 7th/8th
	// argument overrides; otherwise this defaults to whatever LoadImage()
	// measured for this texture (or the full 1, 1 if it wasn't loaded
	// through LoadImage()).
	float uWidth = 1.f;
	float vHeight = 1.f;

	if (argc == 7)
	{
		uWidth = (float)lua_tonumber(L, 6);
		vHeight = (float)lua_tonumber(L, 7);
	}
	else
	{
		std::map<GLuint, ImageCrop>::iterator it = s_ImageCrop.find((GLuint)textureId);
		if (it != s_ImageCrop.end())
		{
			uWidth = it->second.uWidth;
			vHeight = it->second.vHeight;
		}
	}

	// RenderBitmap() (ZzzOpenglUtil.cpp) never touches GL_BLEND itself - a
	// texture's alpha channel is silently ignored (drawn fully opaque)
	// unless something upstream already left blending enabled. A prior
	// session's commit message (a870875) claimed this was already wrapped
	// here, but the actual diff never added it - confirmed by grepping this
	// file for EnableAlphaTest/DisableAlphaBlend before this fix, both
	// absent. Symptom this caused: any texture with real transparency (a
	// rounded corner's outside-the-curve area, etc.) rendered its "should
	// be see-through" pixels using their raw RGB instead - invisible for
	// op1_back1.OZT-style native assets (their transparent areas happen to
	// already be near-black, coincidentally close to the dark backdrop they
	// sit over), but glaringly wrong for Panel9's corner art (transparent
	// area is (0,0,0,0) - solid black square instead of the game world
	// showing through). Same EnableAlphaTest()/DisableAlphaBlend() pair
	// every native textured-UI Render() in this codebase already wraps
	// itself in (e.g. NewUIWindowMenu.cpp::Render()).
	EnableAlphaTest();
	RenderBitmap(textureId, x, y, w, h, 0.f, 0.f, uWidth, vHeight);
	DisableAlphaBlend();

	return 0;
}

// RenderImage()/DrawPanel()/DrawText() (this file) all go through
// RenderBitmap()/RenderColor() (ZzzOpenglUtil.cpp), which unconditionally
// scale every x/y/w/h via ConvertX/Y ("x * WindowWidth / 640.f") - i.e. the
// whole Lua rendering API works in a fixed 640x480 virtual canvas,
// auto-stretched to the real resolution. MousePosX()/MousePosY() are
// already converted back into that same 640x480 space for the same reason
// (Winmain.cpp: "LOWORD(lParam) / g_fScreenRate_x"). These two return that
// canvas size as fixed constants (not a real screen query - CInput::
// GetScreenWidth()/Height() is real, unscaled pixels, a DIFFERENT space
// used only by the native CWin/CSprite window system) so a script's own
// centering math ((ScreenWidth() - w) / 2) lines up with where
// RenderImage()/DrawPanel() actually draw, at any real resolution.
int LuaGetScreenWidth(lua_State* L) // OK
{
	lua_pushinteger(L, 640);
	return 1;
}

int LuaGetScreenHeight(lua_State* L) // OK
{
	lua_pushinteger(L, 480);
	return 1;
}
