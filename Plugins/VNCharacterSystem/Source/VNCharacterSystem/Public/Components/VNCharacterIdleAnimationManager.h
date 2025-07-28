#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/Engine.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "Data/VNCharacterIdleAnimationStructs.h"
#include "VNCharacterIdleAnimationManager.generated.h"

// Forward declarations
class AVNCharacter;
class UPaperSpriteComponent;

/**
 * Менеджер idle анимаций для VN персонажей
 * 
 * Управляет тремя типами idle анимаций:
 * - Моргание (Blink)
 * - Разговор (Talk)
 * - Случайные движения глаз (Eyes Random)
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(VNCharacter), meta=(BlueprintSpawnableComponent))
class VNCHARACTERSYSTEM_API UVNCharacterIdleAnimationManager : public UActorComponent
{
    GENERATED_BODY()

public:
    UVNCharacterIdleAnimationManager();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // =====================================================
    // ПУБЛИЧНЫЕ МЕТОДЫ - API ДЛЯ ПОЛЬЗОВАТЕЛЯ
    // =====================================================

    /**
     * Включить/выключить анимацию моргания
     * @param bEnable true для включения, false для выключения
     */
    UFUNCTION(BlueprintCallable, Category = "VN Idle Animations")
    void SetBlinkEnabled(bool bEnable);

    /**
     * Включить/выключить анимацию разговора
     * @param bEnable true для включения, false для выключения
     */
    UFUNCTION(BlueprintCallable, Category = "VN Idle Animations")
    void SetTalkEnabled(bool bEnable);

    /**
     * Включить/выключить анимацию случайных движений глаз
     * @param bEnable true для включения, false для выключения
     */
    UFUNCTION(BlueprintCallable, Category = "VN Idle Animations")
    void SetEyesRandomEnabled(bool bEnable);

    /**
     * Установить конфигурацию всех idle анимаций
     * @param NewConfig Новая конфигурация
     */
    UFUNCTION(BlueprintCallable, Category = "VN Idle Animations")
    void SetIdleAnimationsConfig(const FVNIdleAnimationsConfig& NewConfig);

    /**
     * Получить текущую конфигурацию idle анимаций
     * @return Текущая конфигурация
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Idle Animations")
    const FVNIdleAnimationsConfig& GetIdleAnimationsConfig() const { return IdleAnimationsConfig; }

    /**
     * Остановить все idle анимации
     */
    UFUNCTION(BlueprintCallable, Category = "VN Idle Animations")
    void StopAllIdleAnimations();

    /**
     * Запустить все включенные idle анимации
     */
    UFUNCTION(BlueprintCallable, Category = "VN Idle Animations")
    void StartAllIdleAnimations();

    /**
     * Проверить, активна ли анимация моргания
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Idle Animations")
    bool IsBlinkActive() const { return IdleAnimationsConfig.BlinkConfig.bEnabled; }

    /**
     * Проверить, активна ли анимация разговора
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Idle Animations")
    bool IsTalkActive() const { return IdleAnimationsConfig.TalkConfig.bEnabled; }

    /**
     * Проверить, активна ли анимация случайных движений глаз
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Idle Animations")
    bool IsEyesRandomActive() const { return IdleAnimationsConfig.EyesRandomConfig.bEnabled; }

protected:
    // =====================================================
    // НАСТРОЙКИ КОМПОНЕНТА
    // =====================================================

    /** Конфигурация всех idle анимаций */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle Animations")
    FVNIdleAnimationsConfig IdleAnimationsConfig;

    /** Отключить все idle анимации (для отладки) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bDisableIdleAnimations = false;

    /** Подробное логирование idle анимаций */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bVerboseLogging = false;

private:
    // =====================================================
    // ВНУТРЕННИЕ ДАННЫЕ
    // =====================================================

    /** Кэшированная ссылка на владельца */
    UPROPERTY(Transient)
    TWeakObjectPtr<AVNCharacter> OwnerCharacter;

    // === ДАННЫЕ ДЛЯ АНИМАЦИИ МОРГАНИЯ ===
    /** Таймеры для анимации моргания */
    FTimerHandle BlinkTimerHandle;
    /** Исходный спрайт век до анимации моргания */
    TSoftObjectPtr<UPaperSprite> OriginalEyelidsSprite;
    /** Выполняется ли анимация моргания */
    bool bIsBlinkAnimationPlaying = false;
    /** Состояние моргания для двойного моргания */
    enum class EBlinkState
    {
        WaitingForBlink,    // Ожидание следующего моргания
        FirstBlinkHalf,     // Первая половина первого моргания (полузакрытые глаза)
        FirstBlinkFull,     // Вторая половина первого моргания (закрытые глаза)
        BetweenBlinks,      // Пауза между двойными морганиями
        SecondBlinkHalf,    // Первая половина второго моргания (полузакрытые глаза)
        SecondBlinkFull     // Вторая половина второго моргания (закрытые глаза)
    };
    EBlinkState CurrentBlinkState = EBlinkState::WaitingForBlink;
    /** Планируется ли двойное моргание */
    bool bPendingDoubleBlink = false;

    // === ДАННЫЕ ДЛЯ АНИМАЦИИ РАЗГОВОРА ===
    /** Таймер для анимации разговора */
    FTimerHandle TalkTimerHandle;
    /** Исходный спрайт рта до анимации разговора */
    TSoftObjectPtr<UPaperSprite> OriginalMouthSprite;

    // === ДАННЫЕ ДЛЯ АНИМАЦИИ СЛУЧАЙНЫХ ДВИЖЕНИЙ ГЛАЗ ===
    /** Таймер для анимации случайных движений глаз */
    FTimerHandle EyesRandomTimerHandle;
    /** Исходный спрайт глаз до анимации */
    TSoftObjectPtr<UPaperSprite> OriginalEyesSprite;
    /** Выполняется ли анимация случайных движений глаз */
    bool bIsEyesRandomAnimationPlaying = false;

    // =====================================================
    // ВНУТРЕННИЕ МЕТОДЫ
    // =====================================================

    /** Получение владельца как VN персонажа */
    AVNCharacter* GetVNCharacterOwner() const;

    /** Логирование для отладки */
    void LogIdleAnimation(const FString& Message, bool bForceLog = false) const;

    // === МЕТОДЫ ДЛЯ АНИМАЦИИ МОРГАНИЯ ===
    /** Начать анимацию моргания */
    void StartBlinkAnimation();
    /** Остановить анимацию моргания */
    void StopBlinkAnimation();
    /** Выполнить моргание (одиночное или двойное) */
    void ExecuteBlink();
    /** Обновить состояние моргания */
    void UpdateBlinkState();
    /** Завершить анимацию моргания */
    void FinishBlinkAnimation();
    /** Запланировать следующее моргание */
    void ScheduleNextBlink();

    // === МЕТОДЫ ДЛЯ АНИМАЦИИ РАЗГОВОРА ===
    /** Начать анимацию разговора */
    void StartTalkAnimation();
    /** Остановить анимацию разговора */
    void StopTalkAnimation();
    /** Обновить кадр анимации разговора */
    void UpdateTalkFrame();

    // === МЕТОДЫ ДЛЯ АНИМАЦИИ СЛУЧАЙНЫХ ДВИЖЕНИЙ ГЛАЗ ===
    /** Начать анимацию случайных движений глаз */
    void StartEyesRandomAnimation();
    /** Остановить анимацию случайных движений глаз */
    void StopEyesRandomAnimation();
    /** Выполнить случайное движение глаз */
    void ExecuteRandomEyesMovement();
    /** Вернуть глаза в исходное положение */
    void ReturnEyesToOriginal();
    /** Запланировать следующее движение глаз */
    void ScheduleNextEyesMovement();

    // === УТИЛИТЫ ДЛЯ РАБОТЫ С FLIPBOOK ===
    /** Получить спрайт из flipbook по индексу */
    UPaperSprite* GetSpriteFromFlipbook(UPaperFlipbook* Flipbook, int32 FrameIndex) const;
    /** Получить количество кадров в flipbook */
    int32 GetFlipbookFrameCount(UPaperFlipbook* Flipbook) const;
    /** Получить случайный спрайт из flipbook (исключая первый кадр) */
    UPaperSprite* GetRandomSpriteFromFlipbook(UPaperFlipbook* Flipbook, bool bExcludeFirstFrame = false) const;

    // === МЕТОДЫ ДЛЯ СОХРАНЕНИЯ И ВОССТАНОВЛЕНИЯ ИСХОДНЫХ СПРАЙТОВ ===
    /** Сохранить исходный спрайт компонента */
    void SaveOriginalSprite(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite>& OriginalSprite);
    /** Восстановить исходный спрайт компонента */
    void RestoreOriginalSprite(UPaperSpriteComponent* Component, const TSoftObjectPtr<UPaperSprite>& OriginalSprite);
};