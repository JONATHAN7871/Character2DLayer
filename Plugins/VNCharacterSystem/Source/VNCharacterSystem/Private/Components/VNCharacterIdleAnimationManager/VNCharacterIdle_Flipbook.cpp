// VNCharacterIdle_Flipbook.cpp - Упрощенная работа с flipbook

#include "Components/VNCharacterIdleAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "PaperFlipbook.h"

// =====================================================
// УПРОЩЕННЫЕ УТИЛИТЫ ДЛЯ РАБОТЫ С FLIPBOOK
// =====================================================

UPaperSprite* UVNCharacterIdleAnimationManager::GetFlipbookSpriteImproved(UPaperFlipbook* Flipbook, int32 FrameIndex) const
{
    if (!Flipbook)
    {
        UE_LOG(LogTemp, Error, TEXT("GetFlipbookSpriteImproved: Flipbook is null"));
        return nullptr;
    }

    // ПРОСТОЕ РЕШЕНИЕ: Используем GetSpriteAtFrame
    UPaperSprite* Sprite = Flipbook->GetSpriteAtFrame(FrameIndex);
    
    if (!Sprite)
    {
        UE_LOG(LogTemp, Warning, TEXT("GetFlipbookSpriteImproved: No sprite at frame %d"), FrameIndex);
        
        // АЛЬТЕРНАТИВНЫЙ СПОСОБ: Попробуем через время
        float TotalDuration = Flipbook->GetTotalDuration();
        if (TotalDuration > 0.0f)
        {
            int32 NumFrames = Flipbook->GetNumFrames();
            if (NumFrames > 0 && FrameIndex < NumFrames)
            {
                float TimePoint = (TotalDuration / NumFrames) * FrameIndex;
                Sprite = Flipbook->GetSpriteAtTime(TimePoint);
                UE_LOG(LogTemp, Warning, TEXT("GetFlipbookSpriteImproved: Got sprite via time method at %.3f"), TimePoint);
            }
        }
    }
    
    return Sprite;
}

UPaperSprite* UVNCharacterIdleAnimationManager::GetRandomFlipbookSpriteImproved(UPaperFlipbook* Flipbook, bool bExcludeFirstFrame) const
{
    if (!Flipbook) return nullptr;

    // ПРОСТОЕ РЕШЕНИЕ: Используем встроенный метод GetNumFrames!
    int32 TotalFrames = Flipbook->GetNumFrames();
    if (TotalFrames <= 0) return nullptr;

    int32 StartIndex = bExcludeFirstFrame ? 1 : 0;
    int32 MaxIndex = TotalFrames - 1;
    
    if (StartIndex > MaxIndex) return nullptr;

    int32 RandomIndex = FMath::RandRange(StartIndex, MaxIndex);
    
    // ПРОСТОЕ РЕШЕНИЕ: Прямой доступ к кадру!
    return Flipbook->GetSpriteAtFrame(RandomIndex);
}

int32 UVNCharacterIdleAnimationManager::GetFlipbookFrameCountImproved(UPaperFlipbook* Flipbook) const
{
    if (!Flipbook) return 0;
    
    // ПРОСТОЕ РЕШЕНИЕ: Встроенный метод!
    return Flipbook->GetNumFrames();
}

// =====================================================
// DEBUG МЕТОД - РЕАЛИЗАЦИЯ
// =====================================================

void UVNCharacterIdleAnimationManager::DebugFlipbook(UPaperFlipbook* Flipbook) const
{
    if (!Flipbook)
    {
        VN_LOG_WARNING(TEXT("DebugFlipbook: Flipbook is null"));
        return;
    }

    VN_LOG(Log, TEXT("=== FLIPBOOK DEBUG INFO ==="));
    VN_LOG(Log, TEXT("Flipbook Name: %s"), *Flipbook->GetName());
    VN_LOG(Log, TEXT("Flipbook Path: %s"), *Flipbook->GetPathName());
    
    int32 FrameCount = Flipbook->GetNumFrames();
    float TotalDuration = Flipbook->GetTotalDuration();
    
    VN_LOG(Log, TEXT("Frame Count: %d"), FrameCount);
    VN_LOG(Log, TEXT("Total Duration: %.3f seconds"), TotalDuration);
    VN_LOG(Log, TEXT("Average Frame Duration: %.3f seconds"), FrameCount > 0 ? TotalDuration / FrameCount : 0.0f);
    
    // Проверяем первые несколько кадров
    int32 MaxFramesToCheck = FMath::Min(FrameCount, 5);
    VN_LOG(Log, TEXT("First %d frames:"), MaxFramesToCheck);
    
    for (int32 i = 0; i < MaxFramesToCheck; ++i)
    {
        UPaperSprite* Sprite = Flipbook->GetSpriteAtFrame(i);
        if (Sprite)
        {
            VN_LOG(Log, TEXT("  Frame %d: %s"), i, *Sprite->GetName());
        }
        else
        {
            VN_LOG(Log, TEXT("  Frame %d: NULL SPRITE"), i);
        }
    }
    
    // Проверяем временные точки
    if (TotalDuration > 0.0f)
    {
        VN_LOG(Log, TEXT("Time-based sprite check:"));
        TArray<float> TestTimes = {0.0f, TotalDuration * 0.25f, TotalDuration * 0.5f, TotalDuration * 0.75f, TotalDuration};
        
        for (float TestTime : TestTimes)
        {
            UPaperSprite* Sprite = Flipbook->GetSpriteAtTime(TestTime);
            if (Sprite)
            {
                VN_LOG(Log, TEXT("  Time %.3fs: %s"), TestTime, *Sprite->GetName());
            }
            else
            {
                VN_LOG(Log, TEXT("  Time %.3fs: NULL SPRITE"), TestTime);
            }
        }
    }
    
    VN_LOG(Log, TEXT("=== END FLIPBOOK DEBUG ==="));
}