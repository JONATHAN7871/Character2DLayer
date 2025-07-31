#include "Components/VNCharacterIdleAnimationManager.h"
#include "Actors/VNCharacter.h"
#include "VNCharacterSystemModule.h"
#include "Engine/World.h"

void UVNCharacterIdleAnimationManager::StartTalkAnimation()
{
    if (!IdleAnimationsConfig.TalkConfig.IsValid() || !GetWorld()) return;
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Mouth_Sprite) return;

    FString CachedSpriteName = Character->GetCachedSprite(E_VN_ComponentID_Sprite::Mouth).ToString();
    UE_LOG(LogTemp, Error, TEXT(">>> StartTalkAnimation. Mouth Cache at start is: %s"), *CachedSpriteName);

    float FrameInterval = IdleAnimationsConfig.TalkConfig.GetFrameInterval();
    GetWorld()->GetTimerManager().SetTimer(TalkTimerHandle, this, &UVNCharacterIdleAnimationManager::UpdateTalkFrame, FrameInterval, true);
}

void UVNCharacterIdleAnimationManager::StopTalkAnimation()
{
    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(TalkTimerHandle);
    }
    
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Mouth_Sprite) {
        FString CachedSpriteName = Character->GetCachedSprite(E_VN_ComponentID_Sprite::Mouth).ToString();
        UE_LOG(LogTemp, Error, TEXT("<<< StopTalkAnimation. Mouth Cache right before restore is: %s"), *CachedSpriteName);
        Character->RestoreSpriteFromCache(E_VN_ComponentID_Sprite::Mouth);
    }
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
        
        // ИСПРАВЛЕНИЕ: Применяем цвет в зависимости от кэша
        TSoftObjectPtr<UPaperSprite> CachedSprite = Character->GetCachedSprite(E_VN_ComponentID_Sprite::Mouth);
        if (!CachedSprite.IsNull())
        {
            // В кэше есть спрайт - используем цвет из кэша
            Character->ApplyComponentColorWithFocus(Character->Mouth_Sprite);
        }
        else
        {
            // В кэше NULL - используем цвет из конфигурации анимации или белый
            FLinearColor AnimationColor = IdleAnimationsConfig.TalkConfig.bUseCustomTalkColor ? 
                IdleAnimationsConfig.TalkConfig.TalkColor : FLinearColor::White;
            
            FLinearColor FinalColor = Character->ApplyFocusToColor(AnimationColor);
            Character->SetComponentColor(Character->Mouth_Sprite, FinalColor);
        }
    }
}

bool UVNCharacterIdleAnimationManager::IsCurrentSpritePartOfTalkAnimation(UPaperSprite* Sprite) const
{
    if (!Sprite) return false;
    
    UPaperFlipbook* TalkFlipbook = IdleAnimationsConfig.TalkConfig.TalkFlipbook.LoadSynchronous();
    if (!TalkFlipbook) return false;
    
    return IsFlipbookSprite(TalkFlipbook, Sprite);
}