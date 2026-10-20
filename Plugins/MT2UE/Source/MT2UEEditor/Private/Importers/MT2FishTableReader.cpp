#include "Importers/MT2ItemImporter.h"
#include "Misc/FileHelper.h"

bool FMT2ItemImporter::LoadFishTable(const FString& Path,
	TMap<int32, FMT2FishDefinition>& OutDefinitions, FString& OutError)
{
	OutDefinitions.Reset(); OutError.Reset();
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *Path)) { OutError = TEXT("File could not be read."); return false; }
	if (Bytes.Num() >= 2 && ((Bytes[0] == 0xff && Bytes[1] == 0xfe) || (Bytes[0] == 0xfe && Bytes[1] == 0xff)))
	{
		FString Text;
		return FFileHelper::LoadFileToString(Text, *Path) && ParseFishTable(Text, OutDefinitions, OutError);
	}
	// Names are unused here (localized names come from item_proto). Preserve ASCII delimiters from
	// the CP949 source: attempting UTF-8 decoding can swallow a tab following an invalid name byte.
	FString Text;
	Text.Reserve(Bytes.Num());
	const int32 Start = Bytes.Num() >= 3 && Bytes[0] == 0xef && Bytes[1] == 0xbb && Bytes[2] == 0xbf ? 3 : 0;
	for (int32 Index = Start; Index < Bytes.Num(); ++Index) Text.AppendChar(Bytes[Index] < 128 ? TCHAR(Bytes[Index]) : TEXT('?'));
	return ParseFishTable(Text, OutDefinitions, OutError);
}

bool FMT2ItemImporter::ParseFishTable(const FString& Text,
	TMap<int32, FMT2FishDefinition>& OutDefinitions, FString& OutError)
{
	OutDefinitions.Reset(); OutError.Reset();
	TMap<int32, FMT2FishDefinition> Candidate;
	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines, false);
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		const FString& Line = Lines[Index];
		if (Line.TrimStartAndEnd().IsEmpty() || Line.TrimStart().StartsWith(TEXT("#"))) continue;
		TArray<FString> Fields;
		Line.ParseIntoArray(Fields, TEXT("\t"), false);
		// The source's spreadsheet footer has no name/VNUMs, only four catch-weight totals.
		if (Fields.Num() >= 4 && Fields[0].IsEmpty() && Fields[1].IsEmpty() && Fields[2].IsEmpty() && Fields[3].IsEmpty()) continue;
		int32 Live = 0, Dead = 0, Grilled = 0;
		if (Fields.Num() < 23 || !LexTryParseString(Live, *Fields[1]) ||
			!LexTryParseString(Dead, *Fields[2]) || !LexTryParseString(Grilled, *Fields[3]) ||
			Live < 0 || Dead < 0 || Grilled < 0)
		{
			OutError = FString::Printf(TEXT("Malformed fishing row at line %d (%d columns; expected 23 or more)."), Index + 1, Fields.Num()); return false;
		}
		if (Live < 27802 || Live > 27830) continue; // The miss/gold rows are not usable fish.
		FMT2FishDefinition Fish;
		Fish.DeadItemVnum = Dead; Fish.GrilledItemVnum = Live >= 27803 ? Grilled : 0;
		static const int32 Weights[] = {1000, 500, 300, 100, 50, 30, 10, 5, 4, 1};
		for (int32 ResultIndex = 0; ResultIndex < UE_ARRAY_COUNT(Weights); ++ResultIndex)
		{
			int32 Token = 0;
			if (!LexTryParseString(Token, *Fields[13 + ResultIndex]) || Token < 0)
			{
				OutError = FString::Printf(TEXT("Invalid fish use outcome at line %d."), Index + 1); return false;
			}
			auto& Outcome = Fish.UseOutcomes.AddDefaulted_GetRef();
			Outcome.Weight = Weights[ResultIndex];
			// USED_NONE, WATER_STONE and TREASURE_MAP intentionally award nothing in this server.
			Outcome.ItemVnum = Token == 1 ? 27987 : Token == 4 ? 27801 : Token > 4 ? Token : 0;
		}
		auto& Bone = Fish.UseOutcomes.AddDefaulted_GetRef(); Bone.ItemVnum = 27799; Bone.Weight = 2000;
		auto& DeadFish = Fish.UseOutcomes.AddDefaulted_GetRef(); DeadFish.ItemVnum = Dead; DeadFish.Weight = 6000;
		if (!Fish.IsValid() || Candidate.Contains(Live) || (Dead > 0 && (Dead == Live || Candidate.Contains(Dead))))
		{
			OutError = FString::Printf(TEXT("Invalid or duplicate fish definition at line %d."), Index + 1); return false;
		}
		Candidate.Add(Live, Fish);
		if (Dead > 0) Candidate.Add(Dead, Fish);
	}
	if (Candidate.IsEmpty()) { OutError = TEXT("No usable fish definitions found."); return false; }
	OutDefinitions = MoveTemp(Candidate);
	return true;
}
