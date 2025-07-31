#include "Components/VNCharacterIdleAnimationManager.h"
#include "Actors/VNCharacter.h"
#include "VNCharacterSystemModule.h"
#include "Engine/World.h"

void UVNCharacterIdleAnimationManager::StartBlinkAnimation()
{
    if (!IdleAnimationsConfig.BlinkConfig.IsValid() || !IdleAnimationsConfig.BlinkConfig.BlinkFlipbook.LoadSynchronous())
    {
        LogIdleAnimation(TEXT("Cannot start blink: invalid config or no flipbook."));
        return;
    }

    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Eyelids_Sprite)
    {
        LogIdleAnimation(TEXT("Cannot start blink: invalid character or Eyelids_Sprite component."));
        return;
    }

    // Сохраняем в кэш И спрайт, И текущее состояние видимости.
    // Это наш "снимок" того, как было до моргания.
    UPaperSprite* CurrentSprite = Character->Eyelids_Sprite->GetSprite();
    Character->SetCachedSprite(E_VN_ComponentID_Sprite::Eyelids, CurrentSprite);
    
    VN_LOG_DEBUG(TEXT("StartBlinkAnimation: Caching initial eyelids state (Sprite: %s)"), 
        CurrentSprite ? *CurrentSprite->GetName() : TEXT("NULL"));
    
    ScheduleNextBlink();
    LogIdleAnimation(TEXT("Blink animation started"));
}

void UVNCharacterIdleAnimationManager::StopBlinkAnimation()
{
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(BlinkTimerHandle);
    }
    bIsBlinkAnimationPlaying = false;

    // Восстанавливаем исходное состояние при полной остановке
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Eyelids_Sprite)
    {
        Character->RestoreSpriteFromCache(E_VN_ComponentID_Sprite::Eyelids);
        
        // Если в кэше был NULL, значит исходное состояние было "скрыто".
        TSoftObjectPtr<UPaperSprite> CachedSprite = Character->GetCachedSprite(E_VN_ComponentID_Sprite::Eyelids);
        if (CachedSprite.IsNull())
        {
            Character->Eyelids_Sprite->SetVisibility(false);
            VN_LOG_DEBUG(TEXT("StopBlinkAnimation: Restored to EMPTY state. Component hidden."));
        }
    }
    
    LogIdleAnimation(TEXT("Blink animation stopped"));
}

void UVNCharacterIdleAnimationManager::ScheduleNextBlink()
{
    if (!IdleAnimationsConfig.BlinkConfig.bEnabled) return;

    UWorld* World = GetWorld();
    if (!World) return;

    float NextBlinkDelay = IdleAnimationsConfig.BlinkConfig.GetRandomBlinkInterval();
    
    World->GetTimerManager().SetTimer(
        BlinkTimerHandle,
        this,
        &UVNCharacterIdleAnimationManager::ExecuteBlink,
        NextBlinkDelay,
        false
    );
}

void UVNCharacterIdleAnimationManager::ExecuteBlink()
{
    if (!IdleAnimationsConfig.BlinkConfig.bEnabled) return;

    UPaperFlipbook* BlinkFlipbook = IdleAnimationsConfig.BlinkConfig.BlinkFlipbook.LoadSynchronous();
    if (!BlinkFlipbook || BlinkFlipbook->GetNumFrames() == 0) 
    {
        ScheduleNextBlink();
        return;
    }
    
    bIsBlinkAnimationPlaying = true;
    float BlinkDuration = IdleAnimationsConfig.BlinkConfig.BlinkDuration;
    
    // ФАЗА 1: Закрываем глаза
    ShowEyelidsAndSetSprite(BlinkFlipbook->GetSpriteAtFrame(BlinkFlipbook->GetNumFrames() - 1)); // Последний кадр - закрытые глаза

    float TimeToOpen = BlinkDuration / 2.0f;
    GetWorld()->GetTimerManager().SetTimer(BlinkTimerHandle, this, &UVNCharacterIdleAnimationManager::FinishBlinkAnimation, TimeToOpen, false);
}

void UVNCharacterIdleAnimationManager::BlinkPhase2_FullyClosed()
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

    // ФАЗА 2: Полностью закрытые глаза (кадр 1) - КОРОТКАЯ ПАУЗА
    int32 NumFrames = BlinkFlipbook->GetNumFrames();
    UPaperSprite* FullyClosed = GetFlipbookSpriteImproved(BlinkFlipbook, NumFrames - 1);
    
    if (FullyClosed)
    {
        // ИСПРАВЛЕНИЕ: Правильный порядок
        Character->Eyelids_Sprite->SetSprite(FullyClosed);  // Сначала спрайт
        ApplyBlinkColorToEyelids(Character);                 // Потом цвет и видимость
        
        UE_LOG(LogTemp, Warning, TEXT("ExecuteBlink: Phase 2 - Fully closed eyes (brief pause)"));
    }
    
    float BlinkDuration = IdleAnimationsConfig.BlinkConfig.BlinkDuration;
    
    // РЕАЛИСТИЧНОЕ ВРЕМЯ: Очень короткая пауза в закрытом состоянии (20% времени)
    GetWorld()->GetTimerManager().SetTimer(
        BlinkTimerHandle,
        this,
        &UVNCharacterIdleAnimationManager::BlinkPhase3_HalfOpen,
        BlinkDuration * 0.20f, // 20% времени - короткая пауза закрытыми
        false
    );
}

void UVNCharacterIdleAnimationManager::BlinkPhase3_HalfOpen()
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

    // ФАЗА 3: Быстро снова полуоткрытые глаза (кадр 0)
    UPaperSprite* HalfClosed = GetFlipbookSpriteImproved(BlinkFlipbook, 0);
    
    if (HalfClosed)
    {
        Character->Eyelids_Sprite->SetSprite(HalfClosed);
        ApplyBlinkColorToEyelids(Character);
        
        UE_LOG(LogTemp, Warning, TEXT("ExecuteBlink: Phase 3 - Half closed eyes (fast opening)"));
    }
    
    float BlinkDuration = IdleAnimationsConfig.BlinkConfig.BlinkDuration;
    
    // ПРОВЕРЯЕМ ДВОЙНОЕ МОРГАНИЕ
    bool bShouldDoSecondBlink = FMath::RandRange(0.0f, 1.0f) <= IdleAnimationsConfig.BlinkConfig.DoubleBlinkChance;
    
    if (bShouldDoSecondBlink)
    {
        UE_LOG(LogTemp, Warning, TEXT("ExecuteBlink: Starting second blink in double sequence"));
        
        // РЕАЛИСТИЧНАЯ ПАУЗА между морганиями в двойной последовательности
        GetWorld()->GetTimerManager().SetTimer(
            BlinkTimerHandle,
            this,
            &UVNCharacterIdleAnimationManager::BlinkPhase4_SecondClosed,
            IdleAnimationsConfig.BlinkConfig.DoubleBlinkPause, // Используем настроенную паузу
            false
        );
    }
    else
    {
        // РЕАЛИСТИЧНОЕ ВРЕМЯ: Более медленное полное открытие (50% времени)
        GetWorld()->GetTimerManager().SetTimer(
            BlinkTimerHandle,
            this,
            &UVNCharacterIdleAnimationManager::FinishBlinkAnimation,
            BlinkDuration * 0.50f, // 50% времени - медленное открытие
            false
        );
    }
}

void UVNCharacterIdleAnimationManager::BlinkPhase4_SecondClosed()
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

    // ФАЗА 4: Второе быстрое закрытие глаз
    int32 NumFrames = BlinkFlipbook->GetNumFrames();
    UPaperSprite* FullyClosed = GetFlipbookSpriteImproved(BlinkFlipbook, NumFrames - 1);
    
    if (FullyClosed)
    {
        Character->Eyelids_Sprite->SetSprite(FullyClosed);
        ApplyBlinkColorToEyelids(Character);
        
        UE_LOG(LogTemp, Warning, TEXT("ExecuteBlink: Phase 4 - Second quick close (double blink)"));
    }
    
    float BlinkDuration = IdleAnimationsConfig.BlinkConfig.BlinkDuration;
    
    // РЕАЛИСТИЧНОЕ ВРЕМЯ: Второе закрытие ещё быстрее (10% времени)
    GetWorld()->GetTimerManager().SetTimer(
        BlinkTimerHandle,
        this,
        &UVNCharacterIdleAnimationManager::BlinkPhase5_FinalHalfOpen,
        BlinkDuration * 0.10f, // 10% времени - очень быстрое второе закрытие
        false
    );
}

void UVNCharacterIdleAnimationManager::BlinkPhase5_FinalHalfOpen()
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

    // ФАЗА 5: Финальные полуоткрытые глаза
    UPaperSprite* HalfClosed = GetFlipbookSpriteImproved(BlinkFlipbook, 0);
    
    if (HalfClosed)
    {
        Character->Eyelids_Sprite->SetSprite(HalfClosed);
        ApplyBlinkColorToEyelids(Character);
        
        UE_LOG(LogTemp, Warning, TEXT("ExecuteBlink: Phase 5 - Final half open (double blink)"));
    }
    
    float BlinkDuration = IdleAnimationsConfig.BlinkConfig.BlinkDuration;
    
    // РЕАЛИСТИЧНОЕ ВРЕМЯ: Медленное финальное открытие (60% времени)
    GetWorld()->GetTimerManager().SetTimer(
        BlinkTimerHandle,
        this,
        &UVNCharacterIdleAnimationManager::FinishBlinkAnimation,
        BlinkDuration * 0.60f, // 60% времени - самое медленное открытие
        false
    );
}

void UVNCharacterIdleAnimationManager::FinishBlinkAnimation()
{
    bIsBlinkAnimationPlaying = false;
    AVNCharacter* Character = GetVNCharacterOwner();
    
    if (Character && Character->Eyelids_Sprite)
    {
        // Восстанавливаем спрайт и видимость из кэша
        Character->RestoreSpriteFromCache(E_VN_ComponentID_Sprite::Eyelids);
        TSoftObjectPtr<UPaperSprite> CachedSprite = Character->GetCachedSprite(E_VN_ComponentID_Sprite::Eyelids);
        
        // Если в кэше был NULL (то есть исходно век не было), скрываем компонент.
        // Иначе он будет видимым с восстановленным спрайтом (если он был).
        const bool bShouldBeVisible = !CachedSprite.IsNull();
        Character->Eyelids_Sprite->SetVisibility(bShouldBeVisible);

        if(bShouldBeVisible)
        {
            // Если он должен быть видимым, восстановим его цвет
            Character->ApplyComponentColorWithFocus(Character->Eyelids_Sprite);
        }

        VN_LOG_DEBUG(TEXT("FinishBlinkAnimation: Restored eyelids to cached state (Visible: %s)"), bShouldBeVisible ? TEXT("TRUE") : TEXT("FALSE"));
    }
    
    // Планируем следующее моргание
    ScheduleNextBlink();
}

bool UVNCharacterIdleAnimationManager::IsCurrentSpritePartOfBlinkAnimation(UPaperSprite* Sprite) const
{
    if (!Sprite) return false;
    
    UPaperFlipbook* BlinkFlipbook = IdleAnimationsConfig.BlinkConfig.BlinkFlipbook.LoadSynchronous();
    if (!BlinkFlipbook) return false;
    
    for (int32 i = 0; i < BlinkFlipbook->GetNumFrames(); ++i)
    {
        if (BlinkFlipbook->GetSpriteAtFrame(i) == Sprite)
        {
            return true;
        }
    }
    return false;
}

void UVNCharacterIdleAnimationManager::ApplyBlinkColorToEyelids(AVNCharacter* Character)
{
    if (!Character || !Character->Eyelids_Sprite) return;
    
    // 1. Гарантируем видимость компонента
    Character->Eyelids_Sprite->SetHiddenInGame(false);
    Character->Eyelids_Sprite->SetVisibility(true);
    
    // 2. БЕРЕМ КОНФИГУРАЦИЮ НАПРЯМУЮ ИЗ МЕНЕДЖЕРА!
    // Это гарантирует, что мы используем самый свежий конфиг, который только что установили.
    const FVNIdleAnimationsConfig& CurrentConfig = GetIdleAnimationsConfig();
    
    FLinearColor BaseColor = CurrentConfig.BlinkConfig.bUseCustomBlinkColor 
        ? CurrentConfig.BlinkConfig.BlinkColor 
        : FLinearColor::White;

    // 3. Применяем цвет с учетом фокуса персонажа.
    FLinearColor FinalColor = Character->ApplyFocusToColor(BaseColor);
    Character->SetComponentColor(Character->Eyelids_Sprite, FinalColor);

    FString LogColorString = BaseColor.ToString();
    UE_LOG(LogTemp, Warning, TEXT("ApplyBlinkColorToEyelids: UseCustomColor=%s. Applied color: %s"), 
        CurrentConfig.BlinkConfig.bUseCustomBlinkColor ? TEXT("TRUE") : TEXT("FALSE"),
        *LogColorString
    );
}

// Новая функция, которая централизует логику показа век
void UVNCharacterIdleAnimationManager::ShowEyelidsAndSetSprite(UPaperSprite* NewSprite)
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Eyelids_Sprite || !NewSprite) return;
    
    // 1. Устанавливаем спрайт
    Character->Eyelids_Sprite->SetSprite(NewSprite);
    
    // 2. Устанавливаем правильный цвет (обычно белый, с учетом фокуса)
    FLinearColor AnimationColor = IdleAnimationsConfig.BlinkConfig.bUseCustomBlinkColor 
        ? IdleAnimationsConfig.BlinkConfig.BlinkColor : FLinearColor::White;
    FLinearColor FinalColor = Character->ApplyFocusToColor(AnimationColor);
    Character->SetComponentColor(Character->Eyelids_Sprite, FinalColor);

    // 3. ПРИНУДИТЕЛЬНО ДЕЛАЕМ ВИДИМЫМ
    // Это ключевой фикс для вашего случая
    Character->Eyelids_Sprite->SetVisibility(true);
}