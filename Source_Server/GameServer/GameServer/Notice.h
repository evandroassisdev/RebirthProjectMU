// Notice.h: interface for the CNotice class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Protocol.h"

#define MAX_NOTICE 100

//**********************************************//
//************ GameServer -> Client ************//
//**********************************************//

struct PMSG_NOTICE_SEND
{
	PBMSG_HEAD header; // C1:0D
	BYTE type;
	BYTE count;
	BYTE opacity;
	WORD delay;
	DWORD color;
	BYTE speed;
	char message[256];
};

struct PMSG_NOTICE_SEND_NEW
{
	PBMSG_HEAD header; // C1:00
	
	BYTE count;
	BYTE opacity;
	WORD delay;
	DWORD color;
	BYTE speed;
	char message[256];
};

struct PMSG_NOTICE_DEV_SEND
{
	PBMSG_HEAD header; // C1:00
	char message[256];
};

struct PMSG_WINDOW_TITLE_SEND
{
	PBMSG_HEAD header; // C1:74
	char title[128];
};

// Sets the local player's client window title (e.g. from a Lua script). Free
// function (not a CNotice method) so it matches the exact call signature the
// LuaSetObjectWindowTitle binding expects.
void GCWindowsNameSend(int aIndex,char* title);

//**********************************************//
//**********************************************//
//**********************************************//

struct NOTICE_INFO
{
	DWORD LastTime;
	char Message[3][128];
	int Type;
	int RepeatTime;
};

class CNotice
{
public:
	CNotice();
	virtual ~CNotice();
	void Load(char* path);
	void SetInfo(NOTICE_INFO info);
	void MainProc();
	void GCNoticeSend(int aIndex,BYTE type,BYTE count,BYTE opacity,WORD delay,DWORD color,BYTE speed,char* message,...);
	void GCNoticeSendToAll(BYTE type,BYTE count,BYTE opacity,WORD delay,DWORD color,BYTE speed,char* message,...);
	void CNotice::NewMessageDevTeam(int aIndex,char* message,...);
	void NewNoticeSend(int aIndex,BYTE count,BYTE opacity,WORD delay,DWORD color,BYTE speed,char* message,...);
private:
	std::vector<NOTICE_INFO> m_NoticeInfo;
	int m_count;
	int m_NoticeValue;
	DWORD m_NoticeTime;
};

extern CNotice gNotice;
