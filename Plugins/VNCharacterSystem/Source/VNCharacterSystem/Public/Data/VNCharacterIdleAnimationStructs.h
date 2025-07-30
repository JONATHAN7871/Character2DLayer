#pragma once

#include "CoreMinimal.h"
#include "PaperFlipbook.h"
#include "VNCharacterIdleAnimationStructs.generated.h"

/**
 * Структура для анимации моргания
 * Использует только 2 кадра: полузакрытые и закрытые глаза
 */
USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API FVNBlinkAnimationConfig
{
    GENERATED_BODY()

    /** Flipbook с кадрами моргания (2 кадра: полузакрытые и закрытые глаза) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blink Animation", 
        meta = (AllowedClasses = "/Script/Paper2D.PaperFlipbook"))
    TSoftObjectPtr<UPaperFlipbook> BlinkFlipbook;

    /** Минимальный интервал между морганиями (в секундах) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blink Animation", 
        meta = (ClampMin = "0.5", ClampMax = "10.0"))
    float MinBlinkInterval = 2.0f;

    /** Максимальный интервал между морганиями (в секундах) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blink Animation", 
        meta = (ClampMin = "0.5", ClampMax = "15.0"))
    float MaxBlinkInterval = 5.0f;

    /** Длительность одного моргания (в секундах) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blink Animation", 
        meta = (ClampMin = "0.05", ClampMax = "0.5"))
    float BlinkDuration = 0.15f;

    /** Пауза между двойными морганиями (в секундах) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blink Animation", 
        meta = (ClampMin = "0.05", ClampMax = "0.3"))
    float DoubleBlinkPause = 0.1f;

    /** Шанс двойного моргания (0.0 - никогда, 1.0 - всегда) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blink Animation", 
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DoubleBlinkChance = 0.3f;

    /** Цвет для анимации моргания (применяется к векам) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blink Animation")
    FLinearColor BlinkColor = FLinearColor::White;

    /** Применять ли кастомный цвет во время моргания */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blink Animation")
    bool bUseCustomBlinkColor = false;

    /** Активна ли анимация моргания */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blink Animation")
    bool bEnabled = false;

    FVNBlinkAnimationConfig()
    {
        BlinkFlipbook = nullptr;
        MinBlinkInterval = 2.0f;
        MaxBlinkInterval = 5.0f;
        BlinkDuration = 0.15f;
        DoubleBlinkPause = 0.1f;
        DoubleBlinkChance = 0.3f;
        BlinkColor = FLinearColor::White;
        bUseCustomBlinkColor = false;
        bEnabled = false;
    }

    /** Проверка валидности конфигурации */
    bool IsValid() const
    {
        return !BlinkFlipbook.IsNull() && 
               MinBlinkInterval > 0.0f && 
               MaxBlinkInterval >= MinBlinkInterval &&
               BlinkDuration > 0.0f;
    }

    /** Получить случайный интервал между морганиями */
    float GetRandomBlinkInterval() const
    {
        return FMath::RandRange(MinBlinkInterval, MaxBlinkInterval);
    }

    /** Определить, будет ли двойное моргание */
    bool ShouldDoubleBlink() const
    {
        return FMath::RandRange(0.0f, 1.0f) <= DoubleBlinkChance;
    }
};

/**
 * Структура для анимации разговора
 */
USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API FVNTalkAnimationConfig
{
    GENERATED_BODY()

    /** Flipbook с анимацией разговора */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talk Animation", 
        meta = (AllowedClasses = "/Script/Paper2D.PaperFlipbook"))
    TSoftObjectPtr<UPaperFlipbook> TalkFlipbook;

    /** Скорость смены кадров при разговоре (кадров в секунду) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talk Animation", 
        meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float TalkSpeed = 3.0f;

    /** Цвет для анимации разговора (применяется к рту) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talk Animation")
    FLinearColor TalkColor = FLinearColor::White;

    /** Применять ли кастомный цвет во время разговора */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talk Animation")
    bool bUseCustomTalkColor = false;

    /** Активна ли анимация разговора */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talk Animation")
    bool bEnabled = false;

    FVNTalkAnimationConfig()
    {
        TalkFlipbook = nullptr;
        TalkSpeed = 3.0f;
        TalkColor = FLinearColor::White;
        bUseCustomTalkColor = false;
        bEnabled = false;
    }

    /** Проверка валидности конфигурации */
    bool IsValid() const
    {
        return !TalkFlipbook.IsNull() && TalkSpeed > 0.0f;
    }

    /** Получить интервал смены кадров */
    float GetFrameInterval() const
    {
        return 1.0f / TalkSpeed;
    }
};

/**
 * Структура для анимации случайных движений глаз
 */
USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API FVNEyesRandomAnimationConfig
{
    GENERATED_BODY()

    /** Flipbook с различными направлениями взгляда */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyes Random Animation", 
        meta = (AllowedClasses = "/Script/Paper2D.PaperFlipbook"))
    TSoftObjectPtr<UPaperFlipbook> EyesDirectionsFlipbook;

    /** Минимальное время показа нового направления взгляда (в секундах) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyes Random Animation", 
        meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float MinLookDuration = 0.2f;

    /** Максимальное время показа нового направления взгляда (в секундах) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyes Random Animation", 
        meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float MaxLookDuration = 0.8f;

    /** Минимальное время ожидания между сменами направления (в секундах) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyes Random Animation", 
        meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float MinWaitDuration = 0.3f;

    /** Максимальное время ожидания между сменами направления (в секундах) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyes Random Animation", 
        meta = (ClampMin = "0.1", ClampMax = "15.0"))
    float MaxWaitDuration = 2.0f;

    /** Цвет для анимации глаз (применяется к глазам) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyes Random Animation")
    FLinearColor EyesColor = FLinearColor::White;

    /** Применять ли кастомный цвет во время анимации глаз */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyes Random Animation")
    bool bUseCustomEyesColor = false;

    /** Активна ли анимация случайных движений глаз */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyes Random Animation")
    bool bEnabled = false;

    FVNEyesRandomAnimationConfig()
    {
        EyesDirectionsFlipbook = nullptr;
        MinLookDuration = 0.2f;
        MaxLookDuration = 0.8f;
        MinWaitDuration = 0.3f;
        MaxWaitDuration = 2.0f;
        EyesColor = FLinearColor::White;
        bUseCustomEyesColor = false;
        bEnabled = false;
    }

    /** Проверка валидности конфигурации */
    bool IsValid() const
    {
        return !EyesDirectionsFlipbook.IsNull() && 
               MinLookDuration > 0.0f && 
               MaxLookDuration >= MinLookDuration &&
               MinWaitDuration > 0.0f && 
               MaxWaitDuration >= MinWaitDuration;
    }

    /** Получить случайную длительность показа направления */
    float GetRandomLookDuration() const
    {
        return FMath::RandRange(MinLookDuration, MaxLookDuration);
    }

    /** Получить случайную длительность ожидания */
    float GetRandomWaitDuration() const
    {
        return FMath::RandRange(MinWaitDuration, MaxWaitDuration);
    }
};

/**
 * Общая структура конфигурации всех idle анимаций
 */
USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API FVNIdleAnimationsConfig
{
    GENERATED_BODY()

    /** Конфигурация анимации моргания */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle Animations")
    FVNBlinkAnimationConfig BlinkConfig;

    /** Конфигурация анимации разговора */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle Animations")
    FVNTalkAnimationConfig TalkConfig;

    /** Конфигурация анимации случайных движений глаз */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle Animations")
    FVNEyesRandomAnimationConfig EyesRandomConfig;

    FVNIdleAnimationsConfig()
    {
        // Конструкторы структур уже инициализируют значения по умолчанию
    }

    /** Проверить, есть ли активные idle анимации */
    bool HasActiveAnimations() const
    {
        return BlinkConfig.bEnabled || TalkConfig.bEnabled || EyesRandomConfig.bEnabled;
    }

    /** Получить количество активных анимаций */
    int32 GetActiveAnimationsCount() const
    {
        int32 Count = 0;
        if (BlinkConfig.bEnabled) Count++;
        if (TalkConfig.bEnabled) Count++;
        if (EyesRandomConfig.bEnabled) Count++;
        return Count;
    }
};