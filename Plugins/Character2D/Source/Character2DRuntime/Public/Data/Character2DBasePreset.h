#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Character2DBasePreset.generated.h"

UCLASS(BlueprintType)
class CHARACTER2DRUNTIME_API UCharacter2DBasePreset : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="Preset") FName CharacterId;
	UPROPERTY(EditAnywhere, Category="Preset") FName PresetName;
};
