#include "stdafx.h"
#include "LuaCloth.h"
#include "LuaStack.hpp"

void InitLuaCloth(lua_State* L)
{
	luaaa::LuaClass<ClothClass> luaCloth(L, "CapeStack");
	luaCloth.ctor<DWORD>();
	luaCloth.fun("Create", &ClothClass::Create);
	luaCloth.fun("SetWindMinMax", &ClothClass::SetWindMax);
	luaCloth.fun("Collision", &ClothClass::AddCollisionSphere);
}
