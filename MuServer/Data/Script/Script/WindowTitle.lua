--############################################################################
-- WindowTitle - test script for the Lua scripting engine
-- Sets the client window title to show the character's nick, level and
-- reset count. Refreshed on login and again immediately on every level up.
-- Reset/master reset also fire OnUserLevelUp (server-side, in
-- DGCommandResetRecv / DGCommandMasterResetRecv) even though they don't
-- go through the normal level-up path, specifically so this stays in sync.
--############################################################################

BridgeFunctionAttach('OnCharacterEntry','WindowTitle_Update')
BridgeFunctionAttach('OnUserLevelUp','WindowTitle_Update')

function WindowTitle_Update(aIndex)

	local Name = GetObjectName(aIndex)

	local Level = GetObjectLevel(aIndex)

	local Reset = GetObjectReset(aIndex)

	SetObjectWindowTitle(aIndex,string.format("RebirthProjectMU || Player: %s || Level: %d || Reset: %d",Name,Level,Reset))

end
