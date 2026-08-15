// MuHelperData.h: MU Helper config struct.
//
// Ported from the working implementation in the sibling project
// (Source_ClientHelper\source\MuHelper), itself adapted from a public forum
// MU Helper package for this same 5.2~6.3 codebase lineage. Differs from
// that source in one way: ConfigDataSerDe (server-side persistence via a
// custom PRECEIVE_MUHELPER_DATA packet) is dropped -- this project's
// classic server has no MU Helper protocol support, so config only lives
// in memory for now (reset on relaunch). Follow-up work if persistence is
// wanted: either add local file save/load, or a real server-side packet.
#pragma once

#include <cstdint>
#include <set>
#include <array>
#include <string>

namespace MUHelper
{
	// Wire size of the 0xAE Helper-data blob (see WSclient.h's
	// PRECEIVE_HELPER_DATA and GameServer's Helper.h) that
	// CMuHelper::SerializeConfig/DeserializeConfig pack ConfigData into.
	// Must match MuOnline.HelperData.Data's DB column width.
	constexpr int MUHELPER_SAVEDATA_SIZE = 320;

	// Fixed-width slot for each extra-item filter name in that wire format
	// (18 usable chars + null), and how many slots fit. 14 slots fit in the
	// remaining budget after the 35 bytes of scalar fields
	// (35 + 14*20 = 315, leaving a few spare bytes in MUHELPER_SAVEDATA_SIZE).
	// Also caps NewUIMuHelper.cpp's "Add Extra Item" text input length.
	constexpr int MUHELPER_ITEM_NAME_SIZE = 20;
	constexpr int MUHELPER_MAX_SAVED_ITEMS = 14;

	enum ESkillActivationBase : uint32_t
	{
		ALWAYS = 0x00000000,
		ON_TIMER = 0x00000001,
		ON_CONDITION = 0x00000002,
	};

	enum ESkillActivationPreCon : uint32_t
	{
		ON_MOBS_NEARBY = 0x00000004,
		ON_MOBS_ATTACKING = 0x00000008
	};

	enum ESkillActivationSubCon : uint32_t
	{
		ON_MORE_THAN_TWO_MOBS = 0x00000010,
		ON_MORE_THAN_THREE_MOBS = 0x00000020,
		ON_MORE_THAN_FOUR_MOBS = 0x00000040,
		ON_MORE_THAN_FIVE_MOBS = 0x00000080
	};

	DEFINE_ENUM_FLAG_OPERATORS(ESkillActivationBase);
	DEFINE_ENUM_FLAG_OPERATORS(ESkillActivationPreCon);
	DEFINE_ENUM_FLAG_OPERATORS(ESkillActivationSubCon);

	constexpr uint32_t MUHELPER_SKILL_PRECON_CLEAR =
		~(static_cast<uint32_t>(ON_MOBS_NEARBY) |
			static_cast<uint32_t>(ON_MOBS_ATTACKING));

	constexpr uint32_t MUHELPER_SKILL_SUBCON_CLEAR =
		~(static_cast<uint32_t>(ON_MORE_THAN_TWO_MOBS) |
			static_cast<uint32_t>(ON_MORE_THAN_THREE_MOBS) |
			static_cast<uint32_t>(ON_MORE_THAN_FOUR_MOBS) |
			static_cast<uint32_t>(ON_MORE_THAN_FIVE_MOBS));

	enum EPetAttackMode : BYTE
	{
		PET_ATTACK_CEASE = 0x00,
		PET_ATTACK_AUTO = 0x01,
		PET_ATTACK_TOGETHER = 0x02
	};

	typedef struct
	{
		int iHuntingRange = 0;

		bool bLongRangeCounterAttack = false;
		bool bReturnToOriginalPosition = false;
		int iMaxSecondsAway = 0;

		std::array<uint32_t, 3> aiSkill = { 0, 0, 0 };
		std::array<uint32_t, 3> aiSkillCondition = { 0, 0, 0 };
		std::array<uint32_t, 3> aiSkillInterval = { 0, 0, 0 };

		bool bUseCombo = false;

		std::array<uint32_t, 3> aiBuff = { 0, 0, 0 };

		bool bBuffDuration = false;
		bool bBuffDurationParty = false;
		int iBuffCastInterval = 0;

		bool bAutoHeal = false;
		int iHealThreshold = 0;
		bool bSupportParty = false;
		bool bAutoHealParty = false;
		int iHealPartyThreshold = 0;

		bool bUseHealPotion = false;
		int iPotionThreshold = 0;

		bool bUseDrainLife = false;
		bool bUseDarkRaven = false;
		int iDarkRavenMode = 0;

		bool bRepairItem = false;
		bool StartOffline = false;

		int iObtainingRange = 0;
		bool bPickAllItems = false;
		bool bPickSelectItems = false;
		bool bPickJewel = false;
		bool bPickZen = false;
		bool bPickAncient = false;
		bool bPickExcellent = false;
		bool bPickExtraItems = false;
		std::set<std::string> aExtraItems;

		bool bUseSelfDefense = false;
		bool bAutoAcceptFriend = false;
		bool bAutoAcceptGuild = false;
		bool bAutoReset = false;
	} ConfigData;
}
