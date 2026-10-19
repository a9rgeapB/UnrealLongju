#include "Items/Templates/MT2ItemArmorTemplate.h"
#include "Items/Templates/MT2ItemBeltTemplate.h"
#include "Items/Templates/MT2ItemMaterialTemplate.h"
#include "Items/Templates/MT2ItemMetinStoneTemplate.h"
#include "Items/Templates/MT2ItemRingTemplate.h"
#include "Items/Templates/MT2ItemUseTemplate.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Items/MT2AccessorySocketRules.h"
#include "Items/MT2ItemTemplate.h"
#include "Items/MT2InventoryComponent.h"
#include "Core/MT2VnumRegistrySubsystem.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Player/MT2PlayerState.h"
#include "Components/MT2HealthComponent.h"
#include "Abilities/MT2CoreAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Player/MT2PlayerController.h"
#include "UI/MT2ItemTooltipWidget.h"
#include "UI/MT2InventorySlotWidget.h"
#include "UI/MT2AtlasImage.h"
#include "Config/MT2PathSettings.h"
#include "Engine/Texture2D.h"
#include "Items/MT2ItemUtils.h"
#include "Items/MT2ItemBonusSettings.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2AccessoryRulesTest,"Metin2.Items.GetAccessorySockets().Rules",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2AccessoryRulesTest::RunTest(const FString&)
{
	using namespace MT2AccessorySockets;
	TestEqual(TEXT("First grade amplifies 100 by ten"),Bonus(100,1),10);
	TestEqual(TEXT("Second grade total twenty"),Bonus(100,2),20);
	TestEqual(TEXT("Third grade total forty, not seventy"),Bonus(100,3),40);
	TestEqual(TEXT("Third insertion adds twenty"),Bonus(100,3)-Bonus(100,2),20);
	for (int32 Grade=1; Grade<=3; ++Grade) TestEqual(TEXT("Low applies retain minimum"),Bonus(5,Grade),Grade);
	TestEqual(TEXT("Truncates fractions"),Bonus(19,1),1);
	TestEqual(TEXT("Grade one wear time"),Duration(1),86400);
	TestEqual(TEXT("Grade two wear time"),Duration(2),43200);
	TestEqual(TEXT("Grade three wear time"),Duration(3),21600);
	TestEqual(TEXT("Initial fill chance"),InsertionChance(0),90);
	TestEqual(TEXT("Second fill chance"),InsertionChance(1),80);
	TestEqual(TEXT("Third fill chance"),InsertionChance(2),70);
	TestEqual(TEXT("Basic bracelet material"),RequiredMineral(14009,EMT2ArmorSubType::Wrist),50623);
	TestEqual(TEXT("Necklace silver exception"),RequiredMineral(16219,EMT2ArmorSubType::Neck),50625);
	TestEqual(TEXT("Necklace gold exception"),RequiredMineral(16239,EMT2ArmorSubType::Neck),50626);
	TestEqual(TEXT("Late wrist mapping"),RequiredMineral(14509,EMT2ArmorSubType::Wrist),50635);
	TestEqual(TEXT("Late ear mapping"),RequiredMineral(17569,EMT2ArmorSubType::Ear),50638);
	TestEqual(TEXT("Body armor not compatible"),RequiredMineral(11209,EMT2ArmorSubType::Body),0);
	FMT2AccessorySocketState Invalid; Invalid.MaximumGrade=3; Invalid.CurrentGrade=999; Invalid.RemainingSeconds=100;
	TestEqual(TEXT("Malformed grade cannot generate filled sockets"),Normalize(Invalid).CurrentGrade,0);
	Invalid.CurrentGrade=2; Invalid.RemainingSeconds=-10;
	TestEqual(TEXT("Negative duration cannot grant a fresh lower-grade lifetime"),Advance(Invalid,0).CurrentGrade,0);
	auto* Belt=NewObject<UMT2ItemBeltTemplate>();
	auto* Filler=NewObject<UMT2ItemUseTemplate>(); Filler->UseSubType=EMT2UseSubType::PutIntoBeltSocket;
	auto* Ore=NewObject<UMT2ItemUseTemplate>(); Ore->UseSubType=EMT2UseSubType::PutIntoAccessorySocket; Ore->Vnum=50623;
	auto* Wrist=NewObject<UMT2ItemArmorTemplate>(); Wrist->ArmorSubType=EMT2ArmorSubType::Wrist; Wrist->Vnum=14000;
	TestTrue(TEXT("Belt filler accepts belts"),Compatible(Filler,Belt));
	TestFalse(TEXT("Belt filler rejects jewelry"),Compatible(Filler,Wrist));
	TestFalse(TEXT("Ordinary ore rejects belts"),Compatible(Ore,Belt));
	auto* Ring=NewObject<UMT2ItemRingTemplate>(); TestFalse(TEXT("Rings do not gain accessory sockets"),IsAccessory(Ring));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2AccessoryTooltipTest,"Metin2.Items.GetAccessorySockets().Tooltips",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2AccessoryTooltipTest::RunTest(const FString&)
{
	auto* Diamond=NewObject<UMT2ItemUseTemplate>(); Diamond->Vnum=50621; Diamond->UseSubType=EMT2UseSubType::AddAccessorySocket;
	auto* Ore=NewObject<UMT2ItemUseTemplate>(); Ore->Vnum=50623; Ore->UseSubType=EMT2UseSubType::PutIntoAccessorySocket;
	auto* Amber=NewObject<UMT2ItemMaterialTemplate>(); Amber->Vnum=50622;
	TestTrue(TEXT("Diamond explains opening"),UMT2ItemTooltipWidget::DescribeAccessoryMaterial(Diamond).Contains(TEXT("50%")));
	TestFalse(TEXT("Diamond does not promise amplification"),UMT2ItemTooltipWidget::DescribeAccessoryMaterial(Diamond).Contains(TEXT("40%")));
	const FString OreDescription=UMT2ItemTooltipWidget::DescribeAccessoryMaterial(Ore);
	TestTrue(TEXT("Mineral description stays compact"),OreDescription.Len()<=140);
	TestFalse(TEXT("Mineral description has no developer VNUM table"),OreDescription.Contains(TEXT("VNUM")));
	TestTrue(TEXT("Mineral description explains socket use"),OreDescription.Contains(TEXT("socket")));
	TestTrue(TEXT("Unsupported mineral promises no bonus"),UMT2ItemTooltipWidget::DescribeAccessoryMaterial(Amber).Contains(TEXT("no compatible")));
	auto* Bracelet=NewObject<UMT2ItemArmorTemplate>(); Bracelet->ArmorSubType=EMT2ArmorSubType::Wrist;
	TestEqual(TEXT("Intrinsic stamina has a readable flat bonus label"),GetDefault<UMT2ItemBonusSettings>()->FormatBonus(58,10),FString(TEXT("Maximum Stamina +10")));
	TestEqual(TEXT("Stamina penalties keep their sign"),GetDefault<UMT2ItemBonusSettings>()->FormatBonus(58,-10),FString(TEXT("Maximum Stamina -10")));
	auto& Stamina=Bracelet->Applies.AddDefaulted_GetRef(); Stamina.Type=EMT2ItemBonusType::MaxStamina; Stamina.Value=10;
	TestTrue(TEXT("Filled accessory mineral names its stamina bonus"),UMT2ItemTooltipWidget::AccessoryBonusLines(Bracelet,0,1).Contains(TEXT("Maximum Stamina +1")));
	auto& Apply=Bracelet->Applies.AddDefaulted_GetRef(); Apply.Type=static_cast<EMT2ItemBonusType>(54); Apply.Value=100;
	TestTrue(TEXT("Third grade preview uses incremental twenty"),UMT2ItemTooltipWidget::AccessoryBonusLines(Bracelet,2,3).Contains(TEXT("20")));
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GEngine)); GI->InitializeStandalone();
	UWorld* World=GI->GetWorld();
	TStrongObjectPtr<UGameViewportClient> Viewport(NewObject<UGameViewportClient>(GEngine));
	auto& Context=GEngine->GetWorldContextFromWorldChecked(World); Context.GameViewport=Viewport.Get(); Viewport->Init(Context,GI.Get(),false);
	ON_SCOPE_EXIT { GI->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	FString Error; auto* LocalPlayer=GI->CreateLocalPlayer(0,Error,false);
	auto* Controller=World->SpawnActor<AMT2PlayerController>(); Controller->SetPlayer(LocalPlayer);
	// This intentionally unstarted world has not run the controller registration hook.
	World->AddController(Controller);
	auto* State=World->SpawnActor<AMT2PlayerState>(); Controller->SetPlayerState(State);
	State->GetAbilitySystemComponent()->AddAttributeSetSubobject(State->GetCoreAttributes());
	auto* Player=World->SpawnActor<AMT2PlayerCharacter>(); Controller->Possess(Player);
	TArray<FMT2ItemSlot> Local; Local.SetNum(180); Local[0].Vnum=14000; Local[0].Count=1;
	Player->GetInventoryComponent()->RestoreItems(Local,{});
	auto* PartnerCell=CreateWidget<UMT2InventorySlotWidget>(Controller,UMT2InventorySlotWidget::StaticClass());
	if (!TestNotNull(TEXT("Actual shared trade slot widget"),PartnerCell)) { return false; }
	PartnerCell->InitializeSlot(0,EMT2InventorySlotKind::Item,FText::GetEmpty()); PartnerCell->SetItemByVnum(50623,1);
	const auto TooltipText=[](UWidget* Widget)
	{
		FString Text;
		TFunction<void(UWidget*)> Visit=[&](UWidget* Child)
		{
			if (auto* Line=Cast<UTextBlock>(Child)) { Text+=Line->GetText().ToString()+TEXT("\n"); }
			if (auto* Panel=Cast<UPanelWidget>(Child)) { for (auto* Item : Panel->GetAllChildren()) { Visit(Item); } }
		};
		if (auto* Tooltip=Cast<UUserWidget>(Widget)) { Visit(Tooltip->GetRootWidget()); }
		return Text;
	};
	const FString OfferedText=TooltipText(PartnerCell->GetToolTip());
	AddInfo(FString::Printf(TEXT("Trade fixture owner=%s pawn=%s tooltip=%s root=%s"),*GetNameSafe(PartnerCell->GetOwningPlayer()),*GetNameSafe(PartnerCell->GetOwningPlayerPawn()),*GetNameSafe(PartnerCell->GetToolTip()),*GetNameSafe(Cast<UUserWidget>(PartnerCell->GetToolTip()) ? Cast<UUserWidget>(PartnerCell->GetToolTip())->GetRootWidget() : nullptr)));
	TestFalse(TEXT("Trade tooltip has actual visible text"),OfferedText.IsEmpty());
	PartnerCell->NativeOnMouseEnter(FGeometry(),FPointerEvent());
	TestEqual(TEXT("Partner hover preserves offered item despite occupied own slot zero"),TooltipText(PartnerCell->GetToolTip()),OfferedText);
	FMT2ItemSlot OfferedAccessory; OfferedAccessory.Vnum=14000; OfferedAccessory.Count=1;
	OfferedAccessory.EditAccessorySockets().MaximumGrade=3; OfferedAccessory.EditAccessorySockets().CurrentGrade=2; OfferedAccessory.EditAccessorySockets().RemainingSeconds=100;
	PartnerCell->SetItemByVnum(14000,1); PartnerCell->SetInstanceTooltipData(OfferedAccessory);
	const auto Images=[](UWidget* Widget)
	{
		TArray<UImage*> Result;
		TFunction<void(UWidget*)> Visit=[&](UWidget* Child)
		{
			if (auto* Image=Cast<UImage>(Child)) { Result.Add(Image); }
			if (auto* Panel=Cast<UPanelWidget>(Child)) { for (auto* Item : Panel->GetAllChildren()) { Visit(Item); } }
		};
		if (auto* Tooltip=Cast<UUserWidget>(Widget)) { Visit(Tooltip->GetRootWidget()); }
		return Result;
	};
	const auto Frames=[&](UWidget* Widget)
	{
		TArray<UMT2AtlasImage*> Result;
		for (auto* Image : Images(Widget)) { if (auto* Frame=Cast<UMT2AtlasImage>(Image)) { Result.Add(Frame); } }
		return Result;
	};
	TestEqual(TEXT("Accessory displays all three opened socket frames"),Frames(PartnerCell->GetToolTip()).Num(),3);
	const auto* Mineral=MT2ItemUtils::ResolveTemplate(Player,50623);
	auto* MineralIcon=Mineral ? Mineral->Icon.LoadSynchronous() : nullptr;
	TestNotNull(TEXT("Imported mineral icon available"),MineralIcon);
	int32 FilledIcons=0;
	for (auto* Image : Images(PartnerCell->GetToolTip())) { if (Image->GetBrush().GetResourceObject()==MineralIcon) { ++FilledIcons; } }
	TestEqual(TEXT("Two filled accessory sockets display mineral icons"),FilledIcons,2);
	for (auto* Frame : Frames(PartnerCell->GetToolTip()))
	{
		TestNotNull(TEXT("Socket frame uses an imported atlas"),Frame->AtlasTexture.Get());
		TestEqual(TEXT("Accessory uses legacy silver socket art"),Frame->AtlasRegion.X,278.f);
	}
	TestTrue(TEXT("Opened unfilled socket is visible"),TooltipText(PartnerCell->GetToolTip()).Contains(TEXT("Empty socket")));
	TestTrue(TEXT("Active mineral row displays remaining wear"),TooltipText(PartnerCell->GetToolTip()).Contains(TEXT("1m 40s")));
	PartnerCell->NativeOnMouseEnter(FGeometry(),FPointerEvent());
	TestEqual(TEXT("Trade hover preserves accessory socket visuals"),Frames(PartnerCell->GetToolTip()).Num(),3);
	OfferedAccessory.EditAccessorySockets().CurrentGrade=0; OfferedAccessory.EditAccessorySockets().RemainingSeconds=0;
	PartnerCell->SetInstanceTooltipData(OfferedAccessory);
	TestEqual(TEXT("Opened but entirely empty accessory still shows frames"),Frames(PartnerCell->GetToolTip()).Num(),3);
	OfferedAccessory.EditAccessorySockets().MaximumGrade=0; PartnerCell->SetInstanceTooltipData(OfferedAccessory);
	TestEqual(TEXT("Unopened accessory has no socket frames"),Frames(PartnerCell->GetToolTip()).Num(),0);
	FMT2ItemSlot Weapon; Weapon.Vnum=19; Weapon.Count=1;
	const auto* Stone=MT2ItemUtils::ResolveTemplate<UMT2ItemMetinStoneTemplate>(Player,28030);
	if (!TestNotNull(TEXT("Ordinary Metin stone fixture"),Stone)) { return false; }
	auto& Socket=Weapon.EditMetinSockets().AddDefaulted_GetRef(); Socket.Type=EMT2MetinSocketType::Gold;
	Socket.Stone=Stone->GetClass(); PartnerCell->SetItemByVnum(19,1); PartnerCell->SetInstanceTooltipData(Weapon);
	const auto GoldFrames=Frames(PartnerCell->GetToolTip());
	if (TestEqual(TEXT("Weapon uses the same socket-row renderer"),GoldFrames.Num(),1))
	{ TestEqual(TEXT("Gold Metin socket retains its legacy frame"),GoldFrames[0]->AtlasRegion.X,242.f); }
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2ItemBonusNamesTest,"Metin2.Items.GetBonuses().DisplayNames",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2ItemBonusNamesTest::RunTest(const FString&)
{
	auto* Defaults=NewObject<UMT2ItemBonusSettings>();
	Defaults->NormalBonuses.Reset(); Defaults->RareBonuses.Reset();
	const UMT2ItemBonusSettings* SettingsVariants[]={Defaults,GetDefault<UMT2ItemBonusSettings>()};
	const UEnum* Types=StaticEnum<EMT2ItemBonusType>();
	for (int32 Index=1; Index<Types->NumEnums()-1; ++Index)
	{
		const int32 Type=static_cast<int32>(Types->GetValueByIndex(Index));
		for (const auto* Settings : SettingsVariants)
		{
			const FString Text=Settings->FormatBonus(Type,10);
			TestFalse(*FString::Printf(TEXT("Apply %d has a readable name without random-pool definitions"),Type),Text.IsEmpty() || Text.StartsWith(FString::Printf(TEXT("Bonus %d "),Type)));
		}
	}
	const struct { int32 Type; const TCHAR* Expected; } Cases[] = {
		{26,TEXT("Recover SP on Damage +10%")},{40,TEXT("Reflect Curse +10%")},
		{42,TEXT("Recover SP on Kill +10%")},{46,TEXT("Potion Effect Bonus +10%")},
		{47,TEXT("Recover HP on Kill +10%")},{50,TEXT("Immune to Knockdown")},
		{51,TEXT("Skill +10")},{52,TEXT("Bow Distance +10m")},
		{54,TEXT("Defense Bonus +10")},{55,TEXT("Magic Attack Value +10")},
		{56,TEXT("Magic Defense Value +10")},{57,TEXT("Curse Chance +10%")},
		{58,TEXT("Maximum Stamina +10")},{64,TEXT("Mall Attack Bonus +10%")},
		{65,TEXT("Mall Defense Bonus +10%")},{66,TEXT("Mall Experience Bonus +10%")},
		{67,TEXT("Mall Item Drop Multiplier x1.0")},{68,TEXT("Mall Yang Multiplier x1.0")},
		{69,TEXT("Maximum HP +10%")},{70,TEXT("Maximum SP +10%")},
		{73,TEXT("Skill Damage Resistance +10%")},{74,TEXT("Average Damage Resistance +10%")},
		{75,TEXT("PC Bang Experience Bonus +10%")},{76,TEXT("PC Bang Drop Bonus +10%")},
		{77,TEXT("Extract HP +10%")},{82,TEXT("Energy +10")},
		{83,TEXT("Defense Value +10")},{84,TEXT("Costume Attribute Bonus +10%")},
		{85,TEXT("Magic Attack +10%")},{86,TEXT("Melee/Magic Attack +10%")},
		{90,TEXT("Critical Resistance +10%")},{91,TEXT("Penetrating Resistance +10%")}
	};
	for (const auto& Case : Cases)
	{
		TestEqual(*FString::Printf(TEXT("Legacy apply %d has its proper name and units"),Case.Type),Defaults->FormatBonus(Case.Type,10),FString(Case.Expected));
	}
	TestEqual(TEXT("Unknown IDs remain diagnosable"),Defaults->FormatBonus(999,10),FString(TEXT("Bonus 999 +10")));
	auto& Override=Defaults->NormalBonuses.AddDefaulted_GetRef(); Override.ApplyType=EMT2ItemBonusType::MaxStamina; Override.DisplayName=TEXT("Custom stamina");
	TestEqual(TEXT("Project display overrides remain supported"),Defaults->FormatBonus(58,10),FString(TEXT("Custom stamina +10")));
	Override.DisplayName.Empty();
	TestEqual(TEXT("Empty configured label falls back to the catalog"),Defaults->FormatBonus(58,10),FString(TEXT("Maximum Stamina +10")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2AccessoryAuthorityTest,"Metin2.Items.GetAccessorySockets().Authority",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2AccessoryAuthorityTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GEngine)); GI->InitializeStandalone();
	UWorld* World=GI->GetWorld();
	ON_SCOPE_EXIT { GI->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	auto* Player=World->SpawnActor<AMT2PlayerCharacter>(); auto* State=World->SpawnActor<AMT2PlayerState>(); Player->SetPlayerState(State);
	auto* ASC=State->GetAbilitySystemComponent(); ASC->AddAttributeSetSubobject(State->GetCoreAttributes()); ASC->InitAbilityActorInfo(State,Player);
	ASC->SetNumericAttributeBase(UMT2CoreAttributeSet::GetMaxHealthAttribute(),100); ASC->SetNumericAttributeBase(UMT2CoreAttributeSet::GetHealthAttribute(),100);
	Player->GetHealthComponent()->InitializeWithAbilitySystem(ASC);
	auto* Registry=GI->GetSubsystem<UMT2VnumRegistrySubsystem>();
	for (int32 Vnum=50621; Vnum<=50638; ++Vnum)
	{
		const auto* Material=Cast<UMT2ItemUseTemplate>(Registry->ResolveItemTemplateClass(Vnum).GetDefaultObject());
		if (Vnum == 50622)
		{
			TestNotNull(TEXT("Unsupported catalog mineral still imported"),Registry->ResolveItemTemplateClass(Vnum).GetDefaultObject());
			TestFalse(TEXT("Unsupported mineral cannot fill jewelry"),MT2AccessorySockets::IsMaterial(Registry->ResolveItemTemplateClass(Vnum).GetDefaultObject()));
		}
		else if (TestNotNull(*FString::Printf(TEXT("Imported refined ore %d"),Vnum),Material)) TestEqual(TEXT("Imported socket use subtype"),Material->UseSubType,Vnum==50621?EMT2UseSubType::AddAccessorySocket:EMT2UseSubType::PutIntoAccessorySocket);
	}
	TArray<FMT2ItemSlot> Slots,Equipment; Slots.SetNum(180); Equipment.SetNum(32);
	Slots[0].Vnum=50621; Slots[0].Count=2; Slots[1].Vnum=14000; Slots[1].Count=1;
	auto* Inv=Player->GetInventoryComponent(); Inv->RestoreItems(Slots,Equipment);
	TestEqual(TEXT("Diamond opens first socket"),Inv->ApplyAccessoryMaterialWithRoll(0,1,false,Slots[0],Slots[1],50),EMT2AccessorySocketApplyResult::Success);
	TestEqual(TEXT("Exactly one diamond consumed"),Inv->GetSlots()[0].Count,1);
	TestEqual(TEXT("Permanent socket opened"),Inv->GetSlots()[1].GetAccessorySockets().MaximumGrade,1);
	TestEqual(TEXT("Opening grants no bonus grade"),Inv->GetSlots()[1].GetAccessorySockets().CurrentGrade,0);
	TestEqual(TEXT("Replayed snapshot cannot consume again"),Inv->ApplyAccessoryMaterialWithRoll(0,1,false,Slots[0],Slots[1],1),EMT2AccessorySocketApplyResult::Invalid);
	for (int32 Grade=0; Grade<3; ++Grade)
	{
		Slots[0].Vnum=50623; Slots[0].Count=2;
		Slots[1].EditAccessorySockets().MaximumGrade=3; Slots[1].EditAccessorySockets().CurrentGrade=Grade;
		Slots[1].EditAccessorySockets().RemainingSeconds=Grade?100:0;
		Inv->RestoreItems(Slots,Equipment);
		TestEqual(TEXT("Fill boundary succeeds"),Inv->ApplyAccessoryMaterialWithRoll(0,1,false,Slots[0],Slots[1],90-Grade*10),EMT2AccessorySocketApplyResult::Success);
		TestEqual(TEXT("Fill increases grade once"),Inv->GetSlots()[1].GetAccessorySockets().CurrentGrade,Grade+1);
		Inv->RestoreItems(Slots,Equipment);
		TestEqual(TEXT("Fill boundary plus one fails"),Inv->ApplyAccessoryMaterialWithRoll(0,1,false,Slots[0],Slots[1],91-Grade*10),EMT2AccessorySocketApplyResult::Failed);
		TestEqual(TEXT("Failed insertion consumes one"),Inv->GetSlots()[0].Count,1);
		TestEqual(TEXT("Failure preserves existing grade"),Inv->GetSlots()[1].GetAccessorySockets().CurrentGrade,Grade);
	}
	Slots[0].Vnum=50621; Slots[1].EditAccessorySockets()={}; Inv->RestoreItems(Slots,Equipment);
	TestEqual(TEXT("Opening roll 51 fails"),Inv->ApplyAccessoryMaterialWithRoll(0,1,false,Slots[0],Slots[1],51),EMT2AccessorySocketApplyResult::Failed);
	TestEqual(TEXT("Failed opening consumes one"),Inv->GetSlots()[0].Count,1);
	Slots[1].EditAccessorySockets().MaximumGrade=3; Inv->RestoreItems(Slots,Equipment);
	TestEqual(TEXT("Fully opened accessory rejects"),Inv->ApplyAccessoryMaterialWithRoll(0,1,false,Inv->GetSlots()[0],Inv->GetSlots()[1],1),EMT2AccessorySocketApplyResult::Full);
	TestEqual(TEXT("Full rejection consumes nothing"),Inv->GetSlots()[0].Count,2);
	Slots[1].Vnum=11200; Inv->RestoreItems(Slots,Equipment);
	TestEqual(TEXT("Body armor rejects accessory material"),Inv->ApplyAccessoryMaterialWithRoll(0,1,false,Slots[0],Slots[1],1),EMT2AccessorySocketApplyResult::Incompatible);
	Slots[1].Vnum=14000; Slots[1].EditAccessorySockets()={}; Inv->RestoreItems(Slots,Equipment);
	Player->ServerApplyAccessoryMaterial_Implementation(1,0,1,false,Slots[0],Slots[1]);
	TestEqual(TEXT("First confirmation consumes once regardless of roll"),Inv->GetSlots()[0].Count,1);
	Player->ServerApplyAccessoryMaterial_Implementation(1,0,1,false,Inv->GetSlots()[0],Inv->GetSlots()[1]);
	TestEqual(TEXT("Repeated confirmation with fresh snapshots still rejected"),Inv->GetSlots()[0].Count,1);
	Inv->RestoreItems(Slots,Equipment);
	TestEqual(TEXT("Aliased source and target snapshots remain safe on consumption"),Inv->ApplyAccessoryMaterialWithRoll(0,1,false,Inv->GetSlots()[0],Inv->GetSlots()[1],1),EMT2AccessorySocketApplyResult::Success);
	Player->GetHealthComponent()->SetHealth(0);
	TestEqual(TEXT("Dead player cannot apply materials"),Inv->ApplyAccessoryMaterialWithRoll(0,1,false,Inv->GetSlots()[0],Inv->GetSlots()[1],1),EMT2AccessorySocketApplyResult::Invalid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2AccessoryWearTest,"Metin2.Items.GetAccessorySockets().Wear",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2AccessoryWearTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GEngine)); GI->InitializeStandalone();
	UWorld* World=GI->GetWorld();
	ON_SCOPE_EXIT { GI->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	auto* Player=World->SpawnActor<AMT2PlayerCharacter>();
	auto* State=World->SpawnActor<AMT2PlayerState>(); Player->SetPlayerState(State);
	auto* ASC=State->GetAbilitySystemComponent(); ASC->AddAttributeSetSubobject(State->GetCoreAttributes()); ASC->InitAbilityActorInfo(State,Player);
	ASC->SetNumericAttributeBase(UMT2CoreAttributeSet::GetMaxHealthAttribute(),100); ASC->SetNumericAttributeBase(UMT2CoreAttributeSet::GetHealthAttribute(),100);
	Player->GetHealthComponent()->InitializeWithAbilitySystem(ASC);
	auto* Controller=World->SpawnActor<AMT2PlayerController>(); Controller->PlayerState=State; Controller->Possess(Player);
	auto* Inv=Player->GetInventoryComponent();
	TArray<FMT2ItemSlot> Slots,Equipment; Slots.SetNum(180); Equipment.SetNum(32);
	Equipment[3].Vnum=14000; Equipment[3].Count=1;
	Equipment[3].EditAccessorySockets().MaximumGrade=3; Equipment[3].EditAccessorySockets().CurrentGrade=3; Equipment[3].EditAccessorySockets().RemainingSeconds=10;
	Inv->RestoreItems(Slots,Equipment);
	const FString Saved=State->CapturePersistentStateJson_Implementation();
	TestTrue(TEXT("Accessory save has current grade"),Saved.Contains(TEXT("\"socket_0\":3")));
	TestTrue(TEXT("Accessory save has remaining wear"),Saved.Contains(TEXT("\"socket_2\":10")));
	TestTrue(TEXT("Accessory save restores"),State->ApplyPersistentStateJson_Implementation(Saved));
	TestEqual(TEXT("Accessory grade survives persistence"),Inv->GetEquipment()[3].GetAccessorySockets().CurrentGrade,3);
	Player->RecalculateEquipmentStats();
	TestEqual(TEXT("Low intrinsic attack speed gets grade minimum"),Player->GetItemApplyBonus(EMT2ItemBonusType::AttackSpeed),4);
	TestEqual(TEXT("Stamina receives only total forty percent"),Player->GetItemApplyBonus(EMT2ItemBonusType::MaxStamina),14);
	Inv->AccessoryWearUpdatedAt=World->GetTimeSeconds()-4.25;
	TestEqual(TEXT("Const projection deducts partial equipped wear"),Inv->GetEffectiveAccessoryState(3).RemainingSeconds,6);
	TestTrue(TEXT("Unequip succeeds"),Inv->UnequipItem(3));
	TestEqual(TEXT("Unequip carries elapsed seconds"),Inv->GetSlots()[0].GetAccessorySockets().RemainingSeconds,6);
	Inv->AccessoryWearUpdatedAt=World->GetTimeSeconds()-30;
	TestEqual(TEXT("Inventory pauses wear"),Inv->GetSlots()[0].GetAccessorySockets().RemainingSeconds,6);
	TestFalse(TEXT("No timer when unworn"),World->GetTimerManager().IsTimerActive(Inv->AccessoryWearTimer));
	FMT2ItemSlot Dropped;
	TestTrue(TEXT("Accessory can be dropped"),Inv->ExtractItemForDrop(0,1,Dropped));
	TestEqual(TEXT("Ground item retains paused duration"),Dropped.GetAccessorySockets().RemainingSeconds,6);
	TestTrue(TEXT("Ground item retains fractional wear"),FMath::IsNearlyEqual(Dropped.GetAccessorySockets().WearFraction,0.25));
	TestEqual(TEXT("Ground pickup restores full instance"),Inv->AddItemSlotPartial(Dropped),1);
	auto* OtherPlayer=World->SpawnActor<AMT2PlayerCharacter>(); auto* OtherInv=OtherPlayer->GetInventoryComponent(); OtherInv->RestoreItems({},{});
	TArray<int32> Offered={0}; TArray<FMT2ItemSlot> Expected={Inv->GetSlots()[0]};
	TestTrue(TEXT("Accessory trade succeeds"),Inv->ExecuteAtomicTrade(*OtherInv,Offered,Expected,{},{}));
	TestEqual(TEXT("Trade preserves paused grade"),OtherInv->GetSlots()[0].GetAccessorySockets().CurrentGrade,3);
	TestEqual(TEXT("Trade preserves paused duration"),OtherInv->GetSlots()[0].GetAccessorySockets().RemainingSeconds,6);
	TestTrue(TEXT("Trade preserves fractional wear"),FMath::IsNearlyEqual(OtherInv->GetSlots()[0].GetAccessorySockets().WearFraction,0.25));
	Inv->RestoreItems(Slots,Equipment); Inv->AccessoryWearUpdatedAt=World->GetTimeSeconds()-0.6; Inv->SettleAccessoryWear();
	Inv->AccessoryWearUpdatedAt=World->GetTimeSeconds()-0.6; Inv->SettleAccessoryWear();
	TestEqual(TEXT("Repeated subsecond settlements cannot grant free wear"),Inv->GetEquipment()[3].GetAccessorySockets().RemainingSeconds,9);
	Inv->AccessoryWearUpdatedAt=World->GetTimeSeconds()-0.6;
	TestTrue(TEXT("Death drop succeeds"),Inv->ExtractRandomEquippedItem(Dropped));
	TestEqual(TEXT("Death drop settles remaining wear"),Dropped.GetAccessorySockets().RemainingSeconds,9);
	TestFalse(TEXT("Death drop stops timer"),World->GetTimerManager().IsTimerActive(Inv->AccessoryWearTimer));
	FMT2AccessorySocketState Late; Late.MaximumGrade=3; Late.CurrentGrade=3; Late.RemainingSeconds=10;
	Late=MT2AccessorySockets::Advance(Late,43215);
	TestEqual(TEXT("Late callback crosses two grades"),Late.CurrentGrade,1);
	TestEqual(TEXT("Residual elapsed applied to lower grade"),Late.RemainingSeconds,86395);
	Equipment[3].EditAccessorySockets().CurrentGrade=1; Equipment[3].EditAccessorySockets().RemainingSeconds=2;
	Inv->RestoreItems(Slots,Equipment); Inv->AccessoryWearUpdatedAt=World->GetTimeSeconds()-3;
	Inv->ProcessAccessoryWear(); Player->RecalculateEquipmentStats();
	TestEqual(TEXT("Expiry removes only filled grade"),Inv->GetEquipment()[3].GetAccessorySockets().CurrentGrade,0);
	TestEqual(TEXT("Expiry keeps open sockets"),Inv->GetEquipment()[3].GetAccessorySockets().MaximumGrade,3);
	TestEqual(TEXT("Expiry removes intrinsic amplification"),Player->GetItemApplyBonus(EMT2ItemBonusType::AttackSpeed),1);
	TestFalse(TEXT("Expired accessory has no active wear timer"),World->GetTimerManager().IsTimerActive(Inv->AccessoryWearTimer));
	return true;
}
#endif
