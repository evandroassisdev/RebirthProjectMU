--############################################################################
-- WarehouseIconTest - proof of concept for rendering a native UI image
-- as-is, the same way the client renders its own buttons (see e.g.
-- ZzzOpenData.cpp's exit_01.jpg/accept_box01.jpg/mix_button1.jpg -
-- BITMAP_INVENTORY_BUTTON family): draw it as a plain rectangle, no colorkey
-- trick, nothing fancy.
--
-- No extension on the LoadImage() path - the real file on disk is always
-- .OZJ or .OZT, never literally ".jpg"/".tga", so there's nothing useful to
-- pass; LoadImage() tries both itself and uses whichever exists.
--
-- Non-power-of-2 source art gets padded up to the next power of 2 at load
-- time, and that padding is uninitialized memory, not transparent - same
-- reason ZzzOpenData.cpp's own button icons pass e.g. "24.f/32.f" to
-- RenderBitmap. LoadImage() measures the real size itself and
-- RenderImage() applies the crop automatically, so no crop math here
-- either - it's just LoadImage()+RenderImage() like any other image.
--
-- Positioned where the native m_BtnWarehouse used to sit inside the
-- inventory window (NewUIMyInventory.cpp: SetPos(m_Pos.x + 124,
-- m_Pos.y + 391), 36x29 - its own Render() call is commented out there).
-- The window can be dragged, so the position is read live every frame via
-- GetInventoryPos(), not cached; only shown while the inventory is open.
--############################################################################

BridgeFunctionAttach('OnMainProc', 'WarehouseIconTest_OnMainProc')

WarehouseIconTest_Image = LoadImage("Custom\\Buttons\\bless")

WarehouseIconTest_W = 36
WarehouseIconTest_H = 29
WarehouseIconTest_OffsetX = 124
WarehouseIconTest_OffsetY = 391

function WarehouseIconTest_OnMainProc()

	if not IsInventoryOpen() then
		return
	end

	local invX, invY = GetInventoryPos()

	RenderImage(WarehouseIconTest_Image, invX + WarehouseIconTest_OffsetX, invY + WarehouseIconTest_OffsetY, WarehouseIconTest_W, WarehouseIconTest_H)

end
