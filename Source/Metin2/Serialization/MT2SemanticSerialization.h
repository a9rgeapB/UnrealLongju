#pragma once
#include "CoreMinimal.h"

namespace MT2SemanticSerialization
{
	// Leave the input untouched on failure so normal structured text import can handle "(...)".
	inline bool ReadInt32Token(const TCHAR*& Buffer,int32& Out)
	{
		const TCHAR* Start=Buffer;
		while (FChar::IsWhitespace(*Start)) { ++Start; }
		const TCHAR* End=Start;
		if (*End==TEXT('-') || *End==TEXT('+')) { ++End; }
		const TCHAR* Digits=End;
		while (FChar::IsDigit(*End)) { ++End; }
		if (End==Digits || (*End && !FChar::IsWhitespace(*End) && *End!=TEXT(',') && *End!=TEXT(')'))) { return false; }
		int64 Wide=0;
		if (!LexTryParseString(Wide,*FString(static_cast<int32>(End-Start),Start)) || Wide<MIN_int32 || Wide>MAX_int32) { return false; }
		Out=static_cast<int32>(Wide); Buffer=End; return true;
	}
}
