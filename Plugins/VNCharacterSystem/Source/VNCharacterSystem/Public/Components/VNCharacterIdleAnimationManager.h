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
 * 
 * НОВЫЕ ВОЗМОЖНОСТИ:
 * - Отслеживание изменений спрайтов во время анимации
 * - Улучшенная работа с flipbook
 * - Сохранение актуального состояния спрайтов
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

    /**
     * Обновить все сохраненные спрайты до текущего состояния
     * Используется при применении DataAsset для синхронизации
     */
    UFUNCTION(BlueprintCallable, Category = "VN Idle Animations")
    void UpdateSavedSprites();
    
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
    /** Сохраненный спрайт век до анимации моргания */
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
    /** Сохраненный спрайт рта до анимации разговора */
    TSoftObjectPtr<UPaperSprite> OriginalMouthSprite;

    // === ДАННЫЕ ДЛЯ АНИМАЦИИ СЛУЧАЙНЫХ ДВИЖЕНИЙ ГЛАЗ ===
    /** Таймер для анимации случайных движений глаз */
    FTimerHandle EyesRandomTimerHandle;
    /** Сохраненный спрайт глаз до анимации */
    TSoftObjectPtr<UPaperSprite> OriginalEyesSprite;
    /** Выполняется ли анимация случайных движений глаз */
    bool bIsEyesRandomAnimationPlaying = false;

    // =====================================================
    // НОВАЯ СИСТЕМА ОТСЛЕЖИВАНИЯ ИЗМЕНЕНИЙ СПРАЙТОВ
    // =====================================================

    /**
     * Проверить, изменились ли спрайты во время анимации
     * Вызывается каждый кадр в Tick
     */
    void CheckForSpriteChanges();

    /**
     * Обработать изменение спрайта извне во время анимации
     * @param Component Компонент, спрайт которого изменился
     * @param NewSprite Новый спрайт
     * @param ComponentName Имя компонента для логирования
     */
    void HandleExternalSpriteChange(UPaperSpriteComponent* Component, UPaperSprite* NewSprite, const FString& ComponentName);

    /**
     * Проверить, является ли спрайт частью текущей анимации
     * @param Component Компонент для проверки
     * @param Sprite Спрайт для проверки
     * @return true если спрайт является частью flipbook анимации
     */
    bool IsAnimationSprite(UPaperSpriteComponent* Component, UPaperSprite* Sprite) const;

    /**
     * Получить текущий спрайт анимации для компонента
     * @param Component Компонент
     * @return Текущий спрайт или nullptr
     */
    UPaperSprite* GetCurrentAnimationSprite(UPaperSpriteComponent* Component) const;

    /**
     * Проверить, содержит ли flipbook данный спрайт
     * @param Flipbook Flipbook для проверки
     * @param Sprite Спрайт для поиска
     * @return true если спрайт найден в flipbook
     */
    bool IsFlipbookSprite(UPaperFlipbook* Flipbook, UPaperSprite* Sprite) const;

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

    // === УЛУЧШЕННЫЕ УТИЛИТЫ ДЛЯ РАБОТЫ С FLIPBOOK ===
    
    /**
     * УЛУЧШЕННАЯ версия получения спрайта из flipbook по индексу
     * Использует временные точки с умной логикой для UE 5.5
     * @param Flipbook Flipbook для извлечения
     * @param FrameIndex Индекс кадра (0, 1, 2...)
     * @return Спрайт или nullptr если не найден
     */
    UPaperSprite* GetFlipbookSpriteImproved(UPaperFlipbook* Flipbook, int32 FrameIndex) const;
    
    /**
     * ПРОСТАЯ версия получения спрайта из flipbook
     * Backup метод с базовой логикой
     * @param Flipbook Flipbook для извлечения
     * @param FrameIndex Индекс кадра
     * @return Спрайт или nullptr
     */
    UPaperSprite* GetFlipbookSpriteSimple(UPaperFlipbook* Flipbook, int32 FrameIndex) const;
    
    /**
     * УЛУЧШЕННАЯ версия получения случайного спрайта из flipbook
     * @param Flipbook Flipbook для извлечения
     * @param bExcludeFirstFrame Исключить первый кадр из выбора
     * @return Случайный спрайт или nullptr
     */
    UPaperSprite* GetRandomFlipbookSpriteImproved(UPaperFlipbook* Flipbook, bool bExcludeFirstFrame = false) const;
    
    /**
     * УЛУЧШЕННАЯ версия получения количества кадров в flipbook
     * Использует тестирование временных точек для UE 5.5
     * @param Flipbook Flipbook для анализа
     * @return Количество кадров
     */
    int32 GetFlipbookFrameCountImproved(UPaperFlipbook* Flipbook) const;

    /**
     * Отладочный метод для анализа flipbook
     * Выводит подробную информацию о содержимом flipbook
     * @param Flipbook Flipbook для анализа
     */
    void DebugFlipbook(UPaperFlipbook* Flipbook) const;

    // === УЛУЧШЕННЫЕ МЕТОДЫ СОХРАНЕНИЯ И ВОССТАНОВЛЕНИЯ СПРАЙТОВ ===
    
    /**
     * Сохранить текущий спрайт компонента (не оригинальный!)
     * @param Component Компонент для сохранения
     * @param SavedSprite Переменная для сохранения спрайта
     */
    void SaveCurrentSprite(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite>& SavedSprite);
    
    /**
     * Восстановить сохраненный спрайт компонента
     * @param Component Компонент для восстановления
     * @param SavedSprite Сохраненный спрайт
     */
    void RestoreCurrentSprite(UPaperSpriteComponent* Component, const TSoftObjectPtr<UPaperSprite>& SavedSprite);

    // === УСТАРЕВШИЕ МЕТОДЫ (ДЛЯ СОВМЕСТИМОСТИ) ===
    
    /** @deprecated Используйте GetFlipbookSpriteImproved */
    UPaperSprite* GetSpriteFromFlipbook(UPaperFlipbook* Flipbook, int32 FrameIndex) const;
    
    /** @deprecated Используйте GetFlipbookFrameCountImproved */
    int32 GetFlipbookFrameCount(UPaperFlipbook* Flipbook) const;
    
    /** @deprecated Используйте GetRandomFlipbookSpriteImproved */
    UPaperSprite* GetRandomSpriteFromFlipbook(UPaperFlipbook* Flipbook, bool bExcludeFirstFrame = false) const;
    
    /** @deprecated Используйте SaveCurrentSprite */
    void SaveOriginalSprite(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite>& OriginalSprite);
    
    /** @deprecated Используйте RestoreCurrentSprite */
    void RestoreOriginalSprite(UPaperSpriteComponent* Component, const TSoftObjectPtr<UPaperSprite>& OriginalSprite);

    /**
 * Проверить, является ли спрайт частью анимации моргания
 * @param Sprite Спрайт для проверки
 * @return true если спрайт является кадром моргания
 */
    bool IsCurrentSpritePartOfBlinkAnimation(UPaperSprite* Sprite) const;

    /**
     * Проверить, является ли спрайт частью анимации разговора
     * @param Sprite Спрайт для проверки
     * @return true если спрайт является кадром разговора
     */
    bool IsCurrentSpritePartOfTalkAnimation(UPaperSprite* Sprite) const;
};