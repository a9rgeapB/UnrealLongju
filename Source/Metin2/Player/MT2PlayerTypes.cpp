/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Player/MT2PlayerTypes.h"

FString MT2GetEmpireStartMap(EMT2Empire Empire)
{
	switch (Empire)
	{
	case EMT2Empire::Shinsoo: return TEXT("yongan");
	case EMT2Empire::Chunjo: return TEXT("joan");
	case EMT2Empire::Jinno: return TEXT("pyungmoo");
	default: return FString();
	}
}
