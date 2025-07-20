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
 * - Применение состояний персонажа и частичных изменений
 * - Интеграцию с менеджером анимаций
 * - Систему фокуса и видимости
 * - Поддержку диалоговой системы
 * - Упрощенную валидацию
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
	virtual void OnConstruction(const FTransform& Transform) override;
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
	 * Применить частичное состояние персонажа (только заполненные поля)
	 * @param PartialState Частичное состояние (пустые поля игнорируются)
	 * @param TransitionDuration Длительность анимации перехода
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character")
	void ApplyPartialState(const F_VN_CharacterState& PartialState, float TransitionDuration = 1.0f);

	/**
	 * Установить главный пресет позы
	 * @param NewPosePreset Новый пресет позы
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character")
	void SetMainPosePreset(const F_VN_CharacterState& NewPosePreset);

	/**
	 * Вернуться к главному пресету позы
	 * @param TransitionDuration Длительность анимации
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character")
	void ReturnToMainPose(float TransitionDuration = 1.0f);

	/**
	 * Получить текущее состояние персонажа
	 * @return Текущее состояние
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character")
	const F_VN_CharacterState& GetCurrentState() const { return CurrentState; }

	/**
	 * Получить главный пресет позы
	 * @return Главный пресет
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character")
	const F_VN_CharacterState& GetMainPosePreset() const { return MainPosePreset; }

	/**
	 * Пропустить анимацию перехода состояния
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character")
	void SkipTransition();

	// =====================================================
	// МЕТОДЫ ДЛЯ ДИАЛОГОВОЙ СИСТЕМЫ - ОТДЕЛЬНЫЕ КОМПОНЕНТЫ
	// =====================================================

	/**
	 * Установить спрайт глаз (null = вернуться к пресету или скрыть)
	 * @param EyesSprite Новый спрайт глаз или null
	 * @param bAnimate Использовать анимацию
	 * @param Duration Длительность анимации
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Dialogue")
	void SetEyes(TSoftObjectPtr<UPaperSprite> EyesSprite, bool bAnimate = true, float Duration = 0.5f);

	/**
	 * Установить спрайт рта (null = вернуться к пресету или скрыть)
	 * @param MouthSprite Новый спрайт рта или null
	 * @param bAnimate Использовать анимацию
	 * @param Duration Длительность анимации
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Dialogue")
	void SetMouth(TSoftObjectPtr<UPaperSprite> MouthSprite, bool bAnimate = true, float Duration = 0.5f);

	/**
	 * Установить спрайт бровей (null = вернуться к пресету или скрыть)
	 * @param EyebrowSprite Новый спрайт бровей или null
	 * @param bAnimate Использовать анимацию
	 * @param Duration Длительность анимации
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Dialogue")
	void SetEyebrows(TSoftObjectPtr<UPaperSprite> EyebrowSprite, bool bAnimate = true, float Duration = 0.5f);

	/**
	 * Установить Skeletal Mesh тела (null = вернуться к пресету или скрыть)
	 * @param BodyMesh Новый mesh тела или null
	 * @param bAnimate Использовать анимацию
	 * @param Duration Длительность анимации
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Dialogue")
	void SetBody(TSoftObjectPtr<USkeletalMesh> BodyMesh, bool bAnimate = true, float Duration = 1.0f);

	/**
	 * Установить Skeletal Mesh рук (null = вернуться к пресету или скрыть)
	 * @param ArmsMesh Новый mesh рук или null
	 * @param bAnimate Использовать анимацию
	 * @param Duration Длительность анимации
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Dialogue")
	void SetArms(TSoftObjectPtr<USkeletalMesh> ArmsMesh, bool bAnimate = true, float Duration = 1.0f);

	/**
	 * Установить лицо целиком (любой параметр может быть null)
	 * @param EyesSprite Спрайт глаз или null
	 * @param MouthSprite Спрайт рта или null
	 * @param EyebrowSprite Спрайт бровей или null
	 * @param bAnimate Использовать анимацию
	 * @param Duration Длительность анимации
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Dialogue")
	void SetFace(TSoftObjectPtr<UPaperSprite> EyesSprite, 
	             TSoftObjectPtr<UPaperSprite> MouthSprite, 
	             TSoftObjectPtr<UPaperSprite> EyebrowSprite, 
	             bool bAnimate = true, float Duration = 0.5f);

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

	// =====================================================
	// ДОПОЛНИТЕЛЬНЫЕ УТИЛИТЫ (BLUEPRINT ДОСТУПНЫЕ)
	// =====================================================

	/**
	 * Безопасно установить имя персонажа
	 * @param NewName Новое имя персонажа
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Identity")
	void SetCharacterNameSafe(const FString& NewName);

	/**
	 * Безопасно получить имя персонажа
	 * @return Имя персонажа или "Unnamed Character" если пусто
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Identity")
	FString GetCharacterNameSafe() const;

	/**
	 * Проверить, есть ли главный пресет позы
	 * @return true если пресет настроен и валиден
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Pose")
	bool HasMainPosePreset() const;

	/**
	 * Получить описание текущего состояния
	 * @return Строковое описание состояния
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Info")
	FString GetCurrentStateDescription() const;

	/**
	 * Получить детальный статус персонажа
	 * @return Детальная информация о персонаже
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Info")
	FString GetDetailedStatusString() const;

	/**
	 * Скрыть все элементы лица
	 * @param bAnimate Использовать анимацию
	 * @param Duration Длительность анимации
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Dialogue")
	void HideAllFacialFeatures(bool bAnimate = true, float Duration = 0.5f);

	/**
	 * Восстановить все элементы лица из пресета
	 * @param bAnimate Использовать анимацию
	 * @param Duration Длительность анимации
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Dialogue")
	void RestoreAllFacialFeaturesFromPreset(bool bAnimate = true, float Duration = 0.5f);

	/**
	 * Установить глобальный тинт для всех компонентов
	 * @param TintColor Цвет тинта
	 * @param bAnimate Использовать анимацию
	 * @param Duration Длительность анимации
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Effects")
	void SetGlobalTint(const FLinearColor& TintColor, bool bAnimate = false, float Duration = 1.0f);

	/**
	 * Сбросить глобальный тинт к белому цвету
	 * @param bAnimate Использовать анимацию
	 * @param Duration Длительность анимации
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Effects")
	void ResetGlobalTint(bool bAnimate = false, float Duration = 1.0f);

	/**
	 * Установить глобальную альфу для всех компонентов
	 * @param Alpha Значение альфы (0.0 - 1.0)
	 * @param bAnimate Использовать анимацию
	 * @param Duration Длительность анимации
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Effects")
	void SetGlobalAlpha(float Alpha, bool bAnimate = false, float Duration = 1.0f);

protected:
	// =====================================================
	// НАСТРОЙКИ ПЕРСОНАЖА
	// =====================================================

	/** Текущее состояние персонажа */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character State")
	F_VN_CharacterState CurrentState;

	/** Имя персонажа для поиска и идентификации */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character|Identity")
	FString CharacterName = TEXT("Unnamed Character");

	/** Главный пресет позы для возврата к базовому состоянию */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character|Pose")
	F_VN_CharacterState MainPosePreset;

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Global Transform", 
		meta = (CallInEditor = "true"))
	FVector GlobalSkeletalOffset = FVector::ZeroVector;

	/** Глобальный масштаб для всех Skeletal Mesh компонентов */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Global Transform", 
		meta = (CallInEditor = "true"))
	float GlobalSkeletalScale = 1.0f;

	/** Глобальное смещение для всех Sprite компонентов */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Global Transform", 
		meta = (CallInEditor = "true"))
	FVector GlobalSpriteOffset = FVector::ZeroVector;

	/** Глобальный масштаб для всех Sprite компонентов */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Global Transform", 
		meta = (CallInEditor = "true"))
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
	// ВНУТРЕННИЕ МЕТОДЫ - ОСНОВНАЯ ЛОГИКА
	// =====================================================

	/** Создание всех компонентов */
	void CreateComponents();

	/** Настройка иерархии компонентов */
	void SetupComponentHierarchy();

	/** Применение состояния персонажа */
	void ApplyCharacterState(const F_VN_CharacterState& State);

	/** Подготовка компонентов для анимации перехода */
	void PrepareTransitionComponents(const F_VN_CharacterState& NewState);

	/** Подготовка Skeletal Mesh компонента для анимации (Body) */
	void PrepareSkeletalTransition(USkeletalMeshComponent* Component, const F_VN_SkeletalConfig_Body& OldConfig, const F_VN_SkeletalConfig_Body& NewConfig);

	/** Подготовка Skeletal Mesh компонента для анимации (Attachment) */
	void PrepareSkeletalAttachmentTransition(USkeletalMeshComponent* Component, const F_VN_SkeletalConfig_Attachment& OldConfig, const F_VN_SkeletalConfig_Attachment& NewConfig);

	/** Подготовка Sprite компонента для анимации (Attachment) */
	void PrepareSpriteAttachmentTransition(UPaperSpriteComponent* Component, const F_VN_SpriteConfig_Attachment& OldConfig, const F_VN_SpriteConfig_Attachment& NewConfig);

	/** Подготовка Sprite компонента для анимации (Simple) */
	void PrepareSpriteSimpleTransition(UPaperSpriteComponent* Component, const F_VN_SpriteConfig_Simple& OldConfig, const F_VN_SpriteConfig_Simple& NewConfig);

	/** Завершение и очистка анимации перехода */
	void FinishAndCleanupTransition();

	// =====================================================
	// МЕТОДЫ ДЛЯ ОБРАБОТКИ NULL ЗНАЧЕНИЙ
	// =====================================================

	/** Получить конфигурацию из пресета или создать скрытую */
	F_VN_SpriteConfig_Simple GetConfigFromPresetOrHidden_Eyes(TSoftObjectPtr<UPaperSprite> Sprite) const;
	F_VN_SpriteConfig_Simple GetConfigFromPresetOrHidden_Mouth(TSoftObjectPtr<UPaperSprite> Sprite) const;
	F_VN_SpriteConfig_Simple GetConfigFromPresetOrHidden_Eyebrows(TSoftObjectPtr<UPaperSprite> Sprite) const;
	F_VN_SkeletalConfig_Body GetConfigFromPresetOrHidden_Body(TSoftObjectPtr<USkeletalMesh> Mesh) const;
	F_VN_SkeletalConfig_Attachment GetConfigFromPresetOrHidden_Arms(TSoftObjectPtr<USkeletalMesh> Mesh) const;

	/** Проверить, заполнена ли конфигурация */
	bool IsSkeletalBodyConfigFilled(const F_VN_SkeletalConfig_Body& Config) const;
	bool IsSkeletalAttachmentConfigFilled(const F_VN_SkeletalConfig_Attachment& Config) const;
	bool IsSpriteSimpleConfigFilled(const F_VN_SpriteConfig_Simple& Config) const;
	bool IsSpriteAttachmentConfigFilled(const F_VN_SpriteConfig_Attachment& Config) const;

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

	/** Проверка, является ли спрайт дочерним элементом Head_Sprite */
	bool IsChildOfHeadSprite(UPaperSpriteComponent* Sprite) const;

	// =====================================================
	// ДОПОЛНИТЕЛЬНЫЕ УТИЛИТЫ ДЛЯ ДИАЛОГОВОЙ СИСТЕМЫ
	// =====================================================

	/** Проверить, видим ли конкретный спрайт компонент */
	bool IsComponentCurrentlyVisible(E_VN_ComponentID_Sprite ComponentID) const;

	/** Проверить, видим ли конкретный Skeletal компонент */
	bool IsComponentCurrentlyVisible(E_VN_ComponentID_Skeletal ComponentID) const;

	/** Получить описание главного пресета */
	FString GetMainPosePresetDescription() const;

	/** Проверить, можно ли применить частичное состояние */
	bool CanApplyPartialState(const F_VN_CharacterState& PartialState) const;

	/** Проверить, эквивалентно ли состояние текущему */
	bool IsStateEquivalentToCurrent(const F_VN_CharacterState& State) const;

	/** Проверить, эквивалентно ли состояние главному пресету */
	bool IsStateEquivalentToMainPose(const F_VN_CharacterState& State) const;

	/** Сбросить к состоянию по умолчанию */
	void ResetToDefaultState();

	/** Скопировать текущее состояние в главный пресет */
	void CopyCurrentStateToMainPose();

	/** Установить глаза из пресета */
	bool SetEyesFromPreset();

	/** Установить рот из пресета */
	bool SetMouthFromPreset();

	/** Установить брови из пресета */
	bool SetEyebrowsFromPreset();

	/** Установить тело из пресета */
	bool SetBodyFromPreset();

	/** Установить руки из пресета */
	bool SetArmsFromPreset();

	/** Получить количество видимых Skeletal компонентов */
	int32 GetVisibleSkeletalComponentsCount() const;

	/** Получить количество видимых Sprite компонентов */
	int32 GetVisibleSpriteComponentsCount() const;

	/** Получить список имен видимых компонентов */
	TArray<FString> GetVisibleComponentNames() const;

	/** Проверить, валидно ли текущее состояние */
	bool HasValidCurrentState() const;

	/** Проверить, валиден ли главный пресет */
	bool HasValidMainPosePreset() const;

	/** Внутренний метод установки состояния с опциональной валидацией */
	void SetCharacterStateInternal(const F_VN_CharacterState& NewState, float TransitionDuration, bool bValidate);

	// =====================================================
	// УПРОЩЕННАЯ ВАЛИДАЦИЯ И ОБРАБОТКА ОШИБОК
	// =====================================================

	/** Упрощенная валидация состояния персонажа */
	bool ValidateCharacterStateSimple(const F_VN_CharacterState& State) const;

	/** Автоматическая коррекция состояния (заполнение пустых элементов) */
	F_VN_CharacterState CorrectCharacterState(const F_VN_CharacterState& State) const;

	/** Применение мобильных оптимизаций */
	void ApplyMobileOptimizations();

	/** Применение глобальных трансформаций */
	void ApplyGlobalTransforms();

	/** Применение глобальных настроек к Skeletal Mesh компонентам */
	void ApplyGlobalSkeletalTransforms();

	/** Применение глобальных настроек к спрайтам */
	void ApplyGlobalSpriteTransforms();

	/** Обновление персонажа для предварительного просмотра в редакторе */
	void UpdateCharacterPreview();

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

	// =====================================================
	// ДРУЖЕСТВЕННЫЕ КЛАССЫ (ДОСТУП ДЛЯ ANIMATION MANAGER)
	// =====================================================

	friend class UVNCharacterAnimationManager;
};