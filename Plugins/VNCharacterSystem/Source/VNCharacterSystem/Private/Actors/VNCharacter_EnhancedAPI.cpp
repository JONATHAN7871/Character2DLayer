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
    
    switch (EmotionState)
    {
        case EIdleEmotionalState::Calm:
            // Спокойное моргание - стандартные интервалы
            CurrentConfig.BlinkConfig.MinBlinkInterval = 3.5f;
            CurrentConfig.BlinkConfig.MaxBlinkInterval = 7.0f;
            CurrentConfig.BlinkConfig.BlinkDuration = 0.16f;        // Чуть медленнее
            CurrentConfig.BlinkConfig.DoubleBlinkChance = 0.15f;    // Редкие двойные
            CurrentConfig.BlinkConfig.DoubleBlinkPause = 0.12f;     // Спокойная пауза
            
            // Спокойные движения глаз
            CurrentConfig.EyesRandomConfig.MinLookDuration = 0.8f;
            CurrentConfig.EyesRandomConfig.MaxLookDuration = 1.5f;
            CurrentConfig.EyesRandomConfig.MinWaitDuration = 3.0f;
            CurrentConfig.EyesRandomConfig.MaxWaitDuration = 6.0f;
            
            // Медленная речь
            CurrentConfig.TalkConfig.TalkSpeed = 2.2f;
            break;
            
        case EIdleEmotionalState::Nervous:
            // Нервное быстрое моргание
            CurrentConfig.BlinkConfig.MinBlinkInterval = 0.8f;
            CurrentConfig.BlinkConfig.MaxBlinkInterval = 2.2f;
            CurrentConfig.BlinkConfig.BlinkDuration = 0.12f;        // Быстрее обычного
            CurrentConfig.BlinkConfig.DoubleBlinkChance = 0.65f;    // Много двойных морганий
            CurrentConfig.BlinkConfig.DoubleBlinkPause = 0.08f;     // Быстрая пауза
            
            // Беспокойные движения глаз
            CurrentConfig.EyesRandomConfig.MinLookDuration = 0.1f;
            CurrentConfig.EyesRandomConfig.MaxLookDuration = 0.4f;
            CurrentConfig.EyesRandomConfig.MinWaitDuration = 0.2f;
            CurrentConfig.EyesRandomConfig.MaxWaitDuration = 1.0f;
            
            // Быстрая нервная речь
            CurrentConfig.TalkConfig.TalkSpeed = 4.8f;
            break;
            
        case EIdleEmotionalState::Sleepy:
            // Сонное медленное моргание
            CurrentConfig.BlinkConfig.MinBlinkInterval = 2.0f;
            CurrentConfig.BlinkConfig.MaxBlinkInterval = 8.0f;
            CurrentConfig.BlinkConfig.BlinkDuration = 0.28f;        // Очень медленное
            CurrentConfig.BlinkConfig.DoubleBlinkChance = 0.05f;    // Почти никаких двойных
            CurrentConfig.BlinkConfig.DoubleBlinkPause = 0.20f;     // Долгая сонная пауза
            
            // Ленивые движения глаз
            CurrentConfig.EyesRandomConfig.MinLookDuration = 1.2f;
            CurrentConfig.EyesRandomConfig.MaxLookDuration = 2.5f;
            CurrentConfig.EyesRandomConfig.MinWaitDuration = 4.0f;
            CurrentConfig.EyesRandomConfig.MaxWaitDuration = 10.0f;
            
            // Медленная сонная речь
            CurrentConfig.TalkConfig.TalkSpeed = 1.8f;
            break;
            
        case EIdleEmotionalState::Excited:
            // Возбужденное живое моргание
            CurrentConfig.BlinkConfig.MinBlinkInterval = 1.2f;
            CurrentConfig.BlinkConfig.MaxBlinkInterval = 3.5f;
            CurrentConfig.BlinkConfig.BlinkDuration = 0.11f;        // Очень быстрое
            CurrentConfig.BlinkConfig.DoubleBlinkChance = 0.45f;    // Средне-высокие двойные
            CurrentConfig.BlinkConfig.DoubleBlinkPause = 0.06f;     // Очень быстрая пауза
            
            // Активные движения глаз
            CurrentConfig.EyesRandomConfig.MinLookDuration = 0.2f;
            CurrentConfig.EyesRandomConfig.MaxLookDuration = 0.7f;
            CurrentConfig.EyesRandomConfig.MinWaitDuration = 0.3f;
            CurrentConfig.EyesRandomConfig.MaxWaitDuration = 1.8f;
            
            // Быстрая возбужденная речь
            CurrentConfig.TalkConfig.TalkSpeed = 4.2f;
            break;
            
        case EIdleEmotionalState::Focused:
            // Сосредоточенное редкое моргание
            CurrentConfig.BlinkConfig.MinBlinkInterval = 8.0f;
            CurrentConfig.BlinkConfig.MaxBlinkInterval = 15.0f;
            CurrentConfig.BlinkConfig.BlinkDuration = 0.14f;        // Контролируемое
            CurrentConfig.BlinkConfig.DoubleBlinkChance = 0.02f;    // Практически никаких двойных
            CurrentConfig.BlinkConfig.DoubleBlinkPause = 0.15f;     // Размеренная пауза
            
            // Целенаправленные движения глаз
            CurrentConfig.EyesRandomConfig.MinLookDuration = 1.0f;
            CurrentConfig.EyesRandomConfig.MaxLookDuration = 3.0f;
            CurrentConfig.EyesRandomConfig.MinWaitDuration = 5.0f;
            CurrentConfig.EyesRandomConfig.MaxWaitDuration = 12.0f;
            
            // Размеренная речь
            CurrentConfig.TalkConfig.TalkSpeed = 2.5f;
            break;
            
        case EIdleEmotionalState::Tired:
            // Усталое неравномерное моргание
            CurrentConfig.BlinkConfig.MinBlinkInterval = 1.8f;
            CurrentConfig.BlinkConfig.MaxBlinkInterval = 9.0f;      // БОЛЬШОЙ разброс
            CurrentConfig.BlinkConfig.BlinkDuration = 0.22f;        // Медленное от усталости
            CurrentConfig.BlinkConfig.DoubleBlinkChance = 0.35f;    // Средние двойные от усталости
            CurrentConfig.BlinkConfig.DoubleBlinkPause = 0.18f;     // Усталая пауза
            
            // Усталые движения глаз
            CurrentConfig.EyesRandomConfig.MinLookDuration = 0.4f;
            CurrentConfig.EyesRandomConfig.MaxLookDuration = 2.0f;
            CurrentConfig.EyesRandomConfig.MinWaitDuration = 1.5f;
            CurrentConfig.EyesRandomConfig.MaxWaitDuration = 5.0f;
            
            // Усталая речь
            CurrentConfig.TalkConfig.TalkSpeed = 2.0f;
            break;
    }
    
    // Восстанавливаем flipbook'и
    CurrentConfig.BlinkConfig.BlinkFlipbook = SavedBlinkFlipbook;
    CurrentConfig.TalkConfig.TalkFlipbook = SavedTalkFlipbook;
    CurrentConfig.EyesRandomConfig.EyesDirectionsFlipbook = SavedEyesFlipbook;
    
    // Применяем обновленную конфигурацию
    IdleAnimationManager->SetIdleAnimationsConfig(CurrentConfig);
    
    VN_LOG_DEBUG(TEXT("SetIdleEmotionalState: Applied %d emotion state with realistic timing"), (int32)EmotionState);
}

// =====================================================
// ИСПРАВЛЕННЫЙ МЕТОД ДЛЯ IDLE ANIMATION DATAASSET
// =====================================================

void AVNCharacter::ApplyIdleAnimationDataAssetWithEmotionalState(UVNCharacterIdleAnimationDataAsset* IdleAnimationData, EIdleEmotionalState EmotionState, bool bRestartAnimations)
{
    if (!IdleAnimationData) {
        VN_LOG_WARNING(TEXT("ApplyIdle...: IdleAnimationData is null"));
        return;
    }
    if (!IdleAnimationManager) {
        VN_LOG_WARNING(TEXT("ApplyIdle...: IdleAnimationManager is null"));
        return;
    }

    VN_LOG_DEBUG(TEXT("Applying Idle DataAsset '%s' with EmotionState '%d'"), *IdleAnimationData->GetName(), (int32)EmotionState);

    // 1. ПОЛНАЯ ОСТАНОВКА И СБРОС.
    StopAndResetIdleAnimations();

    // 2. ПОЛУЧАЕМ НОВУЮ КОНФИГУРАЦИЮ.
    FVNIdleAnimationsConfig NewConfig = IdleAnimationData->GetIdleAnimationsConfig();

    // 3. ПРИМЕНЯЕМ КОНФИГУРАЦИЮ В МЕНЕДЖЕР.
    IdleAnimationManager->SetIdleAnimationsConfig(NewConfig);

    // 4. (Опционально) ПРИМЕНЯЕМ ЭМОЦИОНАЛЬНЫЕ НАСТРОЙКИ ПОВЕРХ.
    if (EmotionState != EIdleEmotionalState::None)
    {
        SetIdleEmotionalState(EmotionState);
    }
    
    // 5. (Опционально) ПЕРЕЗАПУСКАЕМ АНИМАЦИИ.
    if (bRestartAnimations)
    {
        IdleAnimationManager->StartAllIdleAnimations();
    }
}