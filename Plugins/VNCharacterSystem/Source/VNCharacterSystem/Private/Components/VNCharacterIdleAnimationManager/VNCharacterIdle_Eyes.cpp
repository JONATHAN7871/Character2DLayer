#include "Components/VNCharacterIdleAnimationManager.h"
#include "Actors/VNCharacter.h"
#include "VNCharacterSystemModule.h"
#include "Engine/World.h"

void UVNCharacterIdleAnimationManager::StartEyesRandomAnimation()
{
    if (!IdleAnimationsConfig.EyesRandomConfig.IsValid())
    {
        LogIdleAnimation(TEXT("Cannot start eyes random: invalid config"));
        return;
    }

    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Eyes_Sprite)
    {
        LogIdleAnimation(TEXT("Cannot start eyes random: invalid character"));
        return;
    }

    VN_LOG_DEBUG(TEXT("StartEyesRandomAnimation: Starting eyes animation"));

    // ИСПРАВЛЕНИЕ: Убеждаемся, что кэш обновлен и не пустой
    Character->UpdateSpriteCache();
    
    // Дополнительная проверка - если кэш глаз пустой, но спрайт есть - исправляем
    TSoftObjectPtr<UPaperSprite> CachedEyes = Character->GetCachedSprite(E_VN_ComponentID_Sprite::Eyes);
    if (CachedEyes.IsNull() && Character->Eyes_Sprite->GetSprite())
    {
        VN_LOG_WARNING(TEXT("StartEyesRandomAnimation: Eyes cache was empty, fixing with current sprite"));
        Character->SetCachedSprite(E_VN_ComponentID_Sprite::Eyes, Character->Eyes_Sprite->GetSprite());
    }
    
    ScheduleNextEyesMovement();
    LogIdleAnimation(TEXT("Eyes random animation started"));
}

void UVNCharacterIdleAnimationManager::StopEyesRandomAnimation()
{
    VN_LOG_DEBUG(TEXT("StopEyesRandomAnimation: Stopping eyes animation"));
    
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(EyesRandomTimerHandle);
    }
    
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Eyes_Sprite)
    {
        // ИСПРАВЛЕНИЕ: Проверяем кэш перед восстановлением
        TSoftObjectPtr<UPaperSprite> CachedEyes = Character->GetCachedSprite(E_VN_ComponentID_Sprite::Eyes);
        
        if (!CachedEyes.IsNull())
        {
            Character->RestoreSpriteFromCache(E_VN_ComponentID_Sprite::Eyes);
            VN_LOG_DEBUG(TEXT("StopEyesRandomAnimation: Restored eyes from cache"));
        }
        else
        {
            VN_LOG_WARNING(TEXT("StopEyesRandomAnimation: Cache is empty, keeping current sprite"));
            // Сохраняем текущий спрайт в кэш для будущих использований
            UPaperSprite* CurrentSprite = Character->Eyes_Sprite->GetSprite();
            if (CurrentSprite)
            {
                Character->SetCachedSprite(E_VN_ComponentID_Sprite::Eyes, CurrentSprite);
            }
        }
    }

    bIsEyesRandomAnimationPlaying = false;
    LogIdleAnimation(TEXT("Eyes random animation stopped"));
}

void UVNCharacterIdleAnimationManager::ScheduleNextEyesMovement()
{
    // ИСПРАВЛЕНИЕ: Проверяем флаг перед планированием
    if (!IdleAnimationsConfig.EyesRandomConfig.bEnabled) 
    {
        VN_LOG_DEBUG(TEXT("ScheduleNextEyesMovement: Animation disabled, not scheduling"));
        return;
    }

    float NextMovementDelay = IdleAnimationsConfig.EyesRandomConfig.GetRandomWaitDuration();
    
    GetWorld()->GetTimerManager().SetTimer(
        EyesRandomTimerHandle,
        this,
        &UVNCharacterIdleAnimationManager::ExecuteRandomEyesMovement,
        NextMovementDelay,
        false
    );
}

void UVNCharacterIdleAnimationManager::ExecuteRandomEyesMovement()
{
    // ИСПРАВЛЕНИЕ: Проверяем флаг перед выполнением
    if (!IdleAnimationsConfig.EyesRandomConfig.bEnabled)
    {
        VN_LOG_DEBUG(TEXT("ExecuteRandomEyesMovement: Animation disabled, stopping"));
        StopEyesRandomAnimation();
        return;
    }

    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Eyes_Sprite) 
    {
        VN_LOG_DEBUG(TEXT("ExecuteRandomEyesMovement: No character or eyes sprite"));
        return;
    }

    UPaperFlipbook* EyesFlipbook = IdleAnimationsConfig.EyesRandomConfig.EyesDirectionsFlipbook.LoadSynchronous();
    if (!EyesFlipbook) 
    {
        VN_LOG_DEBUG(TEXT("ExecuteRandomEyesMovement: No eyes flipbook"));
        return;
    }

    UPaperSprite* RandomDirection = GetRandomFlipbookSpriteImproved(EyesFlipbook, false);
    if (RandomDirection)
    {
        Character->Eyes_Sprite->SetSprite(RandomDirection);
        
        // ИСПРАВЛЕНИЕ: Применяем цвет в зависимости от кэша
        TSoftObjectPtr<UPaperSprite> CachedSprite = Character->GetCachedSprite(E_VN_ComponentID_Sprite::Eyes);
        if (!CachedSprite.IsNull())
        {
            // В кэше есть спрайт - используем цвет из кэша
            Character->ApplyComponentColorWithFocus(Character->Eyes_Sprite);
        }
        else
        {
            // В кэше NULL - используем цвет из конфигурации анимации или белый
            FLinearColor AnimationColor = IdleAnimationsConfig.EyesRandomConfig.bUseCustomEyesColor ? 
                IdleAnimationsConfig.EyesRandomConfig.EyesColor : FLinearColor::White;
            
            FLinearColor FinalColor = Character->ApplyFocusToColor(AnimationColor);
            Character->SetComponentColor(Character->Eyes_Sprite, FinalColor);
        }
        
        bIsEyesRandomAnimationPlaying = true;
        
        float LookDuration = IdleAnimationsConfig.EyesRandomConfig.GetRandomLookDuration();
        GetWorld()->GetTimerManager().SetTimer(
            EyesRandomTimerHandle,
            this,
            &UVNCharacterIdleAnimationManager::ReturnEyesToOriginal,
            LookDuration,
            false
        );
        
        VN_LOG_DEBUG(TEXT("ExecuteRandomEyesMovement: Set random direction, will return in %.2fs"), LookDuration);
    }
}

void UVNCharacterIdleAnimationManager::ReturnEyesToOriginal()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Eyes_Sprite)
    {
        // ИСПРАВЛЕНИЕ: Проверяем кэш перед восстановлением
        TSoftObjectPtr<UPaperSprite> CachedEyes = Character->GetCachedSprite(E_VN_ComponentID_Sprite::Eyes);
        
        if (!CachedEyes.IsNull())
        {
            Character->RestoreSpriteFromCache(E_VN_ComponentID_Sprite::Eyes);
            Character->ApplyComponentColorWithFocus(Character->Eyes_Sprite);
            VN_LOG_DEBUG(TEXT("ReturnEyesToOriginal: Restored from cache and applied cached color"));
        }
        else
        {
            VN_LOG_WARNING(TEXT("ReturnEyesToOriginal: Cache is empty, keeping current sprite and applying default color"));
            
            // Применяем цвет по умолчанию (белый) с учетом фокуса
            FLinearColor DefaultColor = FLinearColor::White;
            FLinearColor FinalColor = Character->ApplyFocusToColor(DefaultColor);
            Character->SetComponentColor(Character->Eyes_Sprite, FinalColor);
            
            // Сохраняем текущий спрайт в кэш для будущих использований
            UPaperSprite* CurrentSprite = Character->Eyes_Sprite->GetSprite();
            if (CurrentSprite)
            {
                Character->SetCachedSprite(E_VN_ComponentID_Sprite::Eyes, CurrentSprite);
            }
        }
    }

    bIsEyesRandomAnimationPlaying = false;
    
    // ИСПРАВЛЕНИЕ: Проверяем флаг перед планированием следующего движения
    if (IdleAnimationsConfig.EyesRandomConfig.bEnabled)
    {
        ScheduleNextEyesMovement();
    }
    else
    {
        VN_LOG_DEBUG(TEXT("ReturnEyesToOriginal: Animation disabled, not scheduling next"));
    }
}