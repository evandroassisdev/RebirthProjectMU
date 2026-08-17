#pragma once

// Exposes BMD (the loaded 3D model) to the client-side Lua engine, so
// scripts can drive model rendering/effects directly instead of everything
// having to be a native C++ change. Ported from the "RoxGaming Main 5.2 -
// 60 FPS UPDATE" source pack (source/LuaBMD.cpp/.h there) - this project's
// BMD/ZzzEffect/MapManager/CDirection classes matched that pack's almost
// verbatim (same members, same signatures), so this is a near-direct port,
// registered via the luaaa binding helper (LuaStack.hpp) instead of our own
// hand-written lua_CFunction wrappers.
//
// Not yet confirmed in-game.

#include <set>
#include "ZzzBMD.h"
#include "ZzzEffect.h"
#include "CDirection.h"
#include "MapManager.h"

// Thin Lua-facing wrapper around a BMD* - luaaa needs a real C++ class (not
// a raw pointer) to attach a metatable/methods to. Constructed from Lua as
// BMD.new(bmdPointer), where bmdPointer is a DWORD - whatever native binding
// hands the script a model (e.g. a future "GetObjectBMD(objIndex)") should
// pass the raw BMD* cast to DWORD.
class BMDClass
{
public:
	BMDClass() : m_Struct(nullptr)
	{
	}

	BMDClass(DWORD Struct)
	{
		this->m_Struct = (BMD*)Struct;
	}

	~BMDClass()
	{
	}

	float GetLight(int value) const
	{
		if (!m_Struct)
		{
			return 0.0f;
		}

		if (value == 0)
		{
			return m_Struct->BodyLight[0];
		}
		else if (value == 1)
		{
			return m_Struct->BodyLight[1];
		}
		else if (value == 2)
		{
			return m_Struct->BodyLight[2];
		}

		return *(float*)m_Struct->BodyLight;
	}

	void SetLight(float r, float g, float b) const
	{
		if (!m_Struct)
		{
			return;
		}

		m_Struct->BodyLight[0] = r;
		m_Struct->BodyLight[1] = g;
		m_Struct->BodyLight[2] = b;
	}

	void RenderBody(int Flag, float Alpha, int BlendMesh, float BlendMeshLight, float BlendMeshTexCoordU, float BlendMeshTexCoordV, int HiddenMesh, int Texture) const
	{
		if (!m_Struct)
		{
			return;
		}

		m_Struct->RenderBody(Flag, Alpha, BlendMesh, BlendMeshLight, BlendMeshTexCoordU, BlendMeshTexCoordV, HiddenMesh, Texture);
	}

	void RenderMesh(int i, int RenderFlag, float Alpha, int BlendMesh, float BlendMeshLight, float BlendMeshTexCoordU, float BlendMeshTexCoordV, int MeshTexture) const
	{
		if (!m_Struct)
		{
			return;
		}

		m_Struct->RenderMesh(i, RenderFlag, Alpha, BlendMesh, BlendMeshLight, BlendMeshTexCoordU, BlendMeshTexCoordV, MeshTexture);
	}

	void glColor3f() const
	{
		if (!m_Struct)
		{
			return;
		}

		glColor3fv(m_Struct->BodyLight);
	}

	void BeginRender(float value) const
	{
		//glPushMatrix();
	}

	void EndRender() const
	{
		//glPopMatrix();
	}

	void setMesh(int value) const
	{
		if (!m_Struct)
		{
			return;
		}

		m_Struct->StreamMesh = value;
	}

	void RenderShadowModel() const
	{
		if (!m_Struct)
		{
			return;
		}

		if (gMapManager.WorldActive != WD_10HEAVEN && gMapManager.InHellas() == FALSE)
		{
			if (!g_Direction.m_CKanturu.IsMayaScene())
			{
				EnableAlphaTest(1);

				if (gMapManager.WorldActive == 7)
				{
					glColor4f(0.0f, 0.0f, 0.0f, 0.2f);
				}
				else
				{
					glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
				}

				m_Struct->RenderBodyShadow(-1, -1, -1, -1);
			}
		}
	}

	void TransformPosition(int Link, float PosX, float PosY, float PosZ)
	{
		if (!m_Struct)
		{
			return;
		}

		if (Link != -1)
		{
			vec3_t vPos;

			Vector(PosX, PosY, PosZ, vPos);

			m_Struct->TransformPosition(BoneTransform[Link], vPos, this->Position, 0);
		}

		this->LinkObject = Link;
	}

	void TransformPosition2(int Link, float PosX, float PosY, float PosZ)
	{
		if (!m_Struct)
		{
			return;
		}

		if (Link != -1)
		{
			vec3_t vPos;

			Vector(PosX, PosY, PosZ, vPos);

			m_Struct->TransformPosition(BoneTransform[Link], vPos, this->Position, 1);
		}

		this->LinkObject = Link;
	}

	void CreateSprites(int Bitmap, float Scale, float LightX, float LightY, float LightZ, DWORD ObjectStruct)
	{
		if (!m_Struct || !ObjectStruct)
		{
			return;
		}

		vec3_t Light;

		Vector(LightX, LightY, LightZ, Light);

		auto obj = (OBJECT*)ObjectStruct;

		if (obj == nullptr)
		{
			return;
		}

		CreateSprite(Bitmap, this->Position, Scale, Light, obj, 0.0, 0);
	}

	void CreateParticles(int Bitmap, int SubType, float Scale, float LightR, float LightG, float LightB, DWORD ObjectStruct)
	{
		if (!this->m_Struct || !ObjectStruct)
		{
			return;
		}

		vec3_t Light;

		Vector(LightR, LightG, LightB, Light);

		auto obj = (OBJECT*)ObjectStruct;

		if (obj == nullptr)
		{
			return;
		}

		CreateParticle(Bitmap, Position, obj->Angle, Light, SubType, Scale, obj);
	}

	void CreateEffects(int Bitmap, int SubType, float LightX, float LightY, float LightZ, DWORD ObjectStruct)
	{
		if (!this->m_Struct || !ObjectStruct)
		{
			return;
		}

		// Upstream (RoxGaming) guards this with their own CCriticalSection,
		// a utility class this project doesn't have. Not reintroduced -
		// this only ever runs from the main-thread render/update loop
		// CClientScriptLoader is hooked into (see ClientScriptLoader.cpp),
		// same as every other client Lua binding.
		auto obj = (OBJECT*)ObjectStruct;

		if (obj == nullptr)
		{
			return;
		}

		vec3_t Light;

		Vector(LightX, LightY, LightZ, Light);

		CreateEffect(Bitmap, (this->LinkObject == -1 ? obj->Position : this->Position), obj->Angle, Light, SubType, obj, -1, 0, 0, 0, 0.0, -1);
	}

public:
	BMD* m_Struct;
	vec3_t Position;
	int LinkObject;
};

// Registers the "BMD" class (BMDClass above) into the given Lua state -
// call once per client Lua state, alongside InitClientLuaFunction().
void InitLuaBMD(lua_State* L);
