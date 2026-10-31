//==============================================================================
// Die Pfanne am Dreibein (31.10.2026, Spieltest).
//
// Vanilla laesst die Pfanne nur im Ofen zu (Fireplace.CanReceiveAttachment), der
// Slot CookingEquipment steht aber schon in ihrer Config. Gekocht wird danach wie
// mit dem Topf: IsCookware() setzt sie als Kochgeschirr (Fireplace.EEItemAttached).
//
// Layer: 4_World.
//==============================================================================
modded class Fireplace
{
	override bool CanReceiveAttachment(EntityAI attachment, int slotId)
	{
		if (attachment.Type() == ATTACHMENT_FRYING_PAN && !IsOven() && FindAttachmentBySlotName("CookingTripod"))
			return slotId == InventorySlots.GetSlotIdFromString("CookingEquipment");

		return super.CanReceiveAttachment(attachment, slotId);
	}
}
