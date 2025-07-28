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

    // Убеждаемся, что кэш обновлен ПЕРЕД началом анимации.
    // Это критически важно для определения, были ли установлены веки изначально.
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
        // Восстанавливаем из кэша. Это вернет веки в то состояние,
        // в котором они были до начала серии морганий.
        Character->RestoreSpriteFromCache(E_VN_ComponentID_Sprite::Eyelids);
    }

    bIsBlinkAnimationPlaying = false;
    LogIdleAnimation(TEXT("Blink animation stopped"));
}

void UVNCharacterIdleAnimationManager::ScheduleNextBlink()
{
    if (!IdleAnimationsConfig.BlinkConfig.bEnabled) return;

    float Interval = IdleAnimationsConfig.BlinkConfig.GetRandomBlinkInterval();
    
    GetWorld()->GetTimerManager().SetTimer(
        BlinkTimerHandle,
        this,
        &UVNCharacterIdleAnimationManager::ExecuteBlink,
        Interval,
        false
    );
}

void UVNCharacterIdleAnimationManager::ExecuteBlink()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Eyelids_Sprite) return;

    UPaperFlipbook* BlinkFlipbook = IdleAnimationsConfig.BlinkConfig.BlinkFlipbook.LoadSynchronous();
    if (!BlinkFlipbook || GetFlipbookFrameCountImproved(BlinkFlipbook) < 2)
    {
        LogIdleAnimation(TEXT("Blink error: Flipbook is invalid or has less than 2 frames."));
        ScheduleNextBlink(); // Попробуем снова позже
        return;
    }

    // Определяем, будет ли это двойное моргание
    bPendingDoubleBlink = IdleAnimationsConfig.BlinkConfig.ShouldDoubleBlink();
    
    bIsBlinkAnimationPlaying = true;

    // НОВАЯ ЛОГИКА: Определяем, с какой фазы начинать анимацию
    TSoftObjectPtr<UPaperSprite> CachedEyelidsSprite = Character->GetCachedSprite(E_VN_ComponentID_Sprite::Eyelids);
    bool bHasInitialEyelids = !CachedEyelidsSprite.IsNull();

    if (bHasInitialEyelids)
    {
        // Сценарий 1: У персонажа уже есть веки.
        // Пропускаем фазу "полузакрытых" глаз и сразу переходим к полному закрытию.
        // Это создает эффект резкого, быстрого моргания.
        CurrentBlinkState = EBlinkState::FirstBlinkFull;
    }
    else
    {
        // Сценарий 2: У персонажа нет век (пустой спрайт).
        // Начинаем анимацию с фазы "полузакрытых" глаз (кадр 0).
        // Это создает эффект появления век для моргания.
        CurrentBlinkState = EBlinkState::FirstBlinkHalf;
    }

    // Запускаем машину состояний, которая выполнит первую фазу анимации
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

    UPaperSprite* HalfClosed = GetFlipbookSpriteImproved(BlinkFlipbook, 0); // Кадр 0
    UPaperSprite* Closed = GetFlipbookSpriteImproved(BlinkFlipbook, 1);     // Кадр 1
    
    if (!HalfClosed || !Closed)
    {
        LogIdleAnimation(TEXT("Blink error: Could not get valid frames from flipbook."), true);
        FinishBlinkAnimation();
        return;
    }

    float BlinkDuration = IdleAnimationsConfig.BlinkConfig.BlinkDuration;
    float DoubleBlinkPause = IdleAnimationsConfig.BlinkConfig.DoubleBlinkPause;

    switch (CurrentBlinkState)
    {
        case EBlinkState::FirstBlinkHalf:
        {
            // Эта фаза выполняется только если изначально век не было.
            Character->Eyelids_Sprite->SetSprite(HalfClosed); // Показываем полузакрытые глаза
            CurrentBlinkState = EBlinkState::FirstBlinkFull;
            
            GetWorld()->GetTimerManager().SetTimer(
                BlinkTimerHandle, this, &UVNCharacterIdleAnimationManager::UpdateBlinkState,
                BlinkDuration * 0.4f, // Короткая пауза
                false
            );
            break;
        }
        
        case EBlinkState::FirstBlinkFull:
        {
            // Эта фаза выполняется в обоих сценариях (с веками и без).
            Character->Eyelids_Sprite->SetSprite(Closed); // Показываем полностью закрытые глаза
            
            if (bPendingDoubleBlink)
            {
                // Если запланировано двойное моргание, переходим в состояние паузы между ними
                CurrentBlinkState = EBlinkState::BetweenBlinks;
                GetWorld()->GetTimerManager().SetTimer(
                    BlinkTimerHandle, this, &UVNCharacterIdleAnimationManager::UpdateBlinkState,
                    BlinkDuration * 0.6f, // Держим глаза закрытыми
                    false
                );
            }
            else
            {
                // Если моргание одиночное, просто завершаем его
                GetWorld()->GetTimerManager().SetTimer(
                    BlinkTimerHandle, this, &UVNCharacterIdleAnimationManager::FinishBlinkAnimation,
                    BlinkDuration * 0.6f, // Держим глаза закрытыми
                    false
                );
            }
            break;
        }
        
        case EBlinkState::BetweenBlinks:
        {
            // Пауза для двойного моргания. Возвращаем оригинальный спрайт век.
            Character->RestoreSpriteFromCache(E_VN_ComponentID_Sprite::Eyelids);
            
            // НОВАЯ ЛОГИКА для второго моргания: повторяем ту же логику, что и для первого.
            TSoftObjectPtr<UPaperSprite> CachedEyelidsSprite = Character->GetCachedSprite(E_VN_ComponentID_Sprite::Eyelids);
            if (!CachedEyelidsSprite.IsNull())
            {
                // Если веки были, второе моргание тоже будет резким (сразу к закрытым)
                CurrentBlinkState = EBlinkState::SecondBlinkFull;
            }
            else
            {
                // Если век не было, второе моргание тоже будет с фазой полузакрытия
                CurrentBlinkState = EBlinkState::SecondBlinkHalf;
            }

            GetWorld()->GetTimerManager().SetTimer(
                BlinkTimerHandle, this, &UVNCharacterIdleAnimationManager::UpdateBlinkState,
                DoubleBlinkPause, // Пауза с открытыми глазами
                false
            );
            break;
        }
        
        case EBlinkState::SecondBlinkHalf:
        {
            // Аналогично FirstBlinkHalf, но для второго моргания
            Character->Eyelids_Sprite->SetSprite(HalfClosed);
            CurrentBlinkState = EBlinkState::SecondBlinkFull;
            GetWorld()->GetTimerManager().SetTimer(
                BlinkTimerHandle, this, &UVNCharacterIdleAnimationManager::UpdateBlinkState,
                BlinkDuration * 0.3f,
                false
            );
            break;
        }
        
        case EBlinkState::SecondBlinkFull:
        {
            // Аналогично FirstBlinkFull, но для второго моргания
            Character->Eyelids_Sprite->SetSprite(Closed);
            GetWorld()->GetTimerManager().SetTimer(
                BlinkTimerHandle, this, &UVNCharacterIdleAnimationManager::FinishBlinkAnimation,
                BlinkDuration * 0.4f,
                false
            );
            break;
        }
        
        default:
            FinishBlinkAnimation();
            break;
    }
}

void UVNCharacterIdleAnimationManager::FinishBlinkAnimation()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Eyelids_Sprite)
    {
        UPaperSprite* CurrentSprite = Character->Eyelids_Sprite->GetSprite();
        
        // Проверяем, является ли текущий спрайт частью анимации.
        // Если да - восстанавливаем из кэша. Если нет - значит, его изменили извне, и мы не должны его трогать.
        if (IsCurrentSpritePartOfBlinkAnimation(CurrentSprite))
        {
            Character->RestoreSpriteFromCache(E_VN_ComponentID_Sprite::Eyelids);
        }
        else
        {
            // Спрайт был изменен извне - обновляем кэш, чтобы следующая анимация началась с нового спрайта
            Character->SetCachedSprite(E_VN_ComponentID_Sprite::Eyelids, CurrentSprite);
        }
    }

    bIsBlinkAnimationPlaying = false;
    CurrentBlinkState = EBlinkState::WaitingForBlink;
    bPendingDoubleBlink = false;
    
    // Планируем следующее моргание, если анимация все еще активна
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
    
    // Спрайт является частью анимации, если это кадр 0 или кадр 1 из флипбука
    UPaperSprite* HalfClosed = GetFlipbookSpriteImproved(BlinkFlipbook, 0);
    UPaperSprite* Closed = GetFlipbookSpriteImproved(BlinkFlipbook, 1);
    
    return (Sprite == HalfClosed || Sprite == Closed);
}