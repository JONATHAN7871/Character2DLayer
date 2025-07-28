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