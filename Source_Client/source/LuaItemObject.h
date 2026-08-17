#pragma once

// Exposes an ITEM struct (_struct.h's tagITEM) to client Lua as a small
// read-only wrapper. Ported from the "RoxGaming Main 5.2 - 60 FPS UPDATE"
// source pack (source/LuaItemObject.cpp/.h there) - see LuaBMD.h for the
// general porting notes.
//
// Not yet build-verified.

#include "_struct.h"

// Constructed from Lua as Item.new(itemPtr), itemPtr a DWORD cast of an
// ITEM*.
class itemObject
{
public:
	itemObject() : itemObj(nullptr)
	{
	}

	itemObject(DWORD objClass)
	{
		this->itemObj = (ITEM*)objClass;
	}

	~itemObject()
	{
	}

	int getLevel() const
	{
		if (this->itemObj == nullptr)
		{
			return 0;
		}

		return this->itemObj->Level * 8;
	}

	int getOption1() const
	{
		if (this->itemObj == nullptr)
		{
			return 0;
		}

		return this->itemObj->Option1;
	}

	int getExc() const
	{
		if (this->itemObj == nullptr)
		{
			return 0;
		}

		return this->itemObj->ExtOption;
	}

private:
	ITEM* itemObj;
};

void InitLuaItemObject(lua_State* L);
