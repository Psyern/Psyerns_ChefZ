# ChefZ Enforce-Script Audit

Stand: 2026-09-03 12:42  
Wurzel: `C:\Users\Administrator\Desktop\Psyerns_ChefZ\Psyerns_ChefZ_Core`  
Regelbasis: enforce-script Hard Rules, Safe-AI-CodingPrompt.md, Tips-Common-Pitfalls.md, DME_129_Audit_Prompt.md  
Vanilla-Klassenkarte: 6069 Klassen aus C:\Users\Administrator\Desktop\Mod Repositories\scripts - 1.29

## Umfang

| Kennzahl | Wert |
|---|---|
| addons | 14 |
| script_files | 174 |
| script_lines | 77231 |
| chefz_classes | 324 |
| json_files | 58 |
| xml_files | 3 |
| stringtable_keys | 367 |
| action_classes | 3 |

## Ergebnis nach Prioritaet

| Prio | Bedeutung | Anzahl |
|---|---|---|
| P0 | Compile-Fehler / Modul wird nicht geladen | 1 |
| P1 | Segfault / Crash-Risiko | 0 |
| P2 | DayZ 1.29 Breaking | 43 |
| P3 | Silent Failure | 4 |
| P4 | Best Practice / Style | 169 |

## Ergebnis nach Regel

| Prio | Regel | Anzahl |
|---|---|---|
| P0 | cfg-requiredAddon-missing | 1 |
| P2 | IsClient-IsServer | 43 |
| P3 | GetObjectsAtPosition | 2 |
| P3 | model-path-missing | 2 |
| P4 | indent-spaces | 169 |

## P0 - Compile-Fehler / Modul wird nicht geladen (1)

### cfg-requiredAddon-missing (1)

- `Addons/ChefZ_Farming/config.cpp:106` ChefZ_Farming verlangt requiredAddons 'ChefZ_Plants_Cultivation', aber kein Addon im Repo deklariert diese CfgPatches-Klasse

## P2 - DayZ 1.29 Breaking (43)

### IsClient-IsServer (43)

- `Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_EventBus.c:1007` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  return g_Game.IsServer();
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/Container/ChefZ_ContainerService.c:235` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/Container/ChefZ_ContainerService.c:341` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/Container/ChefZ_ContainerService.c:399` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/Cooking/ChefZ_Applicator.c:131` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/Cooking/ChefZ_CookActor.c:170` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/Cooking/ChefZ_CookingDeviceAdapter.c:211` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/Cooking/ChefZ_CookingHook.c:66` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/Portion/ChefZ_PortionedFood_Base.c:186` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_ActionProcessAtStation.c:673` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_GenericCraftRecipe.c:686` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_ProcessRunner.c:112` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_ProcessRunner.c:198` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_ProcessingStation_Base.c:541` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_ProcessingStation_Base.c:633` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_ProcessingStation_Base.c:713` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_ProcessingStation_Base.c:985` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_ProcessingStation_Base.c:1229` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_ProcessingStation_Base.c:1306` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/State/ChefZ_ItemDecay.c:136` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/State/ChefZ_ItemDecay.c:309` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/State/ChefZ_ItemDecay.c:352` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/State/ChefZ_ItemStateComponent.c:971` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  return g_Game && g_Game.IsServer();
  ```
- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/State/ChefZ_ItemTransform.c:85` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Core/Scripts/5_Mission/ChefZ/Diagnostics/ChefZ_Diagnostics.c:107` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Farming/Scripts/4_World/ChefZ/Farming/ChefZ_Apiary.c:390` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Farming/Scripts/4_World/ChefZ/Farming/ChefZ_Apiary.c:494` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Farming/Scripts/4_World/ChefZ/Farming/ChefZ_Apiary.c:553` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Farming/Scripts/4_World/ChefZ/Farming/ChefZ_Apiary.c:816` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Farming/Scripts/4_World/ChefZ/Farming/ChefZ_Apiary.c:981` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Farming/Scripts/4_World/ChefZ/Farming/ChefZ_Apiary.c:1356` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Farming/Scripts/4_World/ChefZ/Farming/ChefZ_Apiary.c:1589` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Farming/Scripts/4_World/ChefZ/Farming/ChefZ_Apiary.c:1612` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Farming/Scripts/4_World/ChefZ/Farming/ChefZ_WildPlants.c:392` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Farming/Scripts/4_World/ChefZ/Farming/ChefZ_WildPlants.c:426` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Farming/Scripts/4_World/ChefZ/Farming/ChefZ_WildPlants.c:543` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Farming/Scripts/4_World/ChefZ/Farming/ChefZ_WildPlants.c:588` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Processing/Scripts/4_World/ChefZ/Preservation/ChefZ_Smoker.c:274` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Processing/Scripts/4_World/ChefZ/Preservation/ChefZ_Smoker.c:289` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Processing/Scripts/4_World/ChefZ/Preservation/ChefZ_Smoker.c:319` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Processing/Scripts/4_World/ChefZ/Processing/ChefZ_DairyStations.c:210` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Processing/Scripts/4_World/ChefZ/Processing/ChefZ_HoneyExtractor.c:200` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```
- `Addons/ChefZ_Processing/Scripts/4_World/ChefZ/Processing/ChefZ_MeatStations.c:187` IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()
  ```c
  if (!g_Game || !g_Game.IsServer())
  ```

## P3 - Silent Failure (4)

### GetObjectsAtPosition (2)

- `Addons/ChefZ_Core/Scripts/4_World/ChefZ/Container/ChefZ_ContainerService.c:654` GetObjectsAtPosition* ist teuer - Wiki raet zu Triggern/statischen Listen (pruefen, ob Aufruf selten ist)
  ```c
  g_Game.GetObjectsAtPosition3D(center, radius, objects, proxies);
  ```
- `Addons/ChefZ_Processing/Scripts/4_World/ChefZ/Salt/ChefZ_FryingPan.c:72` GetObjectsAtPosition* ist teuer - Wiki raet zu Triggern/statischen Listen (pruefen, ob Aufruf selten ist)
  ```c
  g_Game.GetObjectsAtPosition(GetPosition(), CHEFZ_HEAT_RADIUS_M, nearby, proxies);
  ```

### model-path-missing (2)

- `Addons/ChefZ_Farming/config.cpp:1619` Modell-/Texturdatei '\ChefZ\ChefZ_Plants\cultivation\models\corn_plant.p3d' fehlt (Addons/ChefZ_Plants/cultivation/models/corn_plant.p3d)
- `Addons/ChefZ_Farming/config.cpp:1731` Modell-/Texturdatei '\ChefZ\ChefZ_Plants\cultivation\models\corn_plant.p3d' fehlt (Addons/ChefZ_Plants/cultivation/models/corn_plant.p3d)

## P4 - Best Practice / Style (169)

### indent-spaces (169)

| Datei | Fundstellen |
|---|---|
| Addons/ChefZ_Cookbook/Scripts/3_Game/ChefZ/Cookbook/ChefZ_CookbookRPC.c | 1 |
| Addons/ChefZ_Cookbook/Scripts/3_Game/ChefZ/Cookbook/ChefZ_KnowledgeManager.c | 1 |
| Addons/ChefZ_Cookbook/Scripts/3_Game/ChefZ/Cookbook/ChefZ_KnowledgeState.c | 1 |
| Addons/ChefZ_Cookbook/Scripts/3_Game/ChefZ/Cookbook/ChefZ_RecipeStatus.c | 1 |
| Addons/ChefZ_Cookbook/Scripts/4_World/ChefZ/Cookbook/ChefZ_ActionOpenCookbook.c | 1 |
| Addons/ChefZ_Cookbook/Scripts/4_World/ChefZ/Cookbook/ChefZ_ActionRegistration.c | 1 |
| Addons/ChefZ_Cookbook/Scripts/4_World/ChefZ/Cookbook/ChefZ_CookbookItem.c | 1 |
| Addons/ChefZ_Cookbook/Scripts/4_World/ChefZ/Cookbook/ChefZ_CookbookOpener.c | 1 |
| Addons/ChefZ_Cookbook/Scripts/4_World/ChefZ/Cookbook/ChefZ_CookbookServer.c | 1 |
| Addons/ChefZ_Cookbook/Scripts/4_World/ChefZ/Cookbook/ChefZ_KnowledgeHooks.c | 1 |
| Addons/ChefZ_Cookbook/Scripts/4_World/ChefZ/Cookbook/ChefZ_PlayerKnowledge.c | 1 |
| Addons/ChefZ_Cookbook/Scripts/5_Mission/ChefZ/Cookbook/ChefZ_CookbookInput.c | 1 |
| Addons/ChefZ_Cooking/Scripts/4_World/ChefZ/Cooking/ChefZ_SauceItems.c | 1 |
| Addons/ChefZ_Cooking/Scripts/4_World/ChefZ/Serving/ChefZ_ServingItems.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_CapabilityGate.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_CategoryClosure.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_CompiledGradeRule.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_CompiledProcess.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_CompiledRecipe.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_CompiledSelector.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_CompiledSlot.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_ContainerDef.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_CookContext.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_CoreSelfTest.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_CoreSettingsDef.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_CraftIntent.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_EventArgs.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_EventNames.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_Identity.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_IngredientInfo.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_ItemFacts.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_LoadReport.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_Log.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_LogDefs.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_MatchResult.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_MatchTrace.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_Matcher.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_MatcherSelfTest.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_NutritionDef.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_NutritionVector.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_PortionSpec.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_PreservationDef.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_PriorityWeights.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_PriorityWeightsDef.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_ProcessContext.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_ProcessDef.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_QualityEvaluation.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_QualityScoring.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_QualityScoringDef.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_QualityTierDef.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_Range.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_RecipeDef.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_RecipeEvaluator.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_RecipeRank.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_Record.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_RecordKind.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_RecordTypes.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_Selector.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_SelectorCompiler.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_SelectorLevels.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_SelectorNode.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_SelfTestTrace.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_SlotEvaluator.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_StateDef.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_StationDef.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_StringOrder.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_SymbolResolver.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_SymbolTable.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_TextList.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_ToolGroupDef.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_TransformDef.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_Undefined.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_VanillaStage.c | 1 |
| Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_VesselKeys.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_AddonJsonSource.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_CapabilityRegistry.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_CategoryManager.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_CategorySelfTest.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_ConfigCppSource.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_ConfigManager.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_ConfigSelfTest.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_ContainerRegistry.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_ContainerSelfTest.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_EventBus.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_EventSelfTest.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_IngredientManager.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_IngredientSelfTest.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_JsonDocs.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_ManagerSymbolResolver.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_NutritionManager.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_NutritionSelfTest.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_PortionManager.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_PortionRequest.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_PortionSelfTest.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_PreservationManager.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_PreservationSelfTest.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_ProcessCompiler.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_ProcessingManager.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_ProcessingSelfTest.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_ProfileOverlaySource.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_ProgressRegistry.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_QualityManager.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_QualitySelfTest.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_RecipeCompiler.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_RecipeEngine.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_RecipeRanker.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_RecipeSelfTest.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_RecordSink.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_RecordSource.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_Registry.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_StateManager.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_StateSelfTest.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_ToolRegistry.c | 1 |
| Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_VanillaNutrition.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/ChefZ_ActionRegistration.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/ChefZ_FactCollector.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Container/ChefZ_ContainerService.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Container/ChefZ_Container_Base.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Cooking/ChefZ_Applicator.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Cooking/ChefZ_ApplicatorSelfTest.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Cooking/ChefZ_CookActor.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Cooking/ChefZ_CookSession.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Cooking/ChefZ_CookingDeviceAdapter.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Cooking/ChefZ_CookingHook.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Cooking/ChefZ_CookingSelfTest.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Cooking/ChefZ_DeviceDescriptor.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Cooking/ChefZ_ModdedCooking.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Cooking/ChefZ_PlannedOutput.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Cooking/ChefZ_SessionState.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Cooking/ChefZ_VesselSignature.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Portion/ChefZ_ActionTakePortion.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Portion/ChefZ_PortionedFood_Base.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_ActionProcessAtStation.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_GenericCraftRecipe.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_HandcraftBridge.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_HandcraftSelfTest.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_ModdedRecipes.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_ModdedWorldCraft.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_ProcessRunner.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_ProcessingStation_Base.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/State/ChefZ_Edible_Base.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/State/ChefZ_ItemDecay.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/State/ChefZ_ItemStateComponent.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/State/ChefZ_ItemTransform.c | 1 |
| Addons/ChefZ_Core/Scripts/4_World/ChefZ/State/ChefZ_Item_Base.c | 1 |
| Addons/ChefZ_Core/Scripts/5_Mission/ChefZ/ChefZ_Boot.c | 1 |
| Addons/ChefZ_Core/Scripts/5_Mission/ChefZ/ChefZ_CompNotice.c | 1 |
| Addons/ChefZ_Core/Scripts/5_Mission/ChefZ/ChefZ_CoreEntry.c | 1 |
| Addons/ChefZ_Core/Scripts/5_Mission/ChefZ/Diagnostics/ChefZ_AdminCommands.c | 1 |
| Addons/ChefZ_Core/Scripts/5_Mission/ChefZ/Diagnostics/ChefZ_Diagnostics.c | 1 |
| Addons/ChefZ_Core/Scripts/5_Mission/ChefZ/Diagnostics/ChefZ_DiagnosticsSelfTest.c | 1 |
| Addons/ChefZ_Core/Tests/V_A_PboJsonSmoke/Scripts/5_Mission/ChefZ/ChefZ_PboProbe.c | 1 |
| Addons/ChefZ_Farming/Scripts/4_World/ChefZ/Farming/ChefZ_Apiary.c | 1 |
| Addons/ChefZ_Farming/Scripts/4_World/ChefZ/Farming/ChefZ_FarmingItems.c | 1 |
| Addons/ChefZ_Farming/Scripts/4_World/ChefZ/Farming/ChefZ_HerbItems.c | 1 |
| Addons/ChefZ_Farming/Scripts/4_World/ChefZ/Farming/ChefZ_ProduceFarming.c | 1 |
| Addons/ChefZ_Farming/Scripts/4_World/ChefZ/Farming/ChefZ_WildPlants.c | 1 |
| Addons/ChefZ_Ingredients/Scripts/4_World/ChefZ/Ingredients/ChefZ_DairyIngredients.c | 1 |
| Addons/ChefZ_Ingredients/Scripts/4_World/ChefZ/Ingredients/ChefZ_SpiceIngredients.c | 1 |
| Addons/ChefZ_Ingredients/Scripts/4_World/ChefZ/Ingredients/ChefZ_VanillaFoodItems.c | 1 |
| Addons/ChefZ_Meat/Scripts/4_World/ChefZ/Meat/ChefZ_MeatItemBase.c | 1 |
| Addons/ChefZ_Preservation/Scripts/4_World/ChefZ/Preservation/ChefZ_PreservedFood_Base.c | 1 |
| Addons/ChefZ_Processing/Scripts/4_World/ChefZ/Preservation/ChefZ_Smoker.c | 1 |
| Addons/ChefZ_Processing/Scripts/4_World/ChefZ/Processing/ChefZ_DairyStations.c | 1 |
| Addons/ChefZ_Processing/Scripts/4_World/ChefZ/Processing/ChefZ_HoneyExtractor.c | 1 |
| Addons/ChefZ_Processing/Scripts/4_World/ChefZ/Processing/ChefZ_MeatStations.c | 1 |
| Addons/ChefZ_Processing/Scripts/4_World/ChefZ/Processing/ChefZ_ProcessingItems.c | 1 |
| Addons/ChefZ_Processing/Scripts/4_World/ChefZ/Processing/ChefZ_StationGate.c | 1 |
| Addons/ChefZ_Processing/Scripts/4_World/ChefZ/Salt/ChefZ_FryingPan.c | 1 |

## Info - Config-Klassen ohne eigene Skriptklasse

Nicht zwingend ein Fehler (die Basisklasse liefert das Verhalten), aber pruefenswert, wenn ChefZ-Logik erwartet wird.

- `ChefZ_BeanSausagePlate` : ChefZ_DishesAPlate (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_BeanSausagePlate` : ChefZ_ServedDish_Base (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_BeefLeg` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_BeehivePlacing` : Inventory_Base (Addons/ChefZ_Farming/config.cpp)
- `ChefZ_Bees_Attack_SoundSet` : baseCharacter_SoundSet (Addons/ChefZ_Farming/config.cpp)
- `ChefZ_Bees_Attack_SoundShader` : baseCharacter_SoundShader (Addons/ChefZ_Farming/config.cpp)
- `ChefZ_BlackPepper` : ChefZ_SpiceBase (Addons/ChefZ_Ingredients/config.cpp)
- `ChefZ_BloodSausagePlate` : ChefZ_DishesAPlate (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_BloodSausagePlate` : ChefZ_ServedDish_Base (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_BoarSausage` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_BoneBroth` : ChefZ_SauceIngredient (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_BoneBroth` : ChefZ_SauceItemBase (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_Butter` : Lard (Addons/ChefZ_Ingredients/config.cpp)
- `ChefZ_ContainerItemBase` : Inventory_Base (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_CookedSausage` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_Cream` : Marmalade (Addons/ChefZ_Ingredients/config.cpp)
- `ChefZ_CreamMushroomPasta` : ChefZ_DishesAPlate (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_CreamMushroomPasta` : ChefZ_ServedDish_Base (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_CreamSauce` : ChefZ_SauceIngredient (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_CreamSauce` : ChefZ_SauceItemBase (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_DicedMeat` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_DriedFish` : ChefZ_PreservedFood_Base (Addons/ChefZ_Preservation/config.cpp)
- `ChefZ_DriedMeat` : ChefZ_PreservedFood_Base (Addons/ChefZ_Preservation/config.cpp)
- `ChefZ_DriedPaprika` : ChefZ_DriedHerbBase (Addons/ChefZ_Ingredients/config.cpp)
- `ChefZ_DriedParsley` : ChefZ_DriedHerbBase (Addons/ChefZ_Ingredients/config.cpp)
- `ChefZ_DriedPeppercorns` : ChefZ_SpiceBase (Addons/ChefZ_Ingredients/config.cpp)
- `ChefZ_DriedRosemary` : ChefZ_DriedHerbBase (Addons/ChefZ_Ingredients/config.cpp)
- `ChefZ_DriedThyme` : ChefZ_DriedHerbBase (Addons/ChefZ_Ingredients/config.cpp)
- `ChefZ_DriedWildGarlic` : ChefZ_DriedHerbBase (Addons/ChefZ_Ingredients/config.cpp)
- `ChefZ_DrySausage` : ChefZ_PreservedFood_Base (Addons/ChefZ_Preservation/config.cpp)
- `ChefZ_FishPotatoPlate` : ChefZ_DishesAPlate (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_FishPotatoPlate` : ChefZ_ServedDish_Base (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_HandRake` : Inventory_Base (Addons/ChefZ_Farming/config.cpp)
- `ChefZ_HerbMix` : ChefZ_SpiceBase (Addons/ChefZ_Ingredients/config.cpp)
- `ChefZ_HerbStationBase` : Inventory_Base (Addons/ChefZ_Processing/config.cpp)
- `ChefZ_HunterPasta` : ChefZ_DishesAPlate (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_HunterPasta` : ChefZ_ServedDish_Base (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_HunterPlate` : ChefZ_DishesAPlate (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_HunterPlate` : ChefZ_ServedDish_Base (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_HunterSausage` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_HunterSeasoning` : ChefZ_SpiceBase (Addons/ChefZ_Ingredients/config.cpp)
- `ChefZ_MacAndCheese` : ChefZ_DishesAPlate (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_MacAndCheese` : ChefZ_ServedDish_Base (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_MilkCan` : Inventory_Base (Addons/ChefZ_Ingredients/config.cpp)
- `ChefZ_MincedBear` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_MincedBoar` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_MincedChicken` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_MincedMeat` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_MincedPork` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_MincedVenison` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_MushroomCreamSauce` : ChefZ_SauceIngredient (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_MushroomCreamSauce` : ChefZ_SauceItemBase (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_MushroomCulture` : ChefZ_SpiceBase (Addons/ChefZ_Ingredients/config.cpp)
- `ChefZ_PorkLeg` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_PorkSausage` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_RawBoarSausage` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_RawHunterSausage` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_RawPorkSausage` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_RawSalt` : GardenLime (Addons/ChefZ_Ingredients/config.cpp)
- `ChefZ_RawSausage` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_RawSpicySausage` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_RawVenisonSausage` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_Salt` : GardenLime (Addons/ChefZ_Ingredients/config.cpp)
- `ChefZ_SaltedFish` : ChefZ_PreservedFood_Base (Addons/ChefZ_Preservation/config.cpp)
- `ChefZ_SaltedMeat` : ChefZ_PreservedFood_Base (Addons/ChefZ_Preservation/config.cpp)
- `ChefZ_SausagePasta` : ChefZ_DishesAPlate (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_SausagePasta` : ChefZ_ServedDish_Base (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_SausagePotatoes` : ChefZ_DishesAPlate (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_SausagePotatoes` : ChefZ_ServedDish_Base (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_SmokedFish` : ChefZ_PreservedFood_Base (Addons/ChefZ_Preservation/config.cpp)
- `ChefZ_SmokedMeat` : ChefZ_PreservedFood_Base (Addons/ChefZ_Preservation/config.cpp)
- `ChefZ_SmokedSausage` : ChefZ_PreservedFood_Base (Addons/ChefZ_Preservation/config.cpp)
- `ChefZ_SpicySausage` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_SurvivorSpaghetti` : ChefZ_DishesAPlate (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_SurvivorSpaghetti` : ChefZ_ServedDish_Base (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_TomatoSauce` : ChefZ_SauceIngredient (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_TomatoSauce` : ChefZ_SauceItemBase (Addons/ChefZ_Cooking/config.cpp)
- `ChefZ_VenisonLeg` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
- `ChefZ_VenisonSausage` : ChefZ_MeatItemBase (Addons/ChefZ_Meat/config.cpp)
