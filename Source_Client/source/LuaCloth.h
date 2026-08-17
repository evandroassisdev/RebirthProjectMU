#pragma once

// Exposes a character's cape/cloth physics slots (CPhysicsCloth, see
// PhysicsManager.h) to client Lua. Ported from the "RoxGaming Main 5.2 -
// 60 FPS UPDATE" source pack (source/LuaCloth.cpp/.h there) - see LuaBMD.h
// for the general porting notes.
//
// Not yet build-verified.

#include "PhysicsManager.h"

// Thin Lua-facing wrapper - constructed from Lua as CapeStack.new(ptr),
// where ptr is a DWORD pointing at an OBJECT's array of CPhysicsCloth
// slots, indexed by numCloth in every method below.
class ClothClass
{
public:
	ClothClass() : m_Struct(nullptr)
	{
	}

	ClothClass(DWORD Struct)
	{
		this->m_Struct = reinterpret_cast<CPhysicsCloth*>(Struct);
	}

	~ClothClass()
	{
	}

	void Create(int numCloth, DWORD ObjectStruct, int iBone, float fxPos, float fyPos, float fzPos, int iNumHor, int iNumVer, float fWidth, float fHeight, int iTexFront, int iTexBack, DWORD dwType)
	{
		if (!m_Struct)
		{
			return;
		}

		OBJECT* o = (OBJECT*)ObjectStruct;
		this->m_Struct[numCloth].Create(o, iBone, fxPos, fyPos, fzPos, iNumHor, iNumVer, fWidth, fHeight, iTexFront, iTexBack, dwType);
	}

	void SetWindMax(int numCloth, int min, int max)
	{
		if (!m_Struct)
		{
			return;
		}

		this->m_Struct[numCloth].SetWindMinMax((BYTE)min, (BYTE)max);
	}

	void AddCollisionSphere(int numCloth, float fXPos, float fYPos, float fZPos, float fRadius, int iBone)
	{
		if (!m_Struct)
		{
			return;
		}

		this->m_Struct[numCloth].AddCollisionSphere(fXPos, fYPos, fZPos, fRadius, iBone);
	}

public:
	CPhysicsCloth* m_Struct;
};

void InitLuaCloth(lua_State* L);
