--############################################################################
-- Coords - one-shot chat print of the player's current tile X,Y, using
-- UserPositionX()/UserPositionY() (LuaUser.cpp) + ChatMessage()
-- (ClientLuaFunction.cpp). Throwaway debug helper, not a permanent menu -
-- remove the require('Coords') line in ScriptMain.lua once you've got the
-- coordinate you need for the /abrirbau safezone investigation.
--############################################################################

local printed = false

function Coords_OnMainProc()

	if printed == false then

		ChatMessage("[Coords] X:" .. UserPositionX() .. " Y:" .. UserPositionY())

		printed = true

	end

end

BridgeFunctionAttach("OnMainProc", "Coords_OnMainProc")
