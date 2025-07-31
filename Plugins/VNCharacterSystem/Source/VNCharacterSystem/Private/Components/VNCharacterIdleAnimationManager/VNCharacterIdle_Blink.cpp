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

    // ИСПРАВЛЕНИЕ: Проверяем, есть ли флипбук для моргания
    UPaperFlipbook* BlinkFlipbook = IdleAnimationsConfig.BlinkConfig.BlinkFlipbook.LoadSynchronous();
    if (!BlinkFlipbook)
    {
        LogIdleAnimation(TEXT("Cannot start blink: no flipbook"));
        return;
    }

    // Проверяем количество кадров
    int32 NumFrames = BlinkFlipbook->GetNumFrames();
    if (NumFrames < 2)
    {
        LogIdleAnimation(TEXT("Cannot start blink: need at least 2 frames"));
        return;
    }

    // ИСПРАВЛЕНИЕ: Сохраняем текущее состояние в кэш - ДАЖЕ ЕСЛИ ОНО NULL
    UPaperSprite* CurrentSprite = Character->Eyelids_Sprite->GetSprite();
    Character->SetCachedSprite(E_VN_ComponentID_Sprite::Eyelids, CurrentSprite);
    
    UE_LOG(LogTemp, Warning, TEXT("StartBlinkAnimation: Cached eyelids sprite: %s"), 
        CurrentSprite ? *CurrentSprite->GetName() : TEXT("NULL (empty state cached)"));
    
    ScheduleNextBlink();
    LogIdleAnimation(TEXT("Blink animation started"));
}

void UVNCharacterIdleAnimationManager::StopBlinkAnimation()
{
    // ИСПРАВЛЕНИЕ: Проверяем World перед использованием
    UWorld* World = GetWorld();
    if (!World)
    {
        VN_LOG_DEBUG(TEXT("StopBlinkAnimation: No valid world, skipping timer clear"));
        bIsBlinkAnimationPlaying = false;
        return;
    }

    World->GetTimerManager().ClearTimer(BlinkTimerHandle);
    
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Eyelids_Sprite)
    {
        // Восстанавливаем спрайт
        Character->RestoreSpriteFromCache(E_VN_ComponentID_Sprite::Eyelids);
        
        // ИСПРАВЛЕНО: Проверяем кэш для HiddenInGame
        TSoftObjectPtr<UPaperSprite> CachedSprite = Character->GetCachedSprite(E_VN_ComponentID_Sprite::Eyelids);
        
        if (CachedSprite.IsNull())
        {
            Character->Eyelids_Sprite->SetHiddenInGame(true);
            UE_LOG(LogTemp, Warning, TEXT("StopBlinkAnimation: Cache NULL → HIDDEN (SetHiddenInGame(true))"));
        }
        else
        {
            Character->Eyelids_Sprite->SetHiddenInGame(false);
            UE_LOG(LogTemp, Warning, TEXT("StopBlinkAnimation: Cache has sprite → VISIBLE (SetHiddenInGame(false))"));
        }
    }

    bIsBlinkAnimationPlaying = false;
    LogIdleAnimation(TEXT("Blink animation stopped"));
}

void UVNCharacterIdleAnimationManager::ScheduleNextBlink()
{
    if (!IdleAnimationsConfig.BlinkConfig.bEnabled) return;

    UWorld* World = GetWorld();
    if (!World)
    {
        VN_LOG_DEBUG(TEXT("ScheduleNextBlink: No valid world, skipping"));
        return;
    }
    
    // УЛУЧШЕННАЯ ЭМОЦИОНАЛЬНАЯ ВАРИАТИВНОСТЬ
    float BaseInterval = IdleAnimationsConfig.BlinkConfig.GetRandomBlinkInterval();
    
    // Создаем более сложные паттерны моргания
    static int32 BlinkCounter = 0;
    BlinkCounter++;
    
    float EmotionalMultiplier = 1.0f;
    float DurationMultiplier = 1.0f; // Новый множитель для скорости моргания
    
    // Создаем реалистичные паттерны
    if (BlinkCounter % 3 == 0)
    {
        // Быстрая серия морганий (нервозность)
        EmotionalMultiplier = 0.4f;
        DurationMultiplier = 0.8f; // Быстрее моргание
    }
    else if (BlinkCounter % 7 == 0) 
    {
        // Долгая пауза (задумчивость)
        EmotionalMultiplier = 2.8f;
        DurationMultiplier = 1.3f; // Медленнее моргание
    }
    else if (BlinkCounter % 5 == 0)
    {
        // Двойные моргания чаще
        EmotionalMultiplier = 0.7f;
        DurationMultiplier = 0.9f;
        // Увеличиваем шанс двойного моргания на этот раз
        const_cast<FVNBlinkAnimationConfig&>(IdleAnimationsConfig.BlinkConfig).DoubleBlinkChance = 
            FMath::Min(0.8f, IdleAnimationsConfig.BlinkConfig.DoubleBlinkChance * 1.5f);
    }
    else if (BlinkCounter % 11 == 0)
    {
        // Медленное "сонное" моргание
        EmotionalMultiplier = 1.8f;
        DurationMultiplier = 1.6f; // Очень медленное моргание
    }
    
    float FinalInterval = FMath::Clamp(BaseInterval * EmotionalMultiplier, 0.3f, 12.0f);
    
    // Применяем множитель длительности к следующему морганию
    // (это влияет на BlinkDuration в ExecuteBlink)
    
    VN_LOG_DEBUG(TEXT("ScheduleNextBlink: Next blink in %.2f seconds (base: %.2f, emotional: %.2f, duration: %.2f)"), 
        FinalInterval, BaseInterval, EmotionalMultiplier, DurationMultiplier);
    
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
    if (!IdleAnimationsConfig.BlinkConfig.bEnabled)
    {
        VN_LOG_DEBUG(TEXT("ExecuteBlink: Blink disabled, stopping"));
        return;
    }

    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Eyelids_Sprite) 
    {
        UE_LOG(LogTemp, Error, TEXT("ExecuteBlink: No character or eyelids sprite"));
        ScheduleNextBlink();
        return;
    }

    UPaperFlipbook* BlinkFlipbook = IdleAnimationsConfig.BlinkConfig.BlinkFlipbook.LoadSynchronous();
    if (!BlinkFlipbook) 
    {
        UE_LOG(LogTemp, Error, TEXT("ExecuteBlink: No blink flipbook"));
        ScheduleNextBlink();
        return;
    }

    // Проверяем количество кадров
    int32 NumFrames = BlinkFlipbook->GetNumFrames();
    if (NumFrames < 2)
    {
        UE_LOG(LogTemp, Error, TEXT("ExecuteBlink: Need at least 2 frames for blinking"));
        ScheduleNextBlink();
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("ExecuteBlink: Starting realistic blink sequence"));

    // ФАЗА 1: Быстрый переход к полуоткрытым глазам (кадр 0)
    UPaperSprite* HalfClosed = GetFlipbookSpriteImproved(BlinkFlipbook, 0);
    if (HalfClosed)
    {
        // ИСПРАВЛЕНИЕ: Правильный порядок операций
        Character->Eyelids_Sprite->SetSprite(HalfClosed);  // Сначала спрайт
        ApplyBlinkColorToEyelids(Character);                // Потом цвет и видимость
        
        UE_LOG(LogTemp, Warning, TEXT("ExecuteBlink: Phase 1 - Half closed eyes (quick)"));
    }

    float BlinkDuration = IdleAnimationsConfig.BlinkConfig.BlinkDuration;
    bool bDoubleBlink = FMath::RandRange(0.0f, 1.0f) <= IdleAnimationsConfig.BlinkConfig.DoubleBlinkChance;
    
    bIsBlinkAnimationPlaying = true;
    
    // РЕАЛИСТИЧНОЕ ВРЕМЯ: Очень быстрый переход к закрытым глазам (15% времени)
    GetWorld()->GetTimerManager().SetTimer(
        BlinkTimerHandle,
        this,
        &UVNCharacterIdleAnimationManager::BlinkPhase2_FullyClosed,
        BlinkDuration * 0.15f, // 15% времени - быстрое закрытие
        false
    );
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
    UE_LOG(LogTemp, Warning, TEXT("=== FinishBlinkAnimation START ==="));
    
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Eyelids_Sprite)
    {
        // Восстанавливаем спрайт из кэша
        Character->RestoreSpriteFromCache(E_VN_ComponentID_Sprite::Eyelids);
        
        // ИСПРАВЛЕНИЕ: Проверяем кэш и правильно управляем видимостью
        TSoftObjectPtr<UPaperSprite> CachedSprite = Character->GetCachedSprite(E_VN_ComponentID_Sprite::Eyelids);
        
        if (CachedSprite.IsNull())
        {
            // В кэше NULL → компонент должен быть скрыт (исходное состояние было пустым)
            Character->Eyelids_Sprite->SetSprite(nullptr);
            Character->Eyelids_Sprite->SetHiddenInGame(true);
            Character->Eyelids_Sprite->SetVisibility(false);
            UE_LOG(LogTemp, Warning, TEXT("FinishBlinkAnimation: Restored to original EMPTY state → HIDDEN"));
        }
        else
        {
            // В кэше есть спрайт → показываем компонент и применяем цвет из кэша
            Character->Eyelids_Sprite->SetHiddenInGame(false);
            Character->Eyelids_Sprite->SetVisibility(true);
            Character->ApplyComponentColorWithFocus(Character->Eyelids_Sprite);
            UE_LOG(LogTemp, Warning, TEXT("FinishBlinkAnimation: Restored to cached sprite → VISIBLE with cached color"));
        }
        
        UPaperSprite* FinalSprite = Character->Eyelids_Sprite->GetSprite();
        bool bFinalHidden = Character->Eyelids_Sprite->bHiddenInGame;
        UE_LOG(LogTemp, Warning, TEXT("FinishBlinkAnimation: Final state - Sprite: %s, HiddenInGame: %s"), 
            FinalSprite ? *FinalSprite->GetName() : TEXT("NULL"),
            bFinalHidden ? TEXT("YES (HIDDEN)") : TEXT("NO (VISIBLE)"));
    }

    bIsBlinkAnimationPlaying = false;
    
    UE_LOG(LogTemp, Warning, TEXT("=== FinishBlinkAnimation END ==="));
    
    // Планируем следующее моргание
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
    
    // Проверяем только последний кадр (закрытые глаза)
    int32 NumFrames = BlinkFlipbook->GetNumFrames();
    if (NumFrames == 0) return false;
    
    UPaperSprite* ClosedEyes = GetFlipbookSpriteImproved(BlinkFlipbook, NumFrames - 1);
    return (Sprite == ClosedEyes);
}

void UVNCharacterIdleAnimationManager::ApplyBlinkColorToEyelids(AVNCharacter* Character)
{
    if (!Character || !Character->Eyelids_Sprite) return;
    
    // ИСПРАВЛЕНИЕ: Сначала убеждаемся, что компонент виден
    Character->Eyelids_Sprite->SetHiddenInGame(false);
    Character->Eyelids_Sprite->SetVisibility(true);
    
    TSoftObjectPtr<UPaperSprite> CachedSprite = Character->GetCachedSprite(E_VN_ComponentID_Sprite::Eyelids);
    if (!CachedSprite.IsNull())
    {
        // В кэше есть спрайт - используем цвет из кэша
        Character->ApplyComponentColorWithFocus(Character->Eyelids_Sprite);
        UE_LOG(LogTemp, Warning, TEXT("ApplyBlinkColorToEyelids: Applied cached color to VISIBLE component"));
    }
    else
    {
        // В кэше NULL - используем цвет из конфигурации анимации или белый
        FLinearColor AnimationColor = IdleAnimationsConfig.BlinkConfig.bUseCustomBlinkColor ? 
            IdleAnimationsConfig.BlinkConfig.BlinkColor : FLinearColor::White;
        
        FLinearColor FinalColor = Character->ApplyFocusToColor(AnimationColor);
        Character->SetComponentColor(Character->Eyelids_Sprite, FinalColor);
        UE_LOG(LogTemp, Warning, TEXT("ApplyBlinkColorToEyelids: Applied animation color %s to VISIBLE component"), 
            *FinalColor.ToString());
    }
}