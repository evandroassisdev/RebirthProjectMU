#pragma once

#define LUA_SCRIPT_CODE_ERROR0 "[No argument's allowed]"
#define LUA_SCRIPT_CODE_ERROR1 "[%d argument's expected]"
#define LUA_SCRIPT_CODE_ERROR2 "[Minimum %d argument's expected]"

void InitLuaFunction(lua_State* L);
int LuaRequire(lua_State* state);
int LuaGetMaxIndex(lua_State* L);
int LuaGetMinUserIndex(lua_State* L);
int LuaGetMaxUserIndex(lua_State* L);
int LuaGetMinMonsterIndex(lua_State* L);
int LuaGetMaxMonsterIndex(lua_State* L);
int LuaGetGameServerCode(lua_State* L);
int LuaGetGameServerVersion(lua_State* L);
int LuaGetGameServerProtocol(lua_State* L);
int LuaGetGameServerCurUser(lua_State* L);
int LuaGetGameServerMaxUser(lua_State* L);
int LuaGetLicenseId(lua_State* L);
int LuaGetLicenseCustomerName(lua_State* L);
int LuaGetObjectConnected(lua_State* L);
int LuaGetObjectIpAddress(lua_State* L);
int LuaGetObjectHardwareId(lua_State* L);
int LuaGetObjectType(lua_State* L);
int LuaGetObjectAccount(lua_State* L);
int LuaGetObjectPassword(lua_State* L);
int LuaGetObjectName(lua_State* L);
int LuaGetObjectPersonalCode(lua_State* L);
int LuaGetObjectClass(lua_State* L);
int LuaGetObjectChangeUp(lua_State* L);
int LuaGetObjectLevel(lua_State* L);
int LuaGetObjectLevelUpPoint(lua_State* L);
int LuaGetObjectMoney(lua_State* L);
int LuaGetObjectStrength(lua_State* L);
int LuaGetObjectDexterity(lua_State* L);
int LuaGetObjectVitality(lua_State* L);
int LuaGetObjectEnergy(lua_State* L);
int LuaGetObjectLeadership(lua_State* L);
int LuaGetObjectExtraStrength(lua_State* L);
int LuaGetObjectExtraDexterity(lua_State* L);
int LuaGetObjectExtraVitality(lua_State* L);
int LuaGetObjectExtraEnergy(lua_State* L);
int LuaGetObjectExtraLeadership(lua_State* L);
int LuaGetObjectDefaultStrength(lua_State* L);
int LuaGetObjectDefaultDexterity(lua_State* L);
int LuaGetObjectDefaultVitality(lua_State* L);
int LuaGetObjectDefaultEnergy(lua_State* L);
int LuaGetObjectDefaultLeadership(lua_State* L);
int LuaGetObjectLive(lua_State* L);
int LuaGetObjectLife(lua_State* L);
int LuaGetObjectMaxLife(lua_State* L);
int LuaGetObjectMana(lua_State* L);
int LuaGetObjectMaxMana(lua_State* L);
int LuaGetObjectBP(lua_State* L);
int LuaGetObjectMaxBP(lua_State* L);
int LuaGetObjectShield(lua_State* L);
int LuaGetObjectMaxShield(lua_State* L);
int LuaGetObjectPKCount(lua_State* L);
int LuaGetObjectPKLevel(lua_State* L);
int LuaGetObjectPKTimer(lua_State* L);
int LuaGetObjectMap(lua_State* L);
int LuaGetObjectMapX(lua_State* L);
int LuaGetObjectMapY(lua_State* L);
int LuaGetObjectDeathMap(lua_State* L);
int LuaGetObjectDeathMapX(lua_State* L);
int LuaGetObjectDeathMapY(lua_State* L);
int LuaGetObjectAuthority(lua_State* L);
int LuaGetObjectPartyNumber(lua_State* L);
int LuaGetObjectGuildNumber(lua_State* L);
int LuaGetObjectGuildStatus(lua_State* L);
int LuaGetObjectGuildName(lua_State* L);
int LuaGetObjectGuildRelationship(lua_State* L);
int LuaGetObjectGuildUnionNumber(lua_State* L);
int LuaGetObjectGuildUnionName(lua_State* L);
int LuaGetObjectChange(lua_State* L);
int LuaGetObjectInterface(lua_State* L);
int LuaGetObjectMasterLevel(lua_State* L);
int LuaGetObjectMasterPoint(lua_State* L);
int LuaGetObjectAccountLevel(lua_State* L);
int LuaGetObjectAccountExpireDate(lua_State* L);
int LuaGetObjectReset(lua_State* L);
int LuaGetObjectMasterReset(lua_State* L);
int LuaGetObjectGensRank(lua_State* L);
int LuaGetObjectGensSymbol(lua_State* L);
int LuaGetObjectGensFamily(lua_State* L);
int LuaGetObjectGensContribution(lua_State* L);
int LuaGetObjectCSGuildSide(lua_State* L);
int LuaGetObjectOfflineFlag(lua_State* L);
int LuaGetObjectIndexByName(lua_State* L);
// Getters/setters below expose OBJECTSTRUCT (User.h) fields that already
// existed natively but had no Lua binding yet - ported from a survey of the
// "RoxGaming Main 5.2 - 60 FPS UPDATE" source pack's LuaUser.cpp against
// this project's real struct (see the README changelog entry for the full
// field-by-field comparison). No gameplay call sites touched - these are
// pure data accessors, same shape as every LuaGetObjectX/LuaSetObjectX
// above.
int LuaGetObjectDefense(lua_State* L);
int LuaSetObjectDefense(lua_State* L);
int LuaGetObjectDefensePvP(lua_State* L);
int LuaSetObjectDefensePvP(lua_State* L);
int LuaGetObjectMagicDamageMax(lua_State* L);
int LuaSetObjectMagicDamageMax(lua_State* L);
int LuaGetObjectMagicDamageMin(lua_State* L);
int LuaSetObjectMagicDamageMin(lua_State* L);
int LuaGetObjectPhysiDamageMaxLeft(lua_State* L);
int LuaSetObjectPhysiDamageMaxLeft(lua_State* L);
int LuaGetObjectPhysiDamageMinLeft(lua_State* L);
int LuaSetObjectPhysiDamageMinLeft(lua_State* L);
int LuaGetObjectPhysiDamageMaxRight(lua_State* L);
int LuaSetObjectPhysiDamageMaxRight(lua_State* L);
int LuaGetObjectPhysiDamageMinRight(lua_State* L);
int LuaSetObjectPhysiDamageMinRight(lua_State* L);
int LuaGetObjectPhysiSpeed(lua_State* L);
int LuaSetObjectPhysiSpeed(lua_State* L);
int LuaGetObjectMagicSpeed(lua_State* L);
int LuaSetObjectMagicSpeed(lua_State* L);
int LuaGetObjectWarehouseCount(lua_State* L);
int LuaSetObjectWarehouseCount(lua_State* L);
int LuaGetObjectReqWarehouseOpen(lua_State* L);
int LuaSetObjectReqWarehouseOpen(lua_State* L);
int LuaGetObjectExtInventory(lua_State* L);
int LuaSetObjectExtInventory(lua_State* L);
int LuaGetObjectExtWarehouse(lua_State* L);
int LuaSetObjectExtWarehouse(lua_State* L);
int LuaGetObjectPShopOpen(lua_State* L);
int LuaSetObjectPShopOpen(lua_State* L);
int LuaGetObjectPShopText(lua_State* L);
int LuaSetObjectPShopText(lua_State* L);
int LuaGetObjectSummonIndex(lua_State* L);
int LuaSetObjectSummonIndex(lua_State* L);
int LuaGetObjectDieRegen(lua_State* L);
int LuaSetObjectDieRegen(lua_State* L);
int LuaGetObjectRegenMapNumber(lua_State* L);
int LuaSetObjectRegenMapNumber(lua_State* L);
int LuaGetObjectRegenMapX(lua_State* L);
int LuaSetObjectRegenMapX(lua_State* L);
int LuaGetObjectRegenMapY(lua_State* L);
int LuaSetObjectRegenMapY(lua_State* L);
int LuaGetObjectRegenTime(lua_State* L);
int LuaSetObjectRegenTime(lua_State* L);
int LuaSetObjectBP(lua_State* L);
int LuaSetObjectMaxBP(lua_State* L);
int LuaSetObjectLife(lua_State* L);
int LuaSetObjectMaxLife(lua_State* L);
int LuaSetObjectMana(lua_State* L);
int LuaSetObjectMaxMana(lua_State* L);
int LuaSetObjectShield(lua_State* L);
int LuaSetObjectMaxShield(lua_State* L);
int LuaSetObjectAuthority(lua_State* L);
int LuaSetObjectLevel(lua_State* L);
int LuaSetObjectLevelUpPoint(lua_State* L);
int LuaSetObjectMoney(lua_State* L);
int LuaSetObjectStrength(lua_State* L);
int LuaSetObjectDexterity(lua_State* L);
int LuaSetObjectVitality(lua_State* L);
int LuaSetObjectEnergy(lua_State* L);
int LuaSetObjectLeadership(lua_State* L);
int LuaSetObjectChatLimitTime(lua_State* L);
int LuaSetObjectPKCount(lua_State* L);
int LuaSetObjectPKLevel(lua_State* L);
int LuaSetObjectPKTimer(lua_State* L);
int LuaSetObjectMap(lua_State* L);
int LuaSetObjectMapX(lua_State* L);
int LuaSetObjectMapY(lua_State* L);
int LuaSetObjectMasterLevel(lua_State* L);
int LuaSetObjectMasterPoint(lua_State* L);
int LuaSetObjectWindowTitle(lua_State* L);
int LuaChatTargetSend(lua_State* L);
// Found while finishing the survey of the RoxGaming pack's LuaFunctions.cpp
// (the rest of it either has no equivalent function in this project under
// any name we could find - e.g. gObjFindIndex/MoveObject/gObjDel/
// SetPositionMonsterRand/GCFireworksSendCustom, a whole DeathSystem this
// project doesn't have - or turned out to already be covered by this
// project's own existing bindings above, just under different names:
// AddMonster/SetMonster~MonsterCreate/MonsterSummonCreate,
// AddEffect/DelEffect~EffectAdd/EffectDel, Teleport~MoveUserEx,
// MoveGate~MoveUser, Fireworks~FireworksSend, PkLevelSend~PKLevelSend,
// MoneySend~MoneySend, GetMapAttr~MapCheckAttr, ChatTargetSend~itself).
// See the README changelog entry for the full breakdown.
int LuaDamageSend(lua_State* L);
int LuaRefreshCharacter(lua_State* L);
int LuaLevelExperience(lua_State* L);
int LuaTeleportMapRange(lua_State* L);
int LuaCommandCheckGameMasterLevel(lua_State* L);
int LuaCommandGetArgNumber(lua_State* L);
int LuaCommandGetArgString(lua_State* L);
int LuaCommandSend(lua_State* L);
int LuaConfigReadNumber(lua_State* L);
int LuaConfigReadString(lua_State* L);
int LuaConfigSaveString(lua_State* L);
int LuaEffectAdd(lua_State* L);
int LuaEffectDel(lua_State* L);
int LuaEffectCheck(lua_State* L);
int LuaEffectClear(lua_State* L);
int LuaFireworksSend(lua_State* L);
int LuaInventoryGetWearSize(lua_State* L);
int LuaInventoryGetMainSize(lua_State* L);
int LuaInventoryGetFullSize(lua_State* L);
int LuaInventoryGetItemTable(lua_State* L);
// The six below round out LuaInventoryGetItemTable (which already reads
// every CItem option field into one table) with the write side and a
// handful of item-state checks it doesn't cover - found by comparing this
// project's item API against the "RoxGaming Main 5.2 - 60 FPS UPDATE"
// source pack's LuaInventory.cpp (an OOP getX()/setX() wrapper over the
// same CItem fields). See the README changelog entry for details.
int LuaInventorySetItemOption(lua_State* L);
int LuaInventorySetItemDurability(lua_State* L);
int LuaInventoryConvertItem(lua_State* L);
int LuaInventoryIsPeriodicItem(lua_State* L);
int LuaInventoryGetPeriodicItemTime(lua_State* L);
int LuaInventoryIsExcItem(lua_State* L);
int LuaInventoryGetItemIndex(lua_State* L);
int LuaInventoryGetItemCount(lua_State* L);
int LuaInventoryDelItemIndex(lua_State* L);
int LuaInventoryDelItemCount(lua_State* L);
int LuaInventoryGetFreeSlotCount(lua_State* L);
int LuaInventoryCheckSpaceByItem(lua_State* L);
int LuaInventoryCheckSpaceBySize(lua_State* L);
int LuaItemDrop(lua_State* L);
int LuaItemDropEx(lua_State* L);
int LuaItemGive(lua_State* L);
int LuaItemGiveEx(lua_State* L);
int LuaLevelUpSend(lua_State* L);
int LuaLogPrint(lua_State* L);
int LuaLogColor(lua_State* L);
int LuaMapCheckAttr(lua_State* L);
int LuaMapGetItemTable(lua_State* L);
int LuaMasterLevelUpSend(lua_State* L);
int LuaMasterSkillTreeRebuild(lua_State* L);
int LuaMessageGet(lua_State* L);
int LuaMoneySend(lua_State* L);
int LuaMonsterCreate(lua_State* L);
int LuaMonsterDelete(lua_State* L);
int LuaMonsterSummonCreate(lua_State* L);
int LuaMonsterSummonDelete(lua_State* L);
int LuaMoveUser(lua_State* L);
int LuaMoveUserEx(lua_State* L);
int LuaMessageSend(lua_State* L);
int LuaMessageSendToAll(lua_State* L);
int LuaNoticeSend(lua_State* L);
int LuaNoticeSendToAll(lua_State* L);
int LuaNoticeGlobalSend(lua_State* L);
int LuaPartyGetMemberCount(lua_State* L);
int LuaPartyGetMemberIndex(lua_State* L);
// CParty (Party.h) already had Create/Destroy/AddMember natively, just no
// Lua binding - found by comparing against the "RoxGaming Main 5.2 - 60
// FPS UPDATE" source pack's LuaFunctions.cpp (see README changelog).
int LuaPartyCreate(lua_State* L);
int LuaPartyDestroy(lua_State* L);
int LuaPartyAddMember(lua_State* L);
int LuaPartyIsLeader(lua_State* L);
int LuaPartyIsMember(lua_State* L);
int LuaPartyIsParty(lua_State* L);
int LuaObjectGetCoin(lua_State* L);
int LuaObjectAddCoin(lua_State* L);
int LuaObjectSubCoin(lua_State* L);
int LuaPermissionCheck(lua_State* L);
int LuaPermissionInsert(lua_State* L);
int LuaPermissionRemove(lua_State* L);
int LuaPKLevelSend(lua_State* L);
int LuaPostSend(lua_State* L);
int LuaQuestStateCheck(lua_State* L);
// CQuest/CQuestReward (Quest.h/QuestReward.h) already had these natively,
// just no Lua binding - same survey as the Party functions above.
int LuaQuestAddList(lua_State* L);
int LuaQuestInsertReward(lua_State* L);
int LuaQuestInfoSend(lua_State* L);
int LuaRandomGetNumber(lua_State* L);
int LuaSkinChangeSend(lua_State* L);
int LuaUserDisconnect(lua_State* L);
int LuaUserGameLogout(lua_State* L);
int LuaUserCalcAttribute(lua_State* L);
int LuaUserInfoSend(lua_State* L);
int LuaUserActionSend(lua_State* L);
int LuaUserSetAccountLevel(lua_State* L);
int LuaSQLConnect(lua_State* L);
int LuaSQLDisconnect(lua_State* L);
int LuaSQLCheck(lua_State* L);
int LuaSQLQuery(lua_State* L);
int LuaSQLClose(lua_State* L);
int LuaSQLFetch(lua_State* L);
int LuaSQLGetResult(lua_State* L);
int LuaSQLGetNumber(lua_State* L);
int LuaSQLGetSingle(lua_State* L);
int LuaSQLGetString(lua_State* L);