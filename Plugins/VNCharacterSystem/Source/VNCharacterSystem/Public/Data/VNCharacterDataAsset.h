#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/VNCharacterDataAssetStructs.h"
#include "VNCharacterDataAsset.generated.h"

/**
 * DataAsset для VN персонажа с полными конфигурациями
 */
UCLASS(BlueprintType, Blueprintable)
class VNCHARACTERSYSTEM_API UVNCharacterDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UVNCharacterDataAsset();

	// =====================================================
	// GLOBAL TRANSFORMS (НОВЫЙ РАЗДЕЛ)
	// =====================================================

	/** Если true, применит глобальные трансформации из этого DataAsset к персонажу. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Transforms")
	bool bOverrideGlobalTransforms = false;

	/** Глобальное смещение для всех Skeletal Mesh компонентов. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Transforms", meta = (EditCondition = "bOverrideGlobalTransforms"))
	FVector GlobalSkeletalOffset = FVector::ZeroVector;

	/** Глобальный масштаб для всех Skeletal Mesh компонентов. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Transforms", meta = (EditCondition = "bOverrideGlobalTransforms"))
	float GlobalSkeletalScale = 1.0f;

	/** Глобальное смещение для всех Sprite компонентов. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Transforms", meta = (EditCondition = "bOverrideGlobalTransforms"))
	FVector GlobalSpriteOffset = FVector::ZeroVector;

	/** Глобальный масштаб для всех Sprite компонентов. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Transforms", meta = (EditCondition = "bOverrideGlobalTransforms"))
	float GlobalSpriteScale = 1.0f;

	// =====================================================
	// SKELETAL MESH КОМПОНЕНТЫ С КОНФИГУРАЦИЯМИ
	// =====================================================

	/** Основное тело персонажа */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes")
	F_VN_SkeletalConfig_Body BodyConfig;

	/** Руки персонажа */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes")
	F_VN_SkeletalConfig_Attachment ArmsConfig;

	/** Голова персонажа */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes")
	F_VN_SkeletalConfig_Attachment HeadConfig;

	/** Дополнительный элемент 1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes")
	F_VN_SkeletalConfig_Attachment Custom01Config;

	/** Дополнительный элемент 2 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes")
	F_VN_SkeletalConfig_Attachment Custom02Config;

	/** Дополнительный элемент 3 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes")
	F_VN_SkeletalConfig_Attachment Custom03Config;

	// =====================================================
	// SPRITE КОМПОНЕНТЫ С КОНФИГУРАЦИЯМИ
	// =====================================================

	/** Спрайт тела (с attachment) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Body")
	F_VN_SpriteConfig_Attachment BodySpriteConfig;

	/** Спрайт рук (с attachment) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Body")
	F_VN_SpriteConfig_Attachment ArmsSpriteConfig;

	/** Спрайт тени тела (с attachment) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Body")
	F_VN_SpriteConfig_Attachment BodyShadowSpriteConfig;

	/** Спрайт головы (с attachment) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Head")
	F_VN_SpriteConfig_Attachment HeadSpriteConfig;

	/** Спрайт бровей (простой) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Head")
	F_VN_SpriteConfig_Simple EyebrowSpriteConfig;

	/** Спрайт глаз (простой) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Head")
	F_VN_SpriteConfig_Simple EyesSpriteConfig;

	/** Спрайт век (простой) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Head")
	F_VN_SpriteConfig_Simple EyelidsSpriteConfig;

	/** Спрайт подмигивания (простой) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Head")
	F_VN_SpriteConfig_Simple WinkSpriteConfig;

	/** Спрайт рта (простой) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Head")
	F_VN_SpriteConfig_Simple MouthSpriteConfig;

	/** Эмоциональные эффекты головы (простые) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Effects")
	F_VN_SpriteConfig_Simple EmotionHeadEffect01SpriteConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Effects")
	F_VN_SpriteConfig_Simple EmotionHeadEffect02SpriteConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Effects")
	F_VN_SpriteConfig_Simple EmotionHeadEffect03SpriteConfig;

	/** Эмоциональные эффекты тела (с attachment) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Effects")
	F_VN_SpriteConfig_Attachment EmotionBodyEffect01SpriteConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Effects")
	F_VN_SpriteConfig_Attachment EmotionBodyEffect02SpriteConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites Effects")
	F_VN_SpriteConfig_Attachment EmotionBodyEffect03SpriteConfig;
};