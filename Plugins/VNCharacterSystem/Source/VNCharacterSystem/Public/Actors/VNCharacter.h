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
#include "Data/VNCharacterIdleAnimationStructs.h"
#include "VNCharacter.generated.h"

class UVNCharacterAnimationManager;
class UVNCharacterIdleAnimationManager;
class UVNCharacterDataAsset;
class UVNCharacterIdleAnimationDataAsset;
class UMaterialInterface;
class UMaterialInstanceDynamic;

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
	// Основные компоненты
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VN Character")
	class UVNCharacterAnimationManager* AnimationManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VN Character")
	class UVNCharacterIdleAnimationManager* IdleAnimationManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* CharacterRoot;

	// Skeletal Mesh компоненты
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

	// Sprite компоненты
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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade") class UPaperSpriteComponent* Head_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade") class UPaperSpriteComponent* Eyebrow_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade") class UPaperSpriteComponent* Eyes_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade") class UPaperSpriteComponent* Eyelids_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade") class UPaperSpriteComponent* Wink_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade") class UPaperSpriteComponent* Mouth_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade") class UPaperSpriteComponent* EmotionHeadEffect01_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade") class UPaperSpriteComponent* EmotionHeadEffect02_Sprite_Fade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade") class UPaperSpriteComponent* EmotionHeadEffect03_Sprite_Fade;
	
public:
	// Основное API
	UFUNCTION(BlueprintCallable, Category = "VN Character|Skeletal") void SetSkeletalMesh(E_VN_ComponentID_Skeletal ComponentID, TSoftObjectPtr<USkeletalMesh> SkeletalMesh, bool bAnimate = true, float Duration = 1.0f);
	UFUNCTION(BlueprintCallable, Category = "VN Character|Sprites") void SetSprite(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> Sprite, bool bAnimate = true, float Duration = 0.5f);
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

	const TSet<TObjectPtr<USceneComponent>>& GetFadingInComponents() const { return FadingInComponents; }
	const TSet<TObjectPtr<USceneComponent>>& GetFadingOutComponents() const { return FadingOutComponents; }

	// Система кэширования спрайтов для idle анимаций
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Sprite Cache")
	TSoftObjectPtr<UPaperSprite> GetCachedSprite(E_VN_ComponentID_Sprite ComponentID) const;
	
	UFUNCTION(BlueprintCallable, Category = "VN Character|Sprite Cache")
	void SetCachedSprite(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> Sprite);
	
	UFUNCTION(BlueprintCallable, Category = "VN Character|Sprite Cache")
	void UpdateSpriteCache();
	
	UFUNCTION(BlueprintCallable, Category = "VN Character|Sprite Cache")
	void RestoreSpriteFromCache(E_VN_ComponentID_Sprite ComponentID);

	// Idle анимации - основное API
	UFUNCTION(BlueprintCallable, Category = "VN Character|Idle Animations") void SetBlinkEnabled(bool bEnable);
	UFUNCTION(BlueprintCallable, Category = "VN Character|Idle Animations") void SetTalkEnabled(bool bEnable);
	UFUNCTION(BlueprintCallable, Category = "VN Character|Idle Animations") void SetEyesRandomEnabled(bool bEnable);
	UFUNCTION(BlueprintCallable, Category = "VN Character|Idle Animations") void SetIdleAnimationsConfig(const FVNIdleAnimationsConfig& NewConfig);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Idle Animations") const FVNIdleAnimationsConfig& GetIdleAnimationsConfig() const;
	UFUNCTION(BlueprintCallable, Category = "VN Character|Idle Animations") void StopAllIdleAnimations();
	UFUNCTION(BlueprintCallable, Category = "VN Character|Idle Animations") void StartAllIdleAnimations();
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Idle Animations") bool IsBlinkActive() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Idle Animations") bool IsTalkActive() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Idle Animations") bool IsEyesRandomActive() const;

	// Idle анимации - расширенное API
	UFUNCTION(BlueprintCallable, Category = "VN Character|Advanced Idle")
	void SetupLivelyBlinking(UPaperFlipbook* BlinkFlipbook, float BaseMinInterval = 2.0f, float BaseMaxInterval = 5.0f, 
		float EmotionalVariation = 1.0f, float BlinkDuration = 0.15f, float DoubleBlinkChance = 0.3f);
	
	UFUNCTION(BlueprintCallable, Category = "VN Character|Advanced Idle")
	void SetIdleEmotionalState(EIdleEmotionalState EmotionState);
	
	UFUNCTION(BlueprintCallable, Category = "VN Character|Idle Animations")
	void ApplyDataAssetWithIdleAnimations(class UVNCharacterIdleAnimationDataAsset* IdleAnimationData, bool bAnimate = true, float Duration = 1.0f);
	
	UFUNCTION(BlueprintCallable, Category = "VN Character|Idle Animations")
	void ApplyIdleAnimationDataAssetSmooth(class UVNCharacterIdleAnimationDataAsset* IdleAnimationData, float DelayBeforeRestart = 0.5f);

	UFUNCTION(BlueprintCallable, Category = "VN Character|Advanced Idle")
	void ApplyIdleAnimationDataAssetWithEmotionalState(class UVNCharacterIdleAnimationDataAsset* IdleAnimationData, EIdleEmotionalState EmotionState, bool bRestartAnimations = true);
	
	// C++ только методы
	void ApplyIdleAnimationDataAsset(UVNCharacterIdleAnimationDataAsset* IdleData, bool bRestartAnimations = true);
	void ApplyDataAssetWithIdleSupport(UVNCharacterDataAsset* CharacterData, bool bAnimate = true, float Duration = 1.0f);
	void ConfigureBlinkAnimation(UPaperFlipbook* BlinkFlipbook, bool bEnabled, float MinInterval, float MaxInterval, float Duration, float DoubleBlinkChance);
	void ConfigureTalkAnimation(UPaperFlipbook* TalkFlipbook, bool bEnabled, float TalkSpeed);
	void ConfigureEyesAnimation(UPaperFlipbook* EyesFlipbook, bool bEnabled, float MinLookDuration, float MaxLookDuration, float MinWaitDuration, float MaxWaitDuration);
	void RestoreComponentStates();
	bool ValidateComponentStates() const;
	FString GetComponentStatusReport() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Debug")
	FString GetSpritesStatusReport() const;

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
	// Кэшированные спрайты для idle анимаций
	UPROPERTY(Transient) TSoftObjectPtr<UPaperSprite> CachedEyesSprite;
	UPROPERTY(Transient) TSoftObjectPtr<UPaperSprite> CachedMouthSprite;
	UPROPERTY(Transient) TSoftObjectPtr<UPaperSprite> CachedEyebrowSprite;
	UPROPERTY(Transient) TSoftObjectPtr<UPaperSprite> CachedEyelidsSprite;
	UPROPERTY(Transient) TSoftObjectPtr<UPaperSprite> CachedWinkSprite;

	// Анимационные списки и данные
	TSet<TObjectPtr<USceneComponent>> FadingInComponents;
	TSet<TObjectPtr<USceneComponent>> FadingOutComponents;
	TMap<TObjectPtr<USceneComponent>, float> ComponentAnimationAlphas;
	TMap<TObjectPtr<USceneComponent>, float> ComponentTargetAlphas;
	
	FTimerHandle CommitTransitionTimerHandle;
	float PendingTransitionDuration = 0.0f;
	
	// === НОВЫЕ МЕТОДЫ ДЛЯ УЛУЧШЕННОГО КЭШИРОВАНИЯ ===
	
	/**
	 * Умное обновление кэша для компонента
	 * Учитывает состояние idle анимаций при обновлении кэша
	 * @param ComponentID ID компонента
	 * @param NewSprite Новый спрайт для кэширования
	 */
	void UpdateCacheForComponent(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> NewSprite);
	
	/**
	 * Проверить, участвует ли компонент в активной idle анимации
	 * @param ComponentID ID компонента для проверки
	 * @return true если компонент сейчас анимируется idle системой
	 */
	bool IsComponentInActiveIdleAnimation(E_VN_ComponentID_Sprite ComponentID) const;
	
	/**
	 * Уведомить IdleAnimationManager об изменении спрайта
	 * @param ComponentID ID изменившегося компонента
	 * @param NewSprite Новый спрайт
	 */
	void NotifyIdleManagerAboutSpriteChange(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> NewSprite);
	
	// Вспомогательные методы
	void CacheSpriteOnSet(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> Sprite);
	void FinalizeCurrentTransition();
	void RequestTransitionCommit(float Duration);
	void CommitTransitions();
	void CreateComponents();
	void SetupComponentHierarchy();
	void ResetComponentAttachmentToDefault(USceneComponent* ComponentToReset);
	bool IsChildOfHeadSprite(USceneComponent* Component) const;
	void UpdateComponentTransform(USceneComponent* Component, const FVector& LocalOffset, float LocalScale);
	void ValidateAndSetupSkeletalComponent(USkeletalMeshComponent* Component, TSoftObjectPtr<USkeletalMesh> SkeletalMesh);
	void ValidateAndSetupSpriteComponent(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite> Sprite);
	void ValidateAndSetupSkeletalComponentSilent(USkeletalMeshComponent* Component, TSoftObjectPtr<USkeletalMesh> SkeletalMesh);
	void ValidateAndSetupSpriteComponentSilent(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite> Sprite);
	void SetAnimationAlpha(USceneComponent* Component, float Alpha);
	void SetTargetAlpha(USceneComponent* Component, float TargetAlpha);
	float GetAnimationAlpha(USceneComponent* Component) const;
	float GetTargetAlpha(USceneComponent* Component) const;
	void ClearAnimationAlphas(USceneComponent* Component);
	void PrepareSkeletalTransition(USkeletalMeshComponent* MainComponent, USkeletalMeshComponent* FadeComponent, TSoftObjectPtr<USkeletalMesh> NewMesh);
	void PrepareSpriteTransition(UPaperSpriteComponent* MainComponent, UPaperSpriteComponent* FadeComponent, TSoftObjectPtr<UPaperSprite> NewSprite);
	void CopySkeletalComponentSettings(USkeletalMeshComponent* Source, USkeletalMeshComponent* Target);
	void CopySpriteComponentSettings(UPaperSpriteComponent* Source, UPaperSpriteComponent* Target);
	void SetComponentAlpha(USceneComponent* Component, float Alpha);
	void SetComponentColor(USceneComponent* Component, const FLinearColor& Color);
	TArray<USceneComponent*> GetAllMainComponents() const;
	TArray<USceneComponent*> GetAllFadeComponents() const;
	void HideAllFadeComponents();
	bool IsChildOfHeadSprite(E_VN_ComponentID_Sprite ComponentID) const;

	void ProcessSkeletalComponentChange(E_VN_ComponentID_Skeletal ID, TSoftObjectPtr<USkeletalMesh> NewMesh, bool bAnimate);
    void ProcessSpriteComponentChange(E_VN_ComponentID_Sprite ID, TSoftObjectPtr<UPaperSprite> NewSprite, bool bAnimate);
	void ApplySkeletalConfigProperties(E_VN_ComponentID_Skeletal ID, const F_VN_SkeletalConfig_Body& Config);
	void ApplySkeletalConfigProperties(E_VN_ComponentID_Skeletal ID, const F_VN_SkeletalConfig_Attachment& Config);
	void ApplySpriteConfigProperties(E_VN_ComponentID_Sprite ID, const F_VN_SpriteConfig_Attachment& Config);
	void ApplySpriteConfigProperties(E_VN_ComponentID_Sprite ID, const F_VN_SpriteConfig_Simple& Config);
	
	USkeletalMeshComponent* GetSkeletalComponentBySpriteTarget(E_SpriteAttachmentTarget Target);
	UFUNCTION() void OnAnimationStarted(EVNAnimationType AnimationType);
	UFUNCTION() void OnAnimationFinished(EVNAnimationType AnimationType);
	UFUNCTION() void OnAnimationProgress(EVNAnimationType AnimationType, float Progress);
	
	void SynchronizeIdleAnimationStates();
	void ApplyFocusStateImmediate();
	void ApplyVisibilityStateImmediate(bool bShouldBeVisible);

public:
	UPROPERTY(BlueprintAssignable, Category = "VN Character Events") FOnVNCharacterFocusChanged OnCharacterFocusChanged;
	UPROPERTY(BlueprintAssignable, Category = "VN Character Events") FOnVNCharacterVisibilityChanged OnCharacterVisibilityChanged;
	UPROPERTY(BlueprintAssignable, Category = "VN Character Events") FOnVNCharacterComponentChanged OnCharacterComponentChanged;

	friend class UVNCharacterAnimationManager;
};