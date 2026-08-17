// ScriptLoader.h: interface for the CScriptLoader class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include "CriticalSection.h"

class CScriptLoader
{
public:
	CScriptLoader();
	virtual ~CScriptLoader();
	void Load(char* path);
	void OnReadScript();
	void OnShutScript();
	void OnTimerThread();
	int OnCommandManager(int aIndex, int code, char* arg);
	void OnCommandDone(int aIndex, int code);
	void OnCharacterEntry(int aIndex);
	void OnCharacterClose(int aIndex);
	int OnNpcTalk(int aIndex, int bIndex);
	void OnMonsterDie(int aIndex, int bIndex);
	void OnUserDie(int aIndex, int bIndex);
	void OnUserRespawn(int aIndex, int KillerType);
	void OnUserLevelUp(int aIndex);
	int OnCheckUserTarget(int aIndex, int bIndex);
	int OnCheckUserKiller(int aIndex, int bIndex);
	int OnUserItemPick(int aIndex, int slot);
	int OnUserItemDrop(int aIndex, int slot, int x, int y);
	int OnUserItemMove(int aIndex, int aFlag, int aSlot, int bFlag, int bSlot);
	// Fires on every chat message (including "/commands"), before this
	// server's own command dispatch/broadcast - a script returning nonzero
	// silently drops the message. Ported from the "RoxGaming Main 5.2 -
	// 60 FPS UPDATE" source pack's ChatProc hook (LuaGameServer.cpp,
	// called from its Protocol.cpp's CGChatRecv - this project's own
	// CGChatRecv is close enough in shape to hook the same way).
	int OnChatProc(int aIndex, const char* text);
private:
	lua_State* m_luaState;
	CCriticalSection m_critical;
};

extern CScriptLoader gScriptLoader;