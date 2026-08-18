// CustomJewelBank.cpp: implementation of the CCustomJewelBank class.
// Ported from Wizard Team EX301KOR - see CustomJewelBank.h for adaptation notes.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CustomJewelBank.h"
#include "DSProtocol.h"
#include "GameMain.h"
#include "ItemManager.h"
#include "JewelMix.h"
#include "Util.h"

CCustomJewelBank gCustomJewelBank;

CCustomJewelBank::CCustomJewelBank() // OK
{
}

CCustomJewelBank::~CCustomJewelBank() // OK
{
}

int CCustomJewelBank::GetJewelBankFieldValue(LPOBJ lpObj, int type) // OK
{
	switch (type)
	{
	case 0: return lpObj->JewelBlessCount;
	case 1: return lpObj->JewelSoulCount;
	case 2: return lpObj->JewelLifeCount;
	case 3: return lpObj->JewelCreationCount;
	case 4: return lpObj->JewelGuardianCount;
	case 5: return lpObj->JewelGemStoneCount;
	case 6: return lpObj->JewelHarmonyCount;
	case 7: return lpObj->JewelChaosCount;
	case 8: return lpObj->JewelLowStoneCount;
	case 9: return lpObj->JewelHighStoneCount;
	}

	return -1;
}

void CCustomJewelBank::AddJewelBankFieldValue(LPOBJ lpObj, int type, int count) // OK
{
	switch (type)
	{
	case 0: lpObj->JewelBlessCount += count; break;
	case 1: lpObj->JewelSoulCount += count; break;
	case 2: lpObj->JewelLifeCount += count; break;
	case 3: lpObj->JewelCreationCount += count; break;
	case 4: lpObj->JewelGuardianCount += count; break;
	case 5: lpObj->JewelGemStoneCount += count; break;
	case 6: lpObj->JewelHarmonyCount += count; break;
	case 7: lpObj->JewelChaosCount += count; break;
	case 8: lpObj->JewelLowStoneCount += count; break;
	case 9: lpObj->JewelHighStoneCount += count; break;
	}
}

void CCustomJewelBank::JewelBankDepositRecv(PMSG_JEWELBANK_DEPOSIT_RECV* lpMsg, int aIndex) // OK
{
	LPOBJ lpObj = &gObj[aIndex];

	if (gObjIsConnectedGP(aIndex) == 0)
	{
		return;
	}

	if (lpObj->Interface.use != 0 && lpObj->Interface.type != INTERFACE_COMMON)
	{
		return;
	}

	if (lpObj->ChaosLock != 0)
	{
		return;
	}

	int Slot = lpMsg->slot;

	if (INVENTORY_FULL_RANGE(Slot) == 0)
	{
		return;
	}

	if (lpObj->Inventory[Slot].IsItem() == 0)
	{
		return;
	}

	int Type = -1;

	for (int n = 0; n < 10; n++)
	{
		if (lpObj->Inventory[Slot].m_Index == gJewelMix.GetJewelSimpleIndex(n))
		{
			Type = n;
			break;
		}
	}

	if (Type < 0)
	{
		return;
	}

	gItemManager.DeleteInventoryItemCount(lpObj, gJewelMix.GetJewelSimpleIndex(Type), 0, 1);

	this->AddJewelBankFieldValue(lpObj, Type, 1);

	SDHP_JEWELBANK_SAVE_SEND pMsg = { 0 };
	pMsg.header.set(0xF7, sizeof(pMsg));
	pMsg.subcode = 0x04;
	memcpy(pMsg.account, lpObj->Account, sizeof(pMsg.account));
	pMsg.type = Type;
	pMsg.count = 1;
	gDataServerConnection.DataSend((BYTE*)&pMsg, sizeof(pMsg));

	this->GCJewelBankInfoSend(lpObj);
}

void CCustomJewelBank::JewelBankWithdrawRecv(PMSG_JEWELBANK_WITHDRAW_RECV* lpMsg, int aIndex) // OK
{
	LPOBJ lpObj = &gObj[aIndex];

	if (gObjIsConnectedGP(aIndex) == 0)
	{
		return;
	}

	if (lpObj->Interface.use != 0 && lpObj->Interface.type != INTERFACE_COMMON)
	{
		return;
	}

	if (lpObj->ChaosLock != 0)
	{
		return;
	}

	int Type = lpMsg->type;
	int Count = lpMsg->count;

	if (Type < 0 || Type > 9 || Count < 0)
	{
		return;
	}

	int Available = this->GetJewelBankFieldValue(lpObj, Type);

	if (Available <= 0)
	{
		return;
	}

	int WithdrawCount = (Count == 99) ? Available : Count;

	if (WithdrawCount > Available)
	{
		return;
	}

	int FreeSpaces = gItemManager.GetInventoryEmptySlotCount(lpObj);

	if (FreeSpaces <= 0 || WithdrawCount <= 0)
	{
		return;
	}

	if (WithdrawCount > FreeSpaces)
	{
		WithdrawCount = FreeSpaces;
	}

	this->AddJewelBankFieldValue(lpObj, Type, -WithdrawCount);

	for (int n = 0; n < WithdrawCount; n++)
	{
		GDCreateItemSend(aIndex, 0xEB, 0, 0, gJewelMix.GetJewelSimpleIndex(Type), 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0xFF, 0);
	}

	SDHP_JEWELBANK_SAVE_SEND pMsg = { 0 };
	pMsg.header.set(0xF7, sizeof(pMsg));
	pMsg.subcode = 0x04;
	memcpy(pMsg.account, lpObj->Account, sizeof(pMsg.account));
	pMsg.type = Type;
	pMsg.count = -WithdrawCount;
	gDataServerConnection.DataSend((BYTE*)&pMsg, sizeof(pMsg));

	this->GCJewelBankInfoSend(lpObj);
}

void CCustomJewelBank::RequestJewelBankInfo(int aIndex) // OK
{
	LPOBJ lpObj = &gObj[aIndex];

	if (gObjIsConnectedGP(aIndex) == 0)
	{
		return;
	}

	SDHP_JEWELBANK_INFO_REQUEST_SEND pMsg = { 0 };
	pMsg.header.set(0xF7, sizeof(pMsg));
	pMsg.subcode = 0x05;
	pMsg.index = lpObj->Index;
	memcpy(pMsg.account, lpObj->Account, sizeof(pMsg.account));
	gDataServerConnection.DataSend((BYTE*)&pMsg, sizeof(pMsg));
}

void CCustomJewelBank::JewelBankInfoRecv(SDHP_JEWELBANK_INFO_RECV* lpMsg) // OK
{
	if (OBJECT_RANGE(lpMsg->index) == 0)
	{
		return;
	}

	LPOBJ lpObj = &gObj[lpMsg->index];

	if (gObjIsConnectedGP(lpMsg->index) == 0)
	{
		return;
	}

	lpObj->JewelBlessCount = lpMsg->Bless;
	lpObj->JewelSoulCount = lpMsg->Soul;
	lpObj->JewelLifeCount = lpMsg->Life;
	lpObj->JewelCreationCount = lpMsg->Creation;
	lpObj->JewelGuardianCount = lpMsg->Guardian;
	lpObj->JewelGemStoneCount = lpMsg->GemStone;
	lpObj->JewelHarmonyCount = lpMsg->Harmony;
	lpObj->JewelChaosCount = lpMsg->Chaos;
	lpObj->JewelLowStoneCount = lpMsg->LowStone;
	lpObj->JewelHighStoneCount = lpMsg->HighStone;

	this->GCJewelBankInfoSend(lpObj);
}

void CCustomJewelBank::GCJewelBankInfoSend(LPOBJ lpObj) // OK
{
	if (gObjIsConnectedGP(lpObj->Index) == 0)
	{
		return;
	}

	PMSG_JEWELBANK_INFO_SEND pMsg = { 0 };
	pMsg.header.set(0xF3, 0xF7, sizeof(pMsg));
	pMsg.Bless = lpObj->JewelBlessCount;
	pMsg.Soul = lpObj->JewelSoulCount;
	pMsg.Life = lpObj->JewelLifeCount;
	pMsg.Creation = lpObj->JewelCreationCount;
	pMsg.Guardian = lpObj->JewelGuardianCount;
	pMsg.GemStone = lpObj->JewelGemStoneCount;
	pMsg.Harmony = lpObj->JewelHarmonyCount;
	pMsg.Chaos = lpObj->JewelChaosCount;
	pMsg.LowStone = lpObj->JewelLowStoneCount;
	pMsg.HighStone = lpObj->JewelHighStoneCount;

	DataSend(lpObj->Index, (BYTE*)&pMsg, pMsg.header.size);
}
