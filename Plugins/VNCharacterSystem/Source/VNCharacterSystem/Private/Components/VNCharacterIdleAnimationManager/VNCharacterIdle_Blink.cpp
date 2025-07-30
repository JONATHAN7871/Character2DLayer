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

    // Определяем режим моргания в зависимости от наличия исходного спрайта
    UPaperSprite* InitialSprite = Character->Eyelids_Sprite->GetSprite();
    bHasInitialEyelidsSprite = (InitialSprite != nullptr);
    
    // ИСПРАВЛЕНИЕ: Принудительно кэшируем текущее состояние
    Character->SetCachedSprite(E_VN_ComponentID_Sprite::Eyelids, InitialSprite);
    
    if (bHasInitialEyelidsSprite)
    {
        VN_LOG_DEBUG(TEXT("StartBlinkAnimation: 2-phase mode (has initial sprite): %s"), *InitialSprite->GetName());
    }
    else
    {
        VN_LOG_DEBUG(TEXT("StartBlinkAnimation: 3-phase mode (no initial sprite - will create appearing eyelids effect)"));
    }
    
    ScheduleNextBlink();
    LogIdleAnimation(TEXT("Blink animation started"));
}

void UVNCharacterIdleAnimationManager::StopBlinkAnimation()
{
    GetWorld()->GetTimerManager().ClearTimer(BlinkTimerHandle);
    
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Eyelids_Sprite)
    {
        // Восстанавливаем исходное состояние (может быть nullptr)
        Character->RestoreSpriteFromCache(E_VN_ComponentID_Sprite::Eyelids);
        VN_LOG_DEBUG(TEXT("StopBlinkAnimation: Restored to %s"), 
            bHasInitialEyelidsSprite ? TEXT("initial sprite") : TEXT("empty"));
    }

    bIsBlinkAnimationPlaying = false;
    CurrentBlinkState = EBlinkState::WaitingForBlink;
    bPendingDoubleBlink = false;
    LogIdleAnimation(TEXT("Blink animation stopped"));
}

void UVNCharacterIdleAnimationManager::ScheduleNextBlink()
{
    if (!IdleAnimationsConfig.BlinkConfig.bEnabled) return;

    float BaseInterval = IdleAnimationsConfig.BlinkConfig.GetRandomBlinkInterval();
    
    static int32 BlinkCounter = 0;
    BlinkCounter++;
    
    float EmotionalMultiplier = 1.0f;
    
    if (BlinkCounter % 4 == 0)
    {
        EmotionalMultiplier = 0.3f;
    }
    else if (BlinkCounter % 7 == 0) 
    {
        EmotionalMultiplier = 2.5f;
    }
    else if (BlinkCounter % 11 == 0)
    {
        EmotionalMultiplier = 0.6f;
    }
    else if (BlinkCounter % 13 == 0)
    {
        EmotionalMultiplier = 1.8f;
    }
    
    float FinalInterval = FMath::Clamp(BaseInterval * EmotionalMultiplier, 0.5f, 8.0f);
    
    VN_LOG_DEBUG(TEXT("Next blink in %.2f sec (mode: %s)"), 
        FinalInterval, bHasInitialEyelidsSprite ? TEXT("2-phase") : TEXT("3-phase"));
    
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
    if (!BlinkFlipbook) 
    {
        VN_LOG_WARNING(TEXT("ExecuteBlink: No blink flipbook!"));
        return;
    }

    UPaperSprite* HalfClosed = GetFlipbookSpriteImproved(BlinkFlipbook, 0);
    UPaperSprite* Closed = GetFlipbookSpriteImproved(BlinkFlipbook, 1);
    
    if (!Closed)
    {
        VN_LOG_WARNING(TEXT("UpdateBlinkState: No closed sprite, finishing"));
        FinishBlinkAnimation();
        return;
    }

    float BaseDuration = IdleAnimationsConfig.BlinkConfig.BlinkDuration;
    float VariableDuration = BaseDuration * FMath::RandRange(0.8f, 1.2f);
    float DoubleBlinkPause = IdleAnimationsConfig.BlinkConfig.DoubleBlinkPause;

    VN_LOG_DEBUG(TEXT("UpdateBlinkState: State=%d, HasInitial=%s"), 
        (int32)CurrentBlinkState, bHasInitialEyelidsSprite ? TEXT("true") : TEXT("false"));

    switch (CurrentBlinkState)
    {
        case EBlinkState::FirstBlinkHalf:
        {
            // Только для 3-фазного режима (когда нет исходного спрайта)
            if (HalfClosed)
            {
                Character->Eyelids_Sprite->SetSprite(HalfClosed);
                VN_LOG_DEBUG(TEXT("UpdateBlinkState: Set half-closed sprite: %s"), *HalfClosed->GetName());
            }
            else
            {
                VN_LOG_WARNING(TEXT("UpdateBlinkState: No half-closed sprite for 3-phase mode!"));
            }
            CurrentBlinkState = EBlinkState::FirstBlinkFull;
            
            GetWorld()->GetTimerManager().SetTimer(
                BlinkTimerHandle,
                this,
                &UVNCharacterIdleAnimationManager::UpdateBlinkState,
                VariableDuration * 0.3f,
                false
            );
            break;
        }
        
        case EBlinkState::FirstBlinkFull:
        {
            Character->Eyelids_Sprite->SetSprite(Closed);
            VN_LOG_DEBUG(TEXT("UpdateBlinkState: Set closed sprite: %s"), *Closed->GetName());
            
            if (bPendingDoubleBlink)
            {
                CurrentBlinkState = EBlinkState::BetweenBlinks;
                GetWorld()->GetTimerManager().SetTimer(
                    BlinkTimerHandle,
                    this,
                    &UVNCharacterIdleAnimationManager::UpdateBlinkState,
                    VariableDuration * 0.4f,
                    false
                );
            }
            else
            {
                GetWorld()->GetTimerManager().SetTimer(
                    BlinkTimerHandle,
                    this,
                    &UVNCharacterIdleAnimationManager::FinishBlinkAnimation,
                    VariableDuration * 0.4f,
                    false
                );
            }
            break;
        }
        
        case EBlinkState::BetweenBlinks:
        {
            // Возврат к исходному состоянию между двойными морганиями
            Character->RestoreSpriteFromCache(E_VN_ComponentID_Sprite::Eyelids);
            VN_LOG_DEBUG(TEXT("UpdateBlinkState: Restored between double blinks"));
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
            // АДАПТИВНАЯ ЛОГИКА для второго моргания
            if (bHasInitialEyelidsSprite)
            {
                // 2-фазное: сразу к закрытым
                CurrentBlinkState = EBlinkState::SecondBlinkFull;
                UpdateBlinkState(); // Рекурсивно переходим к следующему состоянию
            }
            else
            {
                // 3-фазное: через полузакрытые
                if (HalfClosed)
                {
                    Character->Eyelids_Sprite->SetSprite(HalfClosed);
                    VN_LOG_DEBUG(TEXT("UpdateBlinkState: Second half-closed: %s"), *HalfClosed->GetName());
                }
                CurrentBlinkState = EBlinkState::SecondBlinkFull;
                
                GetWorld()->GetTimerManager().SetTimer(
                    BlinkTimerHandle,
                    this,
                    &UVNCharacterIdleAnimationManager::UpdateBlinkState,
                    VariableDuration * 0.2f,
                    false
                );
            }
            break;
        }
        
        case EBlinkState::SecondBlinkFull:
        {
            Character->Eyelids_Sprite->SetSprite(Closed);
            VN_LOG_DEBUG(TEXT("UpdateBlinkState: Second closed: %s"), *Closed->GetName());
            
            GetWorld()->GetTimerManager().SetTimer(
                BlinkTimerHandle,
                this,
                &UVNCharacterIdleAnimationManager::FinishBlinkAnimation,
                VariableDuration * 0.3f,
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
            // ИСПРАВЛЕНИЕ: Всегда восстанавливаем из актуального кэша
            Character->RestoreSpriteFromCache(E_VN_ComponentID_Sprite::Eyelids);
            
            // Обновляем режим на основе кэша
            TSoftObjectPtr<UPaperSprite> CachedSprite = Character->GetCachedSprite(E_VN_ComponentID_Sprite::Eyelids);
            bHasInitialEyelidsSprite = !CachedSprite.IsNull();
            
            VN_LOG_DEBUG(TEXT("FinishBlinkAnimation: Restored from cache, mode now: %s"), 
                bHasInitialEyelidsSprite ? TEXT("2-phase") : TEXT("3-phase"));
        }
        else
        {
            // Спрайт был изменен извне во время анимации - кэш уже должен быть обновлен
            UPaperSprite* NewSprite = Character->Eyelids_Sprite->GetSprite();
            bHasInitialEyelidsSprite = (NewSprite != nullptr);
            
            VN_LOG_DEBUG(TEXT("FinishBlinkAnimation: External change detected, mode updated to: %s"), 
                bHasInitialEyelidsSprite ? TEXT("2-phase") : TEXT("3-phase"));
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

void UVNCharacterIdleAnimationManager::UpdateBlinkState()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Eyelids_Sprite)
    {
        VN_LOG_WARNING(TEXT("UpdateBlinkState: No character or eyelids sprite, finishing"));
        FinishBlinkAnimation();
        return;
    }

    UPaperFlipbook* BlinkFlipbook = IdleAnimationsConfig.BlinkConfig.BlinkFlipbook.LoadSynchronous();
    if (!BlinkFlipbook)
    {
        VN_LOG_WARNING(TEXT("UpdateBlinkState: No blink flipbook, finishing"));
        FinishBlinkAnimation();
        return;
    }

    UPaperSprite* HalfClosed = GetFlipbookSpriteImproved(BlinkFlipbook, 0);
    UPaperSprite* Closed = GetFlipbookSpriteImproved(BlinkFlipbook, 1);
    
    VN_LOG_DEBUG(TEXT("ExecuteBlink: HalfClosed=%s, Closed=%s"), 
        HalfClosed ? *HalfClosed->GetName() : TEXT("NULL"),
        Closed ? *Closed->GetName() : TEXT("NULL"));
    
    // ИСПРАВЛЕНИЕ: Для 3-фазного режима нужны полузакрытые, для 2-фазного только закрытые
    if (bHasInitialEyelidsSprite && !Closed)
    {
        VN_LOG_WARNING(TEXT("ExecuteBlink: 2-phase mode needs Closed sprite but it's missing!"));
        return;
    }
    
    if (!bHasInitialEyelidsSprite && (!HalfClosed || !Closed))
    {
        VN_LOG_WARNING(TEXT("ExecuteBlink: 3-phase mode needs both HalfClosed and Closed sprites!"));
        return;
    }

    // Логика двойного моргания
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
    
    // АДАПТИВНЫЙ ВЫБОР НАЧАЛЬНОГО СОСТОЯНИЯ
    if (bHasInitialEyelidsSprite)
    {
        // 2-фазное моргание: исходный -> закрытые -> исходный
        CurrentBlinkState = EBlinkState::FirstBlinkFull;
        VN_LOG_DEBUG(TEXT("ExecuteBlink: Starting 2-phase blink (skip half-closed)"));
    }
    else
    {
        // 3-фазное моргание: пусто -> полузакрытые -> закрытые -> пусто
        CurrentBlinkState = EBlinkState::FirstBlinkHalf;
        VN_LOG_DEBUG(TEXT("ExecuteBlink: Starting 3-phase blink (with half-closed)"));
    }
    
    UpdateBlinkState();
}