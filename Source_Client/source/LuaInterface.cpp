#include "stdafx.h"
#include "LuaInterface.h"
#include "LuaStack.hpp"
#include "ZzzOpenglUtil.h"
#include "UIControls.h"
#include "NewUISystem.h"
#include "NewUIChatLogWindow.h"
#include "ZzzInterface.h"
#include "DSPlaySound.h"
#include "GlobalBitmap.h"
#include "_GlobalFunctions.h"

// Deliberately NOT ported from upstream LuaInterface.cpp, each for a
// specific reason:
//   - MousePosX/MousePosY - this project's ClientLuaFunction.cpp already
//     registers these exact Lua names (MouseX/MouseY-backed); re-registering
//     here would just silently shadow that existing, working binding.
//   - RenderImage (+ its "RenderImage2"/"RenderImageLua" upstream
//     duplicates) - this project's ClientLuaFunction.cpp already registers
//     "RenderImage" for its own LoadImage()-based texture system. Upstream's
//     version (SEASON3B::RenderImage, the client's *native* UI images by
//     BITMAP_* id) is a genuinely different, useful capability, so it's
//     ported here under a different name instead: RenderNativeImage.
//   - RenderImageScale/RenderImageScaleAuto (registered as "RenderImage3"/
//     "RenderImageAuto") - call SEASON3B::RenderImageScale/
//     RenderImageScaleAuto, which don't exist anywhere in this project.
//   - ShowDescriptionComplete/ShowItemDescription (full item tooltip
//     rendering) - both build a raw item byte buffer using
//     gItemManager.GET_ITEM(15, 0), a method this project's CItemManager
//     doesn't have (same blocker as LuaEffects.cpp, see LuaEffectsNormal.h).
//   - LockPlayerWalk/UnlockPlayerWalk - the LockPlayerWalk global this pack
//     toggles doesn't exist anywhere in this project.
//   - GetWideX - calls GetCenterX(), which doesn't exist in this project.
//   - WindowGetWidth/WidthAdd/WindowGetHeight/HeighthAdd - read a
//     GWidescreen singleton and a GetWindowsY global, neither of which
//     exist in this project (it has no such widescreen-letterboxing layer).
//   - DisableClickClient - sets gInterface.interfaceLock, which doesn't
//     exist in this project; without that lock it would just be a duplicate
//     of ResetMouseL/R/M under a different name.
//
// Two real bugs fixed along the way (upstream-only, not introduced here):
//   - GetTextPosY passed sizeof(text) (the *pointer's* size, 4 or 8) to
//     _GetTextExtentPoint32 instead of the string's length - corrected to
//     strlen(text).
//   - SetFontType's default case read a gCreateFont.m_newFont map that
//     doesn't exist in this project (no custom user-loaded fonts here) -
//     dropped, falls back to g_hFont like every other unrecognized value.
//
// Also cleaned up: upstream registers "RenderText"/"RenderText2"/
// "RenderText3" as three different names for the exact same function, and
// "RenderText4"/"RenderText5" for two more that only differ in whether a
// box width is passed. Collapsed to two clearly-named entries:
// RenderText(x,y,text,width,sort) and RenderTextSimple(x,y,text,sort).

namespace
{
	void OpenBrowser(char* link)
	{
		ShellExecute(NULL, "open", link, NULL, NULL, SW_SHOW);
	}

	void LuaglColor3f(float r, float g, float b)
	{
		glColor3f(r, g, b);
	}

	void LuaglColor4f(float r, float g, float b, float a)
	{
		glColor4f(r, g, b, a);
	}

	void LuaEnableAlphaTest()
	{
		EnableAlphaTest();
	}

	void LuaDisableAlphaBlend()
	{
		DisableAlphaBlend();
	}

	int LuaSetBlend(lua_State* L)
	{
		EnableAlphaTest(true);
		return 0;
	}

	void LuaDrawBar(float x, float y, float w, float h)
	{
		RenderColor(x, y, w, h);
	}

	void LuaEndDrawBar()
	{
		EndRenderColor();
	}

	void LuaSetFontType(int font)
	{
		switch (font)
		{
		case 0:
			g_pRenderText->SetFont(g_hFont);
			break;
		case 1:
			g_pRenderText->SetFont(g_hFontBold);
			break;
		case 2:
			g_pRenderText->SetFont(g_hFontBig);
			break;
		case 3:
			g_pRenderText->SetFont(g_hFixFont);
			break;
		default:
			g_pRenderText->SetFont(g_hFont);
			break;
		}
	}

	void LuaSetFontBg(int r, int g, int b, int a)
	{
		g_pRenderText->SetBgColor(RGBA((BYTE)r, (BYTE)g, (BYTE)b, (BYTE)a));
	}

	void LuaSetTextColor(int r, int g, int b, int a)
	{
		g_pRenderText->SetTextColor(RGBA((BYTE)r, (BYTE)g, (BYTE)b, (BYTE)a));
	}

	void LuaRenderText(int x, int y, char* text, int width, int sort)
	{
		g_pRenderText->RenderText(x, y, text, width, 0, sort);
	}

	void LuaRenderTextSimple(int x, int y, char* text, int sort)
	{
		g_pRenderText->RenderText(x, y, text, 0, 0, sort);
	}

	int LuaGetTextPosY(int font, char* text, int y, int height)
	{
		SIZE Fontsize;
		switch (font)
		{
		case 0:
			g_pRenderText->SetFont(g_hFont);
			break;
		case 1:
			g_pRenderText->SetFont(g_hFontBold);
			break;
		case 2:
			g_pRenderText->SetFont(g_hFontBig);
			break;
		case 3:
			g_pRenderText->SetFont(g_hFixFont);
			break;
		default:
			g_pRenderText->SetFont(g_hFont);
			break;
		}

		// Upstream passed sizeof(text) here - the pointer's own size (4 or
		// 8), not the string's length. Corrected to strlen(text).
		g_pMultiLanguage->_GetTextExtentPoint32(g_pRenderText->GetFontDC(), text, (int)strlen(text), &Fontsize);

		Fontsize.cy = (LONG)(Fontsize.cy / ((float)WindowHeight / 480));

		int posy = y + ((height / 2) - (Fontsize.cy / 2));

		return posy;
	}

	// The client's *native* UI images (BITMAP_* ids, already loaded at
	// startup) - distinct from this project's Lua LoadImage()/RenderImage()
	// pair (ClientLuaFunction.cpp), which loads new images from disk at
	// runtime. Registered as RenderNativeImage, not RenderImage, so it
	// can't shadow that existing binding.
	void RenderNativeImage(int imageID, float x, float y, float width, float height)
	{
		SEASON3B::RenderImage(imageID, x, y, width, height);
	}

	void LuaRenderBitmap(int Texture, float x, float y, float w, float h, float u, float v, float uWidth, float vHeight, int Scale, int StartScale, float Alpha)
	{
		RenderBitmap(Texture, x, y, w, h, u, v, uWidth, vHeight, Scale != 0, StartScale != 0, Alpha);
	}

	int LuaCheckIsRepeatKey(int Key)
	{
		return SEASON3B::IsRepeat(Key);
	}

	int LuaCheckPressedKey(int Key)
	{
		return SEASON3B::IsPress(Key);
	}

	int LuaCheckReleasedKey(int Key)
	{
		return SEASON3B::IsRelease(Key);
	}

	int LuaCheckWindowOpen(int Key)
	{
		return g_pNewUISystem->IsVisible((DWORD)Key);
	}

	void LuaCloseWindow(int Key)
	{
		g_pNewUISystem->Hide((DWORD)Key);
	}

	void LuaOpenWindow(int Key)
	{
		g_pNewUISystem->Show((DWORD)Key);
	}

	void LuaResetMouseL()
	{
		MouseLButton = false;
		MouseLButtonPop = false;
		MouseLButtonPush = false;
	}

	void LuaResetMouseR()
	{
		MouseRButton = false;
		MouseRButtonPop = false;
		MouseRButtonPush = false;
	}

	void LuaResetMouseM()
	{
		MouseMButton = false;
		MouseMButtonPop = false;
		MouseMButtonPush = false;
	}

	int LuaCheckClickClient(lua_State* L)
	{
		lua_pushnumber(L, MouseLButton);
		return 1;
	}

	float LuaGetImageWidth(int texture)
	{
		BITMAP_t* pImage = Bitmaps.GetTexture(texture);
		return pImage ? pImage->Width : 0.f;
	}

	float LuaGetImageHeight(int texture)
	{
		BITMAP_t* pImage = Bitmaps.GetTexture(texture);
		return pImage ? pImage->Height : 0.f;
	}

	// Renders a full 3D item icon preview (own camera/projection, not the
	// 2D inventory-grid icon) - e.g. for a custom "item of the day" panel.
	// Upstream calls this "CreateItem", kept as-is for consistency with the
	// rest of this port even though it doesn't create anything.
	void LuaRenderItem(float sx, float sy, float w, float h, int Type, int Level, int Option1, int ExtOption)
	{
		EndBitmap();

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glViewport2(0, 0, WindowWidth, WindowHeight);
		gluPerspective2(1.f, (float)(WindowWidth) / (float)(WindowHeight), RENDER_ITEMVIEW_NEAR, RENDER_ITEMVIEW_FAR);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		GetOpenGLMatrix(CameraMatrix);
		EnableDepthTest();
		EnableDepthMask();

		RenderItem3D(sx, sy, w, h, Type, Level * 8, Option1, ExtOption, false);

		UpdateMousePositionn();

		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();

		BeginBitmap();
	}

	void LuaSendMessageClient(char* text)
	{
		g_pChatListBox->AddText("", text, SEASON3B::TYPE_SYSTEM_MESSAGE);
	}

	void LuaDrawTooltip(int x, int y, char* text)
	{
		RenderTipText(x, y, text);
	}

	void LuaSetLockInterfaces()
	{
		ErrorMessage = 1;
	}

	void LuaSetUnlockInterfaces()
	{
		ErrorMessage = 0;
	}

	std::string LuaGetLanguage()
	{
		return g_strSelectedML;
	}

	void LuaPlaySound(int id)
	{
		PlayBuffer(id);
	}

	void LuaStopSound(int id)
	{
		StopBuffer(id, true);
	}

	int LuaGLSwitchBlend(lua_State* L)
	{
		EnableAlphaBlend();
		return 0;
	}

	int LuaGLSwitch(lua_State* L)
	{
		DisableAlphaBlend();
		return 0;
	}

}

void InitLuaInterface(lua_State* L)
{
	luaaa::LuaModule(L).fun("OpenBrowser", &OpenBrowser);
	luaaa::LuaModule(L).fun("RenderText", &LuaRenderText);
	luaaa::LuaModule(L).fun("RenderTextSimple", &LuaRenderTextSimple);
	luaaa::LuaModule(L).fun("EnableAlphaTest", &LuaEnableAlphaTest);
	luaaa::LuaModule(L).fun("DisableAlphaBlend", &LuaDisableAlphaBlend);
	luaaa::LuaModule(L).fun("glColor3f", &LuaglColor3f);
	luaaa::LuaModule(L).fun("glColor4f", &LuaglColor4f);
	luaaa::LuaModule(L).fun("DrawBar", &LuaDrawBar);
	luaaa::LuaModule(L).fun("EndDrawBar", &LuaEndDrawBar);
	luaaa::LuaModule(L).fun("SetFontType", &LuaSetFontType);
	luaaa::LuaModule(L).fun("SetTextBg", &LuaSetFontBg);
	luaaa::LuaModule(L).fun("SetTextColor", &LuaSetTextColor);
	luaaa::LuaModule(L).fun("GetTextPosY", &LuaGetTextPosY);
	luaaa::LuaModule(L).fun("PlaySound", &LuaPlaySound);
	luaaa::LuaModule(L).fun("StopSound", &LuaStopSound);
	luaaa::LuaModule(L).fun("RenderNativeImage", &RenderNativeImage);
	luaaa::LuaModule(L).fun("RenderBitmap", &LuaRenderBitmap);
	luaaa::LuaModule(L).fun("CheckRepeatKey", &LuaCheckIsRepeatKey);
	luaaa::LuaModule(L).fun("CheckPressedKey", &LuaCheckPressedKey);
	luaaa::LuaModule(L).fun("CheckReleasedKey", &LuaCheckReleasedKey);
	luaaa::LuaModule(L).fun("CheckWindowOpen", &LuaCheckWindowOpen);
	luaaa::LuaModule(L).fun("CloseWindow", &LuaCloseWindow);
	luaaa::LuaModule(L).fun("OpenWindow", &LuaOpenWindow);
	luaaa::LuaModule(L).fun("ResetMouseL", &LuaResetMouseL);
	luaaa::LuaModule(L).fun("ResetMouseR", &LuaResetMouseR);
	luaaa::LuaModule(L).fun("ResetMouseM", &LuaResetMouseM);
	luaaa::LuaModule(L).fun("GetImageWidth", &LuaGetImageWidth);
	luaaa::LuaModule(L).fun("GetImageHeight", &LuaGetImageHeight);
	luaaa::LuaModule(L).fun("SendMessageClient", &LuaSendMessageClient);
	luaaa::LuaModule(L).fun("CreateItem", &LuaRenderItem);
	luaaa::LuaModule(L).fun("DrawTooltip", &LuaDrawTooltip);
	luaaa::LuaModule(L).fun("SetLockInterfaces", &LuaSetLockInterfaces);
	luaaa::LuaModule(L).fun("SetUnlockInterfaces", &LuaSetUnlockInterfaces);
	luaaa::LuaModule(L).fun("GetLanguage", &LuaGetLanguage);

	lua_register(L, "CheckClickClient", LuaCheckClickClient);
	lua_register(L, "SetBlend", LuaSetBlend);
	lua_register(L, "GLSwitchBlend", LuaGLSwitchBlend);
	lua_register(L, "GLSwitch", LuaGLSwitch);
}
