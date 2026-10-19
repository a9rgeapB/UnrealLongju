/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "UI/MT2ItemDropDialogWidget.h"
#include "Items/Templates/MT2ItemUseTemplate.h"

#include "Items/MT2AccessorySocketRules.h"
#include "UI/MT2ItemTooltipWidget.h"
#include "Components/CanvasPanelSlot.h"

#include "Characters/MT2PlayerCharacter.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Items/MT2ItemTemplate.h"
#include "Items/MT2ItemUtils.h"
#include "UI/MT2BoardWidget.h"

void UMT2ItemDropDialogWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BoardWidget->InitializeBoard(FVector2D(340.0f, 105.0f));
	ConfirmButton->OnClicked.AddUniqueDynamic(this, &UMT2ItemDropDialogWidget::HandleConfirmClicked);
	CancelButton->OnClicked.AddUniqueDynamic(this, &UMT2ItemDropDialogWidget::HandleCancelClicked);
}

void UMT2ItemDropDialogWidget::OpenDropDialog(
	int32 InventorySlot, int32 Vnum, int32 Count)
{
	PendingAccessoryConfirmation = 0;
	SetDialogHeight(105);
	bPendingMetinAttach = false;
	bPendingTargetEquipped = false;
	PendingTargetSlot = INDEX_NONE;
	PendingInventorySlot = InventorySlot;
	PendingVnum = Vnum;
	PendingCount = FMath::Max(Count, 1);

	const UMT2ItemTemplate* Template = MT2ItemUtils::ResolveTemplate(this, Vnum);
	const FText ItemName = MT2ItemUtils::GetDisplayName(Template, Vnum);
	MessageText->SetText(FText::Format(
		NSLOCTEXT("MT2", "DropItemQuestion", "Drop {0} x{1} on the ground?"),
		ItemName, FText::AsNumber(PendingCount)));
	SetVisibility(ESlateVisibility::Visible);
}

void UMT2ItemDropDialogWidget::OpenMetinAttachDialog(
	int32 StoneSlot, int32 TargetSlot, bool bTargetEquipped,
	const FText& StoneName, const FText& TargetName)
{
	PendingAccessoryConfirmation = 0;
	SetDialogHeight(105);
	bPendingMetinAttach = true;
	bPendingTargetEquipped = bTargetEquipped;
	PendingInventorySlot = StoneSlot;
	PendingTargetSlot = TargetSlot;
	PendingVnum = 0;
	PendingCount = 1;
	MessageText->SetText(FText::Format(
		NSLOCTEXT("MT2", "AttachMetinQuestion",
			"Attach {0} to {1}?\nThe stone is consumed even if attachment fails."),
		StoneName, TargetName));
	SetVisibility(ESlateVisibility::Visible);
}

void UMT2ItemDropDialogWidget::SetDialogHeight(float Height)
{
	ConfirmButton->SetIsEnabled(true);
	BoardWidget->InitializeBoard(FVector2D(340,Height));
	if (auto* CanvasSlot = Cast<UCanvasPanelSlot>(BoardWidget->Slot)) { CanvasSlot->SetSize(FVector2D(340,Height)); }
	if (auto* CanvasSlot = Cast<UCanvasPanelSlot>(MessageText->Slot)) { CanvasSlot->SetSize(FVector2D(310,Height-65)); CanvasSlot->SetPosition(FVector2D(0,-14)); }
	MessageText->SetAutoWrapText(true);
	for (auto* Button : {ConfirmButton.Get(),CancelButton.Get()})
	{
		if (auto* CanvasSlot = Cast<UCanvasPanelSlot>(Button->Slot)) { auto Position=CanvasSlot->GetPosition(); Position.Y=Height/2-24; CanvasSlot->SetPosition(Position); }
	}
}

void UMT2ItemDropDialogWidget::OpenAccessoryDialog(int32 SourceSlot, int32 TargetSlot, bool bTargetEquipped,
	const FMT2ItemSlot& Source, const FMT2ItemSlot& Target)
{
	auto* Player = Cast<AMT2PlayerCharacter>(GetOwningPlayerPawn());
	const auto* Material = Cast<UMT2ItemUseTemplate>(MT2ItemUtils::ResolveTemplate(this,Source.Vnum));
	const auto* TargetTemplate = MT2ItemUtils::ResolveTemplate(this,Target.Vnum);
	const auto State = MT2AccessorySockets::Normalize(Target.GetAccessorySockets());
	const bool bOpening = Material && Material->UseSubType == EMT2UseSubType::AddAccessorySocket;
	const bool bValid = Player && MT2AccessorySockets::Compatible(Material,TargetTemplate) &&
		(bOpening ? State.MaximumGrade < 3 : State.CurrentGrade < State.MaximumGrade);
	PendingAccessoryConfirmation = Player ? Player->NewAccessoryConfirmationId() : 0;
	PendingInventorySlot = SourceSlot; PendingTargetSlot = TargetSlot; bPendingTargetEquipped = bTargetEquipped;
	PendingAccessorySource = Source; PendingAccessoryTarget = Target; bPendingMetinAttach = false;
	FString Text;
	if (!bValid) { Text = TEXT("This material is incompatible, or the accessory has no available socket. Nothing will be consumed."); }
	else
	{
		Text = FString::Printf(TEXT("Use %s on %s?\nSuccess: %d%%. One material is consumed even on failure.\n"),
			*MT2ItemUtils::GetDisplayName(Material,Source.Vnum).ToString(),*MT2ItemUtils::GetDisplayName(TargetTemplate,Target.Vnum).ToString(),
			bOpening ? 50 : MT2AccessorySockets::InsertionChance(State.CurrentGrade));
		if (bOpening) { Text += TEXT("Opens one permanent socket. No combat bonus until filled."); }
		else
		{
			const FString Gains=UMT2ItemTooltipWidget::AccessoryBonusLines(TargetTemplate,State.CurrentGrade,State.CurrentGrade+1);
			Text += Gains.IsEmpty() ? TEXT("No intrinsic stat gain.") : Gains;
			Text += FString::Printf(TEXT("\nNew grade wear time: %d hours (equipped only)."),MT2AccessorySockets::Duration(State.CurrentGrade+1)/3600);
		}
	}
	SetDialogHeight(240);
	MessageText->SetText(FText::FromString(Text));
	ConfirmButton->SetIsEnabled(bValid);
	SetVisibility(ESlateVisibility::Visible);
}

void UMT2ItemDropDialogWidget::HandleConfirmClicked()
{
	if (PendingAccessoryConfirmation)
	{
		auto* Character = Cast<AMT2PlayerCharacter>(GetOwningPlayerPawn());
		const uint32 Id = PendingAccessoryConfirmation;
		const int32 SourceSlot = PendingInventorySlot, TargetSlot = PendingTargetSlot;
		const bool bEquipped = bPendingTargetEquipped;
		const FMT2ItemSlot Source = PendingAccessorySource, Target = PendingAccessoryTarget;
		CloseDialog();
		if (Character) { Character->ServerApplyAccessoryMaterial(Id,SourceSlot,TargetSlot,bEquipped,Source,Target); }
		return;
	}
	if (AMT2PlayerCharacter* Character = Cast<AMT2PlayerCharacter>(GetOwningPlayerPawn()))
	{
		if (bPendingMetinAttach)
		{
			Character->ServerApplyMetinStone(
				PendingInventorySlot, PendingTargetSlot, bPendingTargetEquipped);
		}
		else
		{
			Character->ServerDropInventoryItem(PendingInventorySlot, PendingCount);
		}
	}
	CloseDialog();
}

void UMT2ItemDropDialogWidget::HandleCancelClicked()
{
	CloseDialog();
}

void UMT2ItemDropDialogWidget::CloseDialog()
{
	PendingAccessoryConfirmation = 0;
	PendingAccessorySource = {}; PendingAccessoryTarget = {};
	ConfirmButton->SetIsEnabled(true);
	PendingInventorySlot = INDEX_NONE;
	PendingVnum = 0;
	PendingCount = 0;
	PendingTargetSlot = INDEX_NONE;
	bPendingMetinAttach = false;
	bPendingTargetEquipped = false;
	RemoveFromParent();
}
