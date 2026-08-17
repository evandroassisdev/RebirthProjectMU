#include "stdafx.h"
#include "LuaEffectsNormal.h"
#include "LuaStack.hpp"
#include "ZzzBMD.h"
#include "ZzzEffect.h"

namespace
{
	void RenderBrightEffects(BMD* b, int Bitmap, int Link, float Scale, vec3_t Light, OBJECT* o)
	{
		vec3_t p, Position;
		Vector(0.f, 0.f, 0.f, p);
		b->TransformPosition(BoneTransform[Link], p, Position, true);
		CreateSprite(Bitmap, Position, Scale, Light, o);
	}

	void FuncCreateParticleNormal(DWORD BMDStruct, int Bitmap, int SubType, int Link, float Scale, vec3_t Light, DWORD ObjectStruct)
	{
		// Upstream guarded this with "if (!&ObjectStruct)" - the address of
		// the local parameter itself, which is never null, so that check
		// never actually fired. Corrected to check the values passed in.
		if (BMDStruct == 0 || ObjectStruct == 0)
		{
			return;
		}

		vec3_t Position, p;
		p[0] = 0.f; p[1] = 0.f; p[2] = 0.f;

		auto b = (BMD*)BMDStruct;
		auto o = (OBJECT*)ObjectStruct;

		b->TransformPosition(BoneTransform[Link], p, Position);

		CreateParticle(Bitmap, Position, o->Angle, Light, SubType, Scale, o);
	}

	void CreateSpriteNormal(DWORD BMDStruct, int Bitmap, int Link, float Scale, float LightR, float LightG, float LightB, DWORD ObjectStruct)
	{
		if (BMDStruct == 0 || ObjectStruct == 0)
		{
			return;
		}

		auto b = (BMD*)BMDStruct;
		auto o = (OBJECT*)ObjectStruct;

		vec3_t Light;
		Vector(LightR, LightG, LightB, Light);
		RenderBrightEffects(b, Bitmap, Link, Scale, Light, o);
	}

	void CreateParticleNormal(DWORD BMDStruct, int Bitmap, int SubType, int Link, float Scale, float LightR, float LightG, float LightB, DWORD ObjectStruct)
	{
		vec3_t Light;
		Vector(LightR, LightG, LightB, Light);
		FuncCreateParticleNormal(BMDStruct, Bitmap, SubType, Link, Scale, Light, ObjectStruct);
	}
}

void InitLuaEffectsNormal(lua_State* L)
{
	luaaa::LuaModule(L).fun("CreateSprite", &CreateSpriteNormal);
	luaaa::LuaModule(L).fun("CreateParticle", &CreateParticleNormal);
}
