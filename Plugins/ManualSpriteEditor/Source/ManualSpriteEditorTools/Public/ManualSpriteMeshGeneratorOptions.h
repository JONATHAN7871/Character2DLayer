#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ManualSpriteMeshGeneratorOptions.generated.h"

UENUM(BlueprintType)
enum class EManualSpriteMeshType : uint8
{
    StaticMesh   UMETA(DisplayName = "Static Mesh"),
    SkeletalMesh UMETA(DisplayName = "Skeletal Mesh")
};

UENUM(BlueprintType)
enum class EManualSpritePivotPlacement : uint8
{
    Origin       UMETA(DisplayName = "World Origin"),
    Center       UMETA(DisplayName = "Center"),
    BottomCenter UMETA(DisplayName = "Bottom Center"),
    Custom       UMETA(DisplayName = "Custom Offset")
};

/**
 * Настройки генератора мешей для Manual Sprite
 */
UCLASS(config = EditorPerProjectUserSettings, DefaultConfig, DisplayName = "Manual Sprite Mesh Generator")
class MANUALSPRITEEDITORTOOLS_API UManualSpriteMeshGeneratorOptions : public UObject
{
    GENERATED_BODY()

public:
    /** Тип генерируемого меша */
    UPROPERTY(EditAnywhere, Config, Category = "General")
    EManualSpriteMeshType MeshType = EManualSpriteMeshType::StaticMesh;

    /** Размещение пивота */
    UPROPERTY(EditAnywhere, Config, Category = "Transform")
    EManualSpritePivotPlacement PivotPlacement = EManualSpritePivotPlacement::Center;

    /** Кастомное смещение пивота (если выбран Custom) */
    UPROPERTY(EditAnywhere, Config, Category = "Transform", 
             meta = (EditCondition = "PivotPlacement == EManualSpritePivotPlacement::Custom"))
    FVector CustomPivotOffset = FVector::ZeroVector;

    /** Масштаб меша */
    UPROPERTY(EditAnywhere, Config, Category = "Transform", 
             meta = (ClampMin = "0.001", UIMin = "0.001", UIMax = "10.0"))
    float MeshScale = 1.0f;

    /** Дополнительное смещение всего меша */
    UPROPERTY(EditAnywhere, Config, Category = "Transform")
    FVector MeshOffset = FVector::ZeroVector;

    /** Путь для сохранения ассетов */
    UPROPERTY(EditAnywhere, Config, Category = "Save", meta = (ContentDir))
    FDirectoryPath SavePath = {"/Game/GeneratedMeshes"};

    /** Базовое имя ассета */
    UPROPERTY(EditAnywhere, Config, Category = "Save")
    FString AssetBaseName = TEXT("ManualSpriteMesh");

    /** Создавать ли материал автоматически */
    UPROPERTY(EditAnywhere, Config, Category = "Material")
    bool bCreateMaterial = true;

    /** Тип создаваемого материала */
    UPROPERTY(EditAnywhere, Config, Category = "Material",
             meta = (EditCondition = "bCreateMaterial"))
    bool bCreateUnlitMaterial = true;

    /** Двусторонний материал */
    UPROPERTY(EditAnywhere, Config, Category = "Material",
             meta = (EditCondition = "bCreateMaterial"))
    bool bTwoSidedMaterial = true;
};