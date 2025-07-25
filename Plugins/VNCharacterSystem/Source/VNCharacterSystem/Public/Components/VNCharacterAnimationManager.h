#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/Engine.h"
#include "Data/VNCharacterEnums.h"
#include "Data/VNCharacterTypes.h"
#include "VNCharacterAnimationManager.generated.h"

// Forward declarations
class AVNCharacter;
class USkeletalMeshComponent;
class UPaperSpriteComponent;

/**
 * Менеджер анимаций для VN персонажей
 * 
 * Единая система управления всеми анимациями персонажа:
 * - Переходы между состояниями
 * - Появление/исчезновение
 * - Смена фокуса
 * - Очередь анимаций
 * - Прерывание и пропуск анимаций
 */

// Делегаты для событий анимации
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVNAnimationStarted, EVNAnimationType, AnimationType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVNAnimationFinished, EVNAnimationType, AnimationType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVNAnimationProgress, EVNAnimationType, AnimationType, float, Progress);

UCLASS(BlueprintType, Blueprintable, ClassGroup=(VNCharacter), meta=(BlueprintSpawnableComponent))
class VNCHARACTERSYSTEM_API UVNCharacterAnimationManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UVNCharacterAnimationManager();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// =====================================================
	// ПУБЛИЧНЫЕ МЕТОДЫ - API ДЛЯ ПОЛЬЗОВАТЕЛЯ
	// =====================================================

	/**
	 * Запустить анимацию перехода между состояниями
	 * @param Duration Длительность анимации в секундах
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Animation")
	void PlayTransition(float Duration = 1.0f);

	/**
	 * Запустить анимацию появления/исчезновения
	 * @param bAppear true для появления, false для исчезновения
	 * @param Duration Длительность анимации в секундах
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Animation")
	void PlaySpawnDespawn(bool bAppear, float Duration = 1.0f);

	/**
	 * Запустить анимацию смены фокуса
	 * @param bInFocus true для фокуса, false для потери фокуса
	 * @param Duration Длительность анимации в секундах
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Animation")
	void PlayFocus(bool bInFocus, float Duration = 1.0f);

	/**
	 * Пропустить текущую анимацию
	 * Мгновенно завершает текущую анимацию и переходит к следующей в очереди
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Animation")
	void SkipCurrentAnimation();

	/**
	 * Очистить очередь анимаций
	 * Останавливает текущую анимацию и удаляет все запросы из очереди
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Animation")
	void ClearAnimationQueue();

	/**
	 * Проверить, выполняется ли анимация
	 * @return true если в данный момент выполняется анимация
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Animation")
	bool IsAnimating() const { return CurrentAnimation.AnimationType != EVNAnimationType::None; }

	/**
	 * Получить тип текущей анимации
	 * @return Тип выполняемой анимации или None если анимация не выполняется
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Animation")
	EVNAnimationType GetCurrentAnimationType() const { return CurrentAnimation.AnimationType; }

	/**
	 * Получить прогресс текущей анимации
	 * @return Прогресс от 0.0 до 1.0, или -1.0 если анимация не выполняется
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Animation")
	float GetCurrentAnimationProgress() const;

	/**
	 * Получить количество анимаций в очереди
	 * @return Количество ожидающих выполнения анимаций
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Animation")
	int32 GetQueuedAnimationsCount() const { return AnimationQueue.Num(); }

	/**
	 * Получить строковое представление очереди анимаций
	 * @return Описание текущей очереди анимаций для отладки
	 */
	UFUNCTION(BlueprintCallable, Category = "VN Animation")
	FString GetQueueDebugString() const;

protected:
	// =====================================================
	// ВНУТРЕННИЕ ДАННЫЕ
	// =====================================================

	/** Текущая выполняемая анимация */
	UPROPERTY(Transient)
	FVNAnimationRequest CurrentAnimation;

	/** Время выполнения текущей анимации */
	UPROPERTY(Transient)
	float CurrentAnimationTime;

	/** Очередь анимаций для выполнения */
	UPROPERTY(Transient)
	TArray<FVNAnimationRequest> AnimationQueue;

	/** Кэшированная ссылка на владельца */
	UPROPERTY(Transient)
	TWeakObjectPtr<AVNCharacter> OwnerCharacter;

	// =====================================================
	// НАСТРОЙКИ КОМПОНЕНТА
	// =====================================================

	/** Максимальный размер очереди анимаций */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings", 
		meta = (ClampMin = "1", ClampMax = "50"))
	int32 MaxQueueSize = 10;

	/** Автоматически пропускать анимации при переполнении очереди */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings")
	bool bAutoSkipOnQueueOverflow = true;

	/** Минимальная длительность анимации (для защиты от слишком быстрых анимаций) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings", 
		meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float MinAnimationDuration = 0.1f;

	/** Отключить анимации (для отладки) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDisableAnimations = false;

	/** Подробное логирование анимаций */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bVerboseLogging = false;

private:
	// =====================================================
	// ВНУТРЕННИЕ МЕТОДЫ
	// =====================================================

	/** Обработка очереди анимаций */
	void ProcessAnimationQueue();

	/** Запуск анимации из запроса */
	void StartAnimation(const FVNAnimationRequest& Request);

	/** Обновление текущей анимации */
	void UpdateCurrentAnimation(float DeltaTime);

	/** Завершение текущей анимации */
	void FinishCurrentAnimation();

	/** Добавление запроса в очередь с проверками */
	bool EnqueueAnimationRequest(const FVNAnimationRequest& Request);

	// =====================================================
	// СПЕЦИАЛИЗИРОВАННЫЕ МЕТОДЫ АНИМАЦИИ
	// =====================================================

	/** Обновление анимации перехода состояний */
	void UpdateTransitionAnimation(float Alpha);

	/** Обновление анимации появления/исчезновения */
	void UpdateSpawnDespawnAnimation(float Alpha);

	/** Обновление анимации смены фокуса */
	void UpdateFocusAnimation(float Alpha);

	// =====================================================
	// УТИЛИТЫ
	// =====================================================

	/** Получение владельца как VN персонажа */
	AVNCharacter* GetVNCharacterOwner() const;

	/** Валидация запроса анимации */
	bool ValidateAnimationRequest(const FVNAnimationRequest& Request) const;

	/** Логирование для отладки */
	void LogAnimation(const FString& Message, bool bForceLog = false) const;

public:
	// =====================================================
	// СОБЫТИЯ
	// =====================================================

	/** Событие начала анимации */
	UPROPERTY(BlueprintAssignable, Category = "VN Animation Events")
	FOnVNAnimationStarted OnAnimationStarted;

	/** Событие завершения анимации */
	UPROPERTY(BlueprintAssignable, Category = "VN Animation Events")
	FOnVNAnimationFinished OnAnimationFinished;

	/** Событие прогресса анимации (вызывается каждый кадр во время анимации) */
	UPROPERTY(BlueprintAssignable, Category = "VN Animation Events")
	FOnVNAnimationProgress OnAnimationProgress;
};