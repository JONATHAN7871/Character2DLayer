#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/SkeletalMesh.h"
#include "PaperSprite.h"
#include "VNCharacterDataAsset.generated.h"

/**
 * Простой DataAsset для VN персонажа
 * Содержит все компоненты персонажа для быстрого применения
 */
UCLASS(BlueprintType, Blueprintable)
class VNCHARACTERSYSTEM_API UVNCharacterDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UVNCharacterDataAsset();

	// =====================================================
	// SKELETAL MESH КОМПОНЕНТЫ
	// =====================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes")
	TSoftObjectPtr<USkeletalMesh> BodyMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes")
	TSoftObjectPtr<USkeletalMesh> ArmsMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes")
	TSoftObjectPtr<USkeletalMesh> HeadMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes")
	TSoftObjectPtr<USkeletalMesh> Custom01Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes")
	TSoftObjectPtr<USkeletalMesh> Custom02Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes")
	TSoftObjectPtr<USkeletalMesh> Custom03Mesh;

	// =====================================================
	// SPRITE КОМПОНЕНТЫ
	// =====================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Body")
	TSoftObjectPtr<UPaperSprite> BodySprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Body")
	TSoftObjectPtr<UPaperSprite> ArmsSprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Body")
	TSoftObjectPtr<UPaperSprite> BodyShadowSprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Head")
	TSoftObjectPtr<UPaperSprite> HeadSprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Head")
	TSoftObjectPtr<UPaperSprite> EyebrowSprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Head")
	TSoftObjectPtr<UPaperSprite> EyesSprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Head")
	TSoftObjectPtr<UPaperSprite> EyelidsSprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Head")
	TSoftObjectPtr<UPaperSprite> WinkSprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Head")
	TSoftObjectPtr<UPaperSprite> MouthSprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Effects")
	TSoftObjectPtr<UPaperSprite> EmotionHeadEffect01Sprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Effects")
	TSoftObjectPtr<UPaperSprite> EmotionHeadEffect02Sprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Effects")
	TSoftObjectPtr<UPaperSprite> EmotionHeadEffect03Sprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Effects")
	TSoftObjectPtr<UPaperSprite> EmotionBodyEffect01Sprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Effects")
	TSoftObjectPtr<UPaperSprite> EmotionBodyEffect02Sprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Effects")
	TSoftObjectPtr<UPaperSprite> EmotionBodyEffect03Sprite;
};