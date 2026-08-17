--############################################################################
-- BauSelector - "SELECIONAR BAU" panel, styled after C:\MuPrime\ScreenShots
-- (08-17.14-04)0000.jpg (Prime Play reference): a Panel9 stone/gold frame,
-- title + "Bau atual: N" subtitle, and an 11-button grid (0-10) with the
-- current vault highlighted in green.
--
-- Command sequencing (send /bau N then /abrirbau) is a straight reuse of
-- Warehouse.lua's already-verified logic, unchanged - see that file's own
-- header comment for the two constraints that logic exists to work around:
--   1. CommandWare (/bau) refuses to run while any interface (including the
--      warehouse itself) is already open - lpObj->Interface.use != 0 guard
--      in CommandManager.cpp. So the picker only shows while the warehouse
--      is closed, and picking a number closes the picker first.
--   2. SendChat() throttles repeat sends (~1-2s ChatTime cooldown) - "/bau N"
--      then "/abrirbau" can't be sent back-to-back in the same frame, so
--      BauSelector_PendingStep spaces them out over real frames, polling
--      GetChatTime() instead of guessing a fixed wait.
--
-- ⚠️ BauSelector_CurrentVault is OPTIMISTIC LOCAL STATE ONLY, not read back
-- from the server - there's no chat-read binding to parse the game's own
-- "Você mudou para o bau N com sucesso" confirmation line, so the highlight
-- just assumes the last click succeeded. Resets to 0 on relogin (matches the
-- server's own default), may drift if /bau is typed by hand instead of
-- picked here. Fine for this UI's purpose; flag if exact sync is ever
-- needed (would need a chat-line-read binding that doesn't exist yet).
--############################################################################

require('System\\Panel9')

BridgeFunctionAttach('OnMainProc', 'BauSelector_OnMainProc')
BridgeFunctionAttach('OnClickEvent', 'BauSelector_OnClickEvent')

BauSelector_TriggerX, BauSelector_TriggerY, BauSelector_TriggerW, BauSelector_TriggerH = 10, 40, 170, 20

BauSelector_Open = false
BauSelector_PanelW, BauSelector_PanelH, BauSelector_CornerSize = 300, 320, 48

BauSelector_StoneTex = LoadImage("Custom\\Panel9\\stone")
BauSelector_Skin = {
	edgeH  = BauSelector_StoneTex,
	edgeV  = BauSelector_StoneTex,
	center = BauSelector_StoneTex,
}

-- Shared close-button asset, same one Panel9Demo.lua uses.
BauSelector_CloseTex = LoadImage("Custom\\Common\\close_btn_x")
BauSelector_CloseSize = 26

BauSelector_CurrentVault = 0

BauSelector_PendingStep = 0
BauSelector_PendingVault = 0

-- Grid geometry, relative to the panel's own top-left corner (same
-- convention Panel9Demo_Elements uses).
BauSelector_Cols = 4
BauSelector_CellW, BauSelector_CellH = 56, 48
BauSelector_GapX, BauSelector_GapY = 8, 8
BauSelector_GridW = BauSelector_Cols * BauSelector_CellW + (BauSelector_Cols - 1) * BauSelector_GapX
BauSelector_GridX = (BauSelector_PanelW - BauSelector_GridW) / 2
BauSelector_GridY = 96

-- Returns the cell's rect relative to the panel's own top-left corner.
function BauSelector_CellRect(n)
	local col = n % BauSelector_Cols
	local row = math.floor(n / BauSelector_Cols)
	local x = BauSelector_GridX + col * (BauSelector_CellW + BauSelector_GapX)
	local y = BauSelector_GridY + row * (BauSelector_CellH + BauSelector_GapY)
	return x, y
end

function BauSelector_OnMainProc()

	-- Mid-switch (waiting to send /abrirbau) - nothing to draw.
	if BauSelector_PendingStep == 1 then
		SendCommand("/bau " .. BauSelector_PendingVault)
		BauSelector_PendingStep = 2
		return
	end

	if BauSelector_PendingStep == 2 then
		if GetChatTime() <= 50 then
			SendCommand("/abrirbau")
			BauSelector_CurrentVault = BauSelector_PendingVault
			BauSelector_PendingStep = 0
		end
		return
	end

	-- The warehouse opened some other way (NPC, /abrirbau typed by hand) -
	-- close our picker, /bau would be rejected right now anyway.
	if IsWarehouseOpen() then
		BauSelector_Open = false
	end

	DrawPanel(BauSelector_TriggerX - 4, BauSelector_TriggerY - 3, BauSelector_TriggerW + 8, BauSelector_TriggerH + 6, 0, 0, 0, 160)
	DrawText(BauSelector_TriggerX, BauSelector_TriggerY, "[Selecionar Bau - Lua]", 0, 255, 150, 255)

	if not BauSelector_Open then
		return
	end

	local x, y = Panel9.DrawCentered(BauSelector_PanelW, BauSelector_PanelH, BauSelector_CornerSize, BauSelector_Skin)

	-- DrawTextCentered() uses the engine's own RT3_SORT_CENTER text layout
	-- (RenderText()'s iBoxWidth/iSort params) - exact centering for any
	-- string/font, no guessed char width.
	DrawTextCentered(x, y + 18, BauSelector_PanelW, "SELECIONAR BAU", 255, 210, 90, 255)
	DrawTextCentered(x, y + 42, BauSelector_PanelW, "Bau atual: " .. BauSelector_CurrentVault, 255, 255, 0, 255)

	local mx, my = MousePosX(), MousePosY()

	for n = 0, 10 do
		local rx, ry = BauSelector_CellRect(n)
		local cx, cy = x + rx, y + ry
		local hover = mx >= cx and mx <= cx + BauSelector_CellW and my >= cy and my <= cy + BauSelector_CellH
		local selected = (n == BauSelector_CurrentVault)

		local br, bg, bb, bthick = 110, 110, 120, 2
		local fr, fg, fb = 25, 25, 32

		if selected then
			-- Thicker, brighter ring so the current vault reads clearly
			-- even at a glance (Evan's "borda verde" reference request).
			br, bg, bb, bthick = 80, 255, 120, 5
			fr, fg, fb = 15, 60, 30
		elseif hover then
			br, bg, bb = 210, 180, 90
		end

		DrawPanel(cx - bthick, cy - bthick, BauSelector_CellW + bthick * 2, BauSelector_CellH + bthick * 2, br, bg, bb, 255)
		DrawPanel(cx, cy, BauSelector_CellW, BauSelector_CellH, fr, fg, fb, 220)

		local numColor = selected and {150, 255, 180} or {255, 255, 255}
		DrawTextBigCentered(cx, cy + (BauSelector_CellH - 20) / 2, BauSelector_CellW, tostring(n), numColor[1], numColor[2], numColor[3], 255)
	end

	-- Close button - same placement pattern Panel9Demo.lua uses.
	local ccx = x + BauSelector_PanelW - BauSelector_CloseSize * 0.65
	local ccy = y - BauSelector_CloseSize * 0.27
	RenderImage(BauSelector_CloseTex, ccx, ccy, BauSelector_CloseSize, BauSelector_CloseSize)

end

function BauSelector_OnClickEvent()

	local mx, my = MousePosX(), MousePosY()

	if mx >= BauSelector_TriggerX - 4 and mx <= BauSelector_TriggerX + BauSelector_TriggerW + 4
		and my >= BauSelector_TriggerY - 3 and my <= BauSelector_TriggerY + BauSelector_TriggerH + 3 then
		ConsumeClick()
		if BauSelector_PendingStep == 0 and not IsWarehouseOpen() then
			BauSelector_Open = not BauSelector_Open
		end
		return
	end

	if not BauSelector_Open or BauSelector_PendingStep ~= 0 or IsWarehouseOpen() then
		return
	end

	local x = (ScreenWidth() - BauSelector_PanelW) / 2
	local y = (ScreenHeight() - BauSelector_PanelH) / 2

	local ccx = x + BauSelector_PanelW - BauSelector_CloseSize * 0.65
	local ccy = y - BauSelector_CloseSize * 0.27
	if mx >= ccx and mx <= ccx + BauSelector_CloseSize and my >= ccy and my <= ccy + BauSelector_CloseSize then
		ConsumeClick()
		BauSelector_Open = false
		return
	end

	for n = 0, 10 do
		local rx, ry = BauSelector_CellRect(n)
		local cx, cy = x + rx, y + ry
		if mx >= cx and mx <= cx + BauSelector_CellW and my >= cy and my <= cy + BauSelector_CellH then
			ConsumeClick()
			BauSelector_Open = false
			BauSelector_PendingVault = n
			BauSelector_PendingStep = 1
			return
		end
	end

	-- Block click-through to the world for the whole panel body, not just
	-- its buttons - same convention Panel9Demo.lua uses.
	if mx >= x and mx <= x + BauSelector_PanelW and my >= y and my <= y + BauSelector_PanelH then
		ConsumeClick()
	end

end
