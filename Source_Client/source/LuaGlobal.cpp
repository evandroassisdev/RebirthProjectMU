#include "stdafx.h"
#include "LuaGlobal.h"
#include "LuaStack.hpp"
#include "ZzzOpenglUtil.h"
#include "Winmain.h"
#include "ZzzInterface.h"
#include "ZzzInfomation.h"
#include "NewUISystem.h"
#include "NewUIInventoryCtrl.h"
#include "MapManager.h"
#include "GlobalText.h"
#include "_define.h"

// Deliberately NOT ported from upstream LuaGlobal.cpp, each for a specific
// reason:
//   - setLang()/SetLanguage() - setLang() calls two hardcoded memory
//     addresses (0x00409B10, 0x00500E80) via inline asm, taken straight
//     from the RoxGaming binary's own layout. Those addresses mean nothing
//     in this project's own compiled executable - calling them would jump
//     into arbitrary code/data. Must never be ported as-is. It also reads/
//     writes g_aszMLSelection, a global that doesn't exist anywhere in
//     this project (this project only has g_strSelectedML, a std::string -
//     already exposed read-only as "GetLanguage" by LuaInterface.cpp).
//   - GetLanguage() - upstream's own version compares g_aszMLSelection,
//     the same nonexistent global setLang() needs. LuaInterface.cpp
//     already registers "GetLanguage" (via the real g_strSelectedML), so
//     this would be both broken and a duplicate registration.
//   - GetCompleteNameByIndex() - a few hundred lines of hardcoded
//     ItemIndex constants (8354, 7842, 0x1EB2, ...) mapped to specific
//     GlobalText[] prefix/suffix strings for that pack's own item
//     database numbering (excellent items, ancient sets, etc). Item IDs
//     like these are content data, not verifiable against this project's
//     source the way a function signature is - porting it would mean
//     trusting RoxGaming's item numbering matches this project's, which
//     there's no way to confirm here. GetNameByIndex (the plain, unamed
//     item name) is ported below instead.
//   - SetHealthBarSwitch/SetGlowSwitch/SetWingSwitch/SetFontValue - dead
//     no-op stubs upstream too (their bodies are commented-out calls into
//     a gMenuWindow settings menu that isn't part of what shipped in that
//     pack's source either), so there's nothing real to port.
//   - GetVolume() - upstream's own body is also just "return 0" (the real
//     implementation is a commented-out hardcoded-address call, same
//     category of hazard as setLang()) - not worth registering a function
//     that can only ever return one constant.
//   - SendMessageClient(std::string) - identical purpose to the
//     SendMessageClient(char*) LuaInterface.cpp already registers under
//     the same Lua name; registering it again here would just be a
//     redundant second definition of the same binding.

namespace
{
	int GetWindowWidth()
	{
		return WindowWidth;
	}

	int GetWindowHeight()
	{
		return WindowHeight;
	}

	int GetFontValue()
	{
		return FontHeight;
	}

	int GetResolution()
	{
		return m_Resolution;
	}

	void SetResolution(int value)
	{
		m_Resolution = value;
	}

	char* GetNameByIndex(int index)
	{
		auto itemAtt = &ItemAttribute[index];

		if (itemAtt != nullptr)
		{
			return itemAtt->Name;
		}

		return "not find";
	}

	int GetWidthByIndex(int index)
	{
		auto itemAtt = &ItemAttribute[index];

		if (itemAtt != nullptr)
		{
			return itemAtt->Width;
		}

		return -1;
	}

	int GetHeightByIndex(int index)
	{
		auto itemAtt = &ItemAttribute[index];

		if (itemAtt != nullptr)
		{
			return itemAtt->Height;
		}

		return -1;
	}

	int GetSlotByIndex(int index)
	{
		auto itemAtt = &ItemAttribute[index];

		if (itemAtt != nullptr)
		{
			return itemAtt->m_byItemSlot;
		}

		return -1;
	}

	int GetInventoryMouseSlot()
	{
		DWORD ItemSlot = g_pMyInventory->GetInventoryCtrl()->FindItemptIndex(MouseX, MouseY);

		if (ItemSlot != (DWORD)-1)
		{
			return (ItemSlot + MAX_EQUIPMENT);
		}

		return -1;
	}

	int GetInventoryMouseItemIndex()
	{
		auto item = g_pMyInventory->GetInventoryCtrl()->FindItemPointedSquareIndex();

		if (item)
		{
			return item->Type;
		}

		return -1;
	}

	int GetInventoryMouseItemLevel()
	{
		auto item = g_pMyInventory->GetInventoryCtrl()->FindItemPointedSquareIndex();

		if (item)
		{
			return ((item->Level >> 3) & 0xF);
		}

		return -1;
	}

	int GetInventoryMouseItemExc()
	{
		auto item = g_pMyInventory->GetInventoryCtrl()->FindItemPointedSquareIndex();

		if (item)
		{
			return item->ExtOption;
		}

		return -1;
	}

	char* GetGlobalText(int index)
	{
		return (char*)GlobalText[index];
	}

	std::string GetMapName(int map)
	{
		return gMapManager.GetMapName(map);
	}

	int GetCountParty()
	{
		return PartyNumber;
	}

	std::string GetMonsterName(int Class)
	{
		for (int i = 0; i < MAX_MONSTER; ++i)
		{
			if (MonsterScript[i].Type == Class)
			{
				return MonsterScript[i].Name;
			}
		}

		return "Not Find";
	}
}

void InitLuaGlobal(lua_State* L)
{
	luaaa::LuaModule(L).fun("GetWindowWidth", &GetWindowWidth);
	luaaa::LuaModule(L).fun("GetWindowHeight", &GetWindowHeight);
	luaaa::LuaModule(L).fun("GetFontValue", &GetFontValue);
	luaaa::LuaModule(L).fun("GetResolution", &GetResolution);
	luaaa::LuaModule(L).fun("SetResolution", &SetResolution);

	luaaa::LuaModule(L).fun("GetNameByIndex", &GetNameByIndex);
	luaaa::LuaModule(L).fun("GetWidthByIndex", &GetWidthByIndex);
	luaaa::LuaModule(L).fun("GetHeightByIndex", &GetHeightByIndex);
	luaaa::LuaModule(L).fun("GetSlotByIndex", &GetSlotByIndex);

	luaaa::LuaModule(L).fun("GetInventoryMouseSlot", &GetInventoryMouseSlot);
	luaaa::LuaModule(L).fun("GetInventoryMouseItemIndex", &GetInventoryMouseItemIndex);
	luaaa::LuaModule(L).fun("GetInventoryMouseItemLevel", &GetInventoryMouseItemLevel);
	luaaa::LuaModule(L).fun("GetInventoryMouseItemExc", &GetInventoryMouseItemExc);

	luaaa::LuaModule(L).fun("GetGlobalText", &GetGlobalText);

	luaaa::LuaModule(L).fun("GetMapName", &GetMapName);

	luaaa::LuaModule(L).fun("GetCountParty", &GetCountParty);

	luaaa::LuaModule(L).fun("GetMonsterName", &GetMonsterName);
}
