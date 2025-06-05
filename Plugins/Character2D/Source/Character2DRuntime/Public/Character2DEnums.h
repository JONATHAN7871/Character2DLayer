#pragma once
#include "Character2DEnums.generated.h"

UENUM(BlueprintType)
enum class ECharacter2DEditMode : uint8
{
	Body  UMETA(DisplayName="Body"),
	Arms  UMETA(DisplayName="Arms"),
	Head  UMETA(DisplayName="Head"),
	Pose  UMETA(DisplayName="Pose")
};
