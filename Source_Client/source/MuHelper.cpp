// MuHelper.cpp: MU Helper auto-play engine.
//
// Ported from Source_ClientHelper\source\MuHelper (sibling project) with one
// deliberate change: that version drives Start/Stop through a client<->server
// round trip (TriggerStart/TriggerStop send a status-change packet, the
// server's reply is what actually calls Start()/Stop() back on WSclient.cpp's
// receive path) and persists config via a PRECEIVE_MUHELPER_DATA packet.
// This project's classic server has neither of those, so here Toggle() calls
// Start()/Stop() directly and Save() just keeps the config in memory -- see
// MuHelperData.h for the same note. Everything else (the actual bot logic)
// is unchanged from the working reference.
#include "stdafx.h"

#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>
#include <mutex>
#include <string>

#include "ZzzAI.h"
#include "ZzzCharacter.h"
#include "ZzzInterface.h"
#include "NewUISystem.h"
#include "Utilities/Log/muConsoleDebug.h"
#include "SkillManager.h"
#include "MapManager.h"
#include "wsclientinline.h"

#include "MuHelper.h"

#include "ZzzInventory.h"

constexpr int MAX_ACTIONABLE_DISTANCE = 10;

std::mutex _targetsLock;
std::mutex _itemsLock;

// Key->pointer lookup helper; FindCharacterIndex() returns MAX_CHARACTERS_CLIENT
// (one past the end of the array) when nothing matches.
static inline CHARACTER* FindCharacterByKey(int iKey)
{
	int iIndex = FindCharacterIndex(iKey);
	if (iIndex < 0 || iIndex >= MAX_CHARACTERS_CLIENT)
	{
		return NULL;
	}
	return &CharactersClient[iIndex];
}

// Derived from the existing globals: PartyNumber (extern int, ZzzInventory.h)
// and PARTY_t::index (>=0 once the party member has been resolved to a
// CharactersClient slot).
static inline bool IsPartyActive()
{
	return PartyNumber > 0;
}

static inline CHARACTER* GetPartyMemberChar(PARTY_t* pMember)
{
	if (!pMember || pMember->index < 0 || pMember->index >= MAX_CHARACTERS_CLIENT)
	{
		return NULL;
	}
	return &CharactersClient[pMember->index];
}

// Zen/jewel identification and display-name helpers.
static inline bool IsMoneyItem(ITEM* pItem)
{
	return pItem && pItem->Type == ITEM_POTION + 15;
}

static inline bool IsJewelItem(ITEM* pItem)
{
	if (!pItem)
	{
		return false;
	}

	switch (pItem->Type)
	{
	case ITEM_POTION + 13:
	case ITEM_POTION + 14:
	case ITEM_POTION + 16:
	case ITEM_POTION + 22:
	case ITEM_POTION + 31:
	case ITEM_POTION + 42:
		return true;
	default:
		return false;
	}
}

static inline std::string GetItemDisplayName(ITEM* pItem)
{
	if (!pItem)
	{
		return std::string();
	}

	char szName[256] = { 0 };
	GetItemName(pItem->Type, (pItem->Level >> 3) & 15, szName);
	return std::string(szName);
}

// Actually firing a skill is normally wired into the click-driven state
// machine (AttackWizard/AttackKnight/etc in ZzzInterface.cpp) rather than
// exposed as a standalone call. MU Helper doesn't need any of that visual/UI
// plumbing -- it can just send the 0x19 magic-cast request directly
// (SendRequestMagic) and let the server validate range/cooldown/etc.
// Returns 1 on cast, 0 if the skill is still on cooldown.
static int ExecuteSkill(CHARACTER* c, int iSkill, float fSkillDistance)
{
	UNREFERENCED_PARAMETER(fSkillDistance);

	// CheckSkillDelay() takes a slot index into CharacterAttribute->Skill[]
	// (the character's learned-skill list), NOT the AT_SKILL_* id itself --
	// same conversion NewUIMainFrameWindow's GetSkillIndex()/UseHotKey() do
	// before calling it.
	int iSkillIndex = g_pSkillList->GetSkillIndex(iSkill);
	if (iSkillIndex == -1)
	{
		return 0;
	}

	if (!gSkillManager.CheckSkillDelay(iSkillIndex))
	{
		return 0;
	}

	extern MovementSkill g_MovementSkill;
	WORD wTargetKey = 0xFFFF;
	int iTargetIndex = g_MovementSkill.m_iTarget;
	if (iTargetIndex >= 0 && iTargetIndex < MAX_CHARACTERS_CLIENT)
	{
		wTargetKey = CharactersClient[iTargetIndex].Key;
	}

	// Evil Spirit (and its Twister sibling) aren't locked-on projectiles --
	// they're client-authoritative directional AoE skills fired from the
	// caster's own position, at an angle toward the target.
	// SendRequestMagic() alone only starts the cast; the server never gets
	// told where to apply damage unless SendRequestMagicContinue (0x1E)
	// follows with the caster's position/facing angle, exactly like
	// UseSkillWizard() sends for a manually-pressed cast.
	if (iSkill == AT_SKILL_EVIL || iSkill == AT_SKILL_STORM)
	{
		if (iTargetIndex < 0 || iTargetIndex >= MAX_CHARACTERS_CLIENT)
		{
			return 0;
		}

		OBJECT* pTargetObj = &CharactersClient[iTargetIndex].Object;
		float fAngle = CreateAngle(c->Object.Position[0], c->Object.Position[1], pTargetObj->Position[0], pTargetObj->Position[1]);
		c->Object.Angle[2] = fAngle;

		SendRequestMagicContinue(iSkill, (int)c->PositionX, (int)c->PositionY, (BYTE)(fAngle / 360.f * 256.f), 0, 0, 0xFFFF, &c->Object.m_bySkillSerialNum);
		SetPlayerMagic(c);
		return 1;
	}

	SendRequestMagic(iSkill, wTargetKey);
	return 1;
}

namespace MUHelper
{
	CMuHelper g_MuHelper;

	CMuHelper::CMuHelper()
	{
		this->ReadyPressRButton = 0;
		this->m_bForcePathMove = false;
		this->m_bActive = false;
		this->m_posOriginal = { 0, 0 };
		this->m_iCurrentItem = 0;
		this->m_iCurrentTarget = 0;
		this->m_iCurrentBuffIndex = 0;
		this->m_iCurrentBuffPartyIndex = 0;
		this->m_iCurrentHealPartyIndex = 0;
		this->m_iComboState = 0;
		this->m_iCurrentSkill = 0;
		this->m_iHuntingDistance = 0;
		this->m_iObtainingDistance = 0;
		this->m_iLoopCounter = 0;
		this->m_iSecondsElapsed = 0;
		this->m_iSecondsAway = 0;
		this->m_bTimerActivatedBuffOngoing = false;
		this->m_bPetActivated = false;
		this->m_iTotalCost = 0;

		this->m_setTargets.clear();
		this->m_setTargetsAttacking.clear();
		this->m_setItems.clear();

		// Sensible out-of-the-box defaults -- there's no config UI wired up
		// for most of these yet (only hunting range and the basic skill, via
		// NewUIMuHelper.cpp), so without this the bot would just stand
		// still with everything zeroed/off. "Save Setting"/"Initialization"
		// in the config window still aren't wired to anything, so these are
		// the only defaults that currently apply.
		this->m_config.iHuntingRange = 8;
		this->m_config.iObtainingRange = 8;
		this->m_config.bUseHealPotion = true;
		this->m_config.iPotionThreshold = 50;
		this->m_config.bPickAllItems = true;
	}

	void CALLBACK CMuHelper::TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
	{
		g_MuHelper.WorkLoop(hwnd, uMsg, idEvent, dwTime);
	}

	void CMuHelper::Save(const ConfigData& config)
	{
		// No server-side persistence -- see the file header comment.
		m_config = config;
	}

	void CMuHelper::Load(const ConfigData& config)
	{
		m_config = config;
	}

	void CMuHelper::SerializeConfig(BYTE* pOutBuffer) const
	{
		ZeroMemory(pOutBuffer, MUHELPER_SAVEDATA_SIZE);
		int pos = 0;

		auto WriteByte = [&](int v) { pOutBuffer[pos++] = (BYTE)v; };
		auto WriteWord = [&](int v) { pOutBuffer[pos++] = (BYTE)(v & 0xFF); pOutBuffer[pos++] = (BYTE)((v >> 8) & 0xFF); };

		WriteByte(m_config.iHuntingRange);

		BYTE flags1 = (m_config.bLongRangeCounterAttack << 0) | (m_config.bReturnToOriginalPosition << 1) |
			(m_config.bUseCombo << 2) | (m_config.bBuffDuration << 3) | (m_config.bBuffDurationParty << 4) |
			(m_config.bAutoHeal << 5) | (m_config.bSupportParty << 6) | (m_config.bAutoHealParty << 7);
		WriteByte(flags1);

		BYTE flags2 = (m_config.bUseHealPotion << 0) | (m_config.bUseDrainLife << 1) | (m_config.bUseDarkRaven << 2) |
			(m_config.bRepairItem << 3) | (m_config.StartOffline << 4) | (m_config.bPickAllItems << 5) |
			(m_config.bPickSelectItems << 6) | (m_config.bPickJewel << 7);
		WriteByte(flags2);

		BYTE flags3 = (m_config.bPickZen << 0) | (m_config.bPickAncient << 1) | (m_config.bPickExcellent << 2) |
			(m_config.bPickExtraItems << 3) | (m_config.bUseSelfDefense << 4) | (m_config.bAutoAcceptFriend << 5) |
			(m_config.bAutoAcceptGuild << 6) | (m_config.bAutoReset << 7);
		WriteByte(flags3);

		WriteWord(m_config.iMaxSecondsAway);

		for (int i = 0; i < 3; i++) WriteWord(m_config.aiSkill[i]);
		for (int i = 0; i < 3; i++) WriteByte(m_config.aiSkillCondition[i]);
		for (int i = 0; i < 3; i++) WriteWord(m_config.aiSkillInterval[i]);
		for (int i = 0; i < 3; i++) WriteWord(m_config.aiBuff[i]);

		WriteWord(m_config.iBuffCastInterval);
		WriteByte(m_config.iHealThreshold);
		WriteByte(m_config.iHealPartyThreshold);
		WriteByte(m_config.iPotionThreshold);
		WriteByte(m_config.iDarkRavenMode);
		WriteByte(m_config.iObtainingRange);

		int iItemCount = (int)m_config.aExtraItems.size();
		if (iItemCount > MUHELPER_MAX_SAVED_ITEMS) iItemCount = MUHELPER_MAX_SAVED_ITEMS;
		WriteByte(iItemCount);

		int iWritten = 0;
		for (const auto& item : m_config.aExtraItems)
		{
			if (iWritten >= iItemCount) break;

			char szName[MUHELPER_ITEM_NAME_SIZE] = {};
			strncpy_s(szName, item.c_str(), MUHELPER_ITEM_NAME_SIZE - 1);
			memcpy(&pOutBuffer[pos], szName, MUHELPER_ITEM_NAME_SIZE);
			pos += MUHELPER_ITEM_NAME_SIZE;
			iWritten++;
		}
	}

	void CMuHelper::DeserializeConfig(const BYTE* pBuffer, ConfigData& outConfig) const
	{
		ConfigData config;
		int pos = 0;

		auto ReadByte = [&]() -> int { return pBuffer[pos++]; };
		auto ReadWord = [&]() -> int { int v = pBuffer[pos] | (pBuffer[pos + 1] << 8); pos += 2; return v; };

		config.iHuntingRange = ReadByte();

		BYTE flags1 = (BYTE)ReadByte();
		config.bLongRangeCounterAttack = (flags1 >> 0) & 1;
		config.bReturnToOriginalPosition = (flags1 >> 1) & 1;
		config.bUseCombo = (flags1 >> 2) & 1;
		config.bBuffDuration = (flags1 >> 3) & 1;
		config.bBuffDurationParty = (flags1 >> 4) & 1;
		config.bAutoHeal = (flags1 >> 5) & 1;
		config.bSupportParty = (flags1 >> 6) & 1;
		config.bAutoHealParty = (flags1 >> 7) & 1;

		BYTE flags2 = (BYTE)ReadByte();
		config.bUseHealPotion = (flags2 >> 0) & 1;
		config.bUseDrainLife = (flags2 >> 1) & 1;
		config.bUseDarkRaven = (flags2 >> 2) & 1;
		config.bRepairItem = (flags2 >> 3) & 1;
		config.StartOffline = (flags2 >> 4) & 1;
		config.bPickAllItems = (flags2 >> 5) & 1;
		config.bPickSelectItems = (flags2 >> 6) & 1;
		config.bPickJewel = (flags2 >> 7) & 1;

		BYTE flags3 = (BYTE)ReadByte();
		config.bPickZen = (flags3 >> 0) & 1;
		config.bPickAncient = (flags3 >> 1) & 1;
		config.bPickExcellent = (flags3 >> 2) & 1;
		config.bPickExtraItems = (flags3 >> 3) & 1;
		config.bUseSelfDefense = (flags3 >> 4) & 1;
		config.bAutoAcceptFriend = (flags3 >> 5) & 1;
		config.bAutoAcceptGuild = (flags3 >> 6) & 1;
		config.bAutoReset = (flags3 >> 7) & 1;

		config.iMaxSecondsAway = ReadWord();

		for (int i = 0; i < 3; i++) config.aiSkill[i] = ReadWord();
		for (int i = 0; i < 3; i++) config.aiSkillCondition[i] = ReadByte();
		for (int i = 0; i < 3; i++) config.aiSkillInterval[i] = ReadWord();
		for (int i = 0; i < 3; i++) config.aiBuff[i] = ReadWord();

		config.iBuffCastInterval = ReadWord();
		config.iHealThreshold = ReadByte();
		config.iHealPartyThreshold = ReadByte();
		config.iPotionThreshold = ReadByte();
		config.iDarkRavenMode = ReadByte();
		config.iObtainingRange = ReadByte();

		int iItemCount = ReadByte();
		if (iItemCount > MUHELPER_MAX_SAVED_ITEMS) iItemCount = MUHELPER_MAX_SAVED_ITEMS;

		for (int i = 0; i < iItemCount; i++)
		{
			char szName[MUHELPER_ITEM_NAME_SIZE + 1] = {};
			memcpy(szName, &pBuffer[pos], MUHELPER_ITEM_NAME_SIZE);
			pos += MUHELPER_ITEM_NAME_SIZE;

			if (szName[0] != '\0')
			{
				config.aExtraItems.insert(szName);
			}
		}

		outConfig = config;
	}

	ConfigData CMuHelper::GetConfig() const {
		return m_config;
	}

	void CMuHelper::Toggle()
	{
		if (m_bActive)
		{
			Stop();
		}
		else
		{
			Start();
		}
	}

	void CMuHelper::TriggerStart()
	{
		Start();
	}

	void CMuHelper::TriggerStop()
	{
		Stop();
	}

	void CMuHelper::Start()
	{
		if (m_bActive || !Hero)
		{
			return;
		}

		m_iTotalCost = 0;
		m_iComboState = 0;
		m_iCurrentBuffIndex = 0;
		m_iCurrentBuffPartyIndex = 0;
		m_iCurrentTarget = -1;
		m_iCurrentSkill = m_config.aiSkill[0];
		m_iCurrentItem = MAX_ITEMS;
		m_posOriginal = { Hero->PositionX, Hero->PositionY };

		m_iHuntingDistance = ComputeDistanceByRange(m_config.iHuntingRange);
		m_iObtainingDistance = ComputeDistanceByRange(m_config.iObtainingRange);

		m_iSecondsElapsed = 0;
		m_iSecondsAway = 0;

		m_bTimerActivatedBuffOngoing = false;
		m_bPetActivated = false;

		m_iLoopCounter = 0;

		m_bActive = true;

		g_ConsoleDebug->Write(MCD_NORMAL, "[MU Helper] Started");
	}

	void CMuHelper::Stop()
	{
		m_bActive = false;
		g_ConsoleDebug->Write(MCD_NORMAL, "[MU Helper] Stopped");
	}

	void CMuHelper::WorkLoop(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
	{
		if (!m_bActive)
		{
			return;
		}

		if (Hero->SafeZone)
		{
			g_ConsoleDebug->Write(MCD_NORMAL, "[MU Helper] Entered safezone. Stopping.");
			Stop();
			return;
		}

		Work();

		if (m_iLoopCounter++ == 4)
		{
			m_iSecondsElapsed++;

			if (ComputeDistanceBetween({ Hero->PositionX, Hero->PositionY }, m_posOriginal) > 1)
			{
				m_iSecondsAway++;
			}
			else
			{
				m_iSecondsAway = 0;
			}

			m_iLoopCounter = 0;
		}
	}

	void CMuHelper::Work()
	{
		try
		{
			if (!ActivatePet())
			{
				return;
			}

			if (!Buff())
			{
				return;
			}

			if (!RecoverHealth())
			{
				return;
			}

			if (!ObtainItem())
			{
				return;
			}

			if (!Regroup())
			{
				return;
			}

			Attack();

			RepairEquipments();
		}
		catch (...)
		{
			g_ConsoleDebug->Write(MCD_NORMAL, "[MU Helper] Exception occurred. Ignoring...");
		}
	}

	void CMuHelper::AddTarget(int iTargetId, bool bIsAttacking)
	{
		if (!m_bActive)
		{
			return;
		}

		CHARACTER* pTarget = FindCharacterByKey(iTargetId);
		if (!pTarget || pTarget == Hero)
		{
			return;
		}

		int iDistance = ComputeDistanceFromTarget(pTarget);

		if ((iDistance <= m_iHuntingDistance)
			|| (bIsAttacking && m_config.bLongRangeCounterAttack))
		{
			_targetsLock.lock();

			m_setTargets.insert(iTargetId);

			if (bIsAttacking)
			{
				m_setTargetsAttacking.insert(iTargetId);
			}

			_targetsLock.unlock();
		}

		if (m_config.bUseSelfDefense)
		{
			pTarget->Object.Kind = KIND_MONSTER;
			m_iCurrentTarget = iTargetId;
		}
	}

	void CMuHelper::DeleteTarget(int iTargetId)
	{
		_targetsLock.lock();

		m_setTargets.erase(iTargetId);
		m_setTargetsAttacking.erase(iTargetId);

		_targetsLock.unlock();

		if (iTargetId == m_iCurrentTarget)
		{
			m_iCurrentTarget = -1;
		}
	}

	void CMuHelper::DeleteAllTargets()
	{
		_targetsLock.lock();

		m_setTargets.clear();
		m_setTargetsAttacking.clear();

		_targetsLock.unlock();
	}

	int CMuHelper::ComputeDistanceByRange(int iRange)
	{
		return ComputeDistanceBetween({ 0, 0 }, { iRange, iRange });
	}

	int CMuHelper::ComputeDistanceFromTarget(CHARACTER* pTarget)
	{
		POINT posA, posB;

		posA = { Hero->PositionX, Hero->PositionY };
		posB = { pTarget->PositionX, pTarget->PositionY };
		int iPrevDistance = ComputeDistanceBetween(posA, posB);

		posA = { Hero->PositionX, Hero->PositionY };
		posB = { pTarget->TargetX, pTarget->TargetX };
		int iNextDistance = ComputeDistanceBetween(posA, posB);

		return min(iPrevDistance, iNextDistance);
	}

	int CMuHelper::ComputeDistanceBetween(POINT posA, POINT posB)
	{
		int iDx = posA.x - posB.x;
		int iDy = posA.y - posB.y;

		return static_cast<int>(std::ceil(std::sqrt(iDx * iDx + iDy * iDy)));
	}

	int CMuHelper::GetNearestTarget()
	{
		int iClosestMonsterId = -1;
		int iMinDistance = m_config.iHuntingRange + 1;

		std::set<int> setTargets;
		{
			_targetsLock.lock();
			setTargets = m_setTargets;
			_targetsLock.unlock();
		}

		for (const int& iMonsterId : setTargets)
		{
			int iIndex = FindCharacterIndex(iMonsterId);
			CHARACTER* pTarget = &CharactersClient[iIndex];

			int iDistance = ComputeDistanceFromTarget(pTarget);
			if (iDistance < iMinDistance)
			{
				iMinDistance = iDistance;
				iClosestMonsterId = iMonsterId;
			}
		}

		return iClosestMonsterId;
	}

	int CMuHelper::GetFarthestAttackingTarget()
	{
		int iFarthestMonsterId = -1;
		int iMaxDistance = -1;

		std::set<int> setTargets;
		{
			_targetsLock.lock();
			setTargets = m_setTargetsAttacking;
			_targetsLock.unlock();
		}

		for (const int& iMonsterId : setTargets)
		{
			int iIndex = FindCharacterIndex(iMonsterId);
			CHARACTER* pTarget = &CharactersClient[iIndex];

			int iDistance = ComputeDistanceFromTarget(pTarget);
			if (iDistance > iMaxDistance)
			{
				iMaxDistance = iDistance;
				iFarthestMonsterId = iMonsterId;
			}
		}

		return iFarthestMonsterId;
	}

	void CMuHelper::CleanupTargets()
	{
		std::set<int> setTargets;
		{
			_targetsLock.lock();
			setTargets = m_setTargets;
			_targetsLock.unlock();
		}

		for (const int& iMonsterId : setTargets)
		{
			int iIndex = FindCharacterIndex(iMonsterId);
			if (iIndex == MAX_CHARACTERS_CLIENT)
			{
				DeleteTarget(iMonsterId);
			}

			CHARACTER* pTarget = &CharactersClient[iIndex];
			if (!pTarget || (pTarget && (pTarget->Dead > 0 || !pTarget->Object.Live)))
			{
				DeleteTarget(iMonsterId);
			}
		}
	}

	// AddTarget() only ever fires reactively, off network events (monster
	// spawns into viewport, moves, or attacks). A monster that was already
	// standing in view before Start() was pressed never triggers any of
	// those, so without this active sweep of what's currently on screen,
	// the hunting range alone isn't enough to find a first target and the
	// bot just stands there.
	void CMuHelper::ScanNearbyTargets()
	{
		for (int i = 0; i < MAX_CHARACTERS_CLIENT; i++)
		{
			CHARACTER* c = &CharactersClient[i];
			if (!c->Object.Live || c->Dead > 0)
			{
				continue;
			}
			if (c->Object.Type < MODEL_MONSTER01 || c->Object.Type >= MODEL_MONSTER_END)
			{
				continue;
			}

			AddTarget(c->Key, false);
		}
	}

	int CMuHelper::ActivatePet()
	{
		if (!m_config.bUseDarkRaven)
		{
			return 1;
		}

		if (m_bPetActivated)
		{
			return 1;
		}

		if (m_config.iDarkRavenMode == PET_ATTACK_CEASE)
		{
			SendRequestPetCommand(PET_TYPE_DARK_SPIRIT, AT_PET_COMMAND_DEFAULT, 0xFFFF);
		}
		else if (m_config.iDarkRavenMode == PET_ATTACK_AUTO)
		{
			SendRequestPetCommand(PET_TYPE_DARK_SPIRIT, AT_PET_COMMAND_RANDOM, 0xFFFF);
		}
		else if (m_config.iDarkRavenMode == PET_ATTACK_TOGETHER)
		{
			SendRequestPetCommand(PET_TYPE_DARK_SPIRIT, AT_PET_COMMAND_OWNER, 0xFFFF);
		}

		m_bPetActivated = true;
		return 1;
	}

	int CMuHelper::Buff()
	{
		if (!HasAssignedBuffSkill())
		{
			return 1;
		}

		if (m_config.bSupportParty && IsPartyActive())
		{
			PARTY_t* pMember = &Party[m_iCurrentBuffPartyIndex];
			CHARACTER* pChar = GetPartyMemberChar(pMember);

			if (pChar != NULL
				&& pMember->Map == gMapManager.WorldActive
				&& ComputeDistanceFromTarget(pChar) <= MAX_ACTIONABLE_DISTANCE)
			{
				if (!m_config.bBuffDurationParty
					&& m_config.iBuffCastInterval != 0
					&& m_iSecondsElapsed % m_config.iBuffCastInterval == 0)
				{
					m_bTimerActivatedBuffOngoing = true;
				}

				if (!BuffTarget(pChar, m_config.aiBuff[m_iCurrentBuffIndex]))
				{
					return 0;
				}
			}

			m_iCurrentBuffPartyIndex = (m_iCurrentBuffPartyIndex + 1) % (sizeof(Party) / sizeof(Party[0]));
		}
		else
		{
			if (!m_config.bBuffDuration
				&& m_config.iBuffCastInterval != 0
				&& m_iSecondsElapsed % m_config.iBuffCastInterval == 0)
			{
				m_bTimerActivatedBuffOngoing = true;
			}

			if (!BuffTarget(Hero, m_config.aiBuff[m_iCurrentBuffIndex]))
			{
				return 0;
			}
		}

		if (m_iCurrentBuffPartyIndex == 0)
		{
			m_iCurrentBuffIndex = (m_iCurrentBuffIndex + 1) % m_config.aiBuff.size();

			// Reaching this branch means everyone's been buffed,
			// so we're resetting the timer activated buff flag
			if (m_iCurrentBuffIndex == 0)
			{
				m_bTimerActivatedBuffOngoing = false;
			}
		}

		return 1;
	}

	int CMuHelper::BuffTarget(CHARACTER* pTargetChar, int iBuffSkill)
	{
		if ((iBuffSkill == AT_SKILL_ATTACK
			|| iBuffSkill == AT_SKILL_ATT_POWER_UP
			|| iBuffSkill == AT_SKILL_ATT_POWER_UP + 1
			|| iBuffSkill == AT_SKILL_ATT_POWER_UP + 2
			|| iBuffSkill == AT_SKILL_ATT_POWER_UP + 3
			|| iBuffSkill == AT_SKILL_ATT_POWER_UP + 4)
			&& (!g_isCharacterBuff((&pTargetChar->Object), eBuff_Attack) || m_bTimerActivatedBuffOngoing))
		{
			return SimulateSkill(iBuffSkill, true, pTargetChar->Key);
		}

		if ((iBuffSkill == AT_SKILL_DEFENSE
			|| iBuffSkill == AT_SKILL_DEF_POWER_UP
			|| iBuffSkill == AT_SKILL_DEF_POWER_UP + 1
			|| iBuffSkill == AT_SKILL_DEF_POWER_UP + 2
			|| iBuffSkill == AT_SKILL_DEF_POWER_UP + 3
			|| iBuffSkill == AT_SKILL_DEF_POWER_UP + 4)
			&& (!g_isCharacterBuff((&pTargetChar->Object), eBuff_Defense) || m_bTimerActivatedBuffOngoing))
		{
			return SimulateSkill(iBuffSkill, true, pTargetChar->Key);
		}

		if ((iBuffSkill == AT_SKILL_INFINITY_ARROW) &&
			(!g_isCharacterBuff((&pTargetChar->Object), eBuff_InfinityArrow)))
		{
			return SimulateSkill(iBuffSkill, false, pTargetChar->Key);
		}

		if ((iBuffSkill == AT_SKILL_WIZARDDEFENSE
			|| iBuffSkill == AT_SKILL_SOUL_UP
			|| iBuffSkill == AT_SKILL_SOUL_UP + 1
			|| iBuffSkill == AT_SKILL_SOUL_UP + 2
			|| iBuffSkill == AT_SKILL_SOUL_UP + 3
			|| iBuffSkill == AT_SKILL_SOUL_UP + 4)
			&& (!g_isCharacterBuff((&pTargetChar->Object), eBuff_PhysDefense) || m_bTimerActivatedBuffOngoing))
		{
			return SimulateSkill(iBuffSkill, true, pTargetChar->Key);
		}

		if ((iBuffSkill == AT_SKILL_VITALITY
			|| iBuffSkill == AT_SKILL_LIFE_UP
			|| iBuffSkill == AT_SKILL_LIFE_UP + 1
			|| iBuffSkill == AT_SKILL_LIFE_UP + 2
			|| iBuffSkill == AT_SKILL_LIFE_UP + 3
			|| iBuffSkill == AT_SKILL_LIFE_UP + 4)
			&& (!g_isCharacterBuff((&pTargetChar->Object), eBuff_HpRecovery) || m_bTimerActivatedBuffOngoing))
		{
			if (m_iComboState == 2)
			{
				return 1;
			}

			return SimulateSkill(iBuffSkill, false, pTargetChar->Key);
		}

		// Master Skill Tree isn't implemented in this codebase (no
		// MASTER_SKILL_ADD_* skill IDs exist), so improved/enhanced Magic
		// Circle variants aren't handled here; only the base skill is checked.
		if ((iBuffSkill == AT_SKILL_SWELL_OF_MAGICPOWER)
			&& (!g_isCharacterBuff((&pTargetChar->Object), eBuff_SwellOfMagicPower)))
		{
			return SimulateSkill(iBuffSkill, false, pTargetChar->Key);
		}

		if ((iBuffSkill == AT_SKILL_ADD_CRITICAL)
			&& (!g_isCharacterBuff((&pTargetChar->Object), eBuff_AddCriticalDamage)))
		{
			return SimulateSkill(iBuffSkill, false, pTargetChar->Key);
		}

		if ((iBuffSkill == AT_SKILL_ALICE_BERSERKER)
			&& (!g_isCharacterBuff((&pTargetChar->Object), eBuff_Berserker)))
		{
			return SimulateSkill(iBuffSkill, false, pTargetChar->Key);
		}
		if ((iBuffSkill == AT_SKILL_ALICE_THORNS)
			&& (!g_isCharacterBuff((&pTargetChar->Object), eBuff_Thorns)))
		{
			return SimulateSkill(iBuffSkill, false, pTargetChar->Key);
		}

		return 1;
	}

	int CMuHelper::ConsumePotion()
	{
		int64_t iLife = CharacterAttribute->Life;
		int64_t iLifeMax = CharacterAttribute->LifeMax;

		if (m_config.bUseHealPotion && iLifeMax > 0 && iLife > 0)
		{
			int64_t iRemaining = (iLife * 100 + iLifeMax - 1) / iLifeMax;
			if (iRemaining <= m_config.iPotionThreshold)
			{
				// No FindHealingItemIndex() on CNewUIMyInventory here; reuse
				// FindItemReverseIndex() (ITEM_POTION+3..+0 = large..small
				// healing potion), preferring the strongest.
				int iPotionIndex = -1;
				for (int iPotionType = ITEM_POTION + 3; iPotionType >= ITEM_POTION; --iPotionType)
				{
					iPotionIndex = g_pMyInventory->FindItemReverseIndex(iPotionType);
					if (iPotionIndex != -1)
					{
						break;
					}
				}
				if (iPotionIndex != -1)
				{
					SendRequestUse(iPotionIndex, 0);
				}
			}
		}

		return 1;
	}

	int CMuHelper::RecoverHealth()
	{
		if (!Heal())
		{
			return 0;
		}

		if (!DrainLife())
		{
			return 0;
		}

		if (!ConsumePotion())
		{
			return 0;
		}

		return 1;
	}

	int CMuHelper::Heal()
	{
		if (!m_config.bAutoHeal)
		{
			return 1;
		}

		int iHealingSkill = GetHealingSkill();
		if (iHealingSkill == -1)
		{
			return 1;
		}

		if (m_config.bAutoHealParty && IsPartyActive())
		{
			PARTY_t* pMember = &Party[m_iCurrentHealPartyIndex];
			CHARACTER* pChar = GetPartyMemberChar(pMember);

			if (pChar != NULL)
			{
				if (pChar == Hero)
				{
					return HealSelf(iHealingSkill);
				}
				else if (pMember->Map == gMapManager.WorldActive
					&& pMember->stepHP * 10 <= m_config.iHealPartyThreshold
					&& ComputeDistanceFromTarget(pChar) <= MAX_ACTIONABLE_DISTANCE)
				{
					return SimulateSkill(iHealingSkill, true, pChar->Key);
				}
			}
			m_iCurrentHealPartyIndex = (m_iCurrentHealPartyIndex + 1) % (sizeof(Party) / sizeof(Party[0]));
		}
		else
		{
			return HealSelf(iHealingSkill);
		}

		return 1;
	}

	int CMuHelper::HealSelf(int iHealingSkill)
	{
		int64_t iLife = CharacterAttribute->Life;
		int64_t iLifeMax = CharacterAttribute->LifeMax;
		int64_t iRemaining = (iLife * 100 + iLifeMax - 1) / iLifeMax;

		if (iRemaining <= m_config.iHealThreshold)
		{
			return SimulateSkill(iHealingSkill, true, HeroKey);
		}

		return 1;
	}

	int CMuHelper::DrainLife()
	{
		if (!m_config.bUseDrainLife)
		{
			return 1;
		}

		int iDrainLife = GetDrainLifeSkill();
		if (iDrainLife == -1)
		{
			return 1;
		}

		int64_t iLife = CharacterAttribute->Life;
		int64_t iLifeMax = CharacterAttribute->LifeMax;
		int64_t iRemaining = (iLife * 100 + iLifeMax - 1) / iLifeMax;

		if (iRemaining <= m_config.iHealThreshold)
		{
			m_iCurrentTarget = GetNearestTarget();
			if (m_iCurrentTarget != -1)
			{
				return SimulateSkill(iDrainLife, true, m_iCurrentTarget);
			}
		}

		return 1;
	}

	int CMuHelper::RepairEquipments()
	{
		if (m_config.bRepairItem)
		{
			// Repairing is instant server-side (one request fully restores
			// durability), so there's no need to re-check every 250ms tick --
			// that just floods 0x34 requests while gear is wearing down in
			// combat. Throttle the whole check to once every 5s.
			DWORD dwNow = GetTickCount();
			if (dwNow - m_dwLastRepairCheck < 5000)
			{
				return 1;
			}
			m_dwLastRepairCheck = dwNow;

			for (int i = 0; i < MAX_EQUIPMENT; i++)
			{
				ITEM* pItem = &CharacterMachine->Equipment[i];
				if (!pItem || pItem->Type == -1)
				{
					continue;
				}

				ITEM_ATTRIBUTE* pAttr = &ItemAttribute[pItem->Type];
				if (!pAttr)
				{
					continue;
				}

				int iLevel = pItem->Level;
				int iDurability = pItem->Durability;
				int iMaxDurability = calcMaxDurability(pItem, pAttr, iLevel);

				int64_t iHealth = (iDurability * 100 + iMaxDurability - 1) / iMaxDurability;

				if (iHealth <= 50)
				{
					// This codebase doesn't expose the self-repair gold-cost
					// formula client-side, so skip the affordability
					// pre-check and let the server reject the request if the
					// player can't pay for it.
					SendRequestRepair(i, 1);
				}
			}
		}

		return 1;
	}

	int CMuHelper::Attack()
	{
		ScanNearbyTargets();

		if (m_iCurrentTarget == -1)
		{
			if (!m_setTargets.empty())
			{
				CleanupTargets();

				if (m_config.bLongRangeCounterAttack)
				{
					m_iCurrentTarget = GetFarthestAttackingTarget();
				}

				if (m_iCurrentTarget == -1)
				{
					m_iCurrentTarget = GetNearestTarget();
				}
			}
			else
			{
				m_iComboState = 0;
				return 0;
			}
		}

		if (m_config.bUseCombo)
		{
			return SimulateComboAttack();
		}

		m_iCurrentSkill = SelectAttackSkill();
		if (m_iCurrentSkill > 0)
		{
			SimulateAttack(m_iCurrentSkill);
		}

		return 1;
	}

	int CMuHelper::SelectAttackSkill()
	{
		// try skill 2 activation conditions
		if (m_config.aiSkill[1] > 0 && m_config.aiSkill[1] < MAX_SKILLS)
		{
			if ((m_config.aiSkillCondition[1] & ON_TIMER)
				&& m_config.aiSkillInterval[1] != 0
				&& m_iSecondsElapsed % m_config.aiSkillInterval[1] == 0)
			{
				return m_config.aiSkill[1];
			}

			if (m_config.aiSkillCondition[1] & ON_CONDITION)
			{
				if (m_config.aiSkillCondition[1] & ON_MOBS_NEARBY)
				{
					int iCount = m_setTargets.size();

					if (((m_config.aiSkillCondition[1] & ON_MORE_THAN_TWO_MOBS) && iCount >= 2)
						|| ((m_config.aiSkillCondition[1] & ON_MORE_THAN_THREE_MOBS) && iCount >= 3)
						|| ((m_config.aiSkillCondition[1] & ON_MORE_THAN_FOUR_MOBS) && iCount >= 4)
						|| ((m_config.aiSkillCondition[1] & ON_MORE_THAN_FIVE_MOBS) && iCount >= 5))
					{
						return m_config.aiSkill[1];
					}
				}
				else if (m_config.aiSkillCondition[1] & ON_MOBS_ATTACKING)
				{
					int iCount = m_setTargetsAttacking.size();

					if (((m_config.aiSkillCondition[1] & ON_MORE_THAN_TWO_MOBS) && iCount >= 2)
						|| ((m_config.aiSkillCondition[1] & ON_MORE_THAN_THREE_MOBS) && iCount >= 3)
						|| ((m_config.aiSkillCondition[1] & ON_MORE_THAN_FOUR_MOBS) && iCount >= 4)
						|| ((m_config.aiSkillCondition[1] & ON_MORE_THAN_FIVE_MOBS) && iCount >= 5))
					{
						return m_config.aiSkill[1];
					}
				}
			}
		}

		// try skill 3 activation conditions
		if (m_config.aiSkill[2] > 0 && m_config.aiSkill[2] < MAX_SKILLS)
		{
			if ((m_config.aiSkillCondition[2] & ON_TIMER)
				&& m_config.aiSkillInterval[2] != 0
				&& m_iSecondsElapsed % m_config.aiSkillInterval[2] == 0)
			{
				return m_config.aiSkill[2];
			}

			if (m_config.aiSkillCondition[2] & ON_CONDITION)
			{
				if (m_config.aiSkillCondition[2] & ON_MOBS_NEARBY)
				{
					int iCount = m_setTargets.size();

					if (((m_config.aiSkillCondition[2] & ON_MORE_THAN_TWO_MOBS) && iCount >= 2)
						|| ((m_config.aiSkillCondition[2] & ON_MORE_THAN_THREE_MOBS) && iCount >= 3)
						|| ((m_config.aiSkillCondition[2] & ON_MORE_THAN_FOUR_MOBS) && iCount >= 4)
						|| ((m_config.aiSkillCondition[2] & ON_MORE_THAN_FIVE_MOBS) && iCount >= 5))
					{
						return m_config.aiSkill[2];
					}
				}
				else if (m_config.aiSkillCondition[2] & ON_MOBS_ATTACKING)
				{
					int iCount = m_setTargetsAttacking.size();

					if (((m_config.aiSkillCondition[2] & ON_MORE_THAN_TWO_MOBS) && iCount >= 2)
						|| ((m_config.aiSkillCondition[2] & ON_MORE_THAN_THREE_MOBS) && iCount >= 3)
						|| ((m_config.aiSkillCondition[2] & ON_MORE_THAN_FOUR_MOBS) && iCount >= 4)
						|| ((m_config.aiSkillCondition[2] & ON_MORE_THAN_FIVE_MOBS) && iCount >= 5))
					{
						return m_config.aiSkill[2];
					}
				}
			}
		}

		// no skill for activation yet, default to basic skill
		if (m_config.aiSkill[0] > 0)
		{
			return m_config.aiSkill[0];
		}

		return -1;
	}

	int CMuHelper::SimulateComboAttack()
	{
		for (int i = 0; i < m_config.aiSkill.size(); i++)
		{
			if (m_config.aiSkill[i] == 0)
			{
				return 0;
			}
		}

		if (SimulateAttack(m_config.aiSkill[m_iComboState]))
		{
			m_iComboState = (m_iComboState + 1) % 3;
		}

		return 1;
	}

	int CMuHelper::SimulateAttack(int iSkill)
	{
		return SimulateSkill(iSkill, true, m_iCurrentTarget);
	}

	int CMuHelper::SimulateSkill(int iSkill, bool bTargetRequired, int iTarget)
	{
		extern MovementSkill g_MovementSkill;
		extern int SelectedCharacter;
		extern int TargetX, TargetY;

		g_MovementSkill.m_iSkill = iSkill;
		g_MovementSkill.m_bMagic = true;

		float fSkillDistance = gSkillManager.GetSkillDistance(iSkill, Hero);

		if (bTargetRequired)
		{
			if (iTarget == -1)
			{
				return 0;
			}

			SelectedCharacter = FindCharacterIndex(iTarget);
			if (SelectedCharacter == MAX_CHARACTERS_CLIENT)
			{
				DeleteTarget(iTarget);
				return 0;
			}

			CHARACTER* pTarget = &CharactersClient[SelectedCharacter];
			if (pTarget->Dead > 0)
			{
				DeleteTarget(iTarget);
				return 0;
			}

			g_MovementSkill.m_iTarget = SelectedCharacter;

			TargetX = (int)(pTarget->Object.Position[0] / TERRAIN_SCALE);
			TargetY = (int)(pTarget->Object.Position[1] / TERRAIN_SCALE);

			PATH_t tempPath;
			bool bHasPath = PathFinding2(Hero->PositionX, Hero->PositionY, TargetX, TargetY, &tempPath, m_iHuntingDistance + fSkillDistance);
			bool bTargetNear = CheckTile(Hero, &Hero->Object, fSkillDistance);
			bool bNoWall = CheckWall(Hero->PositionX, Hero->PositionY, TargetX, TargetY);

			// target not reachable, ignore it
			if (!bHasPath)
			{
				DeleteTarget(iTarget);
				return 0;
			}

			// target is not near or the path is obstructed by a wall, move closer
			if (!bTargetNear || !bNoWall)
			{
				// PATH_t in this codebase has no Lock member -- the
				// WM_TIMER-driven MU Helper loop and the normal game tick
				// both run on the main thread, so there's no concurrent
				// writer to guard against here; plain field writes are
				// enough.

				// Limit movement to 2 steps at a time
				int pathNum = min(tempPath.PathNum, 2);
				for (int i = 0; i < pathNum; i++)
				{
					Hero->Path.PathX[i] = tempPath.PathX[i];
					Hero->Path.PathY[i] = tempPath.PathY[i];
				}
				Hero->Path.PathNum = pathNum;
				Hero->Path.CurrentPath = 0;
				Hero->Path.CurrentPathFloat = 0;

				SendMove(Hero, &Hero->Object);
				return 0;
			}
		}
		else
		{
			TargetX = Hero->PositionX;
			TargetY = Hero->PositionY;
		}

		int iSkillResult = ExecuteSkill(Hero, iSkill, fSkillDistance);
		if (iSkillResult == -1)
		{
			DeleteTarget(iTarget);
		}

		return (int)(iSkillResult == 1);
	}

	int CMuHelper::Regroup()
	{
		if (m_config.bReturnToOriginalPosition && m_iSecondsAway > m_config.iMaxSecondsAway)
		{
			if (!SimulateMove(m_posOriginal))
			{
				return 0;
			}

			m_iSecondsAway = 0;
			m_iComboState = 0;
			m_iCurrentTarget = -1;
		}

		return 1;
	}

	int CMuHelper::SimulateMove(POINT posMove)
	{
		extern int TargetX, TargetY;

		Hero->MovementType = MOVEMENT_MOVE;
		TargetX = (int)posMove.x;
		TargetY = (int)posMove.y;

		if (!CheckTile(Hero, &Hero->Object, 1.5f))
		{
			if (PathFinding2((Hero->PositionX), (Hero->PositionY), TargetX, TargetY, &Hero->Path))
			{
				SendMove(Hero, &Hero->Object);
			}
			return 0;
		}

		return 1;
	}

	bool CMuHelper::HasAssignedBuffSkill()
	{
		for (int i = 0; i < m_config.aiBuff.size(); i++)
		{
			if (m_config.aiBuff[i] != 0)
			{
				return true;
			}
		}

		return false;
	}

	int CMuHelper::GetHealingSkill()
	{
		std::vector<int> aiHealingSkills =
		{
			AT_SKILL_HEAL_UP,
			AT_SKILL_HEAL_UP + 1,
			AT_SKILL_HEAL_UP + 2,
			AT_SKILL_HEAL_UP + 3,
			AT_SKILL_HEAL_UP + 4,
			AT_SKILL_HEALING
		};

		for (int i = 0; i < aiHealingSkills.size(); i++)
		{
			int iSkillIndex = g_pSkillList->GetSkillIndex(aiHealingSkills[i]);
			if (iSkillIndex != -1)
			{
				return aiHealingSkills[i];
			}
		}

		return -1;
	}

	int CMuHelper::GetDrainLifeSkill()
	{
		std::vector<int> aiDrainLifeSkills =
		{
			AT_SKILL_ALICE_DRAINLIFE_UP,
			AT_SKILL_ALICE_DRAINLIFE_UP + 1,
			AT_SKILL_ALICE_DRAINLIFE_UP + 2,
			AT_SKILL_ALICE_DRAINLIFE_UP + 3,
			AT_SKILL_ALICE_DRAINLIFE_UP + 4,
			AT_SKILL_ALICE_DRAINLIFE,
		};

		for (int i = 0; i < aiDrainLifeSkills.size(); i++)
		{
			int iSkillIndex = g_pSkillList->GetSkillIndex(aiDrainLifeSkills[i]);
			if (iSkillIndex != -1)
			{
				return aiDrainLifeSkills[i];
			}
		}

		return -1;
	}

	int CMuHelper::ObtainItem()
	{
		if (m_iCurrentItem == MAX_ITEMS)
		{
			m_iCurrentItem = SelectItemToObtain();
			if (m_iCurrentItem == MAX_ITEMS)
			{
				return 1;
			}
		}

		ITEM_t* pDrop = &Items[m_iCurrentItem];
		ITEM* pItem = &pDrop->Item;

		if (!pDrop->Object.Live)
		{
			DeleteItem(m_iCurrentItem);
			return 1;
		}

		extern int TargetX;
		extern int TargetY;

		TargetX = (int)(Items[m_iCurrentItem].Object.Position[0] / TERRAIN_SCALE);
		TargetY = (int)(Items[m_iCurrentItem].Object.Position[1] / TERRAIN_SCALE);

		int iDistance = ComputeDistanceBetween({ Hero->PositionX, Hero->PositionY }, { TargetX, TargetY });
		if (iDistance <= m_iObtainingDistance)
		{
			if (!CheckTile(Hero, &Hero->Object, 1.5f))
			{
				if (PathFinding2((Hero->PositionX), (Hero->PositionY), TargetX, TargetY, &Hero->Path))
				{
					SendMove(Hero, &Hero->Object);
				}

				return 0;
			}
			else
			{
				SendRequestGetItem(m_iCurrentItem);
				DeleteItem(m_iCurrentItem);
			}
		}

		return 1;
	}

	bool CMuHelper::ShouldObtainItem(int iItemId)
	{
		ITEM_t* pDrop = &Items[iItemId];
		ITEM* pItem = &pDrop->Item;

		if (m_config.bPickAllItems)
		{
			return true;
		}

		if (!m_config.bPickSelectItems)
		{
			return false;
		}

		if ((m_config.bPickZen && IsMoneyItem(pItem)) ||
			(m_config.bPickJewel && IsJewelItem(pItem)) ||
			(m_config.bPickAncient && pItem->ExtOption) ||
			(m_config.bPickExcellent && pItem->Option1))
		{
			return true;
		}

		if (m_config.bPickExtraItems)
		{
			std::string strDisplayName = GetItemDisplayName(pItem);
			for (const auto& str : m_config.aExtraItems)
			{
				if (strDisplayName.find(str) != std::string::npos)
				{
					return true;
				}
			}
		}

		return false;
	}

	void CMuHelper::AddItem(int iItemId, POINT posWhere)
	{
		_itemsLock.lock();
		m_setItems.insert(iItemId);
		_itemsLock.unlock();
	}

	void CMuHelper::DeleteItem(int iItemId)
	{
		_itemsLock.lock();
		m_setItems.erase(iItemId);
		_itemsLock.unlock();

		if (iItemId == m_iCurrentItem)
		{
			m_iCurrentItem = MAX_ITEMS;
		}
	}

	int CMuHelper::SelectItemToObtain()
	{
		int iClosestItemId = MAX_ITEMS;
		int iMinDistance = m_config.iObtainingRange + 1;

		std::set<int> setItems;
		{
			_itemsLock.lock();
			setItems = m_setItems;
			_itemsLock.unlock();
		}

		for (const int& iItemId : setItems)
		{
			if (!ShouldObtainItem(iItemId))
			{
				continue;
			}

			int iItemX = (int)(Items[iItemId].Object.Position[0] / TERRAIN_SCALE);
			int iItemY = (int)(Items[iItemId].Object.Position[1] / TERRAIN_SCALE);

			int iDistance = ComputeDistanceBetween({ Hero->PositionX, Hero->PositionY }, { iItemX, iItemY });
			if (iDistance < iMinDistance)
			{
				iMinDistance = iDistance;
				iClosestItemId = iItemId;
			}
		}

		return iClosestItemId;
	}

	void CMuHelper::SimulateMoveToPos(POINT pos)
	{
		if (!Hero || !Hero->Object.Live || !m_bActive)
			return;

		m_posOriginal = pos;

		m_iSecondsAway = m_config.iMaxSecondsAway + 1;

		m_iCurrentTarget = -1;
	}
}
