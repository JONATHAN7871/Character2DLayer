#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Components/SceneComponent.h"
#include "Data/VNCharacterEnums.h"
#include "Data/VNCharacterTypes.h"
#include "VNCharacter.generated.h"

// Forward declarations
class UVNCharacterAnimationManager;
class UVNCharacterDataAsset;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * Главный актор системы VN персонажей (Переработанная версия)
 * 
 * Основные изменения:
 * - Убрана система состояний персонажа
 * - Созданы заранее все fade-компоненты
 * - Упрощена валидация (работает с конкретными компонентами)
 * - Прямое управление спрайтами и мешами через enum'ы
 * - Правильное применение глобальных трансформаций
 */

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
	// РАБОТА С DATA ASSET
	// =====================================================

	/**
	 * Применить DataAsset к персонажу
	 * @param CharacterData DataAsset с компонентами персонажа
	 * @param bAnimate Использовать анимацию при смене компонентов
	 * @param Duration Длительность анимации
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Data Asset")
	void ApplyDataAsset(class UVNCharacterDataAsset* CharacterData, bool bAnimate = true, float Duration = 1.0f);

	// =====================================================
	// SKELETAL MESH КОМПОНЕНТЫ (ОСНОВНЫЕ + FADE)
	// =====================================================

	// Основные компоненты
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes")
	USkeletalMeshComponent* Body_Skeletal;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes")
	USkeletalMeshComponent* Arms_Skeletal;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes")
	USkeletalMeshComponent* Head_Skeletal;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes")
	USkeletalMeshComponent* Custom01_Skeletal;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes")
	USkeletalMeshComponent* Custom02_Skeletal;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes")
	USkeletalMeshComponent* Custom03_Skeletal;

	// Fade компоненты
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes Fade")
	USkeletalMeshComponent* Body_Skeletal_Fade;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes Fade")
	USkeletalMeshComponent* Arms_Skeletal_Fade;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes Fade")
	USkeletalMeshComponent* Head_Skeletal_Fade;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes Fade")
	USkeletalMeshComponent* Custom01_Skeletal_Fade;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes Fade")
	USkeletalMeshComponent* Custom02_Skeletal_Fade;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skeletal Meshes Fade")
	USkeletalMeshComponent* Custom03_Skeletal_Fade;

	// =====================================================
	// SPRITE КОМПОНЕНТЫ (ОСНОВНЫЕ + FADE)
	// =====================================================

	// Основные спрайты тела
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body")
	class UPaperSpriteComponent* Body_Sprite;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body")
	class UPaperSpriteComponent* Arms_Sprite;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body")
	class UPaperSpriteComponent* BodyShadow_Sprite;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body")
	class UPaperSpriteComponent* EmotionBodyEffect01_Sprite;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body")
	class UPaperSpriteComponent* EmotionBodyEffect02_Sprite;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body")
	class UPaperSpriteComponent* EmotionBodyEffect03_Sprite;

	// Fade спрайты тела
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body Fade")
	class UPaperSpriteComponent* Body_Sprite_Fade;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body Fade")
	class UPaperSpriteComponent* Arms_Sprite_Fade;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body Fade")
	class UPaperSpriteComponent* BodyShadow_Sprite_Fade;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body Fade")
	class UPaperSpriteComponent* EmotionBodyEffect01_Sprite_Fade;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body Fade")
	class UPaperSpriteComponent* EmotionBodyEffect02_Sprite_Fade;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Body Fade")
	class UPaperSpriteComponent* EmotionBodyEffect03_Sprite_Fade;

	// Основные спрайты головы
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head")
	class UPaperSpriteComponent* Head_Sprite;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head")
	class UPaperSpriteComponent* Eyebrow_Sprite;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head")
	class UPaperSpriteComponent* Eyes_Sprite;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head")
	class UPaperSpriteComponent* Eyelids_Sprite;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head")
	class UPaperSpriteComponent* Wink_Sprite;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head")
	class UPaperSpriteComponent* Mouth_Sprite;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head")
	class UPaperSpriteComponent* EmotionHeadEffect01_Sprite;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head")
	class UPaperSpriteComponent* EmotionHeadEffect02_Sprite;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head")
	class UPaperSpriteComponent* EmotionHeadEffect03_Sprite;

	// Fade спрайты головы
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade")
	class UPaperSpriteComponent* Eyebrow_Sprite_Fade;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade")
	class UPaperSpriteComponent* Eyes_Sprite_Fade;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade")
	class UPaperSpriteComponent* Eyelids_Sprite_Fade;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade")
	class UPaperSpriteComponent* Wink_Sprite_Fade;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade")
	class UPaperSpriteComponent* Mouth_Sprite_Fade;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade")
	class UPaperSpriteComponent* EmotionHeadEffect01_Sprite_Fade;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade")
	class UPaperSpriteComponent* EmotionHeadEffect02_Sprite_Fade;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprites Head Fade")
	class UPaperSpriteComponent* EmotionHeadEffect03_Sprite_Fade;

public:
	// =====================================================
	// ОСНОВНОЕ API - ПРЯМОЕ УПРАВЛЕНИЕ КОМПОНЕНТАМИ (ИСПРАВЛЕНО)
	// =====================================================

	/**
	 * Установить Skeletal Mesh для компонента
	 * @param ComponentID Enum идентификатор компонента
	 * @param SkeletalMesh Новый mesh или null для скрытия
	 * @param bAnimate Использовать анимацию перехода
	 * @param Duration Длительность анимации
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Skeletal")
	void SetSkeletalMesh(E_VN_ComponentID_Skeletal ComponentID, TSoftObjectPtr<USkeletalMesh> SkeletalMesh, bool bAnimate = true, float Duration = 1.0f);

	/**
	 * Установить спрайт для компонента
	 * @param ComponentID Enum идентификатор компонента
	 * @param Sprite Новый спрайт или null для скрытия
	 * @param bAnimate Использовать анимацию перехода
	 * @param Duration Длительность анимации
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Sprites")
	void SetSprite(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> Sprite, bool bAnimate = true, float Duration = 0.5f);

	/**
	 * Упрощенные методы для часто используемых элементов
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Quick Access")
	void SetEyes(TSoftObjectPtr<UPaperSprite> EyesSprite, bool bAnimate = true, float Duration = 0.5f);

	UFUNCTION(BlueprintCallable, Category = "VN Character|Quick Access")
	void SetMouth(TSoftObjectPtr<UPaperSprite> MouthSprite, bool bAnimate = true, float Duration = 0.5f);

	UFUNCTION(BlueprintCallable, Category = "VN Character|Quick Access")
	void SetEyebrows(TSoftObjectPtr<UPaperSprite> EyebrowSprite, bool bAnimate = true, float Duration = 0.5f);

	UFUNCTION(BlueprintCallable, Category = "VN Character|Quick Access")
	void SetBody(TSoftObjectPtr<USkeletalMesh> BodyMesh, bool bAnimate = true, float Duration = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "VN Character|Quick Access")
	void SetArms(TSoftObjectPtr<USkeletalMesh> ArmsMesh, bool bAnimate = true, float Duration = 1.0f);

	/**
	 * Установить несколько элементов лица одновременно
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Character|Quick Access")
	void SetFace(TSoftObjectPtr<UPaperSprite> EyesSprite, 
	             TSoftObjectPtr<UPaperSprite> MouthSprite, 
	             TSoftObjectPtr<UPaperSprite> EyebrowSprite, 
	             bool bAnimate = true, float Duration = 0.5f);

	// =====================================================
	// СИСТЕМА ФОКУСА
	// =====================================================

	UFUNCTION(BlueprintCallable, Category = "VN Character")
	void SetFocus(bool bInFocus, float Duration = 1.0f);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character")
	bool IsInFocus() const { return bIsInFocus; }

	UFUNCTION(BlueprintCallable, Category = "VN Character")
	void SkipFocusAnimation();

	// =====================================================
	// СИСТЕМА ВИДИМОСТИ
	// =====================================================

	UFUNCTION(BlueprintCallable, Category = "VN Character")
	void Appear(float Duration = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "VN Character")
	void Disappear(float Duration = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "VN Character")
	void SkipSpawnDespawnAnimation();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character")
	bool IsVisible() const;

	// =====================================================
	// УТИЛИТЫ И ИНФОРМАЦИЯ
	// =====================================================

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character")
	bool IsAnimating() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character")
	FLinearColor GetTargetColorForComponent(USceneComponent* Component) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character")
	FLinearColor GetBaseColorForComponent(USceneComponent* Component) const;

	/**
	 * Получить основной компонент по enum'у
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Components")
	USkeletalMeshComponent* GetSkeletalComponent(E_VN_ComponentID_Skeletal ComponentID) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Components")
	UPaperSpriteComponent* GetSpriteComponent(E_VN_ComponentID_Sprite ComponentID) const;

	/**
	 * Получить fade компонент по enum'у
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Components")
	USkeletalMeshComponent* GetSkeletalFadeComponent(E_VN_ComponentID_Skeletal ComponentID) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character|Components")
	UPaperSpriteComponent* GetSpriteFadeComponent(E_VN_ComponentID_Sprite ComponentID) const;

protected:
	// =====================================================
	// НАСТРОЙКИ ПЕРСОНАЖА
	// =====================================================

	/** Имя персонажа для поиска и идентификации */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character")
	FString CharacterName = TEXT("Unnamed Character");

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Global Transform")
	FVector GlobalSkeletalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Global Transform")
	float GlobalSkeletalScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Global Transform")
	FVector GlobalSpriteOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VN Character Global Transform")
	float GlobalSpriteScale = 1.0f;

private:
	// =====================================================
	// ВНУТРЕННИЕ МЕТОДЫ
	// =====================================================

	/** Создание всех компонентов */
	void CreateComponents();

	/** Настройка иерархии компонентов */
	void SetupComponentHierarchy();

	/** Применение глобальных трансформаций */
	void ApplyGlobalTransforms();

	/** Применение глобальных настроек к Skeletal Mesh компонентам */
	void ApplyGlobalSkeletalTransforms();

	/** Применение глобальных настроек к спрайтам */
	void ApplyGlobalSpriteTransforms();

	/** Применение Identity трансформаций к дочерним спрайтам головы */
	void ApplyChildHeadSpriteTransforms();

	/** Валидация и настройка конкретного Skeletal компонента */
	void ValidateAndSetupSkeletalComponent(USkeletalMeshComponent* Component, TSoftObjectPtr<USkeletalMesh> SkeletalMesh);

	/** Валидация и настройка конкретного Sprite компонента */
	void ValidateAndSetupSpriteComponent(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite> Sprite);

	/** Копирование настроек между Skeletal компонентами */
	void CopySkeletalComponentSettings(USkeletalMeshComponent* Source, USkeletalMeshComponent* Target);

	/** Копирование настроек между Sprite компонентами */
	void CopySpriteComponentSettings(UPaperSpriteComponent* Source, UPaperSpriteComponent* Target);

	/** Подготовка Skeletal компонентов для анимации перехода */
	void PrepareSkeletalTransition(USkeletalMeshComponent* MainComponent, USkeletalMeshComponent* FadeComponent, TSoftObjectPtr<USkeletalMesh> NewMesh);

	/** Подготовка Sprite компонентов для анимации перехода */
	void PrepareSpriteTransition(UPaperSpriteComponent* MainComponent, UPaperSpriteComponent* FadeComponent, TSoftObjectPtr<UPaperSprite> NewSprite);

	/** Завершение анимации перехода для конкретных компонентов */
	void FinishTransition(USceneComponent* MainComponent, USceneComponent* FadeComponent);

	/** Установка альфа-канала компонента */
	void SetComponentAlpha(USceneComponent* Component, float Alpha);

	/** Установка цвета компонента */
	void SetComponentColor(USceneComponent* Component, const FLinearColor& Color);

	/** Получение всех основных компонентов */
	TArray<USceneComponent*> GetAllMainComponents() const;

	/** Получение всех fade компонентов */
	TArray<USceneComponent*> GetAllFadeComponents() const;

	/** Скрытие всех fade компонентов */
	void HideAllFadeComponents();

	/** Применение индивидуальных трансформаций к спрайту */
	void ApplyIndividualSpriteTransform(UPaperSpriteComponent* SpriteComponent, E_VN_ComponentID_Sprite ComponentID);

	/** Проверка, является ли спрайт дочерним элементом Head_Sprite */
	bool IsChildOfHeadSprite(E_VN_ComponentID_Sprite ComponentID) const;

	// =====================================================
	// ОБРАБОТЧИКИ СОБЫТИЙ АНИМАЦИИ
	// =====================================================

	UFUNCTION()
	void OnAnimationStarted(EVNAnimationType AnimationType);

	UFUNCTION()
	void OnAnimationFinished(EVNAnimationType AnimationType);

	UFUNCTION()
	void OnAnimationProgress(EVNAnimationType AnimationType, float Progress);

public:
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

	// =====================================================
	// ОТЛАДОЧНЫЕ МЕТОДЫ
	// =====================================================

#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category = "Debug")
	void PrintDebugInfo();

	UFUNCTION(CallInEditor, Category = "Debug")
	void ValidateAllComponents();
#endif

	// =====================================================
	// ДРУЖЕСТВЕННЫЕ КЛАССЫ
	// =====================================================

	friend class UVNCharacterAnimationManager;
};