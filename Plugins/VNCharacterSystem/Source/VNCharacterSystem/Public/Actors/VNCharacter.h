#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Components/SceneComponent.h"
#include "Data/VNCharacterEnums.h"
#include "Data/VNCharacterStructs.h"
#include "Data/VNCharacterTypes.h"
#include "VNCharacter.generated.h"

// Forward declarations
class UVNCharacterAnimationManager;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * Главный актор системы VN персонажей
 * 
 * Обеспечивает:
 * - Управление всеми компонентами (Skeletal Mesh + Sprites)
 * - Применение состояний персонажа
 * - Интеграцию с менеджером анимаций
 * - Систему фокуса и видимости
 * - Валидацию и обработку ошибок
 */

// Делегаты для событий персонажа
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVNCharacterStateChanged, const F_VN_CharacterState&, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVNCharacterFocusChanged, bool, bIsInFocus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVNCharacterVisibilityChanged, bool, bIsVisible);

UCLASS(BlueprintType, Blueprintable, ClassGroup=(VNCharacter))
class VNCHARACTERSYSTEM_API AVNCharacter : public AActor
{
	GENERATED_BODY()

public:
	AVNCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	// =====================================================
	// ОСНОВНЫЕ КОМПОНЕНТЫ
	// =====================================================

	/** Менеджер анимаций */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VN Character")
	class UVNCharacterAnimationManager* AnimationManager;

	/** Корневой трансформ для всех Skeletal Mesh компонентов */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* GlobalSkeletalMeshTransform;

	/** Корневой трансформ для всех Sprite компонентов */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* GlobalSpriteTransform;

	// =====================================================
	// SKELETAL MESH КОМПОНЕНТЫ
	// =====================================================

	/** Основное тело персонажа */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes")
	USkeletalMeshComponent* Body_Skeletal;

	/** Руки персонажа */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes")
	USkeletalMeshComponent* Arms_Skeletal;

	/** Голова персонажа */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes")
	USkeletalMeshComponent* Head_Skeletal;

	/** Дополнительный элемент 1 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes")
	USkeletalMeshComponent* Custom01_Skeletal;

	/** Дополнительный элемент 2 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes")
	USkeletalMeshComponent* Custom02_Skeletal;

	/** Дополнительный элемент 3 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes")
	USkeletalMeshComponent* Custom03_Skeletal;

	// =====================================================
	// SPRITE КОМПОНЕНТЫ
	// =====================================================

	/** Спрайт тела */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites")
	class UPaperSpriteComponent* Body_Sprite;

	/** Спрайт рук */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites")
	class UPaperSpriteComponent* Arms_Sprite;

	/** Спрайт головы */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites")
	class UPaperSpriteComponent* Head_Sprite;

	/** Брови */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites")
	class UPaperSpriteComponent* Eyebrow_Sprite;

	/** Глаза */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites")
	class UPaperSpriteComponent* Eyes_Sprite;

	/** Веки */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites")
	class UPaperSpriteComponent* Eyelids_Sprite;

	/** Подмигивание */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites")
	class UPaperSpriteComponent* Wink_Sprite;

	/** Рот */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites")
	class UPaperSpriteComponent* Mouth_Sprite;

	/** Тень тела */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites")
	class UPaperSpriteComponent* BodyShadow_Sprite;

	/** Эмоциональные эффекты головы */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites")
	class UPaperSpriteComponent* EmotionHead01_Sprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites")
	class UPaperSpriteComponent* EmotionHead02_Sprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites")
	class UPaperSpriteComponent* EmotionHead03_Sprite;

	/** Эмоциональные эффекты тела */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites")
	class UPaperSpriteComponent* EmotionBody01_Sprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites")
	class UPaperSpriteComponent* EmotionBody02_Sprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites")
	class UPaperSpriteComponent* EmotionBody03_Sprite;

public:
	// =====================================================
	// ОСНОВНОЕ API - УПРАВЛЕНИЕ СОСТОЯНИЕМ
	// =====================================================

	/**
	 * Установить новое состояние персонажа
	 * @param NewState Новое состояние для применения
	 * @param TransitionDuration Длительность анимации перехода (0 = мгновенно)
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character")
	void SetCharacterState(const F_VN_CharacterState& NewState, float TransitionDuration = 1.0f);

	/**
	 * Получить текущее состояние персонажа
	 * @return Текущее состояние
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character")
	const F_VN_CharacterState& GetCurrentState() const { return CurrentState; }

	/**
	 * Пропустить анимацию перехода состояния
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character")
	void SkipTransition();

	// =====================================================
	// СИСТЕМА ФОКУСА
	// =====================================================

	/**
	 * Установить фокус персонажа
	 * @param bInFocus true для фокуса, false для потери фокуса
	 * @param Duration Длительность анимации
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character")
	void SetFocus(bool bInFocus, float Duration = 1.0f);

	/**
	 * Проверить, в фокусе ли персонаж
	 * @return true если персонаж в фокусе
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character")
	bool IsInFocus() const { return bIsInFocus; }

	/**
	 * Пропустить анимацию смены фокуса
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character")
	void SkipFocusAnimation();

	// =====================================================
	// СИСТЕМА ВИДИМОСТИ
	// =====================================================

	/**
	 * Появление персонажа с анимацией
	 * @param Duration Длительность анимации появления
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character")
	void Appear(float Duration = 1.0f);

	/**
	 * Исчезновение персонажа с анимацией
	 * @param Duration Длительность анимации исчезновения
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character")
	void Disappear(float Duration = 1.0f);

	/**
	 * Пропустить анимацию появления/исчезновения
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character")
	void SkipSpawnDespawnAnimation();

	/**
	 * Проверить, видим ли персонаж
	 * @return true если персонаж видим
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character")
	bool IsVisible() const;

	// =====================================================
	// УТИЛИТЫ И ИНФОРМАЦИЯ
	// =====================================================

	/**
	 * Проверить, выполняется ли анимация
	 * @return true если выполняется любая анимация
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character")
	bool IsAnimating() const;

	/**
	 * Получить цвет компонента с учетом фокуса
	 * @param Component Компонент для проверки
	 * @return Целевой цвет компонента
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character")
	FLinearColor GetTargetColorForComponent(USceneComponent* Component) const;

	/**
	 * Получить базовый цвет компонента (без учета фокуса)
	 * @param Component Компонент для проверки
	 * @return Базовый цвет из конфигурации
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character")
	FLinearColor GetBaseColorForComponent(USceneComponent* Component) const;

protected:
	// =====================================================
	// НАСТРОЙКИ ПЕРСОНАЖА
	// =====================================================

	/** Текущее состояние персонажа */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character State")
	F_VN_CharacterState CurrentState;

	/** Настройки рендеринга */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Rendering")
	FVNCharacterRenderSettings RenderSettings;

	// =====================================================
	// НАСТРОЙКИ ФОКУСА
	// =====================================================

	/** Находится ли персонаж в фокусе */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Focus")
	bool bIsInFocus = true;

	/** Множитель цвета для состояния вне фокуса */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Focus")
	FLinearColor DimColorMultiplier = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	// =====================================================
	// ГЛОБАЛЬНЫЕ НАСТРОЙКИ ТРАНСФОРМАЦИИ
	// =====================================================

	/** Глобальное смещение для всех Skeletal Mesh компонентов */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Global Transform")
	FVector GlobalSkeletalOffset = FVector::ZeroVector;

	/** Глобальный масштаб для всех Skeletal Mesh компонентов */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Global Transform")
	float GlobalSkeletalScale = 1.0f;

	/** Глобальное смещение для всех Sprite компонентов */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Global Transform")
	FVector GlobalSpriteOffset = FVector::ZeroVector;

	/** Глобальный масштаб для всех Sprite компонентов */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Global Transform")
	float GlobalSpriteScale = 1.0f;

	// =====================================================
	// ДАННЫЕ ДЛЯ АНИМАЦИИ ПЕРЕХОДОВ
	// =====================================================

	/** Skeletal Mesh компоненты для fade out */
	UPROPERTY(Transient)
	TArray<USkeletalMeshComponent*> SkeletalMeshesToFadeOut;

	/** Sprite компоненты для fade out */
	UPROPERTY(Transient)
	TArray<UPaperSpriteComponent*> SpritesToFadeOut;

	/** Skeletal Mesh компоненты для fade in */
	UPROPERTY(Transient)
	TArray<USkeletalMeshComponent*> SkeletalMeshesToFadeIn;

	/** Sprite компоненты для fade in */
	UPROPERTY(Transient)
	TArray<UPaperSpriteComponent*> SpritesToFadeIn;

private:
	// =====================================================
	// ВНУТРЕННИЕ МЕТОДЫ
	// =====================================================

	/** Создание всех компонентов */
	void CreateComponents();

	/** Настройка иерархии компонентов */
	void SetupComponentHierarchy();

	/** Применение состояния персонажа */
	void ApplyCharacterState(const F_VN_CharacterState& State);

	/** Подготовка компонентов для анимации перехода */
	void PrepareTransitionComponents(const F_VN_CharacterState& NewState);

	/** Завершение и очистка анимации перехода */
	void FinishAndCleanupTransition();

	// =====================================================
	// МЕТОДЫ НАСТРОЙКИ КОМПОНЕНТОВ
	// =====================================================

	/** Настройка Skeletal Mesh компонента из конфигурации Body */
	void SetupComponentFromConfig(USkeletalMeshComponent* Component, const F_VN_SkeletalConfig_Body& Config);

	/** Настройка Skeletal Mesh компонента из конфигурации Attachment */
	void SetupComponentFromConfig(USkeletalMeshComponent* Component, const F_VN_SkeletalConfig_Attachment& Config);

	/** Настройка Sprite компонента из конфигурации Attachment */
	void SetupComponentFromConfig(UPaperSpriteComponent* Component, const F_VN_SpriteConfig_Attachment& Config);

	/** Настройка Sprite компонента из конфигурации Simple */
	void SetupComponentFromConfig(UPaperSpriteComponent* Component, const F_VN_SpriteConfig_Simple& Config);

	// =====================================================
	// УТИЛИТЫ ДЛЯ РАБОТЫ С КОМПОНЕНТАМИ
	// =====================================================

	/** Установка альфа-канала компонента */
	void SetComponentAlpha(USceneComponent* Component, float Alpha);

	/** Установка цвета компонента */
	void SetComponentColor(USceneComponent* Component, const FLinearColor& Color);

	/** Получение Skeletal Mesh компонента по ID */
	USkeletalMeshComponent* GetSkeletalComponent(E_VN_ComponentID_Skeletal ComponentID) const;

	/** Получение Sprite компонента по ID */
	UPaperSpriteComponent* GetSpriteComponent(E_VN_ComponentID_Sprite ComponentID) const;

	/** Получение всех рендер-компонентов */
	TArray<USceneComponent*> GetAllRenderComponents() const;

	/** Получение всех Skeletal Mesh компонентов */
	TArray<USkeletalMeshComponent*> GetAllSkeletalComponents() const;

	/** Получение всех Sprite компонентов */
	TArray<UPaperSpriteComponent*> GetAllSpriteComponents() const;

	// =====================================================
	// ВАЛИДАЦИЯ И ОБРАБОТКА ОШИБОК
	// =====================================================

	/** Валидация состояния персонажа */
	bool ValidateCharacterState(const F_VN_CharacterState& State) const;

	/** Проверка корректности ассетов */
	bool ValidateAssets(const F_VN_CharacterState& State) const;

	/** Применение мобильных оптимизаций */
	void ApplyMobileOptimizations();

	/** Применение глобальных трансформаций */
	void ApplyGlobalTransforms();

	/** Обновление LOD системы */
	void UpdateLOD();

	// =====================================================
	// ОБРАБОТЧИКИ СОБЫТИЙ АНИМАЦИИ
	// =====================================================

	/** Обработчик начала анимации */
	UFUNCTION()
	void OnAnimationStarted(EVNAnimationType AnimationType);

	/** Обработчик завершения анимации */
	UFUNCTION()
	void OnAnimationFinished(EVNAnimationType AnimationType);

	/** Обработчик прогресса анимации */
	UFUNCTION()
	void OnAnimationProgress(EVNAnimationType AnimationType, float Progress);

public:
	// =====================================================
	// СОБЫТИЯ
	// =====================================================

	/** Событие изменения состояния персонажа */
	UPROPERTY(BlueprintAssignable, Category = "VN Character Events")
	FOnVNCharacterStateChanged OnCharacterStateChanged;

	/** Событие изменения фокуса */
	UPROPERTY(BlueprintAssignable, Category = "VN Character Events")
	FOnVNCharacterFocusChanged OnCharacterFocusChanged;

	/** Событие изменения видимости */
	UPROPERTY(BlueprintAssignable, Category = "VN Character Events")
	FOnVNCharacterVisibilityChanged OnCharacterVisibilityChanged;

	// =====================================================
	// ОТЛАДОЧНЫЕ МЕТОДЫ
	// =====================================================

#if WITH_EDITOR
	/** Печать отладочной информации */
	UFUNCTION(CallInEditor, Category = "Debug")
	void PrintDebugInfo();

	/** Валидация всех компонентов */
	UFUNCTION(CallInEditor, Category = "Debug")
	void ValidateAllComponents();
#endif

	/** Получение информации о состоянии для отладки */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	FString GetDebugString() const;

	// =====================================================
	// ДРУЖЕСТВЕННЫЕ КЛАССЫ (ДОСТУП ДЛЯ ANIMATION MANAGER)
	// =====================================================

	friend class UVNCharacterAnimationManager;
};