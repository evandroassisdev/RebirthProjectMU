--############################################################################
-- Warehouse - vault picker for the /bau <num> and /abrirbau commands (both
-- already native GameServer features - see CommandManager.cpp - just never
-- had a client-side UI). No new protocol needed: this just sends the same
-- command text the chat input box would.
--
-- IMPORTANT: CommandWare (/bau) refuses to run while any interface
-- (including the warehouse itself) is already open - see the
-- lpObj->Interface.use != 0 guard in CommandManager.cpp. So the picker has
-- to run BEFORE the warehouse opens, not after: a small always-visible
-- "Trocar Bau" trigger button shows the 0-10 grid (only when the warehouse
-- is currently closed); picking a number sends /bau N, then /abrirbau to
-- open it fresh - works from anywhere, no NPC needed.
--
-- /bau also closes whatever interface is open as a side effect, and
-- SendChat() throttles repeat sends for ~1-2s (ChatTime cooldown) - so
-- "/bau N" then "/abrirbau" can't be sent back-to-back in the same frame,
-- the second one would just get silently dropped. Warehouse_PendingStep
-- spaces them out over real frames instead.
--############################################################################

BridgeFunctionAttach('OnMainProc', 'Warehouse_OnMainProc')
BridgeFunctionAttach('OnClickEvent', 'Warehouse_OnClickEvent')

Warehouse_TriggerX = 10
Warehouse_TriggerY = 130
Warehouse_TriggerW = 150
Warehouse_TriggerH = 14

Warehouse_GridOpen = false
Warehouse_GridX = 10
Warehouse_GridY = 150
Warehouse_CellW = 45
Warehouse_CellH = 28
Warehouse_Cols = 4

Warehouse_PendingStep = 0
Warehouse_PendingVault = 0

function Warehouse_CellRect(n)

	local col = n % Warehouse_Cols
	local row = math.floor(n / Warehouse_Cols)

	local x = Warehouse_GridX + col * Warehouse_CellW
	local y = Warehouse_GridY + row * Warehouse_CellH

	return x, y
end

function Warehouse_OnMainProc()

	-- Mid-switch (waiting to send /abrirbau) - nothing to draw.
	if Warehouse_PendingStep == 1 then

		SendCommand("/bau " .. Warehouse_PendingVault)
		Warehouse_PendingStep = 2

		return
	end

	if Warehouse_PendingStep == 2 then

		-- SendChat() drops anything sent while ChatTime is above 50 (see
		-- wsclientinline.h) - poll the real value instead of guessing a
		-- fixed wait, fires the instant it's actually allowed again.
		if GetChatTime() <= 50 then
			SendCommand("/abrirbau")
			Warehouse_PendingStep = 0
		end

		return
	end

	-- The warehouse opened some other way (NPC, /abrirbau typed by hand) -
	-- close our picker, /bau would be rejected right now anyway.
	if IsWarehouseOpen() then
		Warehouse_GridOpen = false
		return
	end

	if Warehouse_GridOpen then
		DrawPanel(Warehouse_TriggerX - 6, Warehouse_TriggerY - 4, 190, 140, 0, 0, 0, 160)
	else
		DrawPanel(Warehouse_TriggerX - 6, Warehouse_TriggerY - 4, 165, 22, 0, 0, 0, 160)
	end

	DrawText(Warehouse_TriggerX, Warehouse_TriggerY, "[Trocar Bau]", 0, 200, 255, 255)

	if Warehouse_GridOpen then

		DrawText(Warehouse_GridX, Warehouse_GridY - 16, "Escolher bau:", 255, 255, 0, 255)

		for n = 0, 10 do
			local x, y = Warehouse_CellRect(n)
			DrawText(x, y, "[" .. n .. "]", 255, 255, 255, 255)
		end

	end

end

function Warehouse_PanelRect()

	if Warehouse_GridOpen then
		return Warehouse_TriggerX - 6, Warehouse_TriggerY - 4, 190, 140
	end

	return Warehouse_TriggerX - 6, Warehouse_TriggerY - 4, 165, 22
end

function Warehouse_OnClickEvent()

	if Warehouse_PendingStep ~= 0 or IsWarehouseOpen() then
		return
	end

	local x = MousePosX()
	local y = MousePosY()

	local px, py, pw, ph = Warehouse_PanelRect()

	if x < px or x > px + pw or y < py or y > py + ph then
		return
	end

	-- Click landed on our panel - don't let it fall through to the world
	-- (moving the character, attacking, etc.) underneath it.
	ConsumeClick()

	if x >= Warehouse_TriggerX and x <= Warehouse_TriggerX + Warehouse_TriggerW and y >= Warehouse_TriggerY and y <= Warehouse_TriggerY + Warehouse_TriggerH then
		Warehouse_GridOpen = not Warehouse_GridOpen
		return
	end

	if not Warehouse_GridOpen then
		return
	end

	for n = 0, 10 do
		local cx, cy = Warehouse_CellRect(n)

		if x >= cx and x <= cx + Warehouse_CellW and y >= cy and y <= cy + Warehouse_CellH then
			Warehouse_GridOpen = false
			Warehouse_PendingVault = n
			Warehouse_PendingStep = 1
			return
		end
	end

end
