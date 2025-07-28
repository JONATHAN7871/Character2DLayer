// VNCharacterIdle_Talk.cpp - Анимация разговора с исправлением пропадания рта

#include "Components/VNCharacterIdleAnimationManager.h"
#include "Actors/VNCharacter.h"
#include "VNCharacterSystemModule.h"
#include "Engine/World.h"

// =====================================================
// СИСТЕМА АНИМАЦИИ РАЗГОВОРА
// =====================================================

void UVNCharacterIdleAnimationManager::StartTalkAnimation()
{
    if (!IdleAnimationsConfig.TalkConfig.IsValid())
    {
        LogIdleAnimation(TEXT("Cannot start talk: invalid config"));
        return;
    }

    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Mouth_Sprite)
    {
        LogIdleAnimation(TEXT("Cannot start talk: invalid character"));
        return;
    }

    // ИСПРАВЛЕНИЕ: Сохраняем ТЕКУЩИЙ спрайт рта
    SaveCurrentSprite(Character->Mouth_Sprite, OriginalMouthSprite);
    
    float FrameInterval = IdleAnimationsConfig.TalkConfig.GetFrameInterval();
    GetWorld()->GetTimerManager().SetTimer(
        TalkTimerHandle,
        this,
        &UVNCharacterIdleAnimationManager::UpdateTalkFrame,
        FrameInterval,
        true
    );
    
    LogIdleAnimation(TEXT("Talk animation started"));
}

void UVNCharacterIdleAnimationManager::StopTalkAnimation()
{
    GetWorld()->GetTimerManager().ClearTimer(TalkTimerHandle);
    
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Mouth_Sprite)
    {
        // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: Умная проверка восстановления
        UPaperSprite* CurrentSprite = Character->Mouth_Sprite->GetSprite();
        
        if (IsCurrentSpritePartOfTalkAnimation(CurrentSprite))
        {
            // Текущий спрайт - часть Talk анимации, восстанавливаем оригинал
            RestoreCurrentSprite(Character->Mouth_Sprite, OriginalMouthSprite);
            VN_LOG_DEBUG(TEXT("Talk: Restored original mouth sprite"));
        }
        else
        {
            // Спрайт был изменен извне во время анимации - оставляем как есть
            OriginalMouthSprite = CurrentSprite;
            VN_LOG_DEBUG(TEXT("Talk: Keeping externally changed mouth sprite"));
        }
    }
    
    LogIdleAnimation(TEXT("Talk animation stopped"));
}

void UVNCharacterIdleAnimationManager::UpdateTalkFrame()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Mouth_Sprite)
    {
        StopTalkAnimation();
        return;
    }

    UPaperFlipbook* TalkFlipbook = IdleAnimationsConfig.TalkConfig.TalkFlipbook.LoadSynchronous();
    if (!TalkFlipbook)
    {
        StopTalkAnimation();
        return;
    }

    // Получаем случайный кадр из flipbook
    UPaperSprite* RandomSprite = GetRandomFlipbookSpriteImproved(TalkFlipbook, false);
    if (RandomSprite)
    {
        Character->Mouth_Sprite->SetSprite(RandomSprite);
    }
}

bool UVNCharacterIdleAnimationManager::IsCurrentSpritePartOfTalkAnimation(UPaperSprite* Sprite) const
{
    if (!Sprite) return false;
    
    UPaperFlipbook* TalkFlipbook = IdleAnimationsConfig.TalkConfig.TalkFlipbook.LoadSynchronous();
    if (!TalkFlipbook) return false;
    
    return IsFlipbookSprite(TalkFlipbook, Sprite);
}