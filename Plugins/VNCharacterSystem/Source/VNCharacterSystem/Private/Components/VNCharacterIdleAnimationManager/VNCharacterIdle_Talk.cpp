#include "Components/VNCharacterIdleAnimationManager.h"
#include "Actors/VNCharacter.h"
#include "VNCharacterSystemModule.h"
#include "Engine/World.h"

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

    VN_LOG_DEBUG(TEXT("StartTalkAnimation: Starting talk animation"));

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
    VN_LOG_DEBUG(TEXT("StopTalkAnimation: Stopping talk animation"));
    
    // ИСПРАВЛЕНИЕ: Сначала очищаем таймер
    GetWorld()->GetTimerManager().ClearTimer(TalkTimerHandle);
    
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Mouth_Sprite)
    {
        UPaperSprite* CurrentSprite = Character->Mouth_Sprite->GetSprite();
        
        if (IsCurrentSpritePartOfTalkAnimation(CurrentSprite))
        {
            // Восстанавливаем из кэша
            Character->RestoreSpriteFromCache(E_VN_ComponentID_Sprite::Mouth);
            VN_LOG_DEBUG(TEXT("StopTalkAnimation: Restored mouth from cache"));
        }
        else
        {
            // Спрайт был изменен извне - обновляем кэш
            Character->SetCachedSprite(E_VN_ComponentID_Sprite::Mouth, CurrentSprite);
            VN_LOG_DEBUG(TEXT("StopTalkAnimation: Updated cache with external mouth sprite"));
        }
    }
    
    LogIdleAnimation(TEXT("Talk animation stopped"));
}

void UVNCharacterIdleAnimationManager::UpdateTalkFrame()
{
    // ИСПРАВЛЕНИЕ: Проверяем, что анимация еще должна работать
    if (!IdleAnimationsConfig.TalkConfig.bEnabled)
    {
        VN_LOG_DEBUG(TEXT("UpdateTalkFrame: Animation disabled, stopping"));
        StopTalkAnimation();
        return;
    }

    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Mouth_Sprite)
    {
        VN_LOG_DEBUG(TEXT("UpdateTalkFrame: No character or mouth sprite, stopping"));
        StopTalkAnimation();
        return;
    }

    UPaperFlipbook* TalkFlipbook = IdleAnimationsConfig.TalkConfig.TalkFlipbook.LoadSynchronous();
    if (!TalkFlipbook)
    {
        VN_LOG_DEBUG(TEXT("UpdateTalkFrame: No talk flipbook, stopping"));
        StopTalkAnimation();
        return;
    }

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