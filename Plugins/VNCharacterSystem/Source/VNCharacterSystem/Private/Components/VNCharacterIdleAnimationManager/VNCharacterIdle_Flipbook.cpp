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
        VN_LOG_ERROR(TEXT("GetFlipbookSpriteImproved: Flipbook is null"));
        return nullptr;
    }

    // ПРОСТОЕ РЕШЕНИЕ: Используем встроенный метод UE!
    return Flipbook->GetSpriteAtFrame(FrameIndex);
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
// УСТАРЕВШИЕ МЕТОДЫ (СОВМЕСТИМОСТЬ)
// =====================================================

UPaperSprite* UVNCharacterIdleAnimationManager::GetSpriteFromFlipbook(UPaperFlipbook* Flipbook, int32 FrameIndex) const
{
    return GetFlipbookSpriteImproved(Flipbook, FrameIndex);
}

int32 UVNCharacterIdleAnimationManager::GetFlipbookFrameCount(UPaperFlipbook* Flipbook) const
{
    return GetFlipbookFrameCountImproved(Flipbook);
}

UPaperSprite* UVNCharacterIdleAnimationManager::GetRandomSpriteFromFlipbook(UPaperFlipbook* Flipbook, bool bExcludeFirstFrame) const
{
    return GetRandomFlipbookSpriteImproved(Flipbook, bExcludeFirstFrame);
}

void UVNCharacterIdleAnimationManager::SaveOriginalSprite(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite>& OriginalSprite)
{
    SaveCurrentSprite(Component, OriginalSprite);
}

void UVNCharacterIdleAnimationManager::RestoreOriginalSprite(UPaperSpriteComponent* Component, const TSoftObjectPtr<UPaperSprite>& OriginalSprite)
{
    RestoreCurrentSprite(Component, OriginalSprite);
}