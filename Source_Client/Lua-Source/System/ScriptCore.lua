--############################################################################
-- RebirthProjectMU - client-side Lua dispatcher
-- ---------------------------------------------------------------------------
-- WARNING: Modifying this file may affect every other script. Add new
-- scripts under Script\ instead - use BridgeFunctionAttach to hook in.
--############################################################################

BridgeFunctionTable = {}

function BridgeFunctionAttach(BridgeName, FunctionName)

	if BridgeFunctionTable[BridgeName] == nil then

		BridgeFunctionTable[BridgeName] = {}

	end

	for _, func in ipairs(BridgeFunctionTable[BridgeName]) do

		if func.Function == FunctionName then

			return

		end
	end

	table.insert(BridgeFunctionTable[BridgeName], { Function = FunctionName })

end


function BridgeFunction_OnMainProc()

	if BridgeFunctionTable["OnMainProc"] ~= nil then

		for _, func in ipairs(BridgeFunctionTable["OnMainProc"]) do

			_G[func.Function]()

		end

	end

end


function BridgeFunction_OnClickEvent()

	if BridgeFunctionTable["OnClickEvent"] ~= nil then

		for _, func in ipairs(BridgeFunctionTable["OnClickEvent"]) do

			_G[func.Function]()

		end

	end

end
