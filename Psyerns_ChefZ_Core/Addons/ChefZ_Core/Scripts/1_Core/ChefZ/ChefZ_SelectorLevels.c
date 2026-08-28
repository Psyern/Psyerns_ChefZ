//==============================================================================
// ChefZ_SelectorL1..L8 - die Ebenen der Selektorkette
//
// Erzeugt und bewusst eintoenig. Der Grund fuer die Kette steht vollstaendig im
// Kopf von ChefZ_SelectorNode.c: der JSON-Deserialisierer der Engine stuerzt an
// einer selbstbezueglichen Klasse ab, ohne Meldung und ohne Aufrufkeller.
//
// Jede Ebene traegt dieselben Blattfelder (geerbt) und verweist auf die
// NAECHSTE Ebene. ChefZ_SelectorL8 hat keine Kinder mehr und beendet die
// Typkette.
//
// Wer eine Ebene ergaenzt: hier einfuegen, LAST hochsetzen, und
// ChefZ_Selector.c bleibt unberuehrt - es zeigt auf L1.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_SelectorL1 : ChefZ_SelectorNode
{
    ref array<ref ChefZ_SelectorL2> anyOf;
    ref array<ref ChefZ_SelectorL2> allOf;
    ref ChefZ_SelectorL2            not;

    override void CollectAnyOf(notnull array<ref ChefZ_SelectorNode> outList)
    {
        if (!anyOf)
            return;
        for (int i = 0; i < anyOf.Count(); i++)
            outList.Insert(anyOf.Get(i));
    }

    override void CollectAllOf(notnull array<ref ChefZ_SelectorNode> outList)
    {
        if (!allOf)
            return;
        for (int i = 0; i < allOf.Count(); i++)
            outList.Insert(allOf.Get(i));
    }

    override ChefZ_SelectorNode GetNot()   { return not; }
    override bool HasAnyOf()               { return anyOf != null; }
    override bool HasAllOf()               { return allOf != null; }
    override bool IsLastLevel()            { return false; }
}

class ChefZ_SelectorL2 : ChefZ_SelectorNode
{
    ref array<ref ChefZ_SelectorL3> anyOf;
    ref array<ref ChefZ_SelectorL3> allOf;
    ref ChefZ_SelectorL3            not;

    override void CollectAnyOf(notnull array<ref ChefZ_SelectorNode> outList)
    {
        if (!anyOf)
            return;
        for (int i = 0; i < anyOf.Count(); i++)
            outList.Insert(anyOf.Get(i));
    }

    override void CollectAllOf(notnull array<ref ChefZ_SelectorNode> outList)
    {
        if (!allOf)
            return;
        for (int i = 0; i < allOf.Count(); i++)
            outList.Insert(allOf.Get(i));
    }

    override ChefZ_SelectorNode GetNot()   { return not; }
    override bool HasAnyOf()               { return anyOf != null; }
    override bool HasAllOf()               { return allOf != null; }
    override bool IsLastLevel()            { return false; }
}

class ChefZ_SelectorL3 : ChefZ_SelectorNode
{
    ref array<ref ChefZ_SelectorL4> anyOf;
    ref array<ref ChefZ_SelectorL4> allOf;
    ref ChefZ_SelectorL4            not;

    override void CollectAnyOf(notnull array<ref ChefZ_SelectorNode> outList)
    {
        if (!anyOf)
            return;
        for (int i = 0; i < anyOf.Count(); i++)
            outList.Insert(anyOf.Get(i));
    }

    override void CollectAllOf(notnull array<ref ChefZ_SelectorNode> outList)
    {
        if (!allOf)
            return;
        for (int i = 0; i < allOf.Count(); i++)
            outList.Insert(allOf.Get(i));
    }

    override ChefZ_SelectorNode GetNot()   { return not; }
    override bool HasAnyOf()               { return anyOf != null; }
    override bool HasAllOf()               { return allOf != null; }
    override bool IsLastLevel()            { return false; }
}

class ChefZ_SelectorL4 : ChefZ_SelectorNode
{
    ref array<ref ChefZ_SelectorL5> anyOf;
    ref array<ref ChefZ_SelectorL5> allOf;
    ref ChefZ_SelectorL5            not;

    override void CollectAnyOf(notnull array<ref ChefZ_SelectorNode> outList)
    {
        if (!anyOf)
            return;
        for (int i = 0; i < anyOf.Count(); i++)
            outList.Insert(anyOf.Get(i));
    }

    override void CollectAllOf(notnull array<ref ChefZ_SelectorNode> outList)
    {
        if (!allOf)
            return;
        for (int i = 0; i < allOf.Count(); i++)
            outList.Insert(allOf.Get(i));
    }

    override ChefZ_SelectorNode GetNot()   { return not; }
    override bool HasAnyOf()               { return anyOf != null; }
    override bool HasAllOf()               { return allOf != null; }
    override bool IsLastLevel()            { return false; }
}

class ChefZ_SelectorL5 : ChefZ_SelectorNode
{
    ref array<ref ChefZ_SelectorL6> anyOf;
    ref array<ref ChefZ_SelectorL6> allOf;
    ref ChefZ_SelectorL6            not;

    override void CollectAnyOf(notnull array<ref ChefZ_SelectorNode> outList)
    {
        if (!anyOf)
            return;
        for (int i = 0; i < anyOf.Count(); i++)
            outList.Insert(anyOf.Get(i));
    }

    override void CollectAllOf(notnull array<ref ChefZ_SelectorNode> outList)
    {
        if (!allOf)
            return;
        for (int i = 0; i < allOf.Count(); i++)
            outList.Insert(allOf.Get(i));
    }

    override ChefZ_SelectorNode GetNot()   { return not; }
    override bool HasAnyOf()               { return anyOf != null; }
    override bool HasAllOf()               { return allOf != null; }
    override bool IsLastLevel()            { return false; }
}

class ChefZ_SelectorL6 : ChefZ_SelectorNode
{
    ref array<ref ChefZ_SelectorL7> anyOf;
    ref array<ref ChefZ_SelectorL7> allOf;
    ref ChefZ_SelectorL7            not;

    override void CollectAnyOf(notnull array<ref ChefZ_SelectorNode> outList)
    {
        if (!anyOf)
            return;
        for (int i = 0; i < anyOf.Count(); i++)
            outList.Insert(anyOf.Get(i));
    }

    override void CollectAllOf(notnull array<ref ChefZ_SelectorNode> outList)
    {
        if (!allOf)
            return;
        for (int i = 0; i < allOf.Count(); i++)
            outList.Insert(allOf.Get(i));
    }

    override ChefZ_SelectorNode GetNot()   { return not; }
    override bool HasAnyOf()               { return anyOf != null; }
    override bool HasAllOf()               { return allOf != null; }
    override bool IsLastLevel()            { return false; }
}

class ChefZ_SelectorL7 : ChefZ_SelectorNode
{
    ref array<ref ChefZ_SelectorL8> anyOf;
    ref array<ref ChefZ_SelectorL8> allOf;
    ref ChefZ_SelectorL8            not;

    override void CollectAnyOf(notnull array<ref ChefZ_SelectorNode> outList)
    {
        if (!anyOf)
            return;
        for (int i = 0; i < anyOf.Count(); i++)
            outList.Insert(anyOf.Get(i));
    }

    override void CollectAllOf(notnull array<ref ChefZ_SelectorNode> outList)
    {
        if (!allOf)
            return;
        for (int i = 0; i < allOf.Count(); i++)
            outList.Insert(allOf.Get(i));
    }

    override ChefZ_SelectorNode GetNot()   { return not; }
    override bool HasAnyOf()               { return anyOf != null; }
    override bool HasAllOf()               { return allOf != null; }
    override bool IsLastLevel()            { return false; }
}

//! Letzte Ebene: keine Kinder mehr. Hier endet die Typkette, und damit
//! endet auch der Typbeschreiber, den die Engine beim Lesen aufbaut.
class ChefZ_SelectorL8 : ChefZ_SelectorNode
{
}

