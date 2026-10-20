#include "Importers/MT2ItemImporter.h"

bool FMT2ItemImporter::ParseBlendTable(const FString& Text,
	TMap<int32, FMT2BlendDefinition>& OutDefinitions, FString& OutError)
{
	OutDefinitions.Reset();
	OutError.Reset();
	const TMap<FString, EMT2ItemBonusType> Tokens = {
		{TEXT("CRITICAL_PCT"), EMT2ItemBonusType::CriticalChance},
		{TEXT("PENETRATE_PCT"), EMT2ItemBonusType::PenetratingChance},
		{TEXT("ATTACK_SPEED"), EMT2ItemBonusType::AttackSpeed},
		{TEXT("RESIST_MAGIC"), EMT2ItemBonusType::MagicResistance},
		{TEXT("ATT_BONUS"), EMT2ItemBonusType::AttackValue},
		{TEXT("DEF_BONUS"), EMT2ItemBonusType::DefenseBonus},
		{TEXT("ENERGY"), EMT2ItemBonusType::Energy}
	};
	TMap<int32, FMT2BlendDefinition> Parsed;
	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines, false);
	FMT2BlendDefinition Current;
	int32 Vnum = 0;
	bool bInSection = false;
	TSet<FString> Seen;
	for (int32 LineIndex = 0; LineIndex < Lines.Num(); ++LineIndex)
	{
		FString Line = Lines[LineIndex];
		int32 Comment;
		if (Line.FindChar(TEXT('#'), Comment)) Line.LeftInline(Comment);
		TArray<FString> Fields;
		Line.ParseIntoArrayWS(Fields);
		if (Fields.IsEmpty()) continue;
		const FString Key = Fields[0].ToLower();
		auto Fail = [&](const FString& Message)
		{
			OutError = FString::Printf(TEXT("Line %d: %s"), LineIndex + 1, *Message);
			return false;
		};
		if (Key == TEXT("section"))
		{
			if (bInSection || Fields.Num() != 1) return Fail(TEXT("Unexpected section."));
			bInSection = true; Vnum = 0; Current = {}; Seen.Reset();
			continue;
		}
		if (!bInSection) return Fail(TEXT("Entry outside section."));
		if (Key == TEXT("end"))
		{
			// The original energy crystal uses a distinct distribution for both independent rolls.
			if (Vnum == 51002) Current.Weights = {5, 10, 45, 25, 15};
			if (Fields.Num() != 1 || Vnum <= 0 || !Current.IsValid() || Parsed.Contains(Vnum))
				return Fail(TEXT("Incomplete, invalid, or duplicate blend definition."));
			Parsed.Add(Vnum, Current); bInSection = false;
			continue;
		}
		if (Seen.Contains(Key)) return Fail(TEXT("Duplicate field."));
		Seen.Add(Key);
		if (Key == TEXT("item_vnum"))
		{
			if (Fields.Num() != 2 || !LexTryParseString(Vnum, *Fields[1]) || Vnum <= 0)
				return Fail(TEXT("Invalid item_vnum."));
		}
		else if (Key == TEXT("apply_type"))
		{
			if (Fields.Num() != 2) return Fail(TEXT("Expected one apply_type."));
			if (const auto* Type = Tokens.Find(Fields[1].ToUpper())) Current.BonusType = *Type;
			else
			{
				int32 Raw;
				if (!LexTryParseString(Raw, *Fields[1]) || !MT2ItemSemanticTypes::TryDecodeBonus(Raw, Current.BonusType))
					return Fail(FString::Printf(TEXT("Unsupported apply_type %s."), *Fields[1]));
			}
		}
		else if (Key == TEXT("apply_value") || Key == TEXT("apply_duration"))
		{
			if (Fields.Num() != 6) return Fail(TEXT("Expected five roll values."));
			auto& Values = Key == TEXT("apply_value") ? Current.Magnitudes : Current.Durations;
			for (int32 Index = 1; Index < Fields.Num(); ++Index)
			{
				int32 Value;
				if (!LexTryParseString(Value, *Fields[Index]) || Value <= 0) return Fail(TEXT("Roll values must be positive integers."));
				Values.Add(Value);
			}
		}
		else return Fail(FString::Printf(TEXT("Unexpected field %s."), *Key));
	}
	if (bInSection || Parsed.IsEmpty())
	{
		OutError = TEXT("Unterminated or empty blend table.");
		return false;
	}
	OutDefinitions = MoveTemp(Parsed);
	return true;
}
