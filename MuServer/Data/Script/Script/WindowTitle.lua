--############################################################################
-- WindowTitle - test script for the Lua scripting engine
-- Sets the client window title to show the character's nick, level and
-- reset count. Refreshed on login and again immediately on every level up
-- (there's no separate "reset" hook - resets also go through a level
-- change, so this covers it too).
--############################################################################

BridgeFunctionAttach('OnCharacterEntry','WindowTitle_Update')
BridgeFunctionAttach('OnUserLevelUp','WindowTitle_Update')

function WindowTitle_Update(aIndex)

	local Name = GetObjectName(aIndex)

	local Level = GetObjectLevel(aIndex)

	local Reset = GetObjectReset(aIndex)

	SetObjectWindowTitle(aIndex,string.format("RebirthProjectMU || Player: %s || Level: %d || Reset: %d",Name,Level,Reset))

end
