// Character2DLayerData.h
#pragma once

#include "CoreMinimal.h"
#include "PaperSprite.h"
#include "Character2DBuilderWindow/Character2DMeshGeneratorOptions.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Character2DLayerData.generated.h"

// ===== Глобальные настройки билд-окна =====
USTRUCT(BlueprintType)
struct FCharacter2DBuilderGlobals
{
	GENERATED_BODY()

	/** Тип выходного меша */
	UPROPERTY(EditAnywhere)
	ECharacter2DMeshOutputType OutputType =
		ECharacter2DMeshOutputType::SkeletalMesh;

	/** Положение корневого пивота */
	UPROPERTY(EditAnywhere)
	ECharacter2DRootBonePlacement PivotPlacement =
		ECharacter2DRootBonePlacement::Origin;
	
	UPROPERTY(EditAnywhere) bool   bShowWireframe = false;
	UPROPERTY(EditAnywhere) float   MeshScale = 1.f;
	UPROPERTY(EditAnywhere) FString AssetName = TEXT("GeneratedMesh");
	UPROPERTY(EditAnywhere) FString SavePath  = TEXT("/Game");
};


/** Один спрайтовый слой */
struct FCharacter2DLayerSlot
{
	TWeakObjectPtr<UPaperSprite> Sprite;
	FVector                      Location;
	bool                         bVisible = true;

	FCharacter2DLayerSlot()
		: Sprite(nullptr), Location(FVector::ZeroVector)
	{}
};

/** Категория слоёв (Body, Arms, Head и т.д.) */
struct FCharacter2DLayerCategory
{
	FName         CategoryName;

	// Параметры генерации для всей категории
	bool          bUseGridMesh   = true;
	int32         GridCellSize   = 32;
	uint8         AlphaThreshold = 64;

	TArray<TSharedPtr<FCharacter2DLayerSlot>> Slots;

	FCharacter2DLayerCategory(FName InName)
		: CategoryName(InName)
	{}
};
