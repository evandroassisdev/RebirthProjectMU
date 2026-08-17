#include "stdafx.h"
#include "LuaCharacter.h"
#include "LuaStack.hpp"
#include "wsclientinline.h"
#include "GuildCache.h"

namespace
{
	// NOTE (carried over from upstream, not introduced by this port): on an
	// out-of-range arrayIndex, these push nothing but still return 1 -
	// scripts get whatever was already on top of the Lua stack instead of a
	// clean nil. Same risk as upstream; callers are expected to pass a valid
	// index (e.g. from a CharacterGetIndex loop), same as there.

	int LuaCharacterGetName(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			lua_pushstring(L, lpObj->ID);
		}
		return 1;
	}

	int LuaCharacterGetKey(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			lua_pushnumber(L, lpObj->Key);
		}
		return 1;
	}

	int LuaCharacterGetIsLive(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			lua_pushnumber(L, lpObj->Object.Live);
		}
		return 1;
	}

	int LuaCharacterGetType(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			lua_pushnumber(L, lpObj->Object.Kind);
		}
		return 1;
	}

	int LuaCharacterX(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			DWORD PosX = (DWORD)(lpObj->Object.Position[0] / 100.f);
			lua_pushnumber(L, (DWORD)(PosX > 0 ? PosX : 0));
		}
		return 1;
	}

	int LuaCharacterY(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			DWORD PosY = (DWORD)(lpObj->Object.Position[1] / 100.f);
			lua_pushnumber(L, (DWORD)(PosY > 0 ? PosY : 0));
		}
		return 1;
	}

	int LuaCharacterPositionX(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			lua_pushnumber(L, (float)lpObj->Object.Position[0]);
		}
		return 1;
	}

	int LuaCharacterPositionY(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			lua_pushnumber(L, (float)lpObj->Object.Position[1]);
		}
		return 1;
	}

	int LuaCharacterPositionZ(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			lua_pushnumber(L, (float)lpObj->Object.Position[2]);
		}
		return 1;
	}

	int LuaCharacterGetHelper(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			lua_pushnumber(L, (lpObj->Helper.Type != (WORD)-1 ? (lpObj->Helper.Type - MODEL_ITEM) : -1));
		}
		return 1;
	}

	int LuaCharacterGetWing(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			lua_pushnumber(L, (lpObj->Wing.Type != (WORD)-1 ? (lpObj->Wing.Type - MODEL_ITEM) : -1));
		}
		return 1;
	}

	int LuaCharacterGetSword(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			lua_pushnumber(L, (lpObj->Weapon[0].Type != (WORD)-1 ? (lpObj->Weapon[0].Type - MODEL_ITEM) : -1));
		}
		return 1;
	}

	int LuaCharacterGetShield(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			lua_pushnumber(L, (lpObj->Weapon[1].Type != (WORD)-1 ? (lpObj->Weapon[1].Type - MODEL_ITEM) : -1));
		}
		return 1;
	}

	int LuaCharacterGetHelm(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			lua_pushnumber(L, (lpObj->BodyPart[BODYPART_HELM].Type != (WORD)-1 ? (lpObj->BodyPart[BODYPART_HELM].Type - MODEL_ITEM) : -1));
		}
		return 1;
	}

	int LuaCharacterGetArmor(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			lua_pushnumber(L, (lpObj->BodyPart[BODYPART_ARMOR].Type != (WORD)-1 ? (lpObj->BodyPart[BODYPART_ARMOR].Type - MODEL_ITEM) : -1));
		}
		return 1;
	}

	int LuaCharacterGetPants(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			lua_pushnumber(L, (lpObj->BodyPart[BODYPART_PANTS].Type != (WORD)-1 ? (lpObj->BodyPart[BODYPART_PANTS].Type - MODEL_ITEM) : -1));
		}
		return 1;
	}

	int LuaCharacterGetGloves(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			lua_pushnumber(L, (lpObj->BodyPart[BODYPART_GLOVES].Type != (WORD)-1 ? (lpObj->BodyPart[BODYPART_GLOVES].Type - MODEL_ITEM) : -1));
		}
		return 1;
	}

	int LuaCharacterGetBoots(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			lua_pushnumber(L, (lpObj->BodyPart[BODYPART_BOOTS].Type != (WORD)-1 ? (lpObj->BodyPart[BODYPART_BOOTS].Type - MODEL_ITEM) : -1));
		}
		return 1;
	}

	int LuaCharacterGetLevel(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			lua_pushnumber(L, lpObj->Level);
		}
		return 1;
	}

	int LuaCharacterGetClass(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];
			lua_pushnumber(L, (lpObj->Class & 7));
		}
		return 1;
	}

	int LuaCharacterGetGuild(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];

			if (lpObj->GuildMarkIndex != (WORD)-1)
			{
				lua_pushnumber(L, lpObj->GuildMarkIndex);
			}
			else
			{
				lua_pushnumber(L, -1);
			}
		}
		return 1;
	}

	int LuaCharacterGuildGetName(lua_State* L)
	{
		int arrayIndex = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			auto lpObj = &CharactersClient[arrayIndex];

			if (lpObj->GuildMarkIndex != (WORD)-1)
			{
				char* gname = GuildMark[lpObj->GuildMarkIndex].GuildName;
				lua_pushstring(L, strlen(gname) <= 0 ? " " : gname);
			}
			else
			{
				lua_pushstring(L, " ");
			}
		}
		return 1;
	}

	int LuaGetPosFromPlayer(lua_State* L)
	{
		vec3_t Position;

		int arrayIndex = (int)lua_tointeger(L, 1);
		float Height = (float)lua_tonumber(L, 2);
		lua_pop(L, 2);

		if (arrayIndex < 0 || arrayIndex > 400)
		{
			return 0;
		}

		auto lpObj = &CharactersClient[arrayIndex];

		VectorCopy(lpObj->Object.Position, Position);
		Position[2] = Position[2] + lpObj->Object.BoundingBoxMax[2] + Height;

		int GetPosX, GetPosY;
		Projection(Position, &GetPosX, &GetPosY);

		lua_pushnumber(L, GetPosX);
		lua_pushnumber(L, GetPosY);
		return 2;
	}

	int LuaSendPlayerChat(lua_State* L)
	{
		SendChat((char*)lua_tostring(L, 1));
		return 0;
	}

	int LuaGetFindCharacterIndex(lua_State* L)
	{
		int index = (int)lua_tointeger(L, 1);
		lua_pop(L, 1);

		lua_pushnumber(L, FindCharacterIndex(index));
		return 1;
	}

	int LuaGetTargetCharacter(lua_State* L)
	{
		lua_pushnumber(L, (SelectedCharacter == -1 ? -1 : SelectedCharacter));
		return 1;
	}

	int LuaSendTradePlayer(lua_State* L)
	{
		// Upstream calls this with a second "0" argument that this
		// project's SendRequestTrade() (wsclientinline.h) doesn't take -
		// it's a 1-argument macro here.
		SendRequestTrade((int)lua_tointeger(L, 1));
		lua_pop(L, 1);
		return 0;
	}

	int LuaSendPartyPlayer(lua_State* L)
	{
		SendRequestParty((int)lua_tointeger(L, 1));
		lua_pop(L, 1);
		return 0;
	}

	int LuaSendGuildPlayer(lua_State* L)
	{
		SendRequestGuild((int)lua_tointeger(L, 1));
		lua_pop(L, 1);
		return 0;
	}

	int LuaSendLojaPlayer(lua_State* L)
	{
		// Upstream reads Lua stack arg 1 for both the index and the shop
		// title, i.e. it never actually reads the title the caller passed
		// in arg 2 - corrected here.
		SendRequestOpenPersonalShop((int)lua_tointeger(L, 1), (char*)lua_tostring(L, 2));
		lua_pop(L, 2);
		return 0;
	}

	float CharacterGetPositionX(int arrayIndex)
	{
		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			return CharactersClient[arrayIndex].Object.Position[0];
		}
		return 0.0f;
	}

	void CharacterSetPositionX(int arrayIndex, float value)
	{
		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			CharactersClient[arrayIndex].Object.Position[0] = value;
		}
	}

	float CharacterGetPositionY(int arrayIndex)
	{
		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			return CharactersClient[arrayIndex].Object.Position[1];
		}
		return 0.0f;
	}

	void CharacterSetPositionY(int arrayIndex, float value)
	{
		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			CharactersClient[arrayIndex].Object.Position[1] = value;
		}
	}

	float CharacterGetPositionZ(int arrayIndex)
	{
		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			return CharactersClient[arrayIndex].Object.Position[2];
		}
		return 0.0f;
	}

	void CharacterSetPositionZ(int arrayIndex, float value)
	{
		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			CharactersClient[arrayIndex].Object.Position[2] = value;
		}
	}

	float CharacterGetAngleX(int arrayIndex)
	{
		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			return CharactersClient[arrayIndex].Object.Angle[0];
		}
		return 0.0f;
	}

	void CharacterSetAngleX(int arrayIndex, float value)
	{
		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			CharactersClient[arrayIndex].Object.Angle[0] = value;
		}
	}

	float CharacterGetAngleY(int arrayIndex)
	{
		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			return CharactersClient[arrayIndex].Object.Angle[1];
		}
		return 0.0f;
	}

	void CharacterSetAngleY(int arrayIndex, float value)
	{
		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			CharactersClient[arrayIndex].Object.Angle[1] = value;
		}
	}

	float CharacterGetAngleZ(int arrayIndex)
	{
		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			return CharactersClient[arrayIndex].Object.Angle[2];
		}
		return 0.0f;
	}

	void CharacterSetAngleZ(int arrayIndex, float value)
	{
		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			CharactersClient[arrayIndex].Object.Angle[2] = value;
		}
	}

	void CharacterSetScale(int arrayIndex, float value)
	{
		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			CharactersClient[arrayIndex].Object.Scale = value;
		}
	}

	int GetSelectedHero()
	{
		return SelectedHero;
	}

	void SetSelectedHero(int value)
	{
		SelectedHero = value;
	}

	void SetTargetCharacter(int value)
	{
		SelectedCharacter = value;
	}

	char* GetClassCharacterName(int Class)
	{
		return (char*)gCharacterManager.GetCharacterClassText((BYTE)Class);
	}

	int CharacterGetGuildStatus(int arrayIndex)
	{
		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			return CharactersClient[arrayIndex].GuildStatus;
		}
		return 255;
	}

	int CharacterGetVisible(int arrayIndex)
	{
		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			return CharactersClient[arrayIndex].Object.Visible;
		}
		return 0;
	}

	void SetCharacterAction(int arrayIndex, int animation)
	{
		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			SetAction(&CharactersClient[arrayIndex].Object, animation);
		}
	}

	int CharacterGetFullClass(int arrayIndex)
	{
		if (arrayIndex >= 0 && arrayIndex < MAX_CHARACTERS_CLIENT)
		{
			return CharactersClient[arrayIndex].Class;
		}
		return 0;
	}
}

void InitLuaCharacter(lua_State* L)
{
	lua_register(L, "CharacterGetName", LuaCharacterGetName);
	lua_register(L, "CharacterGetIndex", LuaCharacterGetKey);
	lua_register(L, "CharacterGetIsLive", LuaCharacterGetIsLive);
	lua_register(L, "CharacterGetType", LuaCharacterGetType);
	lua_register(L, "CharacterGetX", LuaCharacterX);
	lua_register(L, "CharacterGetY", LuaCharacterY);
	lua_register(L, "CharacterGetPositionX", LuaCharacterPositionX);
	lua_register(L, "CharacterGetPositionY", LuaCharacterPositionY);
	lua_register(L, "CharacterGetPositionZ", LuaCharacterPositionZ);
	lua_register(L, "CharacterGetHelper", LuaCharacterGetHelper);
	lua_register(L, "CharacterGetWing", LuaCharacterGetWing);
	lua_register(L, "CharacterGetSword", LuaCharacterGetSword);
	lua_register(L, "CharacterGetShield", LuaCharacterGetShield);
	lua_register(L, "CharacterGetHelm", LuaCharacterGetHelm);
	lua_register(L, "CharacterGetArmor", LuaCharacterGetArmor);
	lua_register(L, "CharacterGetPants", LuaCharacterGetPants);
	lua_register(L, "CharacterGetGloves", LuaCharacterGetGloves);
	lua_register(L, "CharacterGetBoots", LuaCharacterGetBoots);
	lua_register(L, "CharacterGetLevel", LuaCharacterGetLevel);
	lua_register(L, "CharacterGetClass", LuaCharacterGetClass);
	lua_register(L, "CharacterGetGuild", LuaCharacterGetGuild);
	lua_register(L, "CharacterGuildGetName", LuaCharacterGuildGetName);
	lua_register(L, "GetPosFromPlayer", LuaGetPosFromPlayer);
	lua_register(L, "SendPlayerChat", LuaSendPlayerChat);
	lua_register(L, "FindCharacterStack", LuaGetFindCharacterIndex);
	lua_register(L, "GetTargetCharacter", LuaGetTargetCharacter);

	lua_register(L, "SendTradePlayer", LuaSendTradePlayer);
	lua_register(L, "SendPartyPlayer", LuaSendPartyPlayer);
	lua_register(L, "SendGuildPlayer", LuaSendGuildPlayer);
	lua_register(L, "SendShopPlayer", LuaSendLojaPlayer);

	luaaa::LuaModule(L).fun("CharacterGetFullClass", &CharacterGetFullClass);

	luaaa::LuaModule(L).fun("CharacterGetVisible", &CharacterGetVisible);

	luaaa::LuaModule(L).fun("CharacterGetAngleX", &CharacterGetAngleX);
	luaaa::LuaModule(L).fun("CharacterGetAngleY", &CharacterGetAngleY);
	luaaa::LuaModule(L).fun("CharacterGetAngleZ", &CharacterGetAngleZ);
	luaaa::LuaModule(L).fun("CharacterSetAngleX", &CharacterSetAngleX);
	luaaa::LuaModule(L).fun("CharacterSetAngleY", &CharacterSetAngleY);
	luaaa::LuaModule(L).fun("CharacterSetAngleZ", &CharacterSetAngleZ);

	luaaa::LuaModule(L).fun("CharacterGetPositionX", &CharacterGetPositionX);
	luaaa::LuaModule(L).fun("CharacterGetPositionY", &CharacterGetPositionY);
	luaaa::LuaModule(L).fun("CharacterGetPositionZ", &CharacterGetPositionZ);
	luaaa::LuaModule(L).fun("CharacterSetPositionX", &CharacterSetPositionX);
	luaaa::LuaModule(L).fun("CharacterSetPositionY", &CharacterSetPositionY);
	luaaa::LuaModule(L).fun("CharacterSetPositionZ", &CharacterSetPositionZ);
	luaaa::LuaModule(L).fun("CharacterSetScale", &CharacterSetScale);

	luaaa::LuaModule(L).fun("GetSelectedHero", &GetSelectedHero);
	luaaa::LuaModule(L).fun("SetSelectedHero", &SetSelectedHero);

	luaaa::LuaModule(L).fun("SetTargetCharacter", &SetTargetCharacter);

	luaaa::LuaModule(L).fun("GetClassName", &GetClassCharacterName);

	luaaa::LuaModule(L).fun("CharacterGetGuildStatus", &CharacterGetGuildStatus);

	luaaa::LuaModule(L).fun("SetCharacterAction", &SetCharacterAction);
}
