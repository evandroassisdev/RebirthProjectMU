--############################################################################
-- WindowTitle - test script for the Lua scripting engine
-- Sets the client window title to show the character's nick, level and
-- reset count, refreshed whenever the character enters the game.
--############################################################################

BridgeFunctionAttach('OnCharacterEntry','WindowTitle_OnCharacterEntry')

function WindowTitle_OnCharacterEntry(aIndex)

	local Name = GetObjectName(aIndex)

	local Level = GetObjectLevel(aIndex)

	local Reset = GetObjectReset(aIndex)

	SetObjectWindowTitle(aIndex,string.format("RebirthProjectMU || Player: %s || Level: %d || Reset: %d",Name,Level,Reset))

end
