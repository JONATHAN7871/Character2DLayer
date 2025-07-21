#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Components/SceneComponent.h"
#include "Data/VNCharacterEnums.h"
#include "Data/VNCharacterTypes.h"
#include "Data/VNCharacterDataAssetStructs.h"
#include "VNCharacter.generated.h"

// Forward declarations
class UVNCharacterAnimationManager;
class UVNCharacterDataAsset;
class UMaterialInterface;
class UMaterialInstanceDynamic;

// Делегаты для событий персонажа
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVNCharacterFocusChanged, bool, bIsInFocus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVNCharacterVisibilityChanged, bool, bIsVisible);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVNCharacterComponentChanged, E_VN_ComponentID_Sprite, ComponentID);

UCLASS(BlueprintType, Blueprintable, ClassGroup=(VNCharacter))
class VNCHARACTERSYSTEM_API AVNCharacter : public AActor
{
	GENERATED_BODY()

public:
	AVNCharacter();

protected:
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	// =====================================================
	// ОСНОВНЫЕ КОМПОНЕНТЫ
	// =====================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VN Character")
	class UVNCharacterAnimationManager* AnimationManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* CharacterRoot;

	// =====================================================
	// SKELETAL MESH КОМПОНЕНТЫ
	// =====================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes") USkeletalMeshComponent* Body_Skeletal;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes") USkeletalMeshComponent* Arms_Skeletal;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes") USkeletalMeshComponent* Head_Skeletal;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes") USkeletalMeshComponent* Custom01_Skeletal;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes") USkeletalMeshComponent* Custom02_Skeletal;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes") USkeletalMeshComponent* Custom03_Skeletal;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes Fade") USkeletalMeshComponent* Body_Skeletal_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes Fade") USkeletalMeshComponent* Arms_Skeletal_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes Fade") USkeletalMeshComponent* Head_Skeletal_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes Fade") USkeletalMeshComponent* Custom01_Skeletal_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes Fade") USkeletalMeshComponent* Custom02_Skeletal_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes Fade") USkeletalMeshComponent* Custom03_Skeletal_Fade;

	// =====================================================
	// SPRITE КОМПОНЕНТЫ
	// =====================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body") class UPaperSpriteComponent* Body_Sprite;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body") class UPaperSpriteComponent* Arms_Sprite;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body") class UPaperSpriteComponent* BodyShadow_Sprite;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body") class UPaperSpriteComponent* EmotionBodyEffect01_Sprite;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body") class UPaperSpriteComponent* EmotionBodyEffect02_Sprite;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body") class UPaperSpriteComponent* EmotionBodyEffect03_Sprite;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body Fade") class UPaperSpriteComponent* Body_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body Fade") class UPaperSpriteComponent* Arms_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body Fade") class UPaperSpriteComponent* BodyShadow_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body Fade") class UPaperSpriteComponent* EmotionBodyEffect01_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body Fade") class UPaperSpriteComponent* EmotionBodyEffect02_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body Fade") class UPaperSpriteComponent* EmotionBodyEffect03_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head") class UPaperSpriteComponent* Head_Sprite;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head") class UPaperSpriteComponent* Eyebrow_Sprite;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head") class UPaperSpriteComponent* Eyes_Sprite;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head") class UPaperSpriteComponent* Eyelids_Sprite;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head") class UPaperSpriteComponent* Wink_Sprite;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head") class UPaperSpriteComponent* Mouth_Sprite;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head") class UPaperSpriteComponent* EmotionHeadEffect01_Sprite;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head") class UPaperSpriteComponent* EmotionHeadEffect02_Sprite;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head") class UPaperSpriteComponent* EmotionHeadEffect03_Sprite;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade") class UPaperSpriteComponent* Eyebrow_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade") class UPaperSpriteComponent* Eyes_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade") class UPaperSpriteComponent* Eyelids_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade") class UPaperSpriteComponent* Wink_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade") class UPaperSpriteComponent* Mouth_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade") class UPaperSpriteComponent* EmotionHeadEffect01_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade") class UPaperSpriteComponent* EmotionHeadEffect02_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade") class UPaperSpriteComponent* EmotionHeadEffect03_Sprite_Fade;
	
public:
	// =====================================================
	// API
	// =====================================================
	UFUNCTION(BlueprintCallable, Category = "VN Character|Skeletal") void SetSkeletalMesh(E_VN_ComponentID_Skeletal ComponentID, TSoftObjectPtr<USkeletalMesh> SkeletalMesh, bool bAnimate = true, float Duration = 1.0f);
	UFUNCTION(BlueprintCallable, Category = "VN Character|Sprites") void SetSprite(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> Sprite, bool bAnimate = true, float Duration = 0.5f);
	UFUNCTION(BlueprintCallable, Category = "VN Character|Quick Access") void SetEyes(TSoftObjectPtr<UPaperSprite> EyesSprite, bool bAnimate = true, float Duration = 0.5f);
	UFUNCTION(BlueprintCallable, Category = "VN Character|Quick Access") void SetMouth(TSoftObjectPtr<UPaperSprite> MouthSprite, bool bAnimate = true, float Duration = 0.5f);
	UFUNCTION(BlueprintCallable, Category = "VN Character|Quick Access") void SetEyebrows(TSoftObjectPtr<UPaperSprite> EyebrowSprite, bool bAnimate = true, float Duration = 0.5f);
	UFUNCTION(BlueprintCallable, Category = "VN Character|Quick Access") void SetBody(TSoftObjectPtr<USkeletalMesh> BodyMesh, bool bAnimate = true, float Duration = 1.0f);
	UFUNCTION(BlueprintCallable, Category = "VN Character|Quick Access") void SetArms(TSoftObjectPtr<USkeletalMesh> ArmsMesh, bool bAnimate = true, float Duration = 1.0f);
	UFUNCTION(BlueprintCallable, Category = "VN Character|Quick Access") void SetFace(TSoftObjectPtr<UPaperSprite> EyesSprite, TSoftObjectPtr<UPaperSprite> MouthSprite, TSoftObjectPtr<UPaperSprite> EyebrowSprite, bool bAnimate = true, float Duration = 0.5f);
	UFUNCTION(BlueprintCallable, Category = "VN Character|Data Asset") void ApplyDataAsset(class UVNCharacterDataAsset* CharacterData, bool bAnimate = true, float Duration = 1.0f);
	UFUNCTION(BlueprintCallable, Category = "VN Character") void SetFocus(bool bInFocus, float Duration = 1.0f);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character") bool IsInFocus() const { return bIsInFocus; }
	UFUNCTION(BlueprintCallable, Category = "VN Character") void SkipFocusAnimation();
	UFUNCTION(BlueprintCallable, Category = "VN Character") void Appear(float Duration = 1.0f);
	UFUNCTION(BlueprintCallable, Category = "VN Character") void Disappear(float Duration = 1.0f);
	UFUNCTION(BlueprintCallable, Category = "VN Character") void SkipSpawnDespawnAnimation();
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character") bool IsVisible() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character") bool IsAnimating() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character") FLinearColor GetTargetColorForComponent(USceneComponent* Component) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character") FLinearColor GetBaseColorForComponent(USceneComponent* Component) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Components") USkeletalMeshComponent* GetSkeletalComponent(E_VN_ComponentID_Skeletal ComponentID) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Components") UPaperSpriteComponent* GetSpriteComponent(E_VN_ComponentID_Sprite ComponentID) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Components") USkeletalMeshComponent* GetSkeletalFadeComponent(E_VN_ComponentID_Skeletal ComponentID) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Components") UPaperSpriteComponent* GetSpriteFadeComponent(E_VN_ComponentID_Sprite ComponentID) const;

	/** Получить компоненты, которые должны плавно появляться */
	const TSet<TObjectPtr<USceneComponent>>& GetFadingInComponents() const { return FadingInComponents; }

	/** Получить компоненты, которые должны плавно исчезать */
	const TSet<TObjectPtr<USceneComponent>>& GetFadingOutComponents() const { return FadingOutComponents; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character") FString CharacterName = TEXT("Unnamed Character");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Rendering") FVNCharacterRenderSettings RenderSettings;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Focus") bool bIsInFocus = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Focus") FLinearColor DimColorMultiplier = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Global Transform") FVector GlobalSkeletalOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Global Transform") float GlobalSkeletalScale = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Global Transform") FVector GlobalSpriteOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Global Transform") float GlobalSpriteScale = 1.0f;

private:
	// --- СПИСКИ ДЛЯ ОТСЛЕЖИВАНИЯ АНИМАЦИИ ---
	TSet<TObjectPtr<USceneComponent>> FadingInComponents;
	TSet<TObjectPtr<USceneComponent>> FadingOutComponents;
	
	void CreateComponents();
	void SetupComponentHierarchy();
	void ResetComponentAttachmentToDefault(USceneComponent* ComponentToReset);
	bool IsChildOfHeadSprite(USceneComponent* Component) const;
	void UpdateComponentTransform(USceneComponent* Component, const FVector& LocalOffset, float LocalScale);
	void ValidateAndSetupSkeletalComponent(USkeletalMeshComponent* Component, TSoftObjectPtr<USkeletalMesh> SkeletalMesh);
	void ValidateAndSetupSpriteComponent(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite> Sprite);
	void CopySkeletalComponentSettings(USkeletalMeshComponent* Source, USkeletalMeshComponent* Target);
	void CopySpriteComponentSettings(UPaperSpriteComponent* Source, UPaperSpriteComponent* Target);
	void PrepareSkeletalTransition(USkeletalMeshComponent* MainComponent, USkeletalMeshComponent* FadeComponent, TSoftObjectPtr<USkeletalMesh> NewMesh);
	void PrepareSpriteTransition(UPaperSpriteComponent* MainComponent, UPaperSpriteComponent* FadeComponent, TSoftObjectPtr<UPaperSprite> NewSprite);
	void FinishTransition(USceneComponent* MainComponent, USceneComponent* FadeComponent);
	void SetComponentAlpha(USceneComponent* Component, float Alpha);
	void SetComponentColor(USceneComponent* Component, const FLinearColor& Color);
	TArray<USceneComponent*> GetAllMainComponents() const;
	TArray<USceneComponent*> GetAllFadeComponents() const;
	void HideAllFadeComponents();
	void ApplyIndividualSpriteTransform(UPaperSpriteComponent* SpriteComponent, E_VN_ComponentID_Sprite ComponentID);
	bool IsChildOfHeadSprite(E_VN_ComponentID_Sprite ComponentID) const;
	void ApplyAllComponentConfigurationsFromDataAsset(const UVNCharacterDataAsset* CharacterData, bool bAnimate);
	USkeletalMeshComponent* GetSkeletalComponentBySpriteTarget(E_SpriteAttachmentTarget Target);
	UFUNCTION() void OnAnimationStarted(EVNAnimationType AnimationType);
	UFUNCTION() void OnAnimationFinished(EVNAnimationType AnimationType);
	UFUNCTION() void OnAnimationProgress(EVNAnimationType AnimationType, float Progress);
	
public:
	UPROPERTY(BlueprintAssignable, Category = "VN Character Events") FOnVNCharacterFocusChanged OnCharacterFocusChanged;
	UPROPERTY(BlueprintAssignable, Category = "VN Character Events") FOnVNCharacterVisibilityChanged OnCharacterVisibilityChanged;
	UPROPERTY(BlueprintAssignable, Category = "VN Character Events") FOnVNCharacterComponentChanged OnCharacterComponentChanged;

#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category = "Debug") void PrintDebugInfo();
	UFUNCTION(CallInEditor, Category = "Debug") void ValidateAllComponents();
#endif

	friend class UVNCharacterAnimationManager;
};