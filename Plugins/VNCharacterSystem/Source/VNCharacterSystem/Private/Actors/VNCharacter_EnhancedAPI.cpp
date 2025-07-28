// VNCharacter_EnhancedAPI.cpp - Улучшенное API для живого моргания

#include "Actors/VNCharacter.h"
#include "Components/VNCharacterIdleAnimationManager.h"
#include "Data/VNCharacterIdleAnimationDataAsset.h"
#include "VNCharacterSystemModule.h"

// =====================================================
// НОВЫЕ BLUEPRINT ФУНКЦИИ ДЛЯ УЛУЧШЕННОГО МОРГАНИЯ
// =====================================================

void AVNCharacter::SetupLivelyBlinking(UPaperFlipbook* BlinkFlipbook, float BaseMinInterval, float BaseMaxInterval, float EmotionalVariation, float BlinkDuration, float DoubleBlinkChance)
{
    if (!IdleAnimationManager || !BlinkFlipbook)
    {
        VN_LOG_WARNING(TEXT("SetupLivelyBlinking: Invalid parameters"));
        return;
    }

    FVNIdleAnimationsConfig NewConfig = IdleAnimationManager->GetIdleAnimationsConfig();
    
    NewConfig.BlinkConfig.BlinkFlipbook = BlinkFlipbook;
    NewConfig.BlinkConfig.MinBlinkInterval = FMath::Clamp(BaseMinInterval, 0.5f, 10.0f);
    NewConfig.BlinkConfig.MaxBlinkInterval = FMath::Clamp(BaseMaxInterval, BaseMinInterval, 15.0f);
    NewConfig.BlinkConfig.BlinkDuration = FMath::Clamp(BlinkDuration, 0.05f, 0.5f);
    NewConfig.BlinkConfig.DoubleBlinkChance = FMath::Clamp(DoubleBlinkChance, 0.0f, 1.0f);
    NewConfig.BlinkConfig.bEnabled = true;

    IdleAnimationManager->SetIdleAnimationsConfig(NewConfig);
    
    VN_LOG_DEBUG(TEXT("SetupLivelyBlinking: Configured with emotional variation %.2f"), EmotionalVariation);
}

void AVNCharacter::SetIdleEmotionalState(EIdleEmotionalState EmotionState)
{
    if (!IdleAnimationManager)
    {
        VN_LOG_WARNING(TEXT("SetIdleEmotionalState: IdleAnimationManager is null"));
        return;
    }

    FVNIdleAnimationsConfig CurrentConfig = IdleAnimationManager->GetIdleAnimationsConfig();
    
    // Если состояние None - оставляем текущие настройки без изменений
    if (EmotionState == EIdleEmotionalState::None)
    {
        VN_LOG_DEBUG(TEXT("SetIdleEmotionalState: Emotion state set to None, keeping current settings"));
        return;
    }
    
    // Сохраняем flipbook'и - они не должны изменяться
    TSoftObjectPtr<UPaperFlipbook> SavedBlinkFlipbook = CurrentConfig.BlinkConfig.BlinkFlipbook;
    TSoftObjectPtr<UPaperFlipbook> SavedTalkFlipbook = CurrentConfig.TalkConfig.TalkFlipbook;
    TSoftObjectPtr<UPaperFlipbook> SavedEyesFlipbook = CurrentConfig.EyesRandomConfig.EyesDirectionsFlipbook;
    
    // Базовые значения для расчетов (средние значения)
    float BaseMinInterval = 2.0f;
    float BaseMaxInterval = 5.0f;
    float BaseDuration = 0.15f;
    float BaseDoubleChance = 0.3f;
    float BaseTalkSpeed = 3.0f;
    float BaseEyesMinLook = 0.2f;
    float BaseEyesMaxLook = 0.8f;
    float BaseEyesMinWait = 0.3f;
    float BaseEyesMaxWait = 2.0f;
    
    switch (EmotionState)
    {
        case EIdleEmotionalState::Calm:
            CurrentConfig.BlinkConfig.MinBlinkInterval = 3.0f;
            CurrentConfig.BlinkConfig.MaxBlinkInterval = 6.0f;
            CurrentConfig.BlinkConfig.BlinkDuration = 0.15f;
            CurrentConfig.BlinkConfig.DoubleBlinkChance = 0.2f;
            CurrentConfig.TalkConfig.TalkSpeed = 2.5f;
            CurrentConfig.EyesRandomConfig.MinLookDuration = 0.5f;
            CurrentConfig.EyesRandomConfig.MaxLookDuration = 1.2f;
            CurrentConfig.EyesRandomConfig.MinWaitDuration = 2.0f;
            CurrentConfig.EyesRandomConfig.MaxWaitDuration = 5.0f;
            break;
            
        case EIdleEmotionalState::Nervous:
            CurrentConfig.BlinkConfig.MinBlinkInterval = 0.8f;
            CurrentConfig.BlinkConfig.MaxBlinkInterval = 2.0f;
            CurrentConfig.BlinkConfig.BlinkDuration = 0.12f;
            CurrentConfig.BlinkConfig.DoubleBlinkChance = 0.6f;
            CurrentConfig.TalkConfig.TalkSpeed = 4.5f;
            CurrentConfig.EyesRandomConfig.MinLookDuration = 0.1f;
            CurrentConfig.EyesRandomConfig.MaxLookDuration = 0.4f;
            CurrentConfig.EyesRandomConfig.MinWaitDuration = 0.2f;
            CurrentConfig.EyesRandomConfig.MaxWaitDuration = 1.0f;
            break;
            
        case EIdleEmotionalState::Sleepy:
            CurrentConfig.BlinkConfig.MinBlinkInterval = 4.0f;
            CurrentConfig.BlinkConfig.MaxBlinkInterval = 8.0f;
            CurrentConfig.BlinkConfig.BlinkDuration = 0.25f;
            CurrentConfig.BlinkConfig.DoubleBlinkChance = 0.1f;
            CurrentConfig.TalkConfig.TalkSpeed = 1.5f;
            CurrentConfig.EyesRandomConfig.MinLookDuration = 1.0f;
            CurrentConfig.EyesRandomConfig.MaxLookDuration = 2.0f;
            CurrentConfig.EyesRandomConfig.MinWaitDuration = 3.0f;
            CurrentConfig.EyesRandomConfig.MaxWaitDuration = 8.0f;
            break;
            
        case EIdleEmotionalState::Excited:
            CurrentConfig.BlinkConfig.MinBlinkInterval = 1.0f;
            CurrentConfig.BlinkConfig.MaxBlinkInterval = 3.0f;
            CurrentConfig.BlinkConfig.BlinkDuration = 0.1f;
            CurrentConfig.BlinkConfig.DoubleBlinkChance = 0.4f;
            CurrentConfig.TalkConfig.TalkSpeed = 5.0f;
            CurrentConfig.EyesRandomConfig.MinLookDuration = 0.2f;
            CurrentConfig.EyesRandomConfig.MaxLookDuration = 0.6f;
            CurrentConfig.EyesRandomConfig.MinWaitDuration = 0.3f;
            CurrentConfig.EyesRandomConfig.MaxWaitDuration = 1.5f;
            break;
            
        case EIdleEmotionalState::Focused:
            CurrentConfig.BlinkConfig.MinBlinkInterval = 6.0f;
            CurrentConfig.BlinkConfig.MaxBlinkInterval = 12.0f;
            CurrentConfig.BlinkConfig.BlinkDuration = 0.13f;
            CurrentConfig.BlinkConfig.DoubleBlinkChance = 0.05f;
            CurrentConfig.TalkConfig.TalkSpeed = 2.0f;
            CurrentConfig.EyesRandomConfig.MinLookDuration = 0.8f;
            CurrentConfig.EyesRandomConfig.MaxLookDuration = 2.5f;
            CurrentConfig.EyesRandomConfig.MinWaitDuration = 4.0f;
            CurrentConfig.EyesRandomConfig.MaxWaitDuration = 10.0f;
            break;
            
        case EIdleEmotionalState::Tired:
            CurrentConfig.BlinkConfig.MinBlinkInterval = 1.5f;
            CurrentConfig.BlinkConfig.MaxBlinkInterval = 7.0f;
            CurrentConfig.BlinkConfig.BlinkDuration = 0.2f;
            CurrentConfig.BlinkConfig.DoubleBlinkChance = 0.45f;
            CurrentConfig.TalkConfig.TalkSpeed = 2.2f;
            CurrentConfig.EyesRandomConfig.MinLookDuration = 0.3f;
            CurrentConfig.EyesRandomConfig.MaxLookDuration = 1.5f;
            CurrentConfig.EyesRandomConfig.MinWaitDuration = 1.0f;
            CurrentConfig.EyesRandomConfig.MaxWaitDuration = 4.0f;
            break;
    }
    
    // Восстанавливаем flipbook'и
    CurrentConfig.BlinkConfig.BlinkFlipbook = SavedBlinkFlipbook;
    CurrentConfig.TalkConfig.TalkFlipbook = SavedTalkFlipbook;
    CurrentConfig.EyesRandomConfig.EyesDirectionsFlipbook = SavedEyesFlipbook;
    
    // Применяем обновленную конфигурацию
    IdleAnimationManager->SetIdleAnimationsConfig(CurrentConfig);
    
    VN_LOG_DEBUG(TEXT("SetIdleEmotionalState: Applied %d emotion state"), (int32)EmotionState);
}

// =====================================================
// ИСПРАВЛЕННЫЙ МЕТОД ДЛЯ IDLE ANIMATION DATAASSET
// =====================================================

void AVNCharacter::ApplyIdleAnimationDataAssetWithEmotionalState(UVNCharacterIdleAnimationDataAsset* IdleAnimationData, EIdleEmotionalState EmotionState, bool bRestartAnimations)
{
    if (!IdleAnimationData)
    {
        VN_LOG_WARNING(TEXT("ApplyIdleAnimationDataAssetWithEmotionalState: IdleAnimationData is null"));
        return;
    }

    if (!IdleAnimationManager)
    {
        VN_LOG_WARNING(TEXT("ApplyIdleAnimationDataAssetWithEmotionalState: IdleAnimationManager is null"));
        return;
    }

    VN_LOG_DEBUG(TEXT("ApplyIdleAnimationDataAssetWithEmotionalState: Applying DataAsset with emotion state %d"), (int32)EmotionState);

    // Останавливаем текущие анимации и синхронизируем состояния
    IdleAnimationManager->StopAllIdleAnimations();
    SynchronizeIdleAnimationStates();

    // Получаем базовую конфигурацию из DataAsset
    FVNIdleAnimationsConfig BaseConfig = IdleAnimationData->GetIdleAnimationsConfig();
    
    // Если эмоциональное состояние не None, модифицируем настройки
    if (EmotionState != EIdleEmotionalState::None)
    {
        // Сохраняем flipbook'и из DataAsset
        TSoftObjectPtr<UPaperFlipbook> BlinkFlipbook = BaseConfig.BlinkConfig.BlinkFlipbook;
        TSoftObjectPtr<UPaperFlipbook> TalkFlipbook = BaseConfig.TalkConfig.TalkFlipbook;
        TSoftObjectPtr<UPaperFlipbook> EyesFlipbook = BaseConfig.EyesRandomConfig.EyesDirectionsFlipbook;
        
        // Применяем базовую конфигурацию
        IdleAnimationManager->SetIdleAnimationsConfig(BaseConfig);
        
        // Применяем эмоциональные модификации
        SetIdleEmotionalState(EmotionState);
        
        VN_LOG_DEBUG(TEXT("Applied emotional state %d over DataAsset configuration"), (int32)EmotionState);
    }
    else
    {
        // Используем настройки из DataAsset как есть
        IdleAnimationManager->SetIdleAnimationsConfig(BaseConfig);
        VN_LOG_DEBUG(TEXT("Applied DataAsset configuration without emotional modifications"));
    }

    // Перезапускаем анимации если нужно  
    if (bRestartAnimations)
    {
        VN_LOG_DEBUG(TEXT("ApplyIdleAnimationDataAssetWithEmotionalState: Restarting animations"));
        IdleAnimationManager->StartAllIdleAnimations();
    }
}