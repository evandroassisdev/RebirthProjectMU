--############################################################################
-- Panel9 - reusable 9-slice panel renderer for custom Lua interfaces.
--
-- Why this exists: the old MU sprite-based windows (SysMenuWin/WinEx.cpp)
-- compose their background from 5 separately-tiled pieces with a specific,
-- undocumented draw order (center painted first, ornate border pieces on
-- top) and rely on GL_REPEAT hardware tiling this Lua engine's LoadImage()
-- doesn't expose - decomposing/reusing that turned into real bugs and a lot
-- of reverse-engineering. A *new*, purpose-made panel doesn't need any of
-- that: 4 fixed-size corners + 4 stretched edges + 1 stretched center is 9
-- plain RenderImage() calls, no tiling math, no draw-order landmines, and
-- works with any art you drop in (or no art at all - flat colors as a
-- placeholder, see below).
--
-- USAGE
--   require('System\\Panel9')
--
--   local skin = {
--       -- A rounded/asymmetric corner can't just be reused at all 4
--       -- corners unrotated - RenderImage() (ClientLuaFunction.cpp) has no
--       -- rotate/flip param - so each corner is its own texture. cornerTL
--       -- is required if any corner art is used at all; the other 3 fall
--       -- back to cornerTL when omitted (fine for a symmetric/simple
--       -- corner design, e.g. a plain right-angle notch with no curve).
--       cornerTL = someImageId,
--       cornerTR = someImageId,  -- falls back to cornerTL if nil
--       cornerBL = someImageId,  -- falls back to cornerTL if nil
--       cornerBR = someImageId,  -- falls back to cornerTL if nil
--       edgeH    = someImageId,  -- horizontal strip, used top+bottom (stretched) - only reusable unflipped like this if the art is symmetric top-to-bottom
--       edgeV    = someImageId,  -- vertical strip, used left+right (stretched) - same caveat, symmetric left-to-right
--       center   = someImageId,  -- optional, stretched to fill
--   }
--   Panel9.Draw(x, y, w, h, cornerSize, skin)
--
-- Any texture field left nil is simply skipped (transparent) - so a window
-- can start as just a flat-colored center (pass `colors` instead/as well)
-- and grow real corner/edge art later without changing the call site.
--
--   local colors = {
--       corner = {60, 60, 60, 220},  -- {r, g, b, a}, only used when the
--       edge   = {40, 40, 40, 200},  -- matching texture field above is nil
--       center = {20, 20, 20, 180},
--   }
--   Panel9.Draw(x, y, w, h, cornerSize, skin, colors)
--############################################################################

Panel9 = {}

local function DrawPiece(x, y, w, h, textureId, color)
	if w <= 0 or h <= 0 then
		return
	end

	if textureId ~= nil then
		RenderImage(textureId, x, y, w, h)
	elseif color ~= nil then
		DrawPanel(x, y, w, h, color[1], color[2], color[3], color[4])
	end
	-- neither given -> intentionally transparent, see file header
end

function Panel9.Draw(x, y, w, h, cornerSize, skin, colors)
	skin = skin or {}
	colors = colors or {}

	local cs = cornerSize
	local innerW = w - cs * 2
	local innerH = h - cs * 2

	-- Background drawn FIRST, corners LAST on top - matches the real
	-- native MU window convention confirmed earlier this project
	-- (CWinEx::Render(), WinEx.cpp: center stone painted first, ornate
	-- border pieces painted over it). An earlier version of this function
	-- drew corners first - harmless for the dedicated-edge-art branch
	-- below (it never spatially overlaps the corners) but actively wrong
	-- for the single-fill branch, which needs to cover the FULL w x h
	-- rect including the corner squares, then have the corners painted
	-- back on top.
	if skin.center ~= nil and skin.edgeH == skin.center and skin.edgeV == skin.center then
		-- Single-photo fill (e.g. a stone/metal texture, not dedicated
		-- edge-strip art): draw it ONCE across the whole panel instead of
		-- as 5 separately-stretched pieces. 5 pieces each stretch a
		-- *different* crop of the same source photo to a *different*
		-- aspect ratio, so they visibly don't line up at their shared
		-- edges (confirmed - this is what Evan's first real-art screenshot
		-- showed: a patchwork of mismatched rectangles, not a seamless
		-- panel). One stretch has nothing to misalign against.
		DrawPiece(x, y, w, h, skin.center, colors.center)
	else
		-- Edges - one piece each side, stretched to fill the gap between
		-- corners (ordinary texture-mapped stretch, NOT the GL_REPEAT
		-- tiling the old windows needed - a single RenderImage() call per
		-- side). Only seam-free if edgeH/edgeV/center are dedicated,
		-- purpose-made strip art (tileable/matching at the seams) rather
		-- than crops of one bigger photo - see the branch above otherwise.
		DrawPiece(x + cs, y, innerW, cs, skin.edgeH, colors.edge)
		DrawPiece(x + cs, y + h - cs, innerW, cs, skin.edgeH, colors.edge)
		DrawPiece(x, y + cs, cs, innerH, skin.edgeV, colors.edge)
		DrawPiece(x + w - cs, y + cs, cs, innerH, skin.edgeV, colors.edge)

		-- Center - stretched to fill.
		DrawPiece(x + cs, y + cs, innerW, innerH, skin.center, colors.center)
	end

	-- Corners - fixed size, never stretched. 4 separate textures (see
	-- USAGE above for why) - colors.corner has no such problem, a flat
	-- color looks the same from every angle.
	DrawPiece(x, y, cs, cs, skin.cornerTL, colors.corner)
	DrawPiece(x + w - cs, y, cs, cs, skin.cornerTR or skin.cornerTL, colors.corner)
	DrawPiece(x, y + h - cs, cs, cs, skin.cornerBL or skin.cornerTL, colors.corner)
	DrawPiece(x + w - cs, y + h - cs, cs, cs, skin.cornerBR or skin.cornerTL, colors.corner)
end

-- Convenience: same as Draw(), but centered on screen (ScreenWidth()/
-- Height() - ClientLuaFunction.cpp - return the same 640x480 canvas
-- RenderImage()/DrawPanel() draw in, see their own comment for why).
-- Returns the top-left (x, y) actually used, in case the caller also needs
-- to position buttons/text relative to the panel.
function Panel9.DrawCentered(w, h, cornerSize, skin, colors)
	local x = (ScreenWidth() - w) / 2
	local y = (ScreenHeight() - h) / 2

	Panel9.Draw(x, y, w, h, cornerSize, skin, colors)

	return x, y
end

return Panel9
