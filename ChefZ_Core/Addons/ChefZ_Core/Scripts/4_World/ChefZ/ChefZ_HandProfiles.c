//==============================================================================
// Haltung in der Hand (26.10.2026).
//
// Die Engine setzt ein Item mit seinem Modellursprung in die Hand und
// spielt dazu die .anm ab, die fuer die naechste registrierte Elternklasse
// eingetragen ist (DayZPlayerCfgBase.c, DayZPlayerTypeRegisterItems). Jede
// .anm ist gegen EIN Vanilla-Modell gestellt; ein eigenes Modell sitzt nur
// dann richtig, wenn Ursprung und Achsen zu diesem Modell passen.
//
// Ohne Eintrag erbt jedes ChefZ-Item die Pose von Edible_Base (banana.anm)
// oder Inventory_Base (apple.anm): Fische steckten quer im Koerper, Pfanne
// und Teller hingen mit dem Stiel im Unterarm. Die Eintraege der
// Asset-Lieferung (ChefZ\*\scripts\4_world\entities\ModItemRegisterCallbacks.c)
// trugen die alten Klassennamen und gingen bei der Core-Migration verloren;
// sie stehen unten unter den heutigen Namen.
//
// FISCHE MIT EIGENEM MODELL: tools/chefz-assets baut sie mit "hold" im
// Manifest in den Rahmen ihrer Pose (import_delivery.py, hold_frame):
//   1h     Rahmen carp_live      - bis 0,7 m
//   2h     Rahmen wooden_plank   - bis 1,0 m
//   heavy  Rahmen wooden_log     - darueber
// Die Liste hier und das Manifest muessen zusammenpassen. Die Bewegungsregeln
// (Tragen, Aufheben) setzt itemBehaviour in der Config.
//
// Jede Klasse steht genau einmal: eine zweite Registrierung derselben Klasse
// in einer anderen Gruppe ueberschreibt die erste.
//
// Layer: 4_World.
//==============================================================================

modded class ModItemRegisterCallbacks
{
	override void RegisterOneHanded(DayZPlayerType pType, DayzPlayerItemBehaviorCfg pBehavior)
	{
		super.RegisterOneHanded(pType, pBehavior);

		string asiOneHand = "dz/anims/workspaces/player/player_main/player_main_1h.asi";
		string asiPipe = "dz/anims/workspaces/player/player_main/weapons/player_main_1h_pipe.asi";
		string asiKnife = "dz/anims/workspaces/player/player_main/weapons/player_main_1h_knife.asi";
		string asiFruit = "dz/anims/workspaces/player/player_main/props/player_main_1h_fruit.asi";
		string asiSodaCan = "dz/anims/workspaces/player/player_main/props/player_main_1h_sodacan.asi";
		string asiSickle = "dz/anims/workspaces/player/player_main/props/player_main_1h_sickle.asi";
		string ik = "dz/anims/anm/player/ik/gear/";

		// Fische: Basen wie Vanillas Carp (pipe) und Mackerel (1h); die
		// Vanilla-Proxys dieser Basen sitzen damit wie im Original.
		pType.AddItemInHandsProfileIK("ChefZ_FreshwaterFish_Base", asiPipe, pBehavior, ik + "carp_live.anm");
		pType.AddItemInHandsProfileIK("ChefZ_SeaFishB_Base", asiPipe, pBehavior, ik + "carp_live.anm");
		pType.AddItemInHandsProfileIK("ChefZ_SaltwaterFishA_Base", asiOneHand, pBehavior, ik + "carp_live.anm");
		pType.AddItemInHandsProfileIK("ChefZ_HorseMackerel", asiOneHand, pBehavior, ik + "sardines_live.anm");
		pType.AddItemInHandsProfileIK("ChefZ_JapaneseSardine", asiOneHand, pBehavior, ik + "sardines_live.anm");
		// eigene Modelle im Rahmen carp_live (hold "1h")
		pType.AddItemInHandsProfileIK("ChefZ_SeaBass", asiPipe, pBehavior, ik + "carp_live.anm");
		pType.AddItemInHandsProfileIK("ChefZ_RedMullet", asiPipe, pBehavior, ik + "carp_live.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Rockfish", asiPipe, pBehavior, ik + "carp_live.anm");
		pType.AddItemInHandsProfileIK("ChefZ_SeaFillet_Base", asiOneHand, pBehavior, ik + "mackerel_fillet.anm");
		pType.AddItemInHandsProfileIK("ChefZ_FreshFillet_Base", asiOneHand, pBehavior, ik + "carp_fillet_raw.anm");
		// Meeresfruechte: ohne passende Vanilla-Pose wie Inventory_Base, nicht
		// wie die Fischbasis darueber
		pType.AddItemInHandsProfileIK("ChefZ_Seafood_Base", asiOneHand, pBehavior, ik + "apple.anm");

		// Kraeuter, Gemuese
		pType.AddItemInHandsProfileIK("ChefZ_Chili", asiFruit, pBehavior, ik + "zucchini_fresh.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Carrot", asiKnife, pBehavior, ik + "screwdriver.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Parsley", asiKnife, pBehavior, ik + "screwdriver.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Rosemary", asiKnife, pBehavior, ik + "screwdriver.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Garlic", asiFruit, pBehavior, ik + "tomato_fresh.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Onion", asiFruit, pBehavior, ik + "tomato_fresh.anm");
		pType.AddItemInHandsProfileIK("ChefZ_PepperBerries", asiFruit, pBehavior, ik + "berries.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Thyme", asiKnife, pBehavior, ik + "screwdriver.anm");   // Rosmarin-Modell seit 30.10.2026

		// Fleisch, Kaese
		pType.AddItemInHandsProfileIK("ChefZ_Cheese", asiOneHand, pBehavior, ik + "rice.anm");
		pType.AddItemInHandsProfileIK("ChefZ_DicedMeat", asiOneHand, pBehavior, ik + "guts_animal.anm");
		pType.AddItemInHandsProfileIK("ChefZ_BeefLeg", asiSodaCan, pBehavior, ik + "soda_can.anm");
		pType.AddItemInHandsProfileIK("ChefZ_PorkLeg", asiSodaCan, pBehavior, ik + "soda_can.anm");
		pType.AddItemInHandsProfileIK("ChefZ_VenisonLeg", asiSodaCan, pBehavior, ik + "soda_can.anm");
		// Wuerste: Griff in der Mitte (Ursprung der Modelle seit 31.10.2026 dort).
		pType.AddItemInHandsProfileIK("ChefZ_CookedSausage", asiSodaCan, pBehavior, ik + "soda_can.anm");
		pType.AddItemInHandsProfileIK("ChefZ_RawSausage", asiSodaCan, pBehavior, ik + "soda_can.anm");
		pType.AddItemInHandsProfileIK("ChefZ_RawPorkSausage", asiSodaCan, pBehavior, ik + "soda_can.anm");
		pType.AddItemInHandsProfileIK("ChefZ_RawVenisonSausage", asiSodaCan, pBehavior, ik + "soda_can.anm");
		pType.AddItemInHandsProfileIK("ChefZ_RawBoarSausage", asiSodaCan, pBehavior, ik + "soda_can.anm");
		pType.AddItemInHandsProfileIK("ChefZ_RawHunterSausage", asiSodaCan, pBehavior, ik + "soda_can.anm");
		pType.AddItemInHandsProfileIK("ChefZ_RawSpicySausage", asiSodaCan, pBehavior, ik + "soda_can.anm");
		pType.AddItemInHandsProfileIK("ChefZ_SmokedSausage", asiSodaCan, pBehavior, ik + "soda_can.anm");
		pType.AddItemInHandsProfileIK("ChefZ_DrySausage", asiSodaCan, pBehavior, ik + "soda_can.anm");
		// Salzstreuer: die geerbte Pose ist Vanillas zweihaendiger Duengersack.
		pType.AddItemInHandsProfileIK("ChefZ_RawSalt", asiSodaCan, pBehavior, ik + "soda_can.anm");

		// Geraete und Behaelter
		pType.AddItemInHandsProfileIK("ChefZ_FryingPan", asiPipe, pBehavior, ik + "frying_pan.anm");
		pType.AddItemInHandsProfileIK("ChefZ_HoneycombFrame_Base", asiOneHand, pBehavior, ik + "TireRepairKit.anm");
		pType.AddItemInHandsProfileIK("ChefZ_EmptyJar", asiOneHand, pBehavior, ik + "marmalade.anm");
		pType.AddItemInHandsProfileIK("ChefZ_BeeSmoker", asiKnife, pBehavior, ik + "screwdriver.anm");
		pType.AddItemInHandsProfileIK("ChefZ_UncappingFork", asiSickle, pBehavior, ik + "sickle.anm");
		pType.AddItemInHandsProfileIK("ChefZ_CookbookItem", asiOneHand, pBehavior, ik + "book.anm");
	}

	override void RegisterTwoHanded(DayZPlayerType pType, DayzPlayerItemBehaviorCfg pBehavior)
	{
		super.RegisterTwoHanded(pType, pBehavior);

		string asiTwoHand = "dz/anims/workspaces/player/player_main/player_main_2h.asi";
		string asiPot = "dz/anims/workspaces/player/player_main/props/player_main_2h_pot.asi";
		string plank = "dz/anims/anm/player/ik/two_handed/wooden_plank.anm";
		string cauldron = "dz/anims/anm/player/ik/two_handed/Cauldron.anm";

		// Fische im Rahmen wooden_plank (hold "2h")
		pType.AddItemInHandsProfileIK("ChefZ_Muskellunge", asiTwoHand, pBehavior, plank);
		pType.AddItemInHandsProfileIK("ChefZ_Eel", asiTwoHand, pBehavior, plank);
		pType.AddItemInHandsProfileIK("ChefZ_Cod", asiTwoHand, pBehavior, plank);
		pType.AddItemInHandsProfileIK("ChefZ_SeaEel", asiTwoHand, pBehavior, plank);
		pType.AddItemInHandsProfileIK("ChefZ_MahiMahi", asiTwoHand, pBehavior, plank);
		pType.AddItemInHandsProfileIK("ChefZ_GiantTrevally", asiTwoHand, pBehavior, plank);
		pType.AddItemInHandsProfileIK("ChefZ_Yellowtail", asiTwoHand, pBehavior, plank);

		// Lieferung: Topf und Schalen
		pType.AddItemInHandsProfileIK("ChefZ_MilkCan", asiPot, pBehavior, "dz/anims/anm/player/ik/two_handed/CookingPot.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Cabbage", asiPot, pBehavior, cauldron);
		pType.AddItemInHandsProfileIK("ChefZ_Bread", asiPot, pBehavior, cauldron);
		pType.AddItemInHandsProfileIK("ChefZ_FarmersBreakfast", asiPot, pBehavior, cauldron);
		pType.AddItemInHandsProfileIK("ChefZ_MeatDumplings", asiPot, pBehavior, cauldron);
		pType.AddItemInHandsProfileIK("ChefZ_SausageBreadPlate", asiPot, pBehavior, cauldron);
		pType.AddItemInHandsProfileIK("ChefZ_VegetableSoupBowl", asiPot, pBehavior, cauldron);
		pType.AddItemInHandsProfileIK("ChefZ_PumpkinSoupBowl", asiPot, pBehavior, cauldron);
		pType.AddItemInHandsProfileIK("ChefZ_BoneBrothSoupBowl", asiPot, pBehavior, cauldron);
		pType.AddItemInHandsProfileIK("ChefZ_HunterStewBowl", asiPot, pBehavior, cauldron);
		pType.AddItemInHandsProfileIK("ChefZ_FishermanStewBowl", asiPot, pBehavior, cauldron);
		pType.AddItemInHandsProfileIK("ChefZ_ChernarusChiliBowl", asiPot, pBehavior, cauldron);
		pType.AddItemInHandsProfileIK("ChefZ_EmptyBowl", asiPot, pBehavior, cauldron);
		pType.AddItemInHandsProfileIK("ChefZ_EmptyBox", asiPot, pBehavior, cauldron);

		// Bausaetze der Imkerei und Anzuchtkasten (31.10.2026): mit beiden Haenden.
		// Die Doppelbeute erbt vom einfachen Bausatz.
		pType.AddItemInHandsProfileIK("ChefZ_BeehiveKit", asiPot, pBehavior, cauldron);
		pType.AddItemInHandsProfileIK("ChefZ_MushroomTray", asiPot, pBehavior, cauldron);
		pType.AddItemInHandsProfileIK("ChefZ_MushroomShelfKit", asiPot, pBehavior, cauldron);
		pType.AddItemInHandsProfileIK("ChefZ_GrainMillKit", asiPot, pBehavior, cauldron);
	}

	override void RegisterHeavy(DayZPlayerType pType, DayzPlayerItemBehaviorCfg pBehavior)
	{
		super.RegisterHeavy(pType, pBehavior);

		string asiHeavy = "dz/anims/workspaces/player/player_main/player_main_heavy.asi";
		string log = "dz/anims/anm/player/ik/heavy/wooden_log.anm";

		// Fische im Rahmen wooden_log (hold "heavy")
		pType.AddItemInHandsProfileIK("ChefZ_BluefinTuna", asiHeavy, pBehavior, log);
		pType.AddItemInHandsProfileIK("ChefZ_YellowfinTuna", asiHeavy, pBehavior, log);
		pType.AddItemInHandsProfileIK("ChefZ_GiantGrouper", asiHeavy, pBehavior, log);
		pType.AddItemInHandsProfileIK("ChefZ_BlueMarlin", asiHeavy, pBehavior, log);
		pType.AddItemInHandsProfileIK("ChefZ_Sailfish", asiHeavy, pBehavior, log);
		pType.AddItemInHandsProfileIK("ChefZ_Tarpon", asiHeavy, pBehavior, log);
	}
}
