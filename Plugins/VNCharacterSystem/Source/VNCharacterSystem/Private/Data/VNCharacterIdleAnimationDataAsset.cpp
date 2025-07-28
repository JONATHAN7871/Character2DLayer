#include "Data/VNCharacterIdleAnimationDataAsset.h"
#include "VNCharacterSystemModule.h"

UVNCharacterIdleAnimationDataAsset::UVNCharacterIdleAnimationDataAsset()
{
    DefaultEmotionalState = EIdleEmotionalState::None;
    bAutoApplyOnLoad = true;

    // Настройки по умолчанию
    BlinkConfig.MinBlinkInterval = 2.0f;
    BlinkConfig.MaxBlinkInterval = 5.0f;
    BlinkConfig.BlinkDuration = 0.15f;
    BlinkConfig.DoubleBlinkChance = 0.3f;
    BlinkConfig.bEnabled = false; // По умолчанию выключено

    TalkConfig.TalkSpeed = 3.0f;
    TalkConfig.bEnabled = false;

    EyesRandomConfig.MinLookDuration = 0.2f;
    EyesRandomConfig.MaxLookDuration = 0.8f;
    EyesRandomConfig.MinWaitDuration = 0.3f;
    EyesRandomConfig.MaxWaitDuration = 2.0f;
    EyesRandomConfig.bEnabled = false;
}

// =====================================================
// БЫСТРЫЕ ПРЕСЕТЫ
// =====================================================

void UVNCharacterIdleAnimationDataAsset::SetCalmPreset()
{
    DefaultEmotionalState = EIdleEmotionalState::Calm;

    // Спокойное моргание
    BlinkConfig.MinBlinkInterval = 3.0f;
    BlinkConfig.MaxBlinkInterval = 6.0f;
    BlinkConfig.BlinkDuration = 0.15f;
    BlinkConfig.DoubleBlinkChance = 0.2f;
    BlinkConfig.bEnabled = true;

    // Редкие движения глаз
    EyesRandomConfig.MinLookDuration = 0.5f;
    EyesRandomConfig.MaxLookDuration = 1.2f;
    EyesRandomConfig.MinWaitDuration = 2.0f;
    EyesRandomConfig.MaxWaitDuration = 5.0f;
    EyesRandomConfig.bEnabled = true;

    // Разговор выключен по умолчанию
    TalkConfig.bEnabled = false;
}

void UVNCharacterIdleAnimationDataAsset::SetNervousPreset()
{
    DefaultEmotionalState = EIdleEmotionalState::Nervous;

    // Нервное частое моргание
    BlinkConfig.MinBlinkInterval = 0.8f;
    BlinkConfig.MaxBlinkInterval = 2.0f;
    BlinkConfig.BlinkDuration = 0.12f;
    BlinkConfig.DoubleBlinkChance = 0.6f; // Много двойных морганий
    BlinkConfig.bEnabled = true;

    // Быстрые движения глаз
    EyesRandomConfig.MinLookDuration = 0.1f;
    EyesRandomConfig.MaxLookDuration = 0.4f;
    EyesRandomConfig.MinWaitDuration = 0.2f;
    EyesRandomConfig.MaxWaitDuration = 1.0f;
    EyesRandomConfig.bEnabled = true;

    TalkConfig.bEnabled = false;
}

void UVNCharacterIdleAnimationDataAsset::SetSleepyPreset()
{
    DefaultEmotionalState = EIdleEmotionalState::Sleepy;

    // Медленное сонное моргание
    BlinkConfig.MinBlinkInterval = 4.0f;
    BlinkConfig.MaxBlinkInterval = 8.0f;
    BlinkConfig.BlinkDuration = 0.25f; // Долгие моргания
    BlinkConfig.DoubleBlinkChance = 0.1f; // Редкие двойные
    BlinkConfig.bEnabled = true;

    // Очень медленные движения глаз
    EyesRandomConfig.MinLookDuration = 1.0f;
    EyesRandomConfig.MaxLookDuration = 2.0f;
    EyesRandomConfig.MinWaitDuration = 3.0f;
    EyesRandomConfig.MaxWaitDuration = 8.0f;
    EyesRandomConfig.bEnabled = true;

    TalkConfig.bEnabled = false;
}

void UVNCharacterIdleAnimationDataAsset::SetExcitedPreset()
{
    DefaultEmotionalState = EIdleEmotionalState::Excited;

    // Быстрое живое моргание
    BlinkConfig.MinBlinkInterval = 1.0f;
    BlinkConfig.MaxBlinkInterval = 3.0f;
    BlinkConfig.BlinkDuration = 0.1f; // Быстрые моргания
    BlinkConfig.DoubleBlinkChance = 0.4f;
    BlinkConfig.bEnabled = true;

    // Активные движения глаз
    EyesRandomConfig.MinLookDuration = 0.2f;
    EyesRandomConfig.MaxLookDuration = 0.6f;
    EyesRandomConfig.MinWaitDuration = 0.3f;
    EyesRandomConfig.MaxWaitDuration = 1.5f;
    EyesRandomConfig.bEnabled = true;

    // Можно включить разговор для живости
    TalkConfig.TalkSpeed = 4.0f;
    TalkConfig.bEnabled = false; // Пользователь включит сам
}

void UVNCharacterIdleAnimationDataAsset::DisableAllAnimations()
{
    DefaultEmotionalState = EIdleEmotionalState::None;

    BlinkConfig.bEnabled = false;
    TalkConfig.bEnabled = false;
    EyesRandomConfig.bEnabled = false;
}

// =====================================================
// УТИЛИТЫ
// =====================================================

bool UVNCharacterIdleAnimationDataAsset::IsValid() const
{
    bool bValid = true;

    if (BlinkConfig.bEnabled && BlinkConfig.BlinkFlipbook.IsNull())
    {
        bValid = false;
    }

    if (TalkConfig.bEnabled && TalkConfig.TalkFlipbook.IsNull())
    {
        bValid = false;
    }

    if (EyesRandomConfig.bEnabled && EyesRandomConfig.EyesDirectionsFlipbook.IsNull())
    {
        bValid = false;
    }

    return bValid;
}

TArray<FString> UVNCharacterIdleAnimationDataAsset::GetMissingAssets() const
{
    TArray<FString> MissingAssets;

    if (BlinkConfig.bEnabled && BlinkConfig.BlinkFlipbook.IsNull())
    {
        MissingAssets.Add(TEXT("Blink Flipbook"));
    }

    if (TalkConfig.bEnabled && TalkConfig.TalkFlipbook.IsNull())
    {
        MissingAssets.Add(TEXT("Talk Flipbook"));
    }

    if (EyesRandomConfig.bEnabled && EyesRandomConfig.EyesDirectionsFlipbook.IsNull())
    {
        MissingAssets.Add(TEXT("Eyes Directions Flipbook"));
    }

    return MissingAssets;
}

FVNIdleAnimationsConfig UVNCharacterIdleAnimationDataAsset::GetIdleAnimationsConfig() const
{
    FVNIdleAnimationsConfig Config;
    Config.BlinkConfig = BlinkConfig;
    Config.TalkConfig = TalkConfig;
    Config.EyesRandomConfig = EyesRandomConfig;
    return Config;
}

FString UVNCharacterIdleAnimationDataAsset::GetConfigSummary() const
{
    FString Summary = FString::Printf(TEXT("Emotional State: %d, "), (int32)DefaultEmotionalState);
    
    TArray<FString> ActiveAnimations;
    if (BlinkConfig.bEnabled) ActiveAnimations.Add(TEXT("Blink"));
    if (TalkConfig.bEnabled) ActiveAnimations.Add(TEXT("Talk"));
    if (EyesRandomConfig.bEnabled) ActiveAnimations.Add(TEXT("Eyes"));

    if (ActiveAnimations.Num() > 0)
    {
        Summary += FString::Join(ActiveAnimations, TEXT(", "));
    }
    else
    {
        Summary += TEXT("No animations");
    }

    return Summary;
}