#include "stdafx.h"
#include "LuaUser.h"

namespace
{
	int LuaUserMap(lua_State* L)
	{
		lua_pushnumber(L, gMapManager.WorldActive);
		return 1;
	}

	int LuaUserGetName(lua_State* L)
	{
		auto lpUser = Hero;
		lua_pushstring(L, lpUser->ID);
		return 1;
	}

	int LuaUserGetKey(lua_State* L)
	{
		auto lpUser = Hero;
		lua_pushnumber(L, lpUser->Key);
		return 1;
	}

	int LuaUserPositionX(lua_State* L)
	{
		auto lpUser = Hero;
		DWORD PosX = (DWORD)(lpUser->Object.Position[0] / 100.f);
		lua_pushnumber(L, (DWORD)(PosX > 0 ? PosX : 0));
		return 1;
	}

	int LuaUserPositionY(lua_State* L)
	{
		auto lpUser = Hero;
		DWORD PosY = (DWORD)(lpUser->Object.Position[1] / 100.f);
		lua_pushnumber(L, (DWORD)(PosY > 0 ? PosY : 0));
		return 1;
	}

	int LuaUserGetHelper(lua_State* L)
	{
		auto lpUser = Hero;
		lua_pushnumber(L, (lpUser->Helper.Type != (WORD)-1 ? (lpUser->Helper.Type - MODEL_ITEM) : -1));
		return 1;
	}

	int LuaUserGetWing(lua_State* L)
	{
		auto lpUser = Hero;
		lua_pushnumber(L, (lpUser->Wing.Type != (WORD)-1 ? (lpUser->Wing.Type - MODEL_ITEM) : -1));
		return 1;
	}

	int LuaUserGetSword(lua_State* L)
	{
		auto lpUser = Hero;
		lua_pushnumber(L, (lpUser->Weapon[0].Type != (WORD)-1 ? (lpUser->Weapon[0].Type - MODEL_ITEM) : -1));
		return 1;
	}

	int LuaUserGetShield(lua_State* L)
	{
		auto lpUser = Hero;
		lua_pushnumber(L, (lpUser->Weapon[1].Type != (WORD)-1 ? (lpUser->Weapon[1].Type - MODEL_ITEM) : -1));
		return 1;
	}

	int LuaUserGetHelm(lua_State* L)
	{
		auto lpUser = Hero;
		lua_pushnumber(L, (lpUser->BodyPart[BODYPART_HELM].Type != (WORD)-1 ? (lpUser->BodyPart[BODYPART_HELM].Type - MODEL_ITEM) : -1));
		return 1;
	}

	int LuaUserGetArmor(lua_State* L)
	{
		auto lpUser = Hero;
		lua_pushnumber(L, (lpUser->BodyPart[BODYPART_ARMOR].Type != (WORD)-1 ? (lpUser->BodyPart[BODYPART_ARMOR].Type - MODEL_ITEM) : -1));
		return 1;
	}

	int LuaUserGetPants(lua_State* L)
	{
		auto lpUser = Hero;
		lua_pushnumber(L, (lpUser->BodyPart[BODYPART_PANTS].Type != (WORD)-1 ? (lpUser->BodyPart[BODYPART_PANTS].Type - MODEL_ITEM) : -1));
		return 1;
	}

	int LuaUserGetGloves(lua_State* L)
	{
		auto lpUser = Hero;
		lua_pushnumber(L, (lpUser->BodyPart[BODYPART_GLOVES].Type != (WORD)-1 ? (lpUser->BodyPart[BODYPART_GLOVES].Type - MODEL_ITEM) : -1));
		return 1;
	}

	int LuaUserGetBoots(lua_State* L)
	{
		auto lpUser = Hero;
		lua_pushnumber(L, (lpUser->BodyPart[BODYPART_BOOTS].Type != (WORD)-1 ? (lpUser->BodyPart[BODYPART_BOOTS].Type - MODEL_ITEM) : -1));
		return 1;
	}

	int LuaUserGetLevel(lua_State* L)
	{
		auto lpUser = Hero;
		lua_pushnumber(L, lpUser->Level);
		return 1;
	}

	int LuaUserGetClass(lua_State* L)
	{
		auto lpUser = Hero;
		lua_pushnumber(L, (lpUser->Class & 7));
		return 1;
	}

	int LuaUserGetGuild(lua_State* L)
	{
		auto lpUser = Hero;

		if (lpUser->GuildMarkIndex != (WORD)-1)
		{
			lua_pushnumber(L, lpUser->GuildMarkIndex);
		}
		else
		{
			lua_pushnumber(L, -1);
		}
		return 1;
	}

	int LuaGuildGetName(lua_State* L)
	{
		auto lpUser = Hero;

		if (lpUser->GuildMarkIndex != (WORD)-1)
		{
			char* gname = GuildMark[lpUser->GuildMarkIndex].GuildName;
			lua_pushstring(L, strlen(gname) <= 0 ? " " : gname);
		}
		else
		{
			lua_pushstring(L, " ");
		}
		return 1;
	}
}

void InitLuaUser(lua_State* L)
{
	lua_register(L, "UserGetMap", LuaUserMap);
	lua_register(L, "UserGetName", LuaUserGetName);
	lua_register(L, "UserGetIndex", LuaUserGetKey);
	lua_register(L, "UserPositionX", LuaUserPositionX);
	lua_register(L, "UserPositionY", LuaUserPositionY);
	lua_register(L, "UserGetHelper", LuaUserGetHelper);
	lua_register(L, "UserGetWing", LuaUserGetWing);
	lua_register(L, "UserGetSword", LuaUserGetSword);
	lua_register(L, "UserGetShield", LuaUserGetShield);
	lua_register(L, "UserGetHelm", LuaUserGetHelm);
	lua_register(L, "UserGetArmor", LuaUserGetArmor);
	lua_register(L, "UserGetPants", LuaUserGetPants);
	lua_register(L, "UserGetGloves", LuaUserGetGloves);
	lua_register(L, "UserGetBoots", LuaUserGetBoots);
	lua_register(L, "UserGetLevel", LuaUserGetLevel);
	lua_register(L, "UserGetClass", LuaUserGetClass);

	lua_register(L, "UserGetGuild", LuaUserGetGuild);
	lua_register(L, "GuildGetName", LuaGuildGetName);
}
