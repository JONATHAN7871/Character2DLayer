#include "Components/VNCharacterIdleAnimationManager.h"
#include "Actors/VNCharacter.h"
#include "VNCharacterSystemModule.h"
#include "Engine/World.h"

void UVNCharacterIdleAnimationManager::StartBlinkAnimation()
{
    if (!IdleAnimationsConfig.BlinkConfig.IsValid())
    {
        LogIdleAnimation(TEXT("Cannot start blink: invalid config"));
        return;
    }

    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Eyelids_Sprite)
    {
        LogIdleAnimation(TEXT("Cannot start blink: invalid character"));
        return;
    }

    // Убеждаемся, что кэш обновлен
    Character->UpdateSpriteCache();
    
    ScheduleNextBlink();
    LogIdleAnimation(TEXT("Blink animation started"));
}

void UVNCharacterIdleAnimationManager::StopBlinkAnimation()
{
    GetWorld()->GetTimerManager().ClearTimer(BlinkTimerHandle);
    
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Eyelids_Sprite)
    {
        // Восстанавливаем из кэша
        Character->RestoreSpriteFromCache(E_VN_ComponentID_Sprite::Eyelids);
    }

    bIsBlinkAnimationPlaying = false;
    LogIdleAnimation(TEXT("Blink animation stopped"));
}

void UVNCharacterIdleAnimationManager::ScheduleNextBlink()
{
    if (!IdleAnimationsConfig.BlinkConfig.bEnabled) return;

    float BaseInterval = IdleAnimationsConfig.BlinkConfig.GetRandomBlinkInterval();
    
    static int32 BlinkCounter = 0;
    BlinkCounter++;
    
    float EmotionalMultiplier = 1.0f;
    
    // Живые паттерны моргания
    if (BlinkCounter % 4 == 0)
    {
        EmotionalMultiplier = 0.3f; // Нервное быстрое моргание
    }
    else if (BlinkCounter % 7 == 0) 
    {
        EmotionalMultiplier = 2.5f; // Расслабленная долгая пауза
    }
    else if (BlinkCounter % 11 == 0)
    {
        EmotionalMultiplier = 0.6f; // Средне-быстрое
    }
    else if (BlinkCounter % 13 == 0)
    {
        EmotionalMultiplier = 1.8f; // Сонливое медленное
    }
    
    float FinalInterval = FMath::Clamp(BaseInterval * EmotionalMultiplier, 0.5f, 8.0f);
    
    VN_LOG_DEBUG(TEXT("Next blink in %.2f sec (base: %.2f, emotion: %.2f)"), 
        FinalInterval, BaseInterval, EmotionalMultiplier);
    
    GetWorld()->GetTimerManager().SetTimer(
        BlinkTimerHandle,
        this,
        &UVNCharacterIdleAnimationManager::ExecuteBlink,
        FinalInterval,
        false
    );
}

void UVNCharacterIdleAnimationManager::ExecuteBlink()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Eyelids_Sprite) return;

    UPaperFlipbook* BlinkFlipbook = IdleAnimationsConfig.BlinkConfig.BlinkFlipbook.LoadSynchronous();
    if (!BlinkFlipbook) return;

    UPaperSprite* HalfClosed = GetFlipbookSpriteImproved(BlinkFlipbook, 0);
    UPaperSprite* Closed = GetFlipbookSpriteImproved(BlinkFlipbook, 1);
    
    if (!HalfClosed || !Closed) return;

    static int32 ConsecutiveSingle = 0;
    float DoubleChance = IdleAnimationsConfig.BlinkConfig.DoubleBlinkChance;
    
    if (ConsecutiveSingle >= 3)
    {
        DoubleChance *= 2.0f;
        ConsecutiveSingle = 0;
    }
    
    bPendingDoubleBlink = FMath::RandRange(0.0f, 1.0f) <= DoubleChance;
    
    if (!bPendingDoubleBlink)
    {
        ConsecutiveSingle++;
    }

    bIsBlinkAnimationPlaying = true;
    CurrentBlinkState = EBlinkState::FirstBlinkHalf;
    UpdateBlinkState();
}

void UVNCharacterIdleAnimationManager::UpdateBlinkState()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Eyelids_Sprite)
    {
        FinishBlinkAnimation();
        return;
    }

    UPaperFlipbook* BlinkFlipbook = IdleAnimationsConfig.BlinkConfig.BlinkFlipbook.LoadSynchronous();
    if (!BlinkFlipbook)
    {
        FinishBlinkAnimation();
        return;
    }

    UPaperSprite* HalfClosed = GetFlipbookSpriteImproved(BlinkFlipbook, 0);
    UPaperSprite* Closed = GetFlipbookSpriteImproved(BlinkFlipbook, 1);
    
    if (!HalfClosed || !Closed)
    {
        FinishBlinkAnimation();
        return;
    }

    float BaseDuration = IdleAnimationsConfig.BlinkConfig.BlinkDuration;
    float VariableDuration = BaseDuration * FMath::RandRange(0.8f, 1.2f);
    float DoubleBlinkPause = IdleAnimationsConfig.BlinkConfig.DoubleBlinkPause;

    switch (CurrentBlinkState)
    {
        case EBlinkState::FirstBlinkHalf:
        {
            Character->Eyelids_Sprite->SetSprite(HalfClosed);
            CurrentBlinkState = EBlinkState::FirstBlinkFull;
            
            GetWorld()->GetTimerManager().SetTimer(
                BlinkTimerHandle,
                this,
                &UVNCharacterIdleAnimationManager::UpdateBlinkState,
                VariableDuration * 0.4f,
                false
            );
            break;
        }
        
        case EBlinkState::FirstBlinkFull:
        {
            Character->Eyelids_Sprite->SetSprite(Closed);
            
            if (bPendingDoubleBlink)
            {
                CurrentBlinkState = EBlinkState::BetweenBlinks;
                GetWorld()->GetTimerManager().SetTimer(
                    BlinkTimerHandle,
                    this,
                    &UVNCharacterIdleAnimationManager::UpdateBlinkState,
                    VariableDuration * 0.6f,
                    false
                );
            }
            else
            {
                GetWorld()->GetTimerManager().SetTimer(
                    BlinkTimerHandle,
                    this,
                    &UVNCharacterIdleAnimationManager::FinishBlinkAnimation,
                    VariableDuration * 0.6f,
                    false
                );
            }
            break;
        }
        
        case EBlinkState::BetweenBlinks:
        {
            // Возврат к оригиналу между двойными морганиями
            Character->RestoreSpriteFromCache(E_VN_ComponentID_Sprite::Eyelids);
            CurrentBlinkState = EBlinkState::SecondBlinkHalf;
            
            float VariablePause = DoubleBlinkPause * FMath::RandRange(0.5f, 1.5f);
            
            GetWorld()->GetTimerManager().SetTimer(
                BlinkTimerHandle,
                this,
                &UVNCharacterIdleAnimationManager::UpdateBlinkState,
                VariablePause,
                false
            );
            break;
        }
        
        case EBlinkState::SecondBlinkHalf:
        {
            Character->Eyelids_Sprite->SetSprite(HalfClosed);
            CurrentBlinkState = EBlinkState::SecondBlinkFull;
            
            GetWorld()->GetTimerManager().SetTimer(
                BlinkTimerHandle,
                this,
                &UVNCharacterIdleAnimationManager::UpdateBlinkState,
                VariableDuration * 0.3f,
                false
            );
            break;
        }
        
        case EBlinkState::SecondBlinkFull:
        {
            Character->Eyelids_Sprite->SetSprite(Closed);
            
            GetWorld()->GetTimerManager().SetTimer(
                BlinkTimerHandle,
                this,
                &UVNCharacterIdleAnimationManager::FinishBlinkAnimation,
                VariableDuration * 0.4f,
                false
            );
            break;
        }
    }
}

void UVNCharacterIdleAnimationManager::FinishBlinkAnimation()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Eyelids_Sprite)
    {
        UPaperSprite* CurrentSprite = Character->Eyelids_Sprite->GetSprite();
        
        if (IsCurrentSpritePartOfBlinkAnimation(CurrentSprite))
        {
            // Восстанавливаем из кэша
            Character->RestoreSpriteFromCache(E_VN_ComponentID_Sprite::Eyelids);
        }
        else
        {
            // Спрайт был изменен извне - обновляем кэш
            Character->SetCachedSprite(E_VN_ComponentID_Sprite::Eyelids, CurrentSprite);
        }
    }

    bIsBlinkAnimationPlaying = false;
    CurrentBlinkState = EBlinkState::WaitingForBlink;
    bPendingDoubleBlink = false;
    
    if (IdleAnimationsConfig.BlinkConfig.bEnabled)
    {
        ScheduleNextBlink();
    }
}

bool UVNCharacterIdleAnimationManager::IsCurrentSpritePartOfBlinkAnimation(UPaperSprite* Sprite) const
{
    if (!Sprite) return false;
    
    UPaperFlipbook* BlinkFlipbook = IdleAnimationsConfig.BlinkConfig.BlinkFlipbook.LoadSynchronous();
    if (!BlinkFlipbook) return false;
    
    UPaperSprite* HalfClosed = GetFlipbookSpriteImproved(BlinkFlipbook, 0);
    UPaperSprite* Closed = GetFlipbookSpriteImproved(BlinkFlipbook, 1);
    
    return (Sprite == HalfClosed || Sprite == Closed);
}