#pragma once
#include "Character2DBasePreset.h"
#include "Character2DPartPreset.h"
#include "Character2DPosePreset.generated.h"

UCLASS(BlueprintType)
class CHARACTER2DRUNTIME_API UCharacter2DPosePreset : public UCharacter2DBasePreset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere) TObjectPtr<UCharacter2DPartPreset> BodyPreset;
	UPROPERTY(EditAnywhere) TObjectPtr<UCharacter2DPartPreset> ArmsPreset;
	UPROPERTY(EditAnywhere) TObjectPtr<UCharacter2DPartPreset> HeadPreset;
};
