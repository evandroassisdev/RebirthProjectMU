// CustomJewelBank.h: interface for the CCustomJewelBank class.
// Ported from Wizard Team EX301KOR (C:\Wizard Team Source\SOURCE\Source\Emulador\GameServer\CustomJewelBank.h),
// adapted to this project's own conventions:
//   - opcodes changed to avoid collisions with existing handlers (see .cpp for the full mapping)
//   - jewel "type" index (0-9) matches this project's own CJewelMix scheme (JewelMix.h/.cpp),
//     not Wizard Team's own numbering - GetJewelSimpleIndex/GetJewelBundleIndex are reused
//     directly from gJewelMix instead of being duplicated here.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Protocol.h"
#include "User.h"

//**********************************************//
//************ Client -> GameServer ************//
//**********************************************//

struct PMSG_JEWELBANK_DEPOSIT_RECV
{
	PSBMSG_HEAD header; // C1:F3:F5
	int slot; // inventory slot of the jewel/stone being deposited
};

struct PMSG_JEWELBANK_WITHDRAW_RECV
{
	PSBMSG_HEAD header; // C1:F3:F6
	int type;  // 0-9, same scheme as CJewelMix
	int count; // 99 = withdraw max available
};

//**********************************************//
//************ GameServer -> Client ************//
//**********************************************//

struct PMSG_JEWELBANK_INFO_SEND
{
	PSBMSG_HEAD header; // C1:F3:F7
	int Bless;
	int Soul;
	int Life;
	int Creation;
	int Guardian;
	int GemStone;
	int Harmony;
	int Chaos;
	int LowStone;
	int HighStone;
};

//**********************************************//
//********** GameServer -> DataServer **********//
//**********************************************//

struct SDHP_JEWELBANK_SAVE_SEND
{
	PWMSG_HEAD header; // C2:F7 sub 0x04
	BYTE subcode;
	char account[11];
	int type;   // 0-9
	int count;  // signed: positive = deposit, negative = withdraw
};

struct SDHP_JEWELBANK_INFO_REQUEST_SEND
{
	PWMSG_HEAD header; // C2:F7 sub 0x05
	BYTE subcode;
	int index; // gObj[] index to reply to
	char account[11];
};

//**********************************************//
//********** DataServer -> GameServer **********//
//**********************************************//

struct SDHP_JEWELBANK_INFO_RECV
{
	PWMSG_HEAD header; // C2:F6
	int index;
	int Bless;
	int Soul;
	int Life;
	int Creation;
	int Guardian;
	int GemStone;
	int Harmony;
	int Chaos;
	int LowStone;
	int HighStone;
};

//**********************************************//
//**********************************************//
//**********************************************//

class CCustomJewelBank
{
public:
	CCustomJewelBank();
	virtual ~CCustomJewelBank();

	void JewelBankDepositRecv(PMSG_JEWELBANK_DEPOSIT_RECV* lpMsg, int aIndex);
	void JewelBankWithdrawRecv(PMSG_JEWELBANK_WITHDRAW_RECV* lpMsg, int aIndex);
	void RequestJewelBankInfo(int aIndex);
	void JewelBankInfoRecv(SDHP_JEWELBANK_INFO_RECV* lpMsg);
	void GCJewelBankInfoSend(LPOBJ lpObj);

private:
	int GetJewelBankFieldValue(LPOBJ lpObj, int type);
	void AddJewelBankFieldValue(LPOBJ lpObj, int type, int count);
};

extern CCustomJewelBank gCustomJewelBank;
