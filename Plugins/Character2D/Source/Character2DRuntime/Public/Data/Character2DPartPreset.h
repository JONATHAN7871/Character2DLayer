#pragma once
#include "Character2DBasePreset.h"
#include "Character2DEnums.h"
#include "Character2DPartPreset.generated.h"

UCLASS(BlueprintType)
class CHARACTER2DRUNTIME_API UCharacter2DPartPreset : public UCharacter2DBasePreset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="Part") ECharacter2DEditMode Part = ECharacter2DEditMode::Body;
	UPROPERTY(EditAnywhere, Category="Part") TObjectPtr<USkeletalMesh> Mesh;
};
