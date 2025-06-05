// Character2DMeshGeneratorOptions.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Character2DMeshGeneratorOptions.generated.h"

UENUM(BlueprintType)
enum class ECharacter2DMeshOutputType : uint8
{
	SkeletalMesh UMETA(DisplayName = "Skeletal Mesh"),
	StaticMesh   UMETA(DisplayName = "Static Mesh")
};

UENUM(BlueprintType)
enum class ECharacter2DRootBonePlacement : uint8
{
	Origin       UMETA(DisplayName = "World Origin"),
	BottomCenter UMETA(DisplayName = "Bottom Center"),
	Center       UMETA(DisplayName = "Center")
};

/**
 * Хранит глобальные настройки генератора (сохраняются в конфиге проекта).
 */
UCLASS(config = EditorPerProjectUserSettings, DefaultConfig, DisplayName = "2D Mesh Generator Settings")
class CHARACTER2DEDITOR_API UCharacter2DMeshGeneratorOptions : public UObject
{
	GENERATED_BODY()

public:
	/** Тип выходного ассета */
	UPROPERTY(EditAnywhere, Config, Category = "General")
	ECharacter2DMeshOutputType OutputType = ECharacter2DMeshOutputType::SkeletalMesh;

	/** Путь для сохранения ассетов (Content Dir) */
	UPROPERTY(EditAnywhere, Config, Category = "Save", meta = (ContentDir))
	FDirectoryPath SavePath = {"/Game/Character2DBuilder/Generated"};

	/** Имя создаваемого ассета */
	UPROPERTY(EditAnywhere, Config, Category = "Save")
	FString AssetName = TEXT("GeneratedMesh");

	/** Пивот/Root Placement */
	UPROPERTY(EditAnywhere, Config, Category = "Pivot")
	ECharacter2DRootBonePlacement PivotPlacement = ECharacter2DRootBonePlacement::Origin;

	/** Глобальный масштаб меша (1 UU = 1 см) */
	UPROPERTY(EditAnywhere, Config, Category = "Transform", meta = (ClampMin = "0.0001", UIMin = "0.0001"))
	float MeshScale = 1.0f;
};
