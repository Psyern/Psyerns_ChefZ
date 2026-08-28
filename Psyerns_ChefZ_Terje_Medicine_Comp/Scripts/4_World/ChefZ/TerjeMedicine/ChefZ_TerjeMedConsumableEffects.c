// ---------------------------------------------------------------------------
// WEICHE ABHAENGIGKEIT: alles unterhalb existiert nur, wenn TerjeMedicine
// geladen ist. Fehlt der Mod, ist TERJE_MEDICINE_MOD nicht gesetzt, der
// Praeprozessor entfernt den gesamten Rumpf, und es bleibt eine leere Datei
// ohne unaufloesbare Bezeichner. Begruendung, Beleg und Vorbilder stehen im
// Kopf der config.cpp, Abschnitt "WEICHE ABHAENGIGKEIT".
// ---------------------------------------------------------------------------
#ifdef TERJE_MEDICINE_MOD
// ============================================================================
// modded class TerjeConsumableEffects  -  ChefZ-Kraeutertees
//
// Vorbild ist TerjeRadiation/Scripts/4_World/Classes/TerjeConsumableEffects.c:
// ein FREMDES Modul haengt sich in dieselbe Kette und liest seine eigenen
// Parameter, ohne eine Terje-Datei anzufassen. Genau dieses Muster wird hier
// wiederholt - nur liegen unsere Parameter unter CfgChefZTerjeMedicine statt
// unter CfgVehicles (Begruendung im Kopf der config.cpp).
//
// AUFRUFKETTE, damit nachvollziehbar bleibt, wann das hier laeuft:
//
//   Edible_Base.Consume(amount, consumer)
//     TerjeCore/Scripts/4_World/Entities/EdibleBase.c:18-27
//       -> ItemBase.ApplyTerjeConsumableEffects(player, amount)
//            TerjeCore/Scripts/4_World/Entities/ItemBase.c:294-316
//            (dort steht das GetGame().IsDedicatedServer()-Tor)
//              -> TerjeConsumableEffects.Apply(...)
//
// Der Effekt entsteht also GENAU EINMAL je Konsumvorgang und ausschliesslich
// nachdem Vanillas Consume() erfolgreich war. Es gibt in diesem Modul keinen
// zweiten Einstiegspunkt - kein Action-Hook, kein Modifier, kein Recipe-Hook.
// ============================================================================

modded class TerjeConsumableEffects
{
    override void Apply(EntityAI entity, string classname, PlayerBase player, float amount)
    {
        // super ZUERST. Terjes eigene Wirkungen und die von TerjeRadiation
        // laufen unveraendert durch; dieses Modul fuegt danach hinzu und nimmt
        // nichts weg.
        super.Apply(entity, classname, player, amount);

        if (!player || !player.GetTerjeStats())
        {
            return;
        }

        string effectPath;
        if (!GetChefZTerjeMedRegistry().Resolve(classname, effectPath))
        {
            // Kein ChefZ-Tee. Das ist der Normalfall fuer jedes andere Item im
            // Spiel und kostet einen Set-Lookup.
            return;
        }

        float servings = GetChefZTerjeMedRegistry().GetServings(effectPath, amount);
        if (servings <= 0)
        {
            return;
        }

        float timeModifier = GetChefZTerjeMedRegistry().GetPharmacologistTimeModifier(player);

        ApplyImmunityGain(player, effectPath, servings, timeModifier);
        ApplyHealthRegen(player, effectPath, servings, timeModifier);
    }

    // Immunity Gain. Rechnung und Reihenfolge sind absichtlich identisch zu
    // TerjeMedicine/Scripts/4_World/Classes/TerjeConsumableEffects.c:320-341.
    //
    // Der Vergleich "force >= aktive force" ist der Grund, warum ein Tee ein
    // laufendes Medikament NIE verdraengen kann: mit 0.20-0.40 gegen 1.00 einer
    // VitaminBottle verliert der Tee diesen Vergleich immer und tut dann gar
    // nichts. Umgekehrt hebt das Medikament den Tee sofort an. Diese Rangordnung
    // ist der ganze Sinn kleiner Force-Werte.
    protected void ApplyImmunityGain(PlayerBase player, string effectPath, float servings, float timeModifier)
    {
        float force = GetTerjeGameConfig().ConfigGetFloat(effectPath + " medImmunityGainForce");
        if (force <= 0)
        {
            return;
        }

        float timeSec = GetTerjeGameConfig().ConfigGetFloat(effectPath + " medImmunityGainTimeSec");
        if (timeSec <= 0)
        {
            return;
        }

        float activeForce = 0;
        float activeTime = 0;
        player.GetTerjeStats().GetImmunityGainValue(activeForce, activeTime);

        if (force < activeForce)
        {
            return;
        }

        float maxTimeSec = GetTerjeGameConfig().ConfigGetFloat(effectPath + " medImmunityGainMaxTimer");
        if (maxTimeSec <= 0)
        {
            // Terje faellt an dieser Stelle auf 1800 zurueck. Fuer einen
            // Kraeutertee waere das die Laufzeit einer VitaminBottle und damit
            // grob zu stark, deshalb ist der Rueckfallwert hier bewusst
            // niedriger. Ein fehlender Deckel darf nie zum staerksten Deckel
            // werden.
            maxTimeSec = 300;
        }

        player.GetTerjeStats().SetImmunityGainValue(force, Math.Min(maxTimeSec, activeTime + (timeSec * servings * timeModifier)));
    }

    // Gesundheitsregeneration, nach demselben Vorbild
    // (TerjeConsumableEffects.c:56-69). Kein Force-Vergleich - Terje kennt hier
    // nur einen Timer, der bis zum Deckel aufaddiert wird.
    protected void ApplyHealthRegen(PlayerBase player, string effectPath, float servings, float timeModifier)
    {
        float timeSec = GetTerjeGameConfig().ConfigGetFloat(effectPath + " medHealthgainTimeSec");
        if (timeSec <= 0)
        {
            return;
        }

        float maxTimeSec = GetTerjeGameConfig().ConfigGetFloat(effectPath + " medHealthgainMaxTimeSec");
        if (maxTimeSec <= 0)
        {
            // Siehe oben: Terjes Rueckfallwert 1800 waere hier staerker als
            // jeder Injektor im Spiel.
            maxTimeSec = 60;
        }

        float activeTime = player.GetTerjeStats().GetHealthExtraRegenTimer();
        player.GetTerjeStats().SetHealthExtraRegenTimer(Math.Min(maxTimeSec, activeTime + (timeSec * servings * timeModifier)));
    }

    // Tooltip. Verwendet Terjes eigene, bereits in 15 Sprachen uebersetzte
    // Schluessel (TerjeMedicine/stringtable.csv:316 und :318) statt eigene
    // anzulegen - diese Datei wird nur mit geladenem TerjeMedicine ueberhaupt
    // kompiliert ("#ifdef TERJE_MEDICINE_MOD" ganz oben), die Schluessel
    // sind also immer da, und ein Spieler soll ChefZ-Tee und Terje-Medikament
    // im selben Wortlaut lesen.
    //
    // Sichtbar wird der Text nur dort, wo Terje ihn ohnehin einblendet:
    // Edible_Base zeigt Consumable-Effekte erst ab dem Perk "surv/expert"
    // (TerjeSkills/Scripts/4_World/Entities/EdibleBase.c:3-17). Dieses Modul
    // aendert diese Bedingung nicht.
    override string Describe(EntityAI entity, string classname)
    {
        string result = super.Describe(entity, classname);

        string effectPath;
        if (!GetChefZTerjeMedRegistry().Resolve(classname, effectPath))
        {
            return result;
        }

        float immunityForce = GetTerjeGameConfig().ConfigGetFloat(effectPath + " medImmunityGainForce");
        float immunityTime = GetTerjeGameConfig().ConfigGetFloat(effectPath + " medImmunityGainTimeSec");
        if (immunityForce > 0 && immunityTime > 0)
        {
            result = result + "<color rgba='255,215,0,255'>#STR_TERJEMED_EFFECT_IMMUNGAIN</color> (" + (int)(immunityTime) + "sec)<br/>";
        }

        float healthTime = GetTerjeGameConfig().ConfigGetFloat(effectPath + " medHealthgainTimeSec");
        if (healthTime > 0)
        {
            result = result + "<color rgba='255,215,0,255'>#STR_TERJEMED_EFFECT_HEALTHREGEN</color> (" + (int)(healthTime) + "sec)<br/>";
        }

        // Eine Zeile in ChefZ-eigener Sprache, damit im Spiel unmissverstaendlich
        // dasteht, dass ein Aufguss kein Medikament ersetzt (Analyse §22).
        result = result + "<color rgba='170,170,170,255'>#STR_CHEFZ_TERJEMED_HERBAL_NOTE</color><br/>";

        return result;
    }
}
#endif // TERJE_MEDICINE_MOD
