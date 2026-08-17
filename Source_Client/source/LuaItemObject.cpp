#include "stdafx.h"
#include "LuaItemObject.h"
#include "LuaStack.hpp"

void InitLuaItemObject(lua_State* L)
{
	luaaa::LuaClass<itemObject> luaItem(L, "Item");
	luaItem.ctor<DWORD>();
	luaItem.fun("getLevel", &itemObject::getLevel);
	luaItem.fun("getOption1", &itemObject::getOption1);
	luaItem.fun("getExc", &itemObject::getExc);
}
