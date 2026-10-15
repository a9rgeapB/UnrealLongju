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
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2AccessoryRulesTest,"Metin2.Items.AccessorySockets.Rules",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
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
	TestEqual(TEXT("Basic bracelet material"),RequiredMineral(14009,3),50623);
	TestEqual(TEXT("Necklace silver exception"),RequiredMineral(16219,5),50625);
	TestEqual(TEXT("Necklace gold exception"),RequiredMineral(16239,5),50626);
	TestEqual(TEXT("Late wrist mapping"),RequiredMineral(14509,3),50635);
	TestEqual(TEXT("Late ear mapping"),RequiredMineral(17569,6),50638);
	TestEqual(TEXT("Body armor not compatible"),RequiredMineral(11209,0),0);
	FMT2AccessorySocketState Invalid; Invalid.MaximumGrade=3; Invalid.CurrentGrade=999; Invalid.RemainingSeconds=100;
	TestEqual(TEXT("Malformed grade cannot generate filled sockets"),Normalize(Invalid).CurrentGrade,0);
	Invalid.CurrentGrade=2; Invalid.RemainingSeconds=-10;
	TestEqual(TEXT("Negative duration cannot grant a fresh lower-grade lifetime"),Advance(Invalid,0).CurrentGrade,0);
	auto* Belt=NewObject<UMT2ItemBeltTemplate>();
	auto* Filler=NewObject<UMT2ItemUseTemplate>(); Filler->UseSubType=29;
	auto* Ore=NewObject<UMT2ItemUseTemplate>(); Ore->UseSubType=21; Ore->Vnum=50623;
	auto* Wrist=NewObject<UMT2ItemArmorTemplate>(); Wrist->ArmorSubType=3; Wrist->Vnum=14000;
	TestTrue(TEXT("Belt filler accepts belts"),Compatible(Filler,Belt));
	TestFalse(TEXT("Belt filler rejects jewelry"),Compatible(Filler,Wrist));
	TestFalse(TEXT("Ordinary ore rejects belts"),Compatible(Ore,Belt));
	auto* Ring=NewObject<UMT2ItemRingTemplate>(); TestFalse(TEXT("Rings do not gain accessory sockets"),IsAccessory(Ring));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2AccessoryTooltipTest,"Metin2.Items.AccessorySockets.Tooltips",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2AccessoryTooltipTest::RunTest(const FString&)
{
	auto* Diamond=NewObject<UMT2ItemUseTemplate>(); Diamond->Vnum=50621; Diamond->UseSubType=20;
	auto* Ore=NewObject<UMT2ItemUseTemplate>(); Ore->Vnum=50623; Ore->UseSubType=21;
	auto* Amber=NewObject<UMT2ItemMaterialTemplate>(); Amber->Vnum=50622;
	TestTrue(TEXT("Diamond explains opening"),UMT2ItemTooltipWidget::DescribeAccessoryMaterial(Diamond).Contains(TEXT("50%")));
	TestFalse(TEXT("Diamond does not promise amplification"),UMT2ItemTooltipWidget::DescribeAccessoryMaterial(Diamond).Contains(TEXT("40%")));
	TestTrue(TEXT("Mineral explains grade scaling"),UMT2ItemTooltipWidget::DescribeAccessoryMaterial(Ore).Contains(TEXT("10% / 20% / 40%")));
	TestTrue(TEXT("Unsupported mineral promises no bonus"),UMT2ItemTooltipWidget::DescribeAccessoryMaterial(Amber).Contains(TEXT("no compatible")));
	auto* Bracelet=NewObject<UMT2ItemArmorTemplate>(); Bracelet->ArmorSubType=3;
	auto& Apply=Bracelet->Applies.AddDefaulted_GetRef(); Apply.Type=static_cast<EMT2ItemBonusType>(54); Apply.Value=100;
	TestTrue(TEXT("Third grade preview uses incremental twenty"),UMT2ItemTooltipWidget::AccessoryBonusLines(Bracelet,2,3).Contains(TEXT("20")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2AccessoryAuthorityTest,"Metin2.Items.AccessorySockets.Authority",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
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
		else if (TestNotNull(*FString::Printf(TEXT("Imported refined ore %d"),Vnum),Material)) TestEqual(TEXT("Imported socket use subtype"),Material->UseSubType,Vnum==50621?20:21);
	}
	TArray<FMT2ItemSlot> Slots,Equipment; Slots.SetNum(180); Equipment.SetNum(32);
	Slots[0].Vnum=50621; Slots[0].Count=2; Slots[1].Vnum=14000; Slots[1].Count=1;
	auto* Inv=Player->GetInventoryComponent(); Inv->RestoreItems(Slots,Equipment);
	TestEqual(TEXT("Diamond opens first socket"),Inv->ApplyAccessoryMaterialWithRoll(0,1,false,Slots[0],Slots[1],50),EMT2AccessorySocketApplyResult::Success);
	TestEqual(TEXT("Exactly one diamond consumed"),Inv->GetSlots()[0].Count,1);
	TestEqual(TEXT("Permanent socket opened"),Inv->GetSlots()[1].AccessorySockets.MaximumGrade,1);
	TestEqual(TEXT("Opening grants no bonus grade"),Inv->GetSlots()[1].AccessorySockets.CurrentGrade,0);
	TestEqual(TEXT("Replayed snapshot cannot consume again"),Inv->ApplyAccessoryMaterialWithRoll(0,1,false,Slots[0],Slots[1],1),EMT2AccessorySocketApplyResult::Invalid);
	for (int32 Grade=0; Grade<3; ++Grade)
	{
		Slots[0].Vnum=50623; Slots[0].Count=2;
		Slots[1].AccessorySockets.MaximumGrade=3; Slots[1].AccessorySockets.CurrentGrade=Grade;
		Slots[1].AccessorySockets.RemainingSeconds=Grade?100:0;
		Inv->RestoreItems(Slots,Equipment);
		TestEqual(TEXT("Fill boundary succeeds"),Inv->ApplyAccessoryMaterialWithRoll(0,1,false,Slots[0],Slots[1],90-Grade*10),EMT2AccessorySocketApplyResult::Success);
		TestEqual(TEXT("Fill increases grade once"),Inv->GetSlots()[1].AccessorySockets.CurrentGrade,Grade+1);
		Inv->RestoreItems(Slots,Equipment);
		TestEqual(TEXT("Fill boundary plus one fails"),Inv->ApplyAccessoryMaterialWithRoll(0,1,false,Slots[0],Slots[1],91-Grade*10),EMT2AccessorySocketApplyResult::Failed);
		TestEqual(TEXT("Failed insertion consumes one"),Inv->GetSlots()[0].Count,1);
		TestEqual(TEXT("Failure preserves existing grade"),Inv->GetSlots()[1].AccessorySockets.CurrentGrade,Grade);
	}
	Slots[0].Vnum=50621; Slots[1].AccessorySockets={}; Inv->RestoreItems(Slots,Equipment);
	TestEqual(TEXT("Opening roll 51 fails"),Inv->ApplyAccessoryMaterialWithRoll(0,1,false,Slots[0],Slots[1],51),EMT2AccessorySocketApplyResult::Failed);
	TestEqual(TEXT("Failed opening consumes one"),Inv->GetSlots()[0].Count,1);
	Slots[1].AccessorySockets.MaximumGrade=3; Inv->RestoreItems(Slots,Equipment);
	TestEqual(TEXT("Fully opened accessory rejects"),Inv->ApplyAccessoryMaterialWithRoll(0,1,false,Inv->GetSlots()[0],Inv->GetSlots()[1],1),EMT2AccessorySocketApplyResult::Full);
	TestEqual(TEXT("Full rejection consumes nothing"),Inv->GetSlots()[0].Count,2);
	Slots[1].Vnum=11200; Inv->RestoreItems(Slots,Equipment);
	TestEqual(TEXT("Body armor rejects accessory material"),Inv->ApplyAccessoryMaterialWithRoll(0,1,false,Slots[0],Slots[1],1),EMT2AccessorySocketApplyResult::Incompatible);
	Slots[1].Vnum=14000; Slots[1].AccessorySockets={}; Inv->RestoreItems(Slots,Equipment);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2AccessoryWearTest,"Metin2.Items.AccessorySockets.Wear",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
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
	Equipment[3].AccessorySockets.MaximumGrade=3; Equipment[3].AccessorySockets.CurrentGrade=3; Equipment[3].AccessorySockets.RemainingSeconds=10;
	Inv->RestoreItems(Slots,Equipment);
	const FString Saved=State->CapturePersistentStateJson_Implementation();
	TestTrue(TEXT("Accessory save has current grade"),Saved.Contains(TEXT("\"socket_0\":3")));
	TestTrue(TEXT("Accessory save has remaining wear"),Saved.Contains(TEXT("\"socket_2\":10")));
	TestTrue(TEXT("Accessory save restores"),State->ApplyPersistentStateJson_Implementation(Saved));
	TestEqual(TEXT("Accessory grade survives persistence"),Inv->GetEquipment()[3].AccessorySockets.CurrentGrade,3);
	Player->RecalculateEquipmentStats();
	TestEqual(TEXT("Low intrinsic attack speed gets grade minimum"),Player->GetItemApplyBonus(7),4);
	TestEqual(TEXT("Stamina receives only total forty percent"),Player->GetItemApplyBonus(58),14);
	Inv->AccessoryWearUpdatedAt=World->GetTimeSeconds()-4.25;
	TestEqual(TEXT("Const projection deducts partial equipped wear"),Inv->GetEffectiveAccessoryState(3).RemainingSeconds,6);
	TestTrue(TEXT("Unequip succeeds"),Inv->UnequipItem(3));
	TestEqual(TEXT("Unequip carries elapsed seconds"),Inv->GetSlots()[0].AccessorySockets.RemainingSeconds,6);
	Inv->AccessoryWearUpdatedAt=World->GetTimeSeconds()-30;
	TestEqual(TEXT("Inventory pauses wear"),Inv->GetSlots()[0].AccessorySockets.RemainingSeconds,6);
	TestFalse(TEXT("No timer when unworn"),World->GetTimerManager().IsTimerActive(Inv->AccessoryWearTimer));
	FMT2ItemSlot Dropped;
	TestTrue(TEXT("Accessory can be dropped"),Inv->ExtractItemForDrop(0,1,Dropped));
	TestEqual(TEXT("Ground item retains paused duration"),Dropped.AccessorySockets.RemainingSeconds,6);
	TestTrue(TEXT("Ground item retains fractional wear"),FMath::IsNearlyEqual(Dropped.AccessorySockets.WearFraction,0.25));
	TestEqual(TEXT("Ground pickup restores full instance"),Inv->AddItemSlotPartial(Dropped),1);
	auto* OtherPlayer=World->SpawnActor<AMT2PlayerCharacter>(); auto* OtherInv=OtherPlayer->GetInventoryComponent(); OtherInv->RestoreItems({},{});
	TArray<int32> Offered={0}; TArray<FMT2ItemSlot> Expected={Inv->GetSlots()[0]};
	TestTrue(TEXT("Accessory trade succeeds"),Inv->ExecuteAtomicTrade(*OtherInv,Offered,Expected,{},{}));
	TestEqual(TEXT("Trade preserves paused grade"),OtherInv->GetSlots()[0].AccessorySockets.CurrentGrade,3);
	TestEqual(TEXT("Trade preserves paused duration"),OtherInv->GetSlots()[0].AccessorySockets.RemainingSeconds,6);
	TestTrue(TEXT("Trade preserves fractional wear"),FMath::IsNearlyEqual(OtherInv->GetSlots()[0].AccessorySockets.WearFraction,0.25));
	Inv->RestoreItems(Slots,Equipment); Inv->AccessoryWearUpdatedAt=World->GetTimeSeconds()-0.6; Inv->SettleAccessoryWear();
	Inv->AccessoryWearUpdatedAt=World->GetTimeSeconds()-0.6; Inv->SettleAccessoryWear();
	TestEqual(TEXT("Repeated subsecond settlements cannot grant free wear"),Inv->GetEquipment()[3].AccessorySockets.RemainingSeconds,9);
	Inv->AccessoryWearUpdatedAt=World->GetTimeSeconds()-0.6;
	TestTrue(TEXT("Death drop succeeds"),Inv->ExtractRandomEquippedItem(Dropped));
	TestEqual(TEXT("Death drop settles remaining wear"),Dropped.AccessorySockets.RemainingSeconds,9);
	TestFalse(TEXT("Death drop stops timer"),World->GetTimerManager().IsTimerActive(Inv->AccessoryWearTimer));
	FMT2AccessorySocketState Late; Late.MaximumGrade=3; Late.CurrentGrade=3; Late.RemainingSeconds=10;
	Late=MT2AccessorySockets::Advance(Late,43215);
	TestEqual(TEXT("Late callback crosses two grades"),Late.CurrentGrade,1);
	TestEqual(TEXT("Residual elapsed applied to lower grade"),Late.RemainingSeconds,86395);
	Equipment[3].AccessorySockets.CurrentGrade=1; Equipment[3].AccessorySockets.RemainingSeconds=2;
	Inv->RestoreItems(Slots,Equipment); Inv->AccessoryWearUpdatedAt=World->GetTimeSeconds()-3;
	Inv->ProcessAccessoryWear(); Player->RecalculateEquipmentStats();
	TestEqual(TEXT("Expiry removes only filled grade"),Inv->GetEquipment()[3].AccessorySockets.CurrentGrade,0);
	TestEqual(TEXT("Expiry keeps open sockets"),Inv->GetEquipment()[3].AccessorySockets.MaximumGrade,3);
	TestEqual(TEXT("Expiry removes intrinsic amplification"),Player->GetItemApplyBonus(7),1);
	TestFalse(TEXT("Expired accessory has no active wear timer"),World->GetTimerManager().IsTimerActive(Inv->AccessoryWearTimer));
	return true;
}
#endif
