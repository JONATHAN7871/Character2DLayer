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

// Forward declarations
class UVNCharacterAnimationManager;
class UVNCharacterIdleAnimationManager;
class UVNCharacterDataAsset;
class UVNCharacterIdleAnimationDataAsset;
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VN Character")
	class UVNCharacterIdleAnimationManager* IdleAnimationManager;

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
	// =====================================================
	// ОСНОВНОЕ API
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

	// =====================================================
	// IDLE АНИМАЦИИ - ОСНОВНОЕ API
	// =====================================================

	/**
	 * Включить/выключить анимацию моргания
	 * @param bEnable true для включения, false для выключения
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Idle Animations")
	void SetBlinkEnabled(bool bEnable);

	/**
	 * Включить/выключить анимацию разговора
	 * @param bEnable true для включения, false для выключения
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Idle Animations") 
	void SetTalkEnabled(bool bEnable);

	/**
	 * Включить/выключить анимацию случайных движений глаз
	 * @param bEnable true для включения, false для выключения
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Idle Animations")
	void SetEyesRandomEnabled(bool bEnable);

	/**
	 * Установить конфигурацию idle анимаций
	 * @param NewConfig Новая конфигурация idle анимаций
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Idle Animations")
	void SetIdleAnimationsConfig(const FVNIdleAnimationsConfig& NewConfig);

	/**
	 * Получить текущую конфигурацию idle анимаций
	 * @return Текущая конфигурация idle анимаций
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Idle Animations")
	const FVNIdleAnimationsConfig& GetIdleAnimationsConfig() const;

	/**
	 * Остановить все idle анимации
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Idle Animations")
	void StopAllIdleAnimations();

	/**
	 * Запустить все включенные idle анимации
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Idle Animations") 
	void StartAllIdleAnimations();

	/**
	 * Проверить, активна ли анимация моргания
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Idle Animations")
	bool IsBlinkActive() const;

	/**
	 * Проверить, активна ли анимация разговора
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Idle Animations")
	bool IsTalkActive() const;

	/**
	 * Проверить, активна ли анимация случайных движений глаз
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Idle Animations")
	bool IsEyesRandomActive() const;

	// =====================================================
	// IDLE АНИМАЦИИ - РАСШИРЕННОЕ API
	// =====================================================
	
	/**
	 * Настроить живое моргание с эмоциональными вариациями
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Advanced Idle")
	void SetupLivelyBlinking(
		UPaperFlipbook* BlinkFlipbook,
		float BaseMinInterval = 2.0f,
		float BaseMaxInterval = 5.0f, 
		float EmotionalVariation = 1.0f,
		float BlinkDuration = 0.15f,
		float DoubleBlinkChance = 0.3f
	);
	
	/**
	 * Установить эмоциональное состояние для idle анимаций
	 * @param EmotionState Эмоциональное состояние (None = использовать настройки DataAsset)
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Advanced Idle")
	void SetIdleEmotionalState(EIdleEmotionalState EmotionState);
	
	/**
	 * ИСПРАВЛЕННЫЙ МЕТОД: Применить DataAsset для idle анимаций (теперь принимает UVNCharacterIdleAnimationDataAsset)
	 * @param IdleAnimationData DataAsset с настройками idle анимаций  
	 * @param bAnimate Анимировать переход (не используется, оставлен для совместимости)
	 * @param Duration Длительность (не используется, оставлен для совместимости)
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Idle Animations")
	void ApplyDataAssetWithIdleAnimations(class UVNCharacterIdleAnimationDataAsset* IdleAnimationData, bool bAnimate = true, float Duration = 1.0f);
	
	/**
	 * Применить DataAsset для idle анимаций с плавным переходом
	 * @param IdleAnimationData DataAsset с настройками idle анимаций  
	 * @param DelayBeforeRestart Задержка перед перезапуском анимаций
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Idle Animations")
	void ApplyIdleAnimationDataAssetSmooth(class UVNCharacterIdleAnimationDataAsset* IdleAnimationData, float DelayBeforeRestart = 0.5f);

	/**
	 * Применить idle анимации с эмоциональным состоянием
	 * @param IdleAnimationData DataAsset с базовыми настройками
	 * @param EmotionState Эмоциональное состояние для модификации настроек
	 * @param bRestartAnimations Перезапустить анимации после применения
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Advanced Idle")
	void ApplyIdleAnimationDataAssetWithEmotionalState(class UVNCharacterIdleAnimationDataAsset* IdleAnimationData, EIdleEmotionalState EmotionState, bool bRestartAnimations = true);
	
	// =====================================================
	// IDLE INTEGRATION API (С++, НЕ BLUEPRINT)
	// =====================================================
	
	// === IDLE ANIMATION INTEGRATION ===
	void ApplyIdleAnimationDataAsset(UVNCharacterIdleAnimationDataAsset* IdleData, bool bRestartAnimations = true);
	void ApplyDataAssetWithIdleSupport(UVNCharacterDataAsset* CharacterData, bool bAnimate = true, float Duration = 1.0f);

	// === ANIMATION CONFIGURATION ===
	void ConfigureBlinkAnimation(UPaperFlipbook* BlinkFlipbook, bool bEnabled, float MinInterval, float MaxInterval, float Duration, float DoubleBlinkChance);
	void ConfigureTalkAnimation(UPaperFlipbook* TalkFlipbook, bool bEnabled, float TalkSpeed);
	void ConfigureEyesAnimation(UPaperFlipbook* EyesFlipbook, bool bEnabled, float MinLookDuration, float MaxLookDuration, float MinWaitDuration, float MaxWaitDuration);

	// === STATE MANAGEMENT ===
	void RestoreComponentStates();
	bool ValidateComponentStates() const;
	FString GetComponentStatusReport() const;

	// === DEPRECATED METHODS ===
	
	/**
	 * @deprecated Этот метод устарел. Используйте ApplyIdleAnimationDataAsset с UVNCharacterIdleAnimationDataAsset
	 */
	UE_DEPRECATED(5.0, "Use ApplyIdleAnimationDataAsset with UVNCharacterIdleAnimationDataAsset instead")
	void ApplyIdleAnimationsFromDataAsset(class UVNCharacterDataAsset* CharacterData);

	/**
	 * Получить отчет о состоянии спрайтов
	 */
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
	// --- СПИСКИ ДЛЯ ОТСЛЕЖИВАНИЯ АНИМАЦИИ ---
	TSet<TObjectPtr<USceneComponent>> FadingInComponents;
	TSet<TObjectPtr<USceneComponent>> FadingOutComponents;
	
	// --- НОВАЯ СИСТЕМА УПРАВЛЕНИЯ АЛЬФОЙ ДЛЯ АНИМАЦИИ ---
	/** Карта текущих значений альфы для компонентов в анимации */
	TMap<TObjectPtr<USceneComponent>, float> ComponentAnimationAlphas;
	/** Карта целевых значений альфы для завершения анимации */
	TMap<TObjectPtr<USceneComponent>, float> ComponentTargetAlphas;
	
	// --- СИСТЕМА ГРУППИРОВКИ (BATCHING) ДЛЯ ИНДИВИДУАЛЬНЫХ ПЕРЕХОДОВ ---
	/** Таймер для отложенного запуска переходов */
	FTimerHandle CommitTransitionTimerHandle;
	/** Накопленная длительность для группированных переходов */
	float PendingTransitionDuration = 0.0f;
	
	// --- ОСНОВНЫЕ ФУНКЦИИ УПРАВЛЕНИЯ ПЕРЕХОДАМИ ---
	void FinalizeCurrentTransition();
	void RequestTransitionCommit(float Duration);
	void CommitTransitions();
	
	void CreateComponents();
	void SetupComponentHierarchy();
	void ResetComponentAttachmentToDefault(USceneComponent* ComponentToReset);
	bool IsChildOfHeadSprite(USceneComponent* Component) const;
	void UpdateComponentTransform(USceneComponent* Component, const FVector& LocalOffset, float LocalScale);
	
	// --- ФУНКЦИИ НАСТРОЙКИ КОМПОНЕНТОВ ---
	void ValidateAndSetupSkeletalComponent(USkeletalMeshComponent* Component, TSoftObjectPtr<USkeletalMesh> SkeletalMesh);
	void ValidateAndSetupSpriteComponent(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite> Sprite);
	void ValidateAndSetupSkeletalComponentSilent(USkeletalMeshComponent* Component, TSoftObjectPtr<USkeletalMesh> SkeletalMesh);
	void ValidateAndSetupSpriteComponentSilent(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite> Sprite);
	
	// --- ФУНКЦИИ УПРАВЛЕНИЯ АЛЬФОЙ АНИМАЦИИ ---
	void SetAnimationAlpha(USceneComponent* Component, float Alpha);
	void SetTargetAlpha(USceneComponent* Component, float TargetAlpha);
	float GetAnimationAlpha(USceneComponent* Component) const;
	float GetTargetAlpha(USceneComponent* Component) const;
	void ClearAnimationAlphas(USceneComponent* Component);
	
	// --- ФУНКЦИИ ПОДГОТОВКИ ПЕРЕХОДОВ ---
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

	// --- HELPER-ФУНКЦИИ ДЛЯ DATA ASSET ---
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
	
	// --- IDLE ANIMATION HELPERS ---
	void SynchronizeIdleAnimationStates();
	void ApplyFocusStateImmediate();
	void ApplyVisibilityStateImmediate(bool bShouldBeVisible);

public:
	UPROPERTY(BlueprintAssignable, Category = "VN Character Events") FOnVNCharacterFocusChanged OnCharacterFocusChanged;
	UPROPERTY(BlueprintAssignable, Category = "VN Character Events") FOnVNCharacterVisibilityChanged OnCharacterVisibilityChanged;
	UPROPERTY(BlueprintAssignable, Category = "VN Character Events") FOnVNCharacterComponentChanged OnCharacterComponentChanged;

	friend class UVNCharacterAnimationManager;
};