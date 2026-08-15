// ClientScriptLoader.cpp: implementation of the CClientScriptLoader class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ClientScriptLoader.h"
#include "./Utilities/Log/muConsoleDebug.h"
#include "ClientLuaFunction.h"

// lua52.lib was built against an old (pre-UCRT) MSVC CRT that calls
// __iob_func() to get at stdin/stdout/stderr. This toolset's
// legacy_stdio_definitions.lib doesn't provide it (same issue as on the
// GameServer side - see ScriptLoader.cpp there for the longer version of
// this comment). Lua only ever touches stdout/stderr through it, so a
// static snapshot of the 3 standard streams is enough.
extern "C"
{
	FILE _client_lua_iob[3] = { *stdin, *stdout, *stderr };

	FILE* __cdecl __iob_func(void)
	{
		return _client_lua_iob;
	}
}

CClientScriptLoader gClientScriptLoader;

namespace
{
	// Not real cryptography - this only has to stop a curious player from
	// opening a script in Notepad, not a determined reverse engineer (same
	// tier of protection as the XOR cipher every .bmd in this client already
	// uses). Ship the .enc, keep the matching .lua source out of the
	// deployed client folder.
	const BYTE ScriptCryptKey[] = {
		0x52, 0x65, 0x62, 0x69, 0x72, 0x74, 0x68, 0x4D,
		0x55, 0x32, 0x30, 0x32, 0x36, 0x21, 0x21, 0x21
	};
	const char ScriptCryptMagic[5] = { 'R', 'P', 'M', 'U', 0x01 };

	void ScriptCryptXor(BYTE* buffer, int size)
	{
		for (int i = 0; i < size; i++)
		{
			buffer[i] ^= ScriptCryptKey[i % sizeof(ScriptCryptKey)];
		}
	}

	// Tries "<base>.enc" (encrypted) first, falls back to "<base>.lua"
	// (plain source, for local dev) if no .enc exists. Leaves the loaded
	// chunk on the Lua stack, exactly like luaL_loadfile - 0 on success,
	// nonzero (with an error message on the stack) on failure.
	int LoadScriptChunk(lua_State* L, const char* base)
	{
		char encPath[256];
		char luaPath[256];

		wsprintf(encPath, "%s.enc", base);
		wsprintf(luaPath, "%s.lua", base);

		FILE* fp = fopen(encPath, "rb");

		if (fp != NULL)
		{
			fseek(fp, 0, SEEK_END);
			long fileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			if (fileSize <= (long)sizeof(ScriptCryptMagic))
			{
				fclose(fp);
				lua_pushfstring(L, "'%s' is too small to be a valid encrypted script", encPath);
				return 1;
			}

			BYTE* fileBuffer = new BYTE[fileSize];
			fread(fileBuffer, 1, fileSize, fp);
			fclose(fp);

			if (memcmp(fileBuffer, ScriptCryptMagic, sizeof(ScriptCryptMagic)) != 0)
			{
				delete[] fileBuffer;
				lua_pushfstring(L, "'%s' has a bad header - wrong key or corrupted file", encPath);
				return 1;
			}

			int bodySize = (int)fileSize - (int)sizeof(ScriptCryptMagic);
			BYTE* body = fileBuffer + sizeof(ScriptCryptMagic);

			ScriptCryptXor(body, bodySize);

			int result = luaL_loadbuffer(L, (const char*)body, bodySize, encPath);

			delete[] fileBuffer;

			return result;
		}

		return luaL_loadfile(L, luaPath);
	}

	int LuaRequire(lua_State* L)
	{
		char base[256];

		wsprintf(base, "Lua\\%s", luaL_checklstring(L, 1, 0));

		lua_settop(L, 1);
		lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
		lua_getfield(L, 2, base);

		if (lua_toboolean(L, -1))
		{
			return 1;
		}

		if (LoadScriptChunk(L, base) != 0)
		{
			return luaL_error(L, "[ClientScriptLoader] Could not load '%s'. %s", base, lua_tostring(L, -1));
		}

		lua_pushstring(L, base);
		lua_call(L, 1, 1);

		lua_pushvalue(L, -1);
		lua_setfield(L, 2, base);

		return 1;
	}
}

CClientScriptLoader::CClientScriptLoader()
{
	m_luaState = 0;
}

CClientScriptLoader::~CClientScriptLoader()
{
	if (m_luaState != 0)
	{
		lua_close(m_luaState);
		m_luaState = 0;
	}
}

void CClientScriptLoader::Load(char* path)
{
	m_luaState = 0;

	lua_State* lua = luaL_newstate();

	luaL_openlibs(lua);
	lua_pushcclosure(lua, LuaRequire, 0);
	lua_setglobal(lua, "require");
	lua_gc(lua, LUA_GCCOLLECT, 0);

	InitClientLuaFunction(lua);

	if (LoadScriptChunk(lua, path) != 0)
	{
		g_ConsoleDebug->Write(MCD_ERROR, "[ClientScriptLoader] Could not load '%s'. %s", path, lua_tostring(lua, -1));
		lua_close(lua);
		return;
	}

	if (lua_pcall(lua, 0, 0, 0) != 0)
	{
		g_ConsoleDebug->Write(MCD_ERROR, "[ClientScriptLoader] Error in Lua-file. %s", lua_tostring(lua, -1));
		lua_close(lua);
		return;
	}

	m_luaState = lua;
}

void CClientScriptLoader::OnMainProc()
{
	if (m_luaState == 0)
	{
		return;
	}

	lua_getglobal(m_luaState, "BridgeFunction_OnMainProc");

	if (lua_isfunction(m_luaState, -1) == 0)
	{
		lua_pop(m_luaState, 1);
		return;
	}

	if (lua_pcall(m_luaState, 0, 0, 0) != 0)
	{
		// OnMainProc runs every frame - only log the first failure so a
		// broken script doesn't flood the debug log hundreds of times/sec.
		static bool s_LoggedMainProcError = false;
		if (!s_LoggedMainProcError)
		{
			g_ConsoleDebug->Write(MCD_ERROR, "[ClientScriptLoader] Error in OnMainProc: %s", lua_tostring(m_luaState, -1));
			s_LoggedMainProcError = true;
		}
		lua_pop(m_luaState, 1);
		return;
	}
}

void CClientScriptLoader::OnClickEvent()
{
	if (m_luaState == 0)
	{
		return;
	}

	lua_getglobal(m_luaState, "BridgeFunction_OnClickEvent");

	if (lua_isfunction(m_luaState, -1) == 0)
	{
		lua_pop(m_luaState, 1);
		return;
	}

	if (lua_pcall(m_luaState, 0, 0, 0) != 0)
	{
		g_ConsoleDebug->Write(MCD_ERROR, "[ClientScriptLoader] Error in OnClickEvent: %s", lua_tostring(m_luaState, -1));
		lua_pop(m_luaState, 1);
		return;
	}
}
