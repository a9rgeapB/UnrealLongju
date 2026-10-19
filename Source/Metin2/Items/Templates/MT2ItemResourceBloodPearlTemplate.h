#pragma once

#include "Items/Templates/MT2ItemResourceTemplate.h"
#include "MT2ItemResourceBloodPearlTemplate.generated.h"

/**
 * Legacy item-proto variant RESOURCE_BLOOD_PEARL (ordinal 3).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemResourceTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemResourceBloodPearlTemplate : public UMT2ItemResourceTemplate
{
	GENERATED_BODY()
};

