// --- START OF FILE VNCharacter.h ---

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Components/SceneComponent.h"
#include "Animation/AnimInstance.h"
#include "Data/VNCharacterEnums.h"
#include "Data/VNCharacterTypes.h"
#include "Data/VNCharacterDataAssetStructs.h"
#include "Data/VNCharacterIdleAnimationStructs.h"
#include "AnimInstance/VNCharacterAnimInstance.h"
#include "VNCharacter.generated.h"

class UVNCharacterAnimationManager;
class UVNCharacterIdleAnimationManager;
class UVNCharacterDataAsset;
class UVNCharacterIdleAnimationDataAsset;
class UVNCharacterAnimInstance;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * Структура для хранения параметров запроса на спавн, 
 * пока ассеты асинхронно загружаются.
 */
USTRUCT()
struct FVNCharacterSpawnRequestPayload
{
	GENERATED_BODY()

	UPROPERTY()
	FString CharacterName;
	UPROPERTY()
	bool bIsNarrator = false;
	UPROPERTY()
	TSoftObjectPtr<UVNCharacterDataAsset> CharacterDataPtr;
	UPROPERTY()
	TSoftObjectPtr<UVNCharacterIdleAnimationDataAsset> IdleDataPtr;
	UPROPERTY()
	bool bAnimateAsset = false;
	UPROPERTY()
	float AssetDuration = 1.0f;
	UPROPERTY()
	bool bShouldAppear = true;
	UPROPERTY()
	float AppearDuration = 1.0f;
};

// =====================================================
// СОБЫТИЯ И ДЕЛЕГАТЫ
// =====================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVNCharacterFocusChanged, bool, bIsInFocus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVNCharacterVisibilityChanged, bool, bIsVisible);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVNCharacterComponentChanged, E_VN_ComponentID_Sprite, ComponentID);

// События движения
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVNCharacterMovementStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVNCharacterMovementFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVNCharacterMovementProgress, float, Progress);

/**
 * Модульная система 2D layered персонажей для визуальных новелл
 * 
 * Основные возможности:
 * - Skeletal Mesh и Sprite компоненты с плавными переходами
 * - Система фокуса и появления/исчезновения
 * - Idle анимации (моргание, разговор, движения глаз)
 * - Система перемещения с интерполяцией
 * - Управление Animation Blueprint
 * - Кэширование состояний для анимаций
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(VNCharacter))
class VNCHARACTERSYSTEM_API AVNCharacter : public AActor
{
	GENERATED_BODY()

	// =====================================================
	// КОНСТРУКТОРЫ И ОСНОВНЫЕ МЕТОДЫ
	// =====================================================

public:
	AVNCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// =====================================================
	// КОМПОНЕНТЫ ПЕРСОНАЖА (PRIVATE)
	// =====================================================

private:
	/** Менеджер анимаций переходов */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "VN Character", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UVNCharacterAnimationManager> AnimationManager;

	/** Менеджер idle анимаций */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "VN Character", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UVNCharacterIdleAnimationManager> IdleAnimationManager;

	/** Корневой компонент персонажа */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> CharacterRoot;

	// === SKELETAL MESH КОМПОНЕНТЫ ===
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<USkeletalMeshComponent> Body_Skeletal;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<USkeletalMeshComponent> Arms_Skeletal;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<USkeletalMeshComponent> Head_Skeletal;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<USkeletalMeshComponent> Custom01_Skeletal;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<USkeletalMeshComponent> Custom02_Skeletal;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<USkeletalMeshComponent> Custom03_Skeletal;

	// === SKELETAL MESH FADE КОМПОНЕНТЫ ===
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<USkeletalMeshComponent> Body_Skeletal_Fade;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<USkeletalMeshComponent> Arms_Skeletal_Fade;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<USkeletalMeshComponent> Head_Skeletal_Fade;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<USkeletalMeshComponent> Custom01_Skeletal_Fade;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<USkeletalMeshComponent> Custom02_Skeletal_Fade;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<USkeletalMeshComponent> Custom03_Skeletal_Fade;

	// === SPRITE КОМПОНЕНТЫ ТЕЛА ===
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> Body_Sprite;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> Arms_Sprite;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> BodyShadow_Sprite;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> EmotionBodyEffect01_Sprite;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> EmotionBodyEffect02_Sprite;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> EmotionBodyEffect03_Sprite;

	// === SPRITE FADE КОМПОНЕНТЫ ТЕЛА ===
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> Body_Sprite_Fade;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> Arms_Sprite_Fade;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> BodyShadow_Sprite_Fade;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> EmotionBodyEffect01_Sprite_Fade;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> EmotionBodyEffect02_Sprite_Fade;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> EmotionBodyEffect03_Sprite_Fade;

	// === SPRITE КОМПОНЕНТЫ ГОЛОВЫ ===
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> Head_Sprite;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> Eyebrow_Sprite;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> Eyes_Sprite;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> Eyelids_Sprite;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> Wink_Sprite;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> Mouth_Sprite;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> EmotionHeadEffect01_Sprite;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> EmotionHeadEffect02_Sprite;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> EmotionHeadEffect03_Sprite;

	// === SPRITE FADE КОМПОНЕНТЫ ГОЛОВЫ ===
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> Head_Sprite_Fade;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> Eyebrow_Sprite_Fade;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> Eyes_Sprite_Fade;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> Eyelids_Sprite_Fade;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> Wink_Sprite_Fade;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> Mouth_Sprite_Fade;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> EmotionHeadEffect01_Sprite_Fade;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> EmotionHeadEffect02_Sprite_Fade;
	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UPaperSpriteComponent> EmotionHeadEffect03_Sprite_Fade;

	// =====================================================
	// ВНУТРЕННИЕ ДАННЫЕ СИСТЕМЫ (PRIVATE)
	// =====================================================

	// === ДАННЫЕ ДЛЯ ДВИЖЕНИЯ ===
	UPROPERTY(Transient)
	bool bIsMoving = false;
	UPROPERTY(Transient)
	FVector StartLocation;
	UPROPERTY(Transient)
	FVector TargetLocation;
	UPROPERTY(Transient)
	FVector StartScale;
	UPROPERTY(Transient)
	FVector TargetScale;
	UPROPERTY(Transient)
	bool bShouldInterpolateScale = false;
	UPROPERTY(Transient)
	float MovementStartTime = 0.0f;
	UPROPERTY(Transient)
	float MovementDuration = 1.0f;

	// === СИСТЕМА КЭШИРОВАНИЯ ЦВЕТОВ ===
	UPROPERTY(Transient)
	TMap<TObjectPtr<USceneComponent>, FLinearColor> CachedBaseColors;
	UPROPERTY(Transient)
	TMap<TObjectPtr<USceneComponent>, FLinearColor> CachedConfigColors;

	// === КЭШИРОВАННЫЕ СПРАЙТЫ ДЛЯ IDLE АНИМАЦИЙ ===
	UPROPERTY(Transient) 
	TSoftObjectPtr<UPaperSprite> CachedEyesSprite;
	UPROPERTY(Transient) 
	TSoftObjectPtr<UPaperSprite> CachedMouthSprite;
	UPROPERTY(Transient) 
	TSoftObjectPtr<UPaperSprite> CachedEyebrowSprite;
	UPROPERTY(Transient) 
	TSoftObjectPtr<UPaperSprite> CachedEyelidsSprite;
	UPROPERTY(Transient) 
	TSoftObjectPtr<UPaperSprite> CachedWinkSprite;

	// === АНИМАЦИОННЫЕ СПИСКИ И ДАННЫЕ ===
	TSet<TObjectPtr<USceneComponent>> FadingInComponents;
	TSet<TObjectPtr<USceneComponent>> FadingOutComponents;
	TMap<TObjectPtr<USceneComponent>, float> ComponentAnimationAlphas;
	TMap<TObjectPtr<USceneComponent>, float> ComponentTargetAlphas;

	// === СИСТЕМА ГРУППИРОВКИ ПЕРЕХОДОВ ===
	FTimerHandle CommitTransitionTimerHandle;
	float PendingTransitionDuration = 0.0f;

	// === АВТОИНИЦИАЛИЗАЦИЯ ===
	FTimerHandle AutoInitTimerHandle;

	/** Хранит данные для текущего асинхронного запроса на спавн. */
	FVNCharacterSpawnRequestPayload CurrentSpawnRequest;

	// =====================================================
	// НАСТРОЙКИ ПЕРСОНАЖА (PROTECTED)
	// =====================================================

protected:
	/** Имя персонажа */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Settings") 
	FString CharacterName = TEXT("Unnamed Character");

	/** Настройки рендеринга */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Settings") 
	FVNCharacterRenderSettings RenderSettings;

	/** Находится ли персонаж в фокусе */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Focus & Visibility") 
	bool bIsInFocus = true;

	/** Множитель цвета для затемнения вне фокуса */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Focus & Visibility") 
	FLinearColor DimColorMultiplier = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	/** Глобальное смещение для Skeletal компонентов */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Transform") 
	FVector GlobalSkeletalOffset = FVector::ZeroVector;

	/** Глобальный масштаб для Skeletal компонентов */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Transform") 
	float GlobalSkeletalScale = 1.0f;

	/** Глобальное смещение для Sprite компонентов */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Transform") 
	FVector GlobalSpriteOffset = FVector::ZeroVector;

	/** Глобальный масштаб для Sprite компонентов */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Transform") 
	float GlobalSpriteScale = 1.0f;

	// === АВТОМАТИЧЕСКАЯ ИНИЦИАЛИЗАЦИЯ ===

	/**
	 * Вызывается, когда все ассеты для спавна были успешно загружены.
	 */
	void OnAssetsLoadedForSpawn();
	
	/** DataAsset для автоматической инициализации персонажа */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Initialize", 
		meta = (DisplayName = "Character DataAsset"))
	TObjectPtr<UVNCharacterDataAsset> AutoInitCharacterData = nullptr;

	/** DataAsset для автоматической инициализации idle анимаций */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Initialize", 
		meta = (DisplayName = "Idle Animations DataAsset"))
	TObjectPtr<UVNCharacterIdleAnimationDataAsset> AutoInitIdleData = nullptr;

	/** Применять ли DataAsset автоматически при BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Initialize")
	bool bAutoApplyOnBeginPlay = true;

	/** Применять ли анимированно при автоинициализации */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Initialize")
	bool bAutoInitWithAnimation = false;

	/** Длительность анимации при автоинициализации */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Initialize", 
		meta = (EditCondition = "bAutoInitWithAnimation", ClampMin = "0.1", ClampMax = "5.0"))
	float AutoInitAnimationDuration = 1.0f;

	/** Задержка перед автоинициализацией (полезно для синхронизации) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Initialize", 
		meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float AutoInitDelay = 0.0f;

	/** Применять ли idle анимации сразу после инициализации персонажа */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Initialize")
	bool bAutoStartIdleAnimations = true;

	
	// =====================================================
	// ПУБЛИЧНОЕ API - ОСНОВНЫЕ МЕТОДЫ УПРАВЛЕНИЯ
	// =====================================================

public:
    // === НОВАЯ ФУНКЦИЯ ИНИЦИАЛИЗАЦИИ ===
    
	/**
	 * Асинхронно запрашивает инициализацию персонажа. Запускает загрузку ассетов.
	 */
	void RequestSpawn(const FString& NewName, bool bIsNarrator, TSoftObjectPtr<UVNCharacterDataAsset> InCharacterData, TSoftObjectPtr<UVNCharacterIdleAnimationDataAsset> InIdleData, 
						bool bAnimateAsset, float AssetDuration, bool bShouldAppear, float AppearDuration);

	// === ОСНОВНОЕ API - УСТАНОВКА КОНТЕНТА ===
	
	/** Установить Skeletal Mesh для компонента */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Skeletal") 
	void SetSkeletalMesh(E_VN_ComponentID_Skeletal ComponentID, TSoftObjectPtr<USkeletalMesh> SkeletalMesh, bool bAnimate = true, float Duration = 1.0f);

	/** Установить спрайт для компонента */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Sprites") 
	void SetSprite(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> Sprite, bool bAnimate = true, float Duration = 0.5f);

	/** Быстрая установка лица (глаза, рот, брови) */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Quick Access") 
	void SetFace(TSoftObjectPtr<UPaperSprite> EyesSprite, TSoftObjectPtr<UPaperSprite> MouthSprite, TSoftObjectPtr<UPaperSprite> EyebrowSprite, bool bAnimate = true, float Duration = 0.5f);

	/** Применить DataAsset с полной конфигурацией персонажа */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Data Asset") 
	void ApplyDataAsset(UVNCharacterDataAsset* CharacterData, bool bAnimate = true, float Duration = 1.0f);

	// === СИСТЕМА ФОКУСА И ВИДИМОСТИ ===
	
	/** Установить фокус персонажа (яркость/затемнение) */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Focus") 
	void SetFocus(bool bInFocus, float Duration = 1.0f);

	/** Проверить, находится ли персонаж в фокусе */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Focus") 
	bool IsInFocus() const { return bIsInFocus; }

	/** Пропустить анимацию фокуса */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Focus") 
	void SkipFocusAnimation();

	/** Плавно показать персонажа */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Visibility") 
	void Appear(float Duration = 1.0f);

	/** Плавно скрыть персонажа */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Visibility") 
	void Disappear(float Duration = 1.0f);

	/** Проверить, виден ли персонаж */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Visibility") 
	bool IsVisible() const;

	/** Пропустить анимацию появления/исчезновения */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Visibility") 
	void SkipSpawnDespawnAnimation();

	// === УПРАВЛЕНИЕ ANIMATION BLUEPRINT ===
	
	/** Установить AnimBP для конкретного Skeletal компонента */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Animation")
	void SetAnimClass(E_VN_ComponentID_Skeletal ComponentID, TSubclassOf<UAnimInstance> AnimClass);

	/** Получить текущий AnimBP класс компонента */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Animation")
	TSubclassOf<UAnimInstance> GetAnimClass(E_VN_ComponentID_Skeletal ComponentID) const;

	/** Установить VNCharacterAnimInstance для всех Skeletal компонентов */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Animation")
	void SetVNAnimInstanceForAllComponents(TSubclassOf<UVNCharacterAnimInstance> CustomVNCharacterClass = nullptr);

	// === СИСТЕМА ПЕРЕМЕЩЕНИЯ И МАСШТАБИРОВАНИЯ ===
	
	/** Переместить персонажа (телепорт или плавно) */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Movement")
	void MoveTo(bool bTeleport, FVector NewLocation, bool bApplyScale = false, FVector NewScale = FVector(1.0f, 1.0f, 1.0f), float Duration = 1.0f);

	/** Переместить персонажа к другому актору */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Movement")
	void MoveToActor(AActor* TargetActor, bool bTeleport, FVector LocationOffset = FVector::ZeroVector, bool bApplyScale = false, FVector NewScale = FVector(1.0f, 1.0f, 1.0f), float Duration = 1.0f);

	/** Остановить текущее движение */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Movement")
	void StopMovement();

	/** Проверить, движется ли персонаж */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Movement")
	bool IsMoving() const;

	/** Получить прогресс движения (0.0-1.0) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Movement")
	float GetMovementProgress() const;

	// === IDLE АНИМАЦИИ - ОСНОВНОЕ УПРАВЛЕНИЕ ===
	
	/** Включить/выключить анимацию моргания */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Idle Animations") 
	void SetBlinkEnabled(bool bEnable);

	/** Включить/выключить анимацию разговора */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Idle Animations") 
	void SetTalkEnabled(bool bEnable);

	/** Включить/выключить случайные движения глаз */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Idle Animations") 
	void SetEyesRandomEnabled(bool bEnable);

	/** Установить конфигурацию idle анимаций */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Idle Animations") 
	void SetIdleAnimationsConfig(const FVNIdleAnimationsConfig& NewConfig);

	/** Получить текущую конфигурацию idle анимаций */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Idle Animations") 
	const FVNIdleAnimationsConfig& GetIdleAnimationsConfig() const;

	/** Остановить все idle анимации */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Idle Animations") 
	void StopAllIdleAnimations();

	/** Запустить все активные idle анимации */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Idle Animations") 
	void StartAllIdleAnimations();

	// === IDLE АНИМАЦИИ - ПРОВЕРКА СОСТОЯНИЯ ===
	
	/** Проверить, активно ли моргание */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Idle Status") 
	bool IsBlinkActive() const;

	/** Проверить, активна ли анимация разговора */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Idle Status") 
	bool IsTalkActive() const;

	/** Проверить, активны ли движения глаз */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Idle Status") 
	bool IsEyesRandomActive() const;

	// === IDLE АНИМАЦИИ - РАСШИРЕННОЕ API ===
	
	/** Настроить живое моргание с эмоциональными вариациями */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Advanced Idle")
	void SetupLivelyBlinking(UPaperFlipbook* BlinkFlipbook, float BaseMinInterval = 2.0f, float BaseMaxInterval = 5.0f, 
		float EmotionalVariation = 1.0f, float BlinkDuration = 0.15f, float DoubleBlinkChance = 0.3f);

	/** Установить эмоциональное состояние для idle анимаций */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Advanced Idle")
	void SetIdleEmotionalState(EIdleEmotionalState EmotionState);

	/** Применить DataAsset с idle анимациями */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Idle Animations")
	void ApplyDataAssetWithIdleAnimations(UVNCharacterIdleAnimationDataAsset* IdleAnimationData, bool bAnimate = true, float Duration = 1.0f);

	/** Применить idle анимации с задержкой */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Idle Animations")
	void ApplyIdleAnimationDataAssetSmooth(UVNCharacterIdleAnimationDataAsset* IdleAnimationData, float DelayBeforeRestart = 0.5f);

	/** Применить idle анимации с эмоциональным состоянием */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Advanced Idle")
	void ApplyIdleAnimationDataAssetWithEmotionalState(UVNCharacterIdleAnimationDataAsset* IdleAnimationData, EIdleEmotionalState EmotionState, bool bRestartAnimations = true);

	// === СИСТЕМА КЭШИРОВАНИЯ СПРАЙТОВ ===
	
	/** Получить кэшированный спрайт для компонента */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Sprite Cache")
	TSoftObjectPtr<UPaperSprite> GetCachedSprite(E_VN_ComponentID_Sprite ComponentID) const;

	/** Установить кэшированный спрайт для компонента */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Sprite Cache")
	void SetCachedSprite(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> Sprite);

	/** Обновить кэш всех спрайтов */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Sprite Cache")
	void UpdateSpriteCache();

	/** Восстановить спрайт из кэша */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Sprite Cache")
	void RestoreSpriteFromCache(E_VN_ComponentID_Sprite ComponentID);

	// === УПРАВЛЕНИЕ ЦВЕТАМИ ===
	
	/** Установить кастомный цвет для спрайт компонента */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Colors")
	void SetComponentCustomColor(E_VN_ComponentID_Sprite ComponentID, const FLinearColor& CustomColor);

	/** Установить кастомный цвет для Skeletal компонента */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Colors")
	void SetSkeletalComponentCustomColor(E_VN_ComponentID_Skeletal ComponentID, const FLinearColor& CustomColor);

	/** Получить базовый цвет спрайт компонента */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Colors")
	FLinearColor GetComponentBaseColor(E_VN_ComponentID_Sprite ComponentID) const;

	/** Получить базовый цвет Skeletal компонента */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Colors")
	FLinearColor GetSkeletalComponentBaseColor(E_VN_ComponentID_Skeletal ComponentID) const;

	/** Сбросить цвет спрайт компонента к белому */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Colors")
	void ResetComponentColor(E_VN_ComponentID_Sprite ComponentID);

	/** Сбросить цвет Skeletal компонента к белому */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Colors")
	void ResetSkeletalComponentColor(E_VN_ComponentID_Skeletal ComponentID);

	// === ДОСТУП К КОМПОНЕНТАМ ===
	
	/** Получить Skeletal компонент по ID */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Components") 
	USkeletalMeshComponent* GetSkeletalComponent(E_VN_ComponentID_Skeletal ComponentID) const;

	/** Получить Sprite компонент по ID */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Components") 
	UPaperSpriteComponent* GetSpriteComponent(E_VN_ComponentID_Sprite ComponentID) const;

	/** Получить Skeletal Fade компонент по ID */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Components") 
	USkeletalMeshComponent* GetSkeletalFadeComponent(E_VN_ComponentID_Skeletal ComponentID) const;

	/** Получить Sprite Fade компонент по ID */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Components") 
	UPaperSpriteComponent* GetSpriteFadeComponent(E_VN_ComponentID_Sprite ComponentID) const;

	/** Получить менеджер анимаций */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Components") 
	UVNCharacterAnimationManager* GetAnimationManager() const { return AnimationManager; }

	/** Получить менеджер idle анимаций */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Components") 
	UVNCharacterIdleAnimationManager* GetIdleAnimationManager() const { return IdleAnimationManager; }

	// === ИНФОРМАЦИЯ О СОСТОЯНИИ ===
	
	/** Получить имя персонажа. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Status")
	FString GetCharacterName() const { return CharacterName; }

	/** Установить имя персонажа. Важно для спавна и последующего поиска. */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Status")
	void SetCharacterName(const FString& NewName) { CharacterName = NewName; }

	/** Проверить, выполняется ли анимация */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Status") 
	bool IsAnimating() const;

	/** Получить целевой цвет для компонента с учетом фокуса */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Status") 
	FLinearColor GetTargetColorForComponent(USceneComponent* Component) const;

	/** Получить базовый цвет компонента */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Status") 
	FLinearColor GetBaseColorForComponent(USceneComponent* Component) const;

	// === АВТОИНИЦИАЛИЗАЦИЯ ===
	
	/** Применить настройки автоинициализации немедленно */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Auto Init", CallInEditor)
	void ApplyAutoInitSettings();

	/** Очистить все настройки персонажа */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Auto Init", CallInEditor)
	void ClearAllSettings();

	/** Применить только Character DataAsset */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Auto Init")
	void ApplyCharacterDataAssetOnly(bool bAnimate = false, float Duration = 1.0f);

	/** Применить только Idle DataAsset */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Auto Init")
	void ApplyIdleDataAssetOnly(bool bRestartAnimations = true);

	/** Установить DataAsset для автоинициализации */
	UFUNCTION(BlueprintCallable, Category = "VN Character | Auto Init")
	void SetAutoInitDataAssets(UVNCharacterDataAsset* CharacterData, UVNCharacterIdleAnimationDataAsset* IdleData);

	// === ОТЛАДКА И ДИАГНОСТИКА ===
	
	/** Получить отчет о состоянии всех спрайтов */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character | Debug")
	FString GetSpritesStatusReport() const;

	// =====================================================
	// СОБЫТИЯ
	// =====================================================

	/** Событие изменения фокуса */
	UPROPERTY(BlueprintAssignable, Category = "VN Character Events") 
	FOnVNCharacterFocusChanged OnCharacterFocusChanged;

	/** Событие изменения видимости */
	UPROPERTY(BlueprintAssignable, Category = "VN Character Events") 
	FOnVNCharacterVisibilityChanged OnCharacterVisibilityChanged;

	/** Событие изменения компонента */
	UPROPERTY(BlueprintAssignable, Category = "VN Character Events") 
	FOnVNCharacterComponentChanged OnCharacterComponentChanged;

	/** Событие начала движения */
	UPROPERTY(BlueprintAssignable, Category = "VN Character Events")
	FOnVNCharacterMovementStarted OnMovementStarted;

	/** Событие завершения движения */
	UPROPERTY(BlueprintAssignable, Category = "VN Character Events")
	FOnVNCharacterMovementFinished OnMovementFinished;

	/** Событие прогресса движения */
	UPROPERTY(BlueprintAssignable, Category = "VN Character Events")
	FOnVNCharacterMovementProgress OnMovementProgress;

	// =====================================================
	// C++ ТОЛЬКО МЕТОДЫ (НЕ ДОСТУПНЫ В BLUEPRINT)
	// =====================================================

	// === ДОСТУП К ВНУТРЕННИМ ДАННЫМ ===
	const TSet<TObjectPtr<USceneComponent>>& GetFadingInComponents() const { return FadingInComponents; }
	const TSet<TObjectPtr<USceneComponent>>& GetFadingOutComponents() const { return FadingOutComponents; }

	// === IDLE АНИМАЦИИ - C++ API ===
	void ApplyIdleAnimationDataAsset(UVNCharacterIdleAnimationDataAsset* IdleData, bool bRestartAnimations = true);
	void ApplyDataAssetWithIdleSupport(UVNCharacterDataAsset* CharacterData, bool bAnimate = true, float Duration = 1.0f);
	void ConfigureBlinkAnimation(UPaperFlipbook* BlinkFlipbook, bool bEnabled, float MinInterval, float MaxInterval, float Duration, float DoubleBlinkChance);
	void ConfigureTalkAnimation(UPaperFlipbook* TalkFlipbook, bool bEnabled, float TalkSpeed);
	void ConfigureEyesAnimation(UPaperFlipbook* EyesFlipbook, bool bEnabled, float MinLookDuration, float MaxLookDuration, float MinWaitDuration, float MaxWaitDuration);

	// === ДИАГНОСТИКА И ВОССТАНОВЛЕНИЕ ===
	void RestoreComponentStates();
	bool ValidateComponentStates() const;
	FString GetComponentStatusReport() const;

	// === ДРУЖЕСТВЕННЫЕ КЛАССЫ ===
	friend class UVNCharacterAnimationManager;
	friend class UVNCharacterIdleAnimationManager;

	// =====================================================
	// ВНУТРЕННИЕ МЕТОДЫ - НЕ ДЛЯ BLUEPRINT
	// =====================================================

private:
	// === ИНИЦИАЛИЗАЦИЯ И НАСТРОЙКА ===
	void CreateComponents();
	void SetupComponentHierarchy();
	void ResetComponentAttachmentToDefault(USceneComponent* ComponentToReset);
	bool IsChildOfHeadSprite(USceneComponent* Component) const;
	bool IsChildOfHeadSprite(E_VN_ComponentID_Sprite ComponentID) const;

	// === УПРАВЛЕНИЕ ТРАНСФОРМАЦИЯМИ ===
	void UpdateComponentTransform(USceneComponent* Component, const FVector& LocalOffset, float LocalScale);

	// === СИСТЕМА ПЕРЕХОДОВ ===
	void RequestTransitionCommit(float Duration);
	void CommitTransitions();
	void FinalizeCurrentTransition();
	void PrepareSkeletalTransition(USkeletalMeshComponent* MainComponent, USkeletalMeshComponent* FadeComponent, TSoftObjectPtr<USkeletalMesh> NewMesh);
	void PrepareSpriteTransition(UPaperSpriteComponent* MainComponent, UPaperSpriteComponent* FadeComponent, TSoftObjectPtr<UPaperSprite> NewSprite);

	// === НАСТРОЙКА КОМПОНЕНТОВ ===
	void ValidateAndSetupSkeletalComponent(USkeletalMeshComponent* Component, TSoftObjectPtr<USkeletalMesh> SkeletalMesh);
	void ValidateAndSetupSpriteComponent(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite> Sprite);
	void ValidateAndSetupSkeletalComponentSilent(USkeletalMeshComponent* Component, TSoftObjectPtr<USkeletalMesh> SkeletalMesh);
	void ValidateAndSetupSpriteComponentSilent(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite> Sprite);
	void CopySkeletalComponentSettings(USkeletalMeshComponent* Source, USkeletalMeshComponent* Target);
	void CopySpriteComponentSettings(UPaperSpriteComponent* Source, UPaperSpriteComponent* Target);

	// === УПРАВЛЕНИЕ АЛЬФОЙ И ЦВЕТОМ ===
	void SetAnimationAlpha(USceneComponent* Component, float Alpha);
	void SetTargetAlpha(USceneComponent* Component, float TargetAlpha);
	float GetAnimationAlpha(USceneComponent* Component) const;
	float GetTargetAlpha(USceneComponent* Component) const;
	void ClearAnimationAlphas(USceneComponent* Component);
	void SetComponentAlpha(USceneComponent* Component, float Alpha);
	void SetComponentColor(USceneComponent* Component, const FLinearColor& Color);

	// === СИСТЕМА ЦВЕТОВ ===
	void CacheComponentBaseColor(USceneComponent* Component, const FLinearColor& BaseColor);
	FLinearColor GetCachedBaseColor(USceneComponent* Component) const;
	void ApplyComponentColorWithFocus(USceneComponent* Component, bool bForceRefresh = false);
	FLinearColor ApplyFocusToColor(const FLinearColor& BaseColor) const;
	void RefreshAllComponentColors();

	// === ПОЛУЧЕНИЕ КОМПОНЕНТОВ ===
	TArray<USceneComponent*> GetAllMainComponents() const;
	TArray<USceneComponent*> GetAllFadeComponents() const;
	void HideAllFadeComponents();

	// === ОБРАБОТКА DATA ASSET ===
	void ProcessSkeletalComponentChange(E_VN_ComponentID_Skeletal ID, TSoftObjectPtr<USkeletalMesh> NewMesh, bool bAnimate);
	void ProcessSpriteComponentChange(E_VN_ComponentID_Sprite ID, TSoftObjectPtr<UPaperSprite> NewSprite, bool bAnimate);
	void ApplySkeletalConfigProperties(E_VN_ComponentID_Skeletal ID, const F_VN_SkeletalConfig_Body& Config);
	void ApplySkeletalConfigProperties(E_VN_ComponentID_Skeletal ID, const F_VN_SkeletalConfig_Attachment& Config);
	void ApplySpriteConfigProperties(E_VN_ComponentID_Sprite ID, const F_VN_SpriteConfig_Attachment& Config);
	void ApplySpriteConfigProperties(E_VN_ComponentID_Sprite ID, const F_VN_SpriteConfig_Simple& Config);
	USkeletalMeshComponent* GetSkeletalComponentBySpriteTarget(E_SpriteAttachmentTarget Target);

	// === СИСТЕМА ДВИЖЕНИЯ ===
	void UpdateMovement(float DeltaTime);
	void FinishMovement();
	void ApplyMovementInterpolation(float Alpha);

	// === УЛУЧШЕННОЕ КЭШИРОВАНИЕ СПРАЙТОВ ===
	void UpdateCacheForComponent(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> NewSprite);
	bool IsComponentInActiveIdleAnimation(E_VN_ComponentID_Sprite ComponentID) const;
	void NotifyIdleManagerAboutSpriteChange(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> NewSprite);
	void CacheSpriteOnSet(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> Sprite);

	// === IDLE АНИМАЦИИ - ИНТЕГРАЦИЯ ===
	void SynchronizeIdleAnimationStates();
	void ApplyFocusStateImmediate();
	void ApplyVisibilityStateImmediate(bool bShouldBeVisible);

	// === СОБЫТИЯ АНИМАЦИЙ ===
	UFUNCTION() 
	void OnAnimationStarted(EVNAnimationType AnimationType);
	UFUNCTION() 
	void OnAnimationFinished(EVNAnimationType AnimationType);
	UFUNCTION() 
	void OnAnimationProgress(EVNAnimationType AnimationType, float Progress);

	// === АВТОИНИЦИАЛИЗАЦИЯ ===
	void PerformAutoInitialization();
};