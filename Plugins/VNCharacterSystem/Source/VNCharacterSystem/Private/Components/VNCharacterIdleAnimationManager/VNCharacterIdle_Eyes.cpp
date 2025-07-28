// VNCharacterIdle_Eyes.cpp - Анимация случайных движений глаз

#include "Components/VNCharacterIdleAnimationManager.h"
#include "Actors/VNCharacter.h"
#include "VNCharacterSystemModule.h"
#include "Engine/World.h"

// =====================================================
// СИСТЕМА АНИМАЦИИ ДВИЖЕНИЙ ГЛАЗ
// =====================================================

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

    // Сохраняем текущий спрайт глаз
    SaveCurrentSprite(Character->Eyes_Sprite, OriginalEyesSprite);
    
    ScheduleNextEyesMovement();
    LogIdleAnimation(TEXT("Eyes random animation started"));
}

void UVNCharacterIdleAnimationManager::StopEyesRandomAnimation()
{
    GetWorld()->GetTimerManager().ClearTimer(EyesRandomTimerHandle);
    
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Eyes_Sprite)
    {
        // Восстанавливаем исходный спрайт глаз
        RestoreCurrentSprite(Character->Eyes_Sprite, OriginalEyesSprite);
    }

    bIsEyesRandomAnimationPlaying = false;
    LogIdleAnimation(TEXT("Eyes random animation stopped"));
}

void UVNCharacterIdleAnimationManager::ScheduleNextEyesMovement()
{
    if (!IdleAnimationsConfig.EyesRandomConfig.bEnabled) return;

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
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Eyes_Sprite) return;

    UPaperFlipbook* EyesFlipbook = IdleAnimationsConfig.EyesRandomConfig.EyesDirectionsFlipbook.LoadSynchronous();
    if (!EyesFlipbook) return;

    // Получаем случайное направление взгляда
    UPaperSprite* RandomDirection = GetRandomFlipbookSpriteImproved(EyesFlipbook, false);
    if (RandomDirection)
    {
        Character->Eyes_Sprite->SetSprite(RandomDirection);
        bIsEyesRandomAnimationPlaying = true;
        
        // Запланировать возврат к исходному положению
        float LookDuration = IdleAnimationsConfig.EyesRandomConfig.GetRandomLookDuration();
        GetWorld()->GetTimerManager().SetTimer(
            EyesRandomTimerHandle,
            this,
            &UVNCharacterIdleAnimationManager::ReturnEyesToOriginal,
            LookDuration,
            false
        );
    }
}

void UVNCharacterIdleAnimationManager::ReturnEyesToOriginal()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Eyes_Sprite)
    {
        RestoreCurrentSprite(Character->Eyes_Sprite, OriginalEyesSprite);
    }

    bIsEyesRandomAnimationPlaying = false;
    
    if (IdleAnimationsConfig.EyesRandomConfig.bEnabled)
    {
        ScheduleNextEyesMovement();
    }
}