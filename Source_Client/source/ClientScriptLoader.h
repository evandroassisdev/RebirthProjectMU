// ClientScriptLoader.h: interface for the CClientScriptLoader class.
// Client-side counterpart of the GameServer's CScriptLoader (see
// Source_Server/GameServer/GameServer/ScriptLoader.h) - embeds Lua in the
// game client so UI/rendering logic can be scripted instead of recompiled.
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CClientScriptLoader
{
public:
	CClientScriptLoader();
	virtual ~CClientScriptLoader();
	void Load(char* path);
	void OnMainProc();
	void OnClickEvent();
private:
	lua_State* m_luaState;
};

extern CClientScriptLoader gClientScriptLoader;
