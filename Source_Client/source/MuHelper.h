// MuHelper.h: the actual MU Helper auto-play engine (attack/heal/buff/
// pickup/repair loop). This is the real, previously-tested implementation --
// ported from Source_ClientHelper\source\MuHelper in the sibling project,
// which itself is an adaptation of a public forum MU Helper package to this
// 5.2~6.3 codebase. See MuHelper.cpp for the (few) changes made porting it
// here, mainly around server-side persistence, which isn't supported by
// this project's classic server.
#pragma once

#include <functional>
#include <array>
#include <set>
#include <string>
#include <thread>
#include <atomic>

#include "MuHelperData.h"

namespace MUHelper
{
	class CMuHelper
	{
	public:
		CMuHelper();
		~CMuHelper() = default;

		static void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

		ConfigData GetConfig() const;
		void Save(const ConfigData& config);
		void Load(const ConfigData& config);

		// Packs/unpacks ConfigData to/from the fixed MUHELPER_SAVEDATA_SIZE
		// wire format sent over the 0xAE Helper-data packet (see WSclient.h
		// and GameServer's Helper.h -- this is real server-side persistence
		// via the HelperData DB table, despite the name being a holdover
		// from that unrelated vanilla feature). Compact bit/byte packed,
		// not a raw struct dump (ConfigData has std::array/std::set members
		// that don't have a stable wire layout).
		void SerializeConfig(BYTE* pOutBuffer) const;
		void DeserializeConfig(const BYTE* pBuffer, ConfigData& outConfig) const;
		void Start();
		void Stop();
		void Toggle();
		void TriggerStart();
		void TriggerStop();
		bool IsActive() { return m_bActive; }
		void AddCost(int iCost) { m_iTotalCost += iCost; }
		int GetTotalCost() { return m_iTotalCost; }

		void AddTarget(int iTargetId, bool bIsAttacking);
		void DeleteTarget(int iTargetId);
		void DeleteAllTargets();

		void AddItem(int iItemId, POINT posDropped);
		void DeleteItem(int iItemId);
		BYTE ReadyPressRButton;
		int SimulateMove(POINT posMove);
		void SimulateMoveToPos(POINT pos);
		bool m_bForcePathMove = false;

		void WorkLoop(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
		void Work();
		int ActivatePet();
		int Buff();
		int BuffTarget(CHARACTER* pTargetChar, int iBuffSkill);
		int RecoverHealth();
		int Heal();
		int HealSelf(int iHealingSkill);
		int DrainLife();
		int ConsumePotion();
		int Attack();
		int RepairEquipments();
		int Regroup();
		int SelectAttackSkill();
		int SimulateAttack(int iSkill);
		int SimulateSkill(int iSkill, bool bTargetRequired, int iTarget);
		int SimulateComboAttack();
		int GetNearestTarget();
		int GetFarthestAttackingTarget();
		void CleanupTargets();
		void ScanNearbyTargets();
		int ComputeDistanceByRange(int iRange);
		int ComputeDistanceFromTarget(CHARACTER* pTarget);
		int ComputeDistanceBetween(POINT posA, POINT posB);
		int ObtainItem();
		int SelectItemToObtain();
		bool ShouldObtainItem(int iItemId);
		int GetHealingSkill();
		int GetDrainLifeSkill();
		bool HasAssignedBuffSkill();

		ConfigData m_config;
		POINT m_posOriginal;
		std::thread m_timerThread;
		std::atomic<bool> m_bActive;
		std::set<int> m_setTargets;
		std::set<int> m_setTargetsAttacking;
		std::set<int> m_setItems;
		int m_iCurrentItem;
		int m_iCurrentTarget;
		int m_iCurrentBuffIndex;
		int m_iCurrentBuffPartyIndex;
		int m_iCurrentHealPartyIndex;
		int m_iComboState;
		int m_iCurrentSkill;
		int m_iHuntingDistance;
		int m_iObtainingDistance;
		int m_iLoopCounter;
		int m_iSecondsElapsed;
		int m_iSecondsAway;
		bool m_bTimerActivatedBuffOngoing;
		bool m_bPetActivated;
		int m_iTotalCost;
		DWORD m_dwLastRepairCheck = 0;
	};

	extern CMuHelper g_MuHelper;
}
