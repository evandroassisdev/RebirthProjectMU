--############################################################################
-- Panel9Demo - proves Panel9.lua's 9-slice math is correct, now with REAL
-- Midjourney-generated art (Evan's prompts, see Imagens\1../3../4../5..) -
-- background removed via Tools\ImageConverter\remove_bg.py (flood-fill
-- keying, preserves enclosed dark design areas like the button plate's
-- black interior), converted via png_to_mu.py, deployed to
-- Data\Custom\Panel9\*.OZT/OZJ and Data\Custom\Common\circle_frame.OZT.
-- Panel9.Draw()'s call site never changed across colors -> procedural skin
-- -> real skin - that was the whole point of the `skin`/`colors` split.
--
-- No separate straight-edge art was generated (only a corner ornament) -
-- edgeH/edgeV/center all reuse the same stone texture (`stone.OZJ`), so the
-- frame reads as "ornate corners set into a plain stone panel" rather than
-- a continuous decorative border bar running the whole edge.
--
-- ⚠️ Corner ornament (Custom\Panel9\corner_tl/tr/bl/br) removed from the
-- skin, per Evan's call after seeing it in-game: `corner_*` and `stone.OZJ`
-- are two INDEPENDENT Midjourney generations, each with its OWN idea of
-- where the frame border sits/how thick it is - there's no way to make an
-- externally-added corner ornament's band line up pixel-for-pixel with a
-- separately-generated texture's built-in frame line, so the two always
-- read as slightly misaligned/disconnected at the corners, no matter how
-- the corner piece is rotated. `stone.OZJ` already has its own complete,
-- self-consistent thin bronze frame + small corner accents baked in (one
-- single generation, nothing to misalign) - just stretching it alone
-- across the whole panel (Panel9.Draw()'s single-fill path, `skin.center`
-- with no `cornerTL` set) reads as a clean, complete framed panel on its
-- own. The corner_*.OZT files are still on disk if a *matching* corner
-- ornament ever gets generated (e.g. via img2img/inpainting FROM this
-- exact stone texture, so the border position/thickness/color really
-- match) - just re-add the 4 `corner*` fields to `Panel9Demo_Skin` below.
--############################################################################

require('System\\Panel9')

BridgeFunctionAttach('OnMainProc', 'Panel9Demo_OnMainProc')
BridgeFunctionAttach('OnClickEvent', 'Panel9Demo_OnClickEvent')

Panel9Demo_Open = false

Panel9Demo_ButtonX, Panel9Demo_ButtonY, Panel9Demo_ButtonW, Panel9Demo_ButtonH = 10, 10, 170, 20

Panel9Demo_PanelW, Panel9Demo_PanelH, Panel9Demo_CornerSize = 260, 320, 48

Panel9Demo_StoneTex = LoadImage("Custom\\Panel9\\stone")

Panel9Demo_Skin = {
	edgeH  = Panel9Demo_StoneTex,
	edgeV  = Panel9Demo_StoneTex,
	center = Panel9Demo_StoneTex,
}

-- Shared across every window that wants a close button.
-- "4 - Botao de fechar (X)", variant _2. Background OUTSIDE the ring
-- removed via Tools\ImageConverter\remove_bg.py's flood fill; the black
-- interior behind the X (enclosed by the gold ring, never touches the
-- image edge) is untouched, exactly as intended.
Panel9Demo_CloseTex = LoadImage("Custom\\Common\\close_btn_x")
Panel9Demo_CloseSize = 26

-- Extra buttons on top of the panel, placed via the Panel9 Layout
-- Designer's drag-and-drop (Tools\PanelDesigner). x/y/w/h are relative to
-- the panel's own top-left corner (the `x, y` Panel9.DrawCentered()
-- returns), same convention the close button already uses above. `action`
-- is optional - a plain RenderImage()'d decoration can omit it; a real
-- button gives one, called from OnClickEvent below when clicked.
Panel9Demo_Elements = {
	{ tex = LoadImage("Interface\\cancel"), x = 91, y = 266, w = 70, h = 21,
	  action = function() Panel9Demo_Open = false end },
}

function Panel9Demo_OnMainProc()

	DrawPanel(Panel9Demo_ButtonX - 4, Panel9Demo_ButtonY - 3, Panel9Demo_ButtonW + 8, Panel9Demo_ButtonH + 6, 0, 0, 0, 160)
	DrawText(Panel9Demo_ButtonX, Panel9Demo_ButtonY, "[Painel 9-slice - Lua]", 0, 255, 150, 255)

	if not Panel9Demo_Open then
		return
	end

	local x, y = Panel9.DrawCentered(Panel9Demo_PanelW, Panel9Demo_PanelH, Panel9Demo_CornerSize, Panel9Demo_Skin)

	-- Close button - overlaps the top-right panel corner slightly, same
	-- placement exemplo.jpg's reference used.
	local cx = x + Panel9Demo_PanelW - Panel9Demo_CloseSize * 0.65
	local cy = y - Panel9Demo_CloseSize * 0.27
	RenderImage(Panel9Demo_CloseTex, cx, cy, Panel9Demo_CloseSize, Panel9Demo_CloseSize)

	for _, el in ipairs(Panel9Demo_Elements) do
		RenderImage(el.tex, x + el.x, y + el.y, el.w, el.h)
	end

end

function Panel9Demo_OnClickEvent()

	local mx = MousePosX()
	local my = MousePosY()

	if mx >= Panel9Demo_ButtonX - 4 and mx <= Panel9Demo_ButtonX + Panel9Demo_ButtonW + 4
		and my >= Panel9Demo_ButtonY - 3 and my <= Panel9Demo_ButtonY + Panel9Demo_ButtonH + 3 then

		ConsumeClick()
		Panel9Demo_Open = not Panel9Demo_Open
		return
	end

	if not Panel9Demo_Open then
		return
	end

	local x = (ScreenWidth() - Panel9Demo_PanelW) / 2
	local y = (ScreenHeight() - Panel9Demo_PanelH) / 2
	local cx = x + Panel9Demo_PanelW - Panel9Demo_CloseSize * 0.65
	local cy = y - Panel9Demo_CloseSize * 0.27

	if mx >= cx and mx <= cx + Panel9Demo_CloseSize and my >= cy and my <= cy + Panel9Demo_CloseSize then
		ConsumeClick()
		Panel9Demo_Open = false
		return
	end

	for _, el in ipairs(Panel9Demo_Elements) do
		local ex, ey = x + el.x, y + el.y
		if mx >= ex and mx <= ex + el.w and my >= ey and my <= ey + el.h then
			ConsumeClick()
			if el.action then
				el.action()
			end
			return
		end
	end

	-- Block click-through to the world for the whole panel body, not just
	-- its buttons - without this, clicking the stone background (anywhere
	-- that isn't the close button) still reaches Attack()/movement in
	-- ZzzInterface.cpp underneath, same "MouseLButtonPush || MouseLButton"
	-- check every native window guards against (see ConsumeClick()'s own
	-- comment, ClientLuaFunction.cpp) - a real window blocks its entire
	-- area, not just its clickable widgets.
	if mx >= x and mx <= x + Panel9Demo_PanelW and my >= y and my <= y + Panel9Demo_PanelH then
		ConsumeClick()
	end

end
