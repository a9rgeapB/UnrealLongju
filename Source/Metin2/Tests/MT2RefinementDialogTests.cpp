#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/PanelWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UObject/StrongObjectPtr.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Player/MT2PlayerController.h"
#include "Player/MT2PlayerState.h"
#include "AbilitySystemComponent.h"
#include "Abilities/MT2CoreAttributeSet.h"
#include "Core/MT2VnumRegistrySubsystem.h"
#include "Items/MT2InventoryComponent.h"
#include "Items/MT2ItemTemplate.h"
#include "UI/MT2InventorySlotWidget.h"
#include "UI/MT2RefinementDialogWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2RefinementMaterialIconsTest,
	"Metin2.UI.Refinement.MaterialIcons",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2RefinementMaterialIconsTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GEngine)); GI->InitializeStandalone();
	UWorld* World=GI->GetWorld();
	TStrongObjectPtr<UGameViewportClient> Viewport(NewObject<UGameViewportClient>(GEngine));
	auto& Context=GEngine->GetWorldContextFromWorldChecked(World); Context.GameViewport=Viewport.Get(); Viewport->Init(Context,GI.Get(),false);
	ON_SCOPE_EXIT { GI->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	FString Error; auto* LocalPlayer=GI->CreateLocalPlayer(0,Error,false);
	auto* Controller=World->SpawnActor<AMT2PlayerController>(); Controller->SetPlayer(LocalPlayer); World->AddController(Controller);
	auto* State=World->SpawnActor<AMT2PlayerState>(); Controller->SetPlayerState(State);
	State->GetAbilitySystemComponent()->AddAttributeSetSubobject(State->GetCoreAttributes());
	auto* Player=World->SpawnActor<AMT2PlayerCharacter>(); Controller->Possess(Player);
	auto* Registry=GI->GetSubsystem<UMT2VnumRegistrySubsystem>();
	auto* Template=const_cast<UMT2ItemTemplate*>(Registry->ResolveItemTemplateClass(10).GetDefaultObject());
	if (!TestNotNull(TEXT("Imported sword fixture"),Template)) { return false; }
	const auto OriginalRecipe=Template->RefinementRecipe; const int32 OriginalRefined=Template->RefinedVnum;
	ON_SCOPE_EXIT { Template->RefinementRecipe=OriginalRecipe; Template->RefinedVnum=OriginalRefined; };
	Template->RefinedVnum=11; Template->RefinementRecipe.YangCost=100; Template->RefinementRecipe.SuccessPercent=100;
	Template->RefinementRecipe.Materials={{30001,2},{30002,1}};
	TArray<FMT2ItemSlot> Items; Items.SetNum(180);
	Items[0].Vnum=10; Items[0].Count=1;
	Items[1].Vnum=30001; Items[1].Count=1;
	Items[2].Vnum=30002; Items[2].Count=1;
	Player->GetInventoryComponent()->RestoreItems(Items,{}); State->SetYang(1000);
	auto* Dialog=CreateWidget<UMT2RefinementDialogWidget>(Controller,UMT2RefinementDialogWidget::StaticClass());
	Dialog->OpenRefinement(0,INDEX_NONE,nullptr);
	auto* Row=Cast<UHorizontalBox>(Dialog->GetWidgetFromName(TEXT("RefinementMaterials")));
	if (!TestNotNull(TEXT("Required materials have one horizontal icon row"),Row)) { return false; }
	TestEqual(TEXT("Each required material has one cell"),Row->GetChildrenCount(),2);
	TArray<UMT2InventorySlotWidget*> Cells;
	TFunction<void(UWidget*)> Visit=[&](UWidget* Widget)
	{
		if (auto* Cell=Cast<UMT2InventorySlotWidget>(Widget)) { Cells.Add(Cell); }
		if (auto* Panel=Cast<UPanelWidget>(Widget)) { for (auto* Child : Panel->GetAllChildren()) { Visit(Child); } }
	};
	Visit(Row);
	if (!TestEqual(TEXT("Materials reuse actual inventory icon widgets"),Cells.Num(),2)) { return false; }
	for (int32 Index=0;Index<2;++Index)
	{
		TestEqual(TEXT("Icons follow recipe order"),Cells[Index]->GetItemId(),FName(Index==0 ? TEXT("30001") : TEXT("30002")));
		auto* Count=Cast<UTextBlock>(Cells[Index]->GetWidgetFromName(TEXT("CountText")));
		if (TestNotNull(TEXT("Inventory count overlay exists"),Count))
		{ TestEqual(TEXT("Overlay shows required count, including one"),Count->GetText().ToString(),FString(Index==0 ? TEXT("2") : TEXT("1"))); }
		auto* Icon=Cast<UImage>(Cells[Index]->GetWidgetFromName(TEXT("IconImage")));
		if (TestNotNull(TEXT("Inventory icon exists"),Icon)) { TestNotNull(TEXT("Material texture is loaded"),Icon->GetBrush().GetResourceObject()); }
		TestEqual(TEXT("Material preview cannot drag or activate inventory items"),Cells[Index]->GetVisibility(),ESlateVisibility::HitTestInvisible);
	}
	auto* Confirm=Cast<UButton>(Dialog->GetWidgetFromName(TEXT("ConfirmRefinement")));
	if (TestNotNull(TEXT("Confirm button"),Confirm)) { TestFalse(TEXT("Missing materials still prevent refinement"),Confirm->GetIsEnabled()); }
	Items[1].Count=2; Player->GetInventoryComponent()->RestoreItems(Items,{});
	Dialog->OpenRefinement(0,INDEX_NONE,nullptr);
	TestEqual(TEXT("Reopening replaces the icon row instead of appending cells"),Row->GetChildrenCount(),2);
	if (Confirm) { TestTrue(TEXT("Enough materials and Yang still allow refinement"),Confirm->GetIsEnabled()); }
	return true;
}
#endif
