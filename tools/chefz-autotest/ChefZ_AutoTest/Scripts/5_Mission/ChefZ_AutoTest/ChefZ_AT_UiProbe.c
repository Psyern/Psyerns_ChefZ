//==============================================================================
// ChefZ_AutoTest - Clientseite: was die Inventar-Umgebung fuer einen Raeucherschrank
// und zum Vergleich fuer Vanillas Feuerstelle tatsaechlich aufbaut. Beide laufen
// ueber AttachmentCategoriesContainer (VicinityContainer.c:447-455); geloggt werden
// Kopfzeile, Kategorien mit Icon und Plaetzen sowie der Cargo.
//
// Layer: 5_Mission (Inventar-GUI).
//==============================================================================

modded class AttachmentCategoriesContainer
{
    override void SetEntity(EntityAI entity)
    {
        super.SetEntity(entity);

        if (!entity || (!entity.IsKindOf("ChefZ_Smoker") && !entity.IsKindOf("FireplaceBase")))
            return;
        TypeAttachmentGroupsDataHolder holder = entity.GetTypeAttachmentGroupInfoHolder();
        if (!holder)
            return;
        TAttachmentGroupMap groupMap = holder.GetAttachmentGroupMap();
        int count = holder.GetValidAttachmnetGroupCount();
        string line = "[CHEFZTEST] client ui " + entity.GetType() + " kopf=" + Widget.TranslateString(entity.GetDisplayName()) + " kategorien=" + count.ToString();
        for (int i = 0; i < count; i++)
        {
            TypeAttachmentGroupData data = groupMap.Get(groupMap.GetKey(i));
            string slots = "";
            foreach (string s : data.m_AttSlots)
            {
                slots = slots + s + "/";
            }
            line = line + " | " + groupMap.GetKey(i) + " (" + Widget.TranslateString(data.m_Name) + ") icon=" + data.m_IconPath;
            line = line + " plaetze=" + slots;
        }
        bool cargo = entity.GetInventory().GetCargo() != null;
        line = line + " | cargo=" + cargo.ToString() + " sichtbar=" + IsDisplayable().ToString();
        Print(line);
    }
}
