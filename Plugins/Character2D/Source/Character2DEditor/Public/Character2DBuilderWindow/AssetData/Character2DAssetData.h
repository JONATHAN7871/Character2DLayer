#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PaperSprite.h"
#include "Character2DLayerData.h"
#include "Character2DAssetData.generated.h"

/* -------- слой --------------- */
USTRUCT(BlueprintType)
struct FCharacter2DLayerSlotData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FName SlotName;
	UPROPERTY(EditAnywhere) TSoftObjectPtr<UPaperSprite> Sprite;
	UPROPERTY(EditAnywhere) FVector Location = FVector::ZeroVector;
	UPROPERTY(EditAnywhere) bool   bVisible  = true;
};

/* -------- категория ---------- */
USTRUCT(BlueprintType)
struct FCharacter2DLayerCategoryData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FName CategoryName;

	// ► новые параметры генерации
	UPROPERTY(EditAnywhere) bool  bUseGridMesh   = true;
	UPROPERTY(EditAnywhere) int32 GridCellSize   = 32;
	UPROPERTY(EditAnywhere) uint8 AlphaThreshold = 64;

	UPROPERTY(EditAnywhere) TArray<FCharacter2DLayerSlotData> Slots;
};

/* -------- asset ---------------- */
UCLASS(BlueprintType)
class CHARACTER2DEDITOR_API UCharacter2DAssetData : public UDataAsset
{
	GENERATED_BODY()

public:
	/* глобальные настройки правой панели */
	UPROPERTY(EditAnywhere) FCharacter2DBuilderGlobals Globals;

	/* список категорий/слоёв */
	UPROPERTY(EditAnywhere) TArray<FCharacter2DLayerCategoryData> Categories;
};
