#pragma once

// Bone-relative sprite/particle creation for client Lua (e.g. "spawn this
// effect at bone N of this model"). Ported from the "RoxGaming Main 5.2 -
// 60 FPS UPDATE" source pack (source/LuaEffectsNormal.cpp/.h there) - see
// LuaBMD.h for the general porting notes.
//
// Deliberately NOT porting that pack's other effect-creation module,
// LuaEffects.cpp/.h: its item-model-range special-casing (choosing between
// BoneTransform sources depending on whether Object->Type falls in a wing
// vs. weapon model range) calls gItemManager.GET_ITEM_MODEL(x, y) - a
// method this project's CItemManager (ItemManager.h) doesn't have (it's an
// empty stub here). Porting that logic would mean guessing at exact item
// model ID ranges instead of verifying them, so it's skipped. This simpler,
// always-bone-relative version covers the common case and has no such
// dependency.
//
// Both would also register the same global Lua function names
// ("CreateSprite"/"CreateParticle") if enabled together - upstream only
// ever registers one or the other per Lua state, never both.
//
// Not yet build-verified.

void InitLuaEffectsNormal(lua_State* L);
