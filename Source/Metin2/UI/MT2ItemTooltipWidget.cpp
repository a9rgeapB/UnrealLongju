/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "UI/MT2ItemTooltipWidget.h"
#include "Items/MT2AccessorySocketRules.h"
#include "Items/MT2InventoryComponent.h"
#include "Items/MT2ItemUtils.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Config/MT2PathSettings.h"
#include "UI/MT2AtlasImage.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Items/MT2ItemTemplate.h"
#include "Items/MT2ItemBonusSettings.h"
#include "Items/MT2ItemBonusUtils.h"
#include "Player/MT2PlayerState.h"
#include "Skills/MT2SkillComponent.h"
#include "Skills/MT2SkillDefinition.h"
#include "Skills/MT2SkillSet.h"
#include "UI/MT2UIStyle.h"

const FLinearColor UMT2ItemTooltipWidget::NormalColor(0.76f, 0.76f, 0.76f);

FString UMT2ItemTooltipWidget::DescribeAccessoryMaterial(const UMT2ItemTemplate* Template)
{
	const auto* Use = Cast<UMT2ItemUseTemplate>(Template);
	if (Use && Use->UseSubType == 20)
	{ return TEXT("Opens a socket in an accessory.\nSuccess chance: 50%."); }
	if (Use && Use->UseSubType == 29)
	{ return TEXT("Fills a belt socket.\nEnhances the belt's bonuses."); }
	if (Template && Template->Vnum == 50622) { return TEXT("This mineral has no compatible accessory."); }
	if (!Use || Use->UseSubType != 21) { return {}; }
	if (Template->Vnum < 50623 || Template->Vnum > 50638) { return TEXT("This mineral has no compatible accessory."); }
	return TEXT("Fills a socket in matching jewelry.\nEnhances the accessory's bonuses.");
}

FString UMT2ItemTooltipWidget::AccessoryBonusLines(const UMT2ItemTemplate* Template, int32 FromGrade, int32 ToGrade)
{
	if (!MT2AccessorySockets::IsAccessory(Template) || FromGrade < 0 || ToGrade > 3 || ToGrade <= FromGrade) { return {}; }
	TArray<FString> Lines;
	for (const auto& Apply : Template->Applies)
	{
		const int32 Type = static_cast<int32>(Apply.Type);
		if (Type == 0 || Type == 51) { continue; }
		const int32 Delta = MT2AccessorySockets::Bonus(Apply.Value,ToGrade) - MT2AccessorySockets::Bonus(Apply.Value,FromGrade);
		if (Delta) { Lines.Add(FormatApply(Type,Delta)); }
	}
	return FString::Join(Lines,TEXT("\n"));
}
const FLinearColor UMT2ItemTooltipWidget::PositiveColor(0.54f, 0.72f, 0.56f);
const FLinearColor UMT2ItemTooltipWidget::NegativeColor(0.90f, 0.47f, 0.46f);
const FLinearColor UMT2ItemTooltipWidget::TitleColor(1.0f, 0.89f, 0.42f);
const FLinearColor UMT2ItemTooltipWidget::MaxBonusColor(0.72f, 0.38f, 0.95f);

namespace
{
	bool IsIntrinsicDamageBonus(
		const UMT2ItemTemplate* Template, const FMT2ItemBonus& Bonus,
		const UMT2ItemBonusSettings* Settings)
	{
		const UMT2ItemEquipmentTemplate* Equipment =
			Cast<UMT2ItemEquipmentTemplate>(Template);
		return Equipment && Settings &&
			Equipment->BonusAddonType == Settings->DamageAddonType &&
			(Bonus.GetTypeId() == Settings->DamageAddonFormula.SkillDamageApplyType ||
			 Bonus.GetTypeId() == Settings->DamageAddonFormula.AverageDamageApplyType);
	}

	bool IsMaximumBonus(
		const UMT2ItemTemplate* Template, const FMT2ItemBonus& Bonus,
		const UMT2ItemBonusSettings* Settings)
	{
		const EMT2ItemBonusTarget Target = MT2ItemBonusUtils::ResolveTarget(Template);
		const FMT2ItemBonusDefinition* Definition = Settings
			? Settings->FindDefinition(Bonus.GetTypeId(), Bonus.Kind) : nullptr;
		const int32* MaximumLevel = Definition
			? Definition->MaxLevelByItemType.Find(Target) : nullptr;
		if (!MaximumLevel)
		{
			return false;
		}
		const int32 MaxLevel = FMath::Clamp(
			*MaximumLevel, 0, Definition->Values.Num());
		return MaxLevel > 0 && Bonus.Value == Definition->Values[MaxLevel - 1];
	}

	const UMT2SkillDefinition* ResolveBookSkill(int32 SkillVnum, const AMT2PlayerState* Viewer)
	{
		if (SkillVnum <= 0)
		{
			return nullptr;
		}
		if (const UMT2SkillComponent* Skills = Viewer ? Viewer->GetSkillComponent() : nullptr)
		{
			if (const UMT2SkillDefinition* Definition = Skills->FindSkillDefinition(SkillVnum))
			{
				return Definition;
			}
		}
		return UMT2SkillSet::FindSkillAcrossSets(SkillVnum);
	}
}

UMT2ItemTooltipWidget* UMT2ItemTooltipWidget::Create(
	APlayerController* Owner, const UMT2ItemTemplate* Template, int32 Count,
	const AMT2PlayerState* Viewer, int64 ShopPrice, const TArray<FMT2ItemBonus>* Bonuses,
	const TArray<FMT2MetinSocket>* MetinSockets, int32 SkillVnum,
	int32 AutoRecoveryRemainingAmount, const FMT2AccessorySocketState* AccessorySockets)
{
	if (!Owner || !Template)
	{
		return nullptr;
	}
	TSubclassOf<UMT2ItemTooltipWidget> TooltipClass = Template->ResolveTooltipClass();
	if (!TooltipClass)
	{
		TooltipClass = UMT2ItemTooltipWidget::StaticClass();
	}
	UMT2ItemTooltipWidget* Tooltip = CreateWidget<UMT2ItemTooltipWidget>(Owner, TooltipClass);
	if (Tooltip)
	{
		Tooltip->BuildTooltip(
			Template, Count, Viewer, ShopPrice, Bonuses, MetinSockets, SkillVnum,
			AutoRecoveryRemainingAmount, AccessorySockets);
	}
	return Tooltip;
}

TSharedRef<SWidget> UMT2ItemTooltipWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		// The old client's tooltip background is ui.ThinBoard: a thin 9-slice frame over a
		// translucent black base, with the text centered inside.
		LinesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ItemTooltipLines"));
		WidgetTree->RootWidget = FMT2UIStyle::ThinBoard(*WidgetTree, LinesBox, FMargin(10.0f, 6.0f));
	}
	return Super::RebuildWidget();
}

void UMT2ItemTooltipWidget::AddLine(const FString& Text, const FLinearColor& Color, int32 FontSize)
{
	UTextBlock* Line = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Line->SetText(FText::FromString(Text));
	FSlateFontInfo Font = Line->GetFont();
	Font.Size = FontSize;
	Line->SetFont(Font);
	Line->SetColorAndOpacity(FSlateColor(Color));
	// The old tooltip centers every line.
	Line->SetJustification(ETextJustify::Center);
	Line->SetShadowOffset(FVector2D(1.0));
	Line->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
	Line->SetAutoWrapText(true);
	Line->SetWrapTextAt(240.0f);
	if (UVerticalBoxSlot* LineSlot = LinesBox->AddChildToVerticalBox(Line))
	{
		LineSlot->SetHorizontalAlignment(HAlign_Center);
	}
}

void UMT2ItemTooltipWidget::AddSpace(float Height)
{
	UTextBlock* Spacer = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FSlateFontInfo Font = Spacer->GetFont();
	Font.Size = FMath::Max(1, FMath::RoundToInt(Height * 0.6f));
	Spacer->SetFont(Font);
	Spacer->SetText(FText::GetEmpty());
	LinesBox->AddChildToVerticalBox(Spacer);
}

FString UMT2ItemTooltipWidget::FormatApply(int32 ApplyType, int32 Value)
{
	return GetDefault<UMT2ItemBonusSettings>()->FormatBonus(ApplyType, Value);
}

void UMT2ItemTooltipWidget::AddLimits(const UMT2ItemTemplate* Template, const AMT2PlayerState* Viewer)
{
	for (const FMT2ItemLimit& Limit : Template->Limits)
	{
		if (Limit.Value <= 0)
		{
			continue;
		}
		// ELimitTypes: 1 = level (the only one the old client shows).
		if (Limit.Type == EMT2ItemLimitType::Level)
		{
			const int32 PlayerLevel = Viewer ? Viewer->GetCharacterLevel() : 0;
			AddLine(FString::Printf(TEXT("Required Level: %d"), Limit.Value),
				PlayerLevel >= Limit.Value ? NormalColor : NegativeColor);
		}
	}
}

void UMT2ItemTooltipWidget::AddApplies(const UMT2ItemTemplate* Template)
{
	for (const FMT2ItemApply& Apply : Template->Applies)
	{
		if (Apply.Type == EMT2ItemBonusType::None || Apply.Value == 0)
		{
			continue;
		}
		AddLine(FormatApply(static_cast<int32>(Apply.Type), Apply.Value),
			Apply.Value > 0 ? PositiveColor : NegativeColor);
	}
}

void UMT2ItemTooltipWidget::AddWearableRaces(const UMT2ItemTemplate* Template)
{
	if (!Cast<UMT2ItemEquipmentTemplate>(Template))
	{
		return;
	}

	struct FWearableRace
	{
		const TCHAR* Name;
		int32 AntiFlag;
	};
	static constexpr FWearableRace Races[] = {
		{ TEXT("Warrior"), 1 << 2 },
		{ TEXT("Ninja"), 1 << 3 },
		{ TEXT("Sura"), 1 << 4 },
		{ TEXT("Shaman"), 1 << 5 }
	};

	TArray<FString> AllowedRaces;
	for (const FWearableRace& Race : Races)
	{
		if ((Template->AntiFlags & Race.AntiFlag) == 0)
		{
			AllowedRaces.Add(Race.Name);
		}
	}
	if (!AllowedRaces.IsEmpty())
	{
		AddSpace(4.0f);
		AddLine(FString::Join(AllowedRaces, TEXT(" ")), FLinearColor::White);
	}
}

void UMT2ItemTooltipWidget::AddMetinSockets(
	const TArray<FMT2MetinSocket>* MetinSockets)
{
	if (!MetinSockets || MetinSockets->IsEmpty())
	{
		return;
	}

	bool bAddedSection = false;
	for (const FMT2MetinSocket& Socket : *MetinSockets)
	{
		const UMT2ItemMetinStoneTemplate* Stone = Socket.Stone.GetDefaultObject();
		if (!Stone)
		{
			continue;
		}

		if (!bAddedSection)
		{
			AddSpace(7.0f);
			bAddedSection = true;
		}
		AddSocketRow(Socket.Type,Stone,FString());
	}
}

void UMT2ItemTooltipWidget::AddAccessorySockets(const UMT2ItemTemplate* Template, const FMT2AccessorySocketState& InState)
{
	const auto State = MT2AccessorySockets::Normalize(InState);
	if (!State.MaximumGrade) { return; }
	const auto* Armor = Cast<UMT2ItemArmorTemplate>(Template);
	// The client uses the same socket row for jewelry minerals and the belt's dark stone.
	const int32 MaterialVnum = Armor ? MT2AccessorySockets::RequiredMineral(Template->Vnum,Armor->ArmorSubType) : 18900;
	const auto* Material = MT2ItemUtils::ResolveTemplate(this,MaterialVnum);
	AddSpace(7.0f);
	for (int32 Grade = 1; Grade <= State.MaximumGrade; ++Grade)
	{
		const bool bFilled = Grade <= State.CurrentGrade;
		AddSocketRow(EMT2MetinSocketType::Silver,bFilled ? Material : nullptr,
			bFilled ? AccessoryBonusLines(Template,Grade-1,Grade) : FString(),
			bFilled && Grade == State.CurrentGrade ? State.RemainingSeconds : 0,bFilled);
	}
}

void UMT2ItemTooltipWidget::AddSocketRow(EMT2MetinSocketType Type, const UMT2ItemTemplate* Material,
	const FString& Bonuses, int32 RemainingSeconds, bool bFilled)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass());
	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass());
	IconSize->SetWidthOverride(36.0f);
	IconSize->SetHeightOverride(36.0f);
	UOverlay* IconLayers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UMT2AtlasImage* Frame = WidgetTree->ConstructWidget<UMT2AtlasImage>(UMT2AtlasImage::StaticClass());
	Frame->SetAtlas(FMT2UIStyle::LoadTexture(UMT2PathSettings::Path(TEXT("UI_WindowsAtlas"))),
		FMT2AtlasRect(Type == EMT2MetinSocketType::Gold ? 242.f : 278.f,152.f,36.f,36.f));
	IconLayers->AddChildToOverlay(Frame);
	if (bFilled && Material)
	{
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Icon->SetBrushFromTexture(Material->Icon.LoadSynchronous(),true);
		USizeBox* ItemSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		ItemSize->SetWidthOverride(32.f); ItemSize->SetHeightOverride(32.f); ItemSize->SetContent(Icon);
		if (UOverlaySlot* IconSlot = IconLayers->AddChildToOverlay(ItemSize))
		{ IconSlot->SetHorizontalAlignment(HAlign_Center); IconSlot->SetVerticalAlignment(VAlign_Center); }
	}
	IconSize->SetContent(IconLayers);
	if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconSize))
	{
		IconSlot->SetPadding(FMargin(0.0f, 1.0f, 7.0f, 1.0f));
		IconSlot->SetVerticalAlignment(VAlign_Center);
	}

	UVerticalBox* Details = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass());
	const auto AddDetailLine = [this, Details](
		const FString& Text, const FLinearColor& Color)
	{
		UTextBlock* Line = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass());
		Line->SetText(FText::FromString(Text));
		FSlateFontInfo Font = Line->GetFont();
		Font.Size = 9;
		Line->SetFont(Font);
		Line->SetColorAndOpacity(FSlateColor(Color));
		Line->SetShadowOffset(FVector2D(1.0f));
		Line->SetShadowColorAndOpacity(
			FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
		Line->SetAutoWrapText(false);
		if (UVerticalBoxSlot* Slot = Details->AddChildToVerticalBox(Line))
		{
			Slot->SetHorizontalAlignment(HAlign_Left);
		}
	};

	const FString Name = Material ? MT2ItemUtils::GetDisplayName(Material,Material->Vnum).ToString()
		: bFilled ? TEXT("Filled socket") : TEXT("Empty socket");
	AddDetailLine(Name, NormalColor);
	if (const auto* Stone = Cast<UMT2ItemMetinStoneTemplate>(Material))
	{
		for (const auto& Apply : Stone->Applies)
		{
			if (Apply.Type != EMT2ItemBonusType::None && Apply.Value != 0)
			{ AddDetailLine(FormatApply(static_cast<int32>(Apply.Type),Apply.Value),Apply.Value > 0 ? PositiveColor : NegativeColor); }
		}
	}
	else if (!Bonuses.IsEmpty()) { AddDetailLine(Bonuses,PositiveColor); }
	if (RemainingSeconds > 0)
	{
		AddDetailLine(FString::Printf(TEXT("Remaining: %dh %dm %ds"),RemainingSeconds/3600,RemainingSeconds/60%60,RemainingSeconds%60),NormalColor);
	}
	if (UHorizontalBoxSlot* DetailsSlot = Row->AddChildToHorizontalBox(Details))
	{
		DetailsSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UVerticalBoxSlot* RowSlot = LinesBox->AddChildToVerticalBox(Row))
	{
		RowSlot->SetPadding(FMargin(2.0f, 1.0f));
		RowSlot->SetHorizontalAlignment(HAlign_Left);
	}
}

void UMT2ItemTooltipWidget::BuildTooltip(
	const UMT2ItemTemplate* Template, int32 Count, const AMT2PlayerState* Viewer, int64 ShopPrice,
	const TArray<FMT2ItemBonus>* Bonuses, const TArray<FMT2MetinSocket>* MetinSockets,
	int32 SkillVnum, int32 AutoRecoveryRemainingAmount, const FMT2AccessorySocketState* AccessorySockets)
{
	TakeWidget();
	if (!LinesBox || !Template)
	{
		return;
	}
	LinesBox->ClearChildren();

	// Name (with stack count), description, then the type-specific body.
	FString Name = Template->DisplayName.IsEmpty() ? Template->InternalName : Template->DisplayName.ToString();
	if (const UMT2ItemSkillBookTemplate* Book = Cast<UMT2ItemSkillBookTemplate>(Template))
	{
		const int32 EffectiveSkillVnum = SkillVnum > 0 ? SkillVnum : Book->SkillVnum;
		if (const UMT2SkillDefinition* Skill = ResolveBookSkill(EffectiveSkillVnum, Viewer))
		{
			Name = FString::Printf(TEXT("%s Skill Book"), *Skill->GetGradeDisplayName(0).ToString());
		}
	}
	if (Count > 1)
	{
		Name += FString::Printf(TEXT(" (%d)"), Count);
	}
	AddLine(Name, TitleColor, 11);

	const bool bEquippable = Cast<UMT2ItemEquipmentTemplate>(Template) != nullptr;
	if (bEquippable)
	{
		const FMT2ItemLimit* LevelLimit = Template->Limits.FindByPredicate(
			[](const FMT2ItemLimit& Limit)
			{
				return Limit.Type == EMT2ItemLimitType::Level;
			});
		AddLine(FString::Printf(TEXT("From Level: %d"),
			LevelLimit ? FMath::Max(LevelLimit->Value, 0) : 0), FLinearColor::White);
	}

	if (!Template->Description.IsEmpty())
	{
		AddLine(Template->Description.ToString(), NormalColor, 9);
	}

	if (!bEquippable)
	{
		AddLimits(Template, Viewer);
	}
	BuildTypeSpecific(Template, Viewer);
	const FString MaterialDescription = DescribeAccessoryMaterial(Template);
	if (!MaterialDescription.IsEmpty())
	{
		AddLine(MaterialDescription, NormalColor);
		const auto* Player = Viewer ? Cast<AMT2PlayerCharacter>(Viewer->GetPawn()) : nullptr;
		const auto* Inventory = Player ? Player->GetInventoryComponent() : nullptr;
		if (Inventory)
		{
			for (int32 Index = 0; Index < Inventory->GetEquipment().Num(); ++Index)
			{
				const auto& Worn = Inventory->GetEquipment()[Index];
				const auto* Target = MT2ItemUtils::ResolveTemplate(this,Worn.Vnum);
				const auto State = Inventory->GetEffectiveAccessoryState(Index);
				const auto* Use = Cast<UMT2ItemUseTemplate>(Template);
				if (Use && Use->UseSubType != 20 && MT2AccessorySockets::Compatible(Template,Target) && State.CurrentGrade < State.MaximumGrade)
				{
					AddLine(FString::Printf(TEXT("Next socket on %s:"), *MT2ItemUtils::GetDisplayName(Target,Worn.Vnum).ToString()),TitleColor);
					const FString Gains = AccessoryBonusLines(Target,State.CurrentGrade,State.CurrentGrade+1);
					AddLine(Gains.IsEmpty() ? TEXT("No intrinsic stat gain.") : Gains,PositiveColor);
				}
			}
		}
	}
	if (MT2AccessorySockets::IsAccessory(Template))
	{
		AddAccessorySockets(Template,AccessorySockets ? *AccessorySockets : FMT2AccessorySocketState());
	}
	if (const UMT2ItemAutoRecoveryTemplate* AutoRecovery =
		Cast<UMT2ItemAutoRecoveryTemplate>(Template))
	{
		const int32 Remaining = AutoRecoveryRemainingAmount >= 0
			? AutoRecoveryRemainingAmount : AutoRecovery->RecoveryCapacity;
		AddLine(FString::Printf(TEXT("Remaining %s: %s"),
			AutoRecovery->Resource == EMT2AutoRecoveryResource::Health ? TEXT("HP") : TEXT("MP"),
			*FText::AsNumber(Remaining).ToString()), PositiveColor);
	}
	if (Cast<UMT2ItemMetinStoneTemplate>(Template))
	{
		AddApplies(Template);
	}
	if (Bonuses)
	{
		const UMT2ItemBonusSettings* Settings = GetDefault<UMT2ItemBonusSettings>();
		const auto AddBonusGroup = [&](auto&& Predicate, bool bUseMaximumColor)
		{
			for (const FMT2ItemBonus& Bonus : *Bonuses)
			{
				if (!Bonus.IsValid() || !Predicate(Bonus))
				{
					continue;
				}
				const FLinearColor Color = bUseMaximumColor &&
					IsMaximumBonus(Template, Bonus, Settings)
					? MaxBonusColor
					: (Bonus.Value > 0 ? PositiveColor : NegativeColor);
				AddLine(FormatApply(Bonus.GetTypeId(), Bonus.Value), Color);
			}
		};

		// Old tooltip order: proto values/applies (already emitted by BuildTypeSpecific), automatic
		// weapon addon values, normal attributes, then the two rare attributes.
		AddBonusGroup([&](const FMT2ItemBonus& Bonus)
		{
			return IsIntrinsicDamageBonus(Template, Bonus, Settings);
		}, false);
		AddBonusGroup([&](const FMT2ItemBonus& Bonus)
		{
			return Bonus.Kind == EMT2ItemBonusKind::Normal &&
				!IsIntrinsicDamageBonus(Template, Bonus, Settings);
		}, true);
		AddBonusGroup([](const FMT2ItemBonus& Bonus)
		{
			return Bonus.Kind == EMT2ItemBonusKind::Rare;
		}, true);
	}
	AddWearableRaces(Template);
	if (!MT2AccessorySockets::IsAccessory(Template)) { AddMetinSockets(MetinSockets); }

	// Shop price line, colored by affordability.
	if (ShopPrice >= 0)
	{
		AddSpace();
		const int64 Yang = Viewer ? Viewer->GetYang() : 0;
		AddLine(FString::Printf(TEXT("Price: %lld Yang"), ShopPrice),
			Yang >= ShopPrice ? PositiveColor : NegativeColor);
	}
}
