#pragma once

#include "Items/Templates/MT2ItemResourceTemplate.h"
#include "MT2ItemResourceBluePearlTemplate.generated.h"

/**
 * Legacy item-proto variant RESOURCE_BLUE_PEARL (ordinal 4).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemResourceTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemResourceBluePearlTemplate : public UMT2ItemResourceTemplate
{
	GENERATED_BODY()
};

