--############################################################################
-- TestButton - Phase 1 proof-of-concept for the client Lua engine.
-- Draws a text "button" in the top-left corner; clicking it posts a system
-- chat message. Nothing fancy - just proves render + click both work.
--############################################################################

BridgeFunctionAttach('OnMainProc', 'TestButton_OnMainProc')
BridgeFunctionAttach('OnClickEvent', 'TestButton_OnClickEvent')

TestButton_X = 10
TestButton_Y = 100
TestButton_W = 160
TestButton_H = 14

function TestButton_OnMainProc()

	DrawText(TestButton_X, TestButton_Y, "[Lua Test Button - Click Me]", 255, 255, 0, 255)

end

function TestButton_OnClickEvent()

	local x = MousePosX()
	local y = MousePosY()

	if x >= TestButton_X and x <= TestButton_X + TestButton_W and y >= TestButton_Y and y <= TestButton_Y + TestButton_H then

		ChatMessage("Lua client engine is working!")

	end

end
