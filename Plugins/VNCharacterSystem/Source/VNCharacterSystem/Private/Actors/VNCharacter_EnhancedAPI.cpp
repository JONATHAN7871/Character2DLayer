// VNCharacter_EnhancedAPI.cpp - Улучшенное API для живого моргания

#include "Actors/VNCharacter.h"
#include "Components/VNCharacterIdleAnimationManager.h"
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

    FVNIdleAnimationsConfig NewConfig = IdleAnimationManager->GetIdleAnimationsConfig();
    
    float BaseMin = NewConfig.BlinkConfig.MinBlinkInterval;
    float BaseMax = NewConfig.BlinkConfig.MaxBlinkInterval;
    float BaseDuration = NewConfig.BlinkConfig.BlinkDuration;
    float BaseDoubleChance = NewConfig.BlinkConfig.DoubleBlinkChance;
    
    switch (EmotionState)
    {
        case EIdleEmotionalState::Calm:
            // Стандартные значения
            break;
            
        case EIdleEmotionalState::Nervous:
            NewConfig.BlinkConfig.MinBlinkInterval = BaseMin * 0.3f;
            NewConfig.BlinkConfig.MaxBlinkInterval = BaseMax * 0.5f;
            NewConfig.BlinkConfig.DoubleBlinkChance = BaseDoubleChance * 2.0f;
            break;
            
        case EIdleEmotionalState::Sleepy:
            NewConfig.BlinkConfig.MinBlinkInterval = BaseMin * 2.0f;
            NewConfig.BlinkConfig.MaxBlinkInterval = BaseMax * 3.0f;
            NewConfig.BlinkConfig.BlinkDuration = BaseDuration * 1.5f;
            break;
            
        case EIdleEmotionalState::Excited:
            NewConfig.BlinkConfig.MinBlinkInterval = BaseMin * 0.4f;
            NewConfig.BlinkConfig.MaxBlinkInterval = BaseMax * 0.7f;
            NewConfig.BlinkConfig.BlinkDuration = BaseDuration * 0.8f;
            break;
            
        case EIdleEmotionalState::Focused:
            NewConfig.BlinkConfig.MinBlinkInterval = BaseMin * 3.0f;
            NewConfig.BlinkConfig.MaxBlinkInterval = BaseMax * 4.0f;
            NewConfig.BlinkConfig.DoubleBlinkChance = BaseDoubleChance * 0.2f;
            break;
            
        case EIdleEmotionalState::Tired:
            NewConfig.BlinkConfig.MinBlinkInterval = BaseMin * 0.8f;
            NewConfig.BlinkConfig.MaxBlinkInterval = BaseMax * 2.5f;
            NewConfig.BlinkConfig.BlinkDuration = BaseDuration * 1.3f;
            NewConfig.BlinkConfig.DoubleBlinkChance = BaseDoubleChance * 1.5f;
            break;
    }
    
    // Ограничиваем значения
    NewConfig.BlinkConfig.MinBlinkInterval = FMath::Clamp(NewConfig.BlinkConfig.MinBlinkInterval, 0.3f, 8.0f);
    NewConfig.BlinkConfig.MaxBlinkInterval = FMath::Clamp(NewConfig.BlinkConfig.MaxBlinkInterval, NewConfig.BlinkConfig.MinBlinkInterval, 20.0f);
    NewConfig.BlinkConfig.BlinkDuration = FMath::Clamp(NewConfig.BlinkConfig.BlinkDuration, 0.05f, 1.0f);
    NewConfig.BlinkConfig.DoubleBlinkChance = FMath::Clamp(NewConfig.BlinkConfig.DoubleBlinkChance, 0.0f, 1.0f);
    
    IdleAnimationManager->SetIdleAnimationsConfig(NewConfig);
    
    VN_LOG_DEBUG(TEXT("SetIdleEmotionalState: Applied %d emotion state"), (int32)EmotionState);
}