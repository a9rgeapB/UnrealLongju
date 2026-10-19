#include "MT2ItemReferenceResolver.h"

#include "Items/MT2ItemTemplate.h"
#include "Items/Templates/MT2ItemRodTemplate.h"
#include "Items/Templates/MT2ItemFishTemplate.h"
#include "Items/Templates/MT2ItemSkillBookTemplate.h"
#include "Skills/MT2SkillSet.h"
#include "Skills/MT2SkillDefinition.h"
#include "Items/Templates/MT2ItemMetinStoneTemplate.h"
#include "Items/Use/MT2HorseBookItemTemplate.h"
#include "Core/MT2VnumRegistry.h"

bool ResolveMT2ItemReferences(const TArray<UMT2ItemTemplate*>& Items, TArray<FString>& Warnings,
	const UMT2VnumRegistry* ExistingRegistry)
{
	TMap<int32, UClass*> Classes;
	TMap<int32, UMT2ItemMetinStoneTemplate*> StoneFamilies;
	TSet<UMT2ItemTemplate*> Definitions;
	for (auto* Item : Items) if (Item) Definitions.Add(Item);
	if (ExistingRegistry)
	{
		for (const auto& Entry : ExistingRegistry->GetItemTemplates())
			if (auto* Class = Entry.Value.LoadSynchronous()) Definitions.Add(Class->GetDefaultObject<UMT2ItemTemplate>());
	}
	for (auto* Item : Definitions)
	{
		if (!Item || Item->Vnum <= 0) continue;
		Classes.Add(Item->Vnum, Item->GetClass());
		if (auto* Stone = Cast<UMT2ItemMetinStoneTemplate>(Item))
		{
			auto*& Representative = StoneFamilies.FindOrAdd(Stone->StoneCategory);
			if (!Representative || Stone->Vnum < Representative->Vnum) Representative = Stone;
		}
		for (int64 Offset = 1; Offset <= Item->VnumRange && int64(Item->Vnum) + Offset <= MAX_int32; ++Offset)
			Classes.Add(Item->Vnum + int32(Offset), Item->GetClass());
	}
	bool bAllResolved = true;
	for (auto* Item : Items)
	{
		if (!Item) continue;
		bool bResolved = true;
		auto Resolve = [&](int32 Vnum, const TCHAR* Field) -> UClass*
		{
			if (Vnum <= 0) return nullptr;
			if (UClass* Class = Classes.FindRef(Vnum)) return Class;
			Warnings.Add(FString::Printf(TEXT("%s: missing %s item %d; retaining its legacy identity."), *Item->GetPathName(), Field, Vnum));
			bResolved = false;
			return nullptr;
		};
		if (!Item->bTypedItemReferences)
		{
			Item->RefinedItemTemplate = Resolve(Item->RefinedVnum, TEXT("refinement target"));
			Item->PreviousRefinedItemTemplate = Resolve(Item->PreviousRefinedVnum, TEXT("refinement downgrade"));
			if (auto* Rod = Cast<UMT2ItemRodTemplate>(Item))
			{
				UClass* Result = Resolve(Rod->RefinementFailureVnum, TEXT("rod failure result"));
				if (Result && !Result->IsChildOf(UMT2ItemRodTemplate::StaticClass()))
				{ Warnings.Add(Item->GetPathName() + TEXT(": rod failure result is not a rod.")); bResolved = false; }
				else Rod->RefinementFailureTemplate = Result;
			}
			if (auto* Book = Cast<UMT2ItemSkillBookTemplate>(Item))
			{
				Book->Skill = Book->SkillVnum > 0 ? UMT2SkillSet::FindSkillAcrossSets(Book->SkillVnum) : nullptr;
				if (Book->SkillVnum > 0 && !Book->Skill) { Warnings.Add(Item->GetPathName() + TEXT(": missing skill definition.")); bResolved = false; }
			}
			if (auto* Book = Cast<UMT2HorseBookItemTemplate>(Item))
			{
				Book->SummonSkill = UMT2SkillSet::FindSkillAcrossSets(Book->SummonSkillVnum);
				if (Book->SummonSkillVnum > 0 && !Book->SummonSkill)
				{ Warnings.Add(Item->GetPathName() + TEXT(": missing horse summon skill.")); bResolved = false; }
			}
			if (auto* Stone = Cast<UMT2ItemMetinStoneTemplate>(Item))
				if (const auto* Representative = StoneFamilies.FindRef(Stone->StoneCategory)) Stone->ConflictFamily = Representative->GetClass();
			Item->bTypedItemReferences = bResolved;
		}
		for (auto& Material : Item->RefinementRecipe.Materials)
		{
			if (Material.bTypedReference) continue;
			Material.ItemTemplate = Resolve(Material.ItemVnum, TEXT("refinement material"));
			Material.bTypedReference = Material.ItemVnum <= 0 || !Material.ItemTemplate.IsNull();
		}
		if (auto* Fish = Cast<UMT2ItemFishTemplate>(Item))
		{
			if (!Fish->Fish.bTypedReferences)
			{
				Fish->Fish.DeadItemTemplate = Resolve(Fish->Fish.DeadItemVnum, TEXT("dead fish"));
				Fish->Fish.GrilledItemTemplate = Resolve(Fish->Fish.GrilledItemVnum, TEXT("grilled fish"));
				Fish->Fish.bTypedReferences = (Fish->Fish.DeadItemVnum <= 0 || !Fish->Fish.DeadItemTemplate.IsNull()) &&
					(Fish->Fish.GrilledItemVnum <= 0 || !Fish->Fish.GrilledItemTemplate.IsNull());
			}
			for (auto& Outcome : Fish->Fish.UseOutcomes)
			{
				if (Outcome.bTypedReference) continue;
				Outcome.ItemTemplate = Resolve(Outcome.ItemVnum, TEXT("fish use reward"));
				Outcome.bTypedReference = Outcome.ItemVnum <= 0 || !Outcome.ItemTemplate.IsNull();
			}
		}
		bAllResolved &= bResolved;
	}
	return bAllResolved;
}
