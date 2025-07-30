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

    // Сохраняем текущее состояние в кэш
    UPaperSprite* CurrentSprite = Character->Eyelids_Sprite->GetSprite();
    Character->SetCachedSprite(E_VN_ComponentID_Sprite::Eyelids, CurrentSprite);
    
    UE_LOG(LogTemp, Warning, TEXT("StartBlinkAnimation: Cached sprite: %s"), 
        CurrentSprite ? *CurrentSprite->GetName() : TEXT("NULL"));
    
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
    
    // ИНТЕГРАЦИЯ С ЭМОЦИОНАЛЬНЫМИ СОСТОЯНИЯМИ
    float BaseInterval = IdleAnimationsConfig.BlinkConfig.GetRandomBlinkInterval();
    
    // Добавляем небольшую эмоциональную вариативность
    static int32 BlinkCounter = 0;
    BlinkCounter++;
    
    float EmotionalMultiplier = 1.0f;
    
    // Простые паттерны для создания более живого моргания
    if (BlinkCounter % 4 == 0)
    {
        EmotionalMultiplier = 0.3f; // Быстрое моргание
    }
    else if (BlinkCounter % 7 == 0) 
    {
        EmotionalMultiplier = 2.5f; // Долгая пауза
    }
    else if (BlinkCounter % 11 == 0)
    {
        EmotionalMultiplier = 0.6f; // Среднее моргание
    }
    
    float FinalInterval = FMath::Clamp(BaseInterval * EmotionalMultiplier, 0.3f, 10.0f);
    
    VN_LOG_DEBUG(TEXT("ScheduleNextBlink: Next blink in %.2f seconds (base: %.2f, multiplier: %.2f)"), 
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

    // Получаем спрайт закрытых глаз
    int32 NumFrames = BlinkFlipbook->GetNumFrames();
    UPaperSprite* ClosedEyes = nullptr;
    
    if (NumFrames >= 1)
    {
        ClosedEyes = GetFlipbookSpriteImproved(BlinkFlipbook, NumFrames - 1);
    }
    
    if (!ClosedEyes)
    {
        UE_LOG(LogTemp, Error, TEXT("ExecuteBlink: Cannot get closed eyes sprite"));
        ScheduleNextBlink();
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("ExecuteBlink: Starting blink"));

    // ИСПРАВЛЕНО: Используем SetHiddenInGame
    Character->Eyelids_Sprite->SetHiddenInGame(false);  // Показываем компонент
    Character->Eyelids_Sprite->SetSprite(ClosedEyes);
    
    UE_LOG(LogTemp, Warning, TEXT("ExecuteBlink: Showed component (SetHiddenInGame(false)) and set closed eyes"));
    
    // Определяем параметры
    float BlinkDuration = IdleAnimationsConfig.BlinkConfig.BlinkDuration;
    bool bDoubleBlink = FMath::RandRange(0.0f, 1.0f) <= IdleAnimationsConfig.BlinkConfig.DoubleBlinkChance;
    
    bIsBlinkAnimationPlaying = true;
    
    if (bDoubleBlink)
    {
        GetWorld()->GetTimerManager().SetTimer(
            BlinkTimerHandle,
            this,
            &UVNCharacterIdleAnimationManager::DoubleBlink_FirstOpen,
            BlinkDuration * 0.4f,
            false
        );
    }
    else
    {
        GetWorld()->GetTimerManager().SetTimer(
            BlinkTimerHandle,
            this,
            &UVNCharacterIdleAnimationManager::FinishBlinkAnimation,
            BlinkDuration,
            false
        );
    }
}

void UVNCharacterIdleAnimationManager::DoubleBlink_FirstOpen()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Eyelids_Sprite) 
    {
        FinishBlinkAnimation();
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("DoubleBlink_FirstOpen: Opening eyes between double blink"));
    
    // Временно восстанавливаем из кэша
    Character->RestoreSpriteFromCache(E_VN_ComponentID_Sprite::Eyelids);
    
    // ИСПРАВЛЕНО: Проверяем кэш для HiddenInGame
    TSoftObjectPtr<UPaperSprite> CachedSprite = Character->GetCachedSprite(E_VN_ComponentID_Sprite::Eyelids);
    
    if (CachedSprite.IsNull())
    {
        // Кэш пустой, но во время анимации показываем компонент
        Character->Eyelids_Sprite->SetHiddenInGame(false);
        UE_LOG(LogTemp, Warning, TEXT("DoubleBlink_FirstOpen: Cache NULL but keeping visible during animation (SetHiddenInGame(false))"));
    }
    else
    {
        // Кэш не пустой - компонент должен быть виден
        Character->Eyelids_Sprite->SetHiddenInGame(false);
        UE_LOG(LogTemp, Warning, TEXT("DoubleBlink_FirstOpen: Cache has sprite - visible (SetHiddenInGame(false))"));
    }
    
    // Планируем второе моргание
    float PauseBetweenBlinks = IdleAnimationsConfig.BlinkConfig.DoubleBlinkPause;
    
    GetWorld()->GetTimerManager().SetTimer(
        BlinkTimerHandle,
        this,
        &UVNCharacterIdleAnimationManager::DoubleBlink_SecondClose,
        PauseBetweenBlinks,
        false
    );
}

void UVNCharacterIdleAnimationManager::DoubleBlink_SecondClose()
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

    UE_LOG(LogTemp, Warning, TEXT("DoubleBlink_SecondClose: Second blink"));
    
    // Снова закрываем глаза (компонент уже виден)
    int32 NumFrames = BlinkFlipbook->GetNumFrames();
    UPaperSprite* ClosedEyes = GetFlipbookSpriteImproved(BlinkFlipbook, NumFrames - 1);
    
    if (ClosedEyes)
    {
        Character->Eyelids_Sprite->SetSprite(ClosedEyes);
    }
    
    // Планируем финальное открытие
    float BlinkDuration = IdleAnimationsConfig.BlinkConfig.BlinkDuration;
    
    GetWorld()->GetTimerManager().SetTimer(
        BlinkTimerHandle,
        this,
        &UVNCharacterIdleAnimationManager::FinishBlinkAnimation,
        BlinkDuration * 0.4f,
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
        
        // ИСПРАВЛЕНО: Проверяем кэш и используем SetHiddenInGame
        TSoftObjectPtr<UPaperSprite> CachedSprite = Character->GetCachedSprite(E_VN_ComponentID_Sprite::Eyelids);
        
        if (CachedSprite.IsNull())
        {
            // В кэше NULL → скрываем компонент
            Character->Eyelids_Sprite->SetHiddenInGame(true);
            UE_LOG(LogTemp, Warning, TEXT("FinishBlinkAnimation: Cache is NULL → HIDDEN component (SetHiddenInGame(true))"));
        }
        else
        {
            // В кэше есть спрайт → показываем компонент
            Character->Eyelids_Sprite->SetHiddenInGame(false);
            UE_LOG(LogTemp, Warning, TEXT("FinishBlinkAnimation: Cache has sprite → VISIBLE component (SetHiddenInGame(false))"));
        }
        
        UPaperSprite* FinalSprite = Character->Eyelids_Sprite->GetSprite();
        bool bFinalHidden = Character->Eyelids_Sprite->bHiddenInGame;
        UE_LOG(LogTemp, Warning, TEXT("FinishBlinkAnimation: Final - Sprite: %s, HiddenInGame: %s"), 
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