#pragma once

#include "Items/Templates/MT2ItemSpecialTemplate.h"
#include "MT2ItemSpecialDocumentTemplate.generated.h"

/**
 * Legacy item-proto variant SPECIAL_DOC (ordinal 2).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemSpecialTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemSpecialDocumentTemplate : public UMT2ItemSpecialTemplate
{
	GENERATED_BODY()
};

