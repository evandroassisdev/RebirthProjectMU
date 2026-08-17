#include "stdafx.h"
#include "LuaObject.h"
#include "LuaStack.hpp"
#include "ZzzOpenglUtil.h"

namespace
{
	// Constructed from Lua as Object.new(objPtr), objPtr a DWORD cast of an
	// OBJECT*.
	class ObjectClass
	{
	public:
		ObjectClass() : m_Struct(nullptr)
		{
		}

		ObjectClass(DWORD Struct)
		{
			m_Struct = (OBJECT*)Struct;
		}

		~ObjectClass()
		{
		}

		float Alpha() const
		{
			return m_Struct ? m_Struct->Alpha : 0.f;
		}

		DWORD Mesh() const
		{
			return m_Struct ? (DWORD)m_Struct->BlendMesh : 0;
		}

		float Light() const
		{
			return m_Struct ? m_Struct->BlendMeshLight : 0.f;
		}

		float TexCoordU() const
		{
			return m_Struct ? m_Struct->BlendMeshTexCoordU : 0.f;
		}

		float TexCoordV() const
		{
			return m_Struct ? m_Struct->BlendMeshTexCoordV : 0.f;
		}

		DWORD Hidden() const
		{
			return m_Struct ? (DWORD)m_Struct->HiddenMesh : 0;
		}

		int getAction() const
		{
			return m_Struct ? m_Struct->CurrentAction : 0;
		}

		float getTime() const
		{
			return m_Struct ? m_Struct->Timer : 0.f;
		}

		void setTime(float value) const
		{
			if (m_Struct)
			{
				m_Struct->Timer = value;
			}
		}

	private:
		OBJECT* m_Struct;
	};

	float worldTimes()
	{
		return WorldTime;
	}
}

float GetDoubleRender(float a1, float a2)
{
	float Return;
	float InitValue;

	InitValue = (float)((int32_t)(a2 * 0.01745f * 1000.0f / a1 + WorldTime) % (int32_t)(6283.185546875f / a1)) * 0.001f * a1;

	if (InitValue >= 3.14f)
	{
		Return = cos(InitValue);
	}
	else
	{
		Return = -cos(InitValue);
	}

	return (float)((Return + 1.0f) * 0.5f);
}

void InitLuaObject(lua_State* L)
{
	luaaa::LuaClass<ObjectClass> luaObject(L, "Object");
	luaObject.ctor<DWORD>();
	luaObject.fun("Alpha", &ObjectClass::Alpha);
	luaObject.fun("Mesh", &ObjectClass::Mesh);
	luaObject.fun("Light", &ObjectClass::Light);
	luaObject.fun("TexCoordU", &ObjectClass::TexCoordU);
	luaObject.fun("TexCoordV", &ObjectClass::TexCoordV);
	luaObject.fun("Hidden", &ObjectClass::Hidden);
	luaObject.fun("getAction", &ObjectClass::getAction);
	luaObject.fun("getTime", &ObjectClass::getTime);
	luaObject.fun("setTime", &ObjectClass::setTime);

	luaaa::LuaModule(L).fun("worldTime", &worldTimes);
	luaaa::LuaModule(L).fun("GetDoubleRender", &GetDoubleRender);
}
