#include "Components/VNCharacterIdleAnimationManager.h"
#include "Actors/VNCharacter.h"
#include "VNCharacterSystemModule.h"
#include "PaperSpriteComponent.h"
#include "Engine/World.h"

UVNCharacterIdleAnimationManager::UVNCharacterIdleAnimationManager()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    
    bDisableIdleAnimations = false;
    bVerboseLogging = false;
    bIsBlinkAnimationPlaying = false;
    bIsEyesRandomAnimationPlaying = false;
}

void UVNCharacterIdleAnimationManager::BeginPlay()
{
    Super::BeginPlay();
    OwnerCharacter = Cast<AVNCharacter>(GetOwner());
    
    if (!OwnerCharacter.IsValid())
    {
        VN_LOG_ERROR(TEXT("IdleAnimationManager: Invalid owner"));
        return;
    }

    LogIdleAnimation(TEXT("Idle Animation Manager initialized"));
    
    // Обновляем кэш ПЕРЕД запуском анимаций
    if (OwnerCharacter.IsValid())
    {
        OwnerCharacter->UpdateSpriteCache();
        VN_LOG_DEBUG(TEXT("BeginPlay: Sprite cache updated before starting animations"));
    }
    
    StartAllIdleAnimations();
}

void UVNCharacterIdleAnimationManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    CheckForSpriteChanges();
}

void UVNCharacterIdleAnimationManager::CheckForSpriteChanges()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character) return;

    // Проверяем веки во время моргания
    if (bIsBlinkAnimationPlaying && Character->Eyelids_Sprite)
    {
        UPaperSprite* CurrentSprite = Character->Eyelids_Sprite->GetSprite();
        if (CurrentSprite && !IsCurrentSpritePartOfBlinkAnimation(CurrentSprite))
        {
            // ИСПРАВЛЕНИЕ: НЕ обновляем кэш здесь, так как это должно происходить в SetSprite
            VN_LOG_DEBUG(TEXT("CheckForSpriteChanges: External eyelids change detected during blink (cache should be updated via SetSprite)"));
        }
    }

    // Проверяем рот - НЕ обновляем кэш, только логируем
    if (IdleAnimationsConfig.TalkConfig.bEnabled && Character->Mouth_Sprite)
    {
        UPaperSprite* CurrentSprite = Character->Mouth_Sprite->GetSprite();
        if (CurrentSprite && !IsCurrentSpritePartOfTalkAnimation(CurrentSprite))
        {
            VN_LOG_DEBUG(TEXT("CheckForSpriteChanges: External mouth change detected (cache should be updated via SetSprite)"));
        }
    }

    // Проверяем глаза - НЕ обновляем кэш, только логируем
    if (bIsEyesRandomAnimationPlaying && Character->Eyes_Sprite)
    {
        UPaperSprite* CurrentSprite = Character->Eyes_Sprite->GetSprite();
        if (CurrentSprite && !IsAnimationSprite(Character->Eyes_Sprite, CurrentSprite))
        {
            VN_LOG_DEBUG(TEXT("CheckForSpriteChanges: External eyes change detected (cache should be updated via SetSprite)"));
        }
    }
}

bool UVNCharacterIdleAnimationManager::IsAnimationSprite(UPaperSpriteComponent* Component, UPaperSprite* Sprite) const
{
    if (!Component || !Sprite) return false;

    if (Component == GetVNCharacterOwner()->Eyelids_Sprite && bIsBlinkAnimationPlaying)
    {
        return IsCurrentSpritePartOfBlinkAnimation(Sprite);
    }
    else if (Component == GetVNCharacterOwner()->Mouth_Sprite && IdleAnimationsConfig.TalkConfig.bEnabled)
    {
        return IsCurrentSpritePartOfTalkAnimation(Sprite);
    }
    else if (Component == GetVNCharacterOwner()->Eyes_Sprite && bIsEyesRandomAnimationPlaying)
    {
        UPaperFlipbook* EyesFlipbook = IdleAnimationsConfig.EyesRandomConfig.EyesDirectionsFlipbook.LoadSynchronous();
        if (EyesFlipbook)
        {
            return IsFlipbookSprite(EyesFlipbook, Sprite);
        }
    }

    return false;
}

bool UVNCharacterIdleAnimationManager::IsFlipbookSprite(UPaperFlipbook* Flipbook, UPaperSprite* Sprite) const
{
    if (!Flipbook || !Sprite) return false;

    float TotalDuration = Flipbook->GetTotalDuration();
    if (TotalDuration <= 0.0f) return false;

    const int32 CheckPoints = 10;
    for (int32 i = 0; i < CheckPoints; ++i)
    {
        float TimePoint = (TotalDuration / CheckPoints) * i;
        UPaperSprite* FlipbookSprite = Flipbook->GetSpriteAtTime(TimePoint);
        if (FlipbookSprite == Sprite)
        {
            return true;
        }
    }
    return false;
}

// =====================================================
// НОВЫЕ МЕТОДЫ ДЛЯ ОБРАБОТКИ ВНЕШНИХ ИЗМЕНЕНИЙ
// =====================================================

void UVNCharacterIdleAnimationManager::HandleExternalSpriteChange(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> NewSprite)
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character) return;

    switch (ComponentID)
    {
    case E_VN_ComponentID_Sprite::Eyelids:
        {
            UPaperSprite* LoadedSprite = NewSprite.IsNull() ? nullptr : NewSprite.LoadSynchronous();
            
            if (!bIsBlinkAnimationPlaying || !IsCurrentSpritePartOfBlinkAnimation(LoadedSprite))
            {
                Character->SetCachedSprite(ComponentID, NewSprite);
            }
            break;
        }
        
    case E_VN_ComponentID_Sprite::Mouth:
        {
            UPaperSprite* LoadedSprite = NewSprite.IsNull() ? nullptr : NewSprite.LoadSynchronous();
            
            if (!IdleAnimationsConfig.TalkConfig.bEnabled || !IsCurrentSpritePartOfTalkAnimation(LoadedSprite))
            {
                Character->SetCachedSprite(ComponentID, NewSprite);
            }
            break;
        }
        
    case E_VN_ComponentID_Sprite::Eyes:
        {
            UPaperSprite* LoadedSprite = NewSprite.IsNull() ? nullptr : NewSprite.LoadSynchronous();
            
            if (!bIsEyesRandomAnimationPlaying || !IsAnimationSprite(Character->Eyes_Sprite, LoadedSprite))
            {
                Character->SetCachedSprite(ComponentID, NewSprite);
            }
            break;
        }
        
    default:
        {
            Character->SetCachedSprite(ComponentID, NewSprite);
            break;
        }
    }
}

void UVNCharacterIdleAnimationManager::UpdateBlinkModeForNewEyelidsState(TSoftObjectPtr<UPaperSprite> NewEyelidsSprite)
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character) return;

    // Просто обновляем кэш - никаких сложных режимов
    Character->SetCachedSprite(E_VN_ComponentID_Sprite::Eyelids, NewEyelidsSprite);
    
    VN_LOG_DEBUG(TEXT("UpdateBlinkModeForNewEyelidsState: Cache updated"));
}

// =====================================================
// ИСПРАВЛЕННЫЕ публичные методы API
// =====================================================

void UVNCharacterIdleAnimationManager::SetBlinkEnabled(bool bEnable)
{
    VN_LOG_DEBUG(TEXT("SetBlinkEnabled called: %s"), bEnable ? TEXT("true") : TEXT("false"));
    
    if (!bEnable && IdleAnimationsConfig.BlinkConfig.bEnabled)
    {
        StopBlinkAnimation();
    }
    
    IdleAnimationsConfig.BlinkConfig.bEnabled = bEnable;
    
    if (bEnable)
    {
        StartBlinkAnimation();
    }
    
    LogIdleAnimation(FString::Printf(TEXT("Blink %s"), bEnable ? TEXT("enabled") : TEXT("disabled")));
}

void UVNCharacterIdleAnimationManager::SetTalkEnabled(bool bEnable)
{
    VN_LOG_DEBUG(TEXT("SetTalkEnabled called: %s"), bEnable ? TEXT("true") : TEXT("false"));
    
    if (!bEnable && IdleAnimationsConfig.TalkConfig.bEnabled)
    {
        AVNCharacter* Character = GetVNCharacterOwner();
        if (Character && Character->Mouth_Sprite)
        {
            // ИСПРАВЛЕНО: Используем кэширование VNCharacter напрямую
            UPaperSprite* CurrentSprite = Character->Mouth_Sprite->GetSprite();
            if (CurrentSprite && !IsCurrentSpritePartOfTalkAnimation(CurrentSprite))
            {
                Character->SetCachedSprite(E_VN_ComponentID_Sprite::Mouth, CurrentSprite);
                VN_LOG_DEBUG(TEXT("SetTalkEnabled: Updated mouth cache before stopping"));
            }
        }
        
        StopTalkAnimation();
    }
    
    IdleAnimationsConfig.TalkConfig.bEnabled = bEnable;
    
    if (bEnable)
    {
        StartTalkAnimation();
    }
    
    LogIdleAnimation(FString::Printf(TEXT("Talk %s"), bEnable ? TEXT("enabled") : TEXT("disabled")));
}

void UVNCharacterIdleAnimationManager::SetEyesRandomEnabled(bool bEnable)
{
    VN_LOG_DEBUG(TEXT("SetEyesRandomEnabled called: %s"), bEnable ? TEXT("true") : TEXT("false"));
    
    if (!bEnable && IdleAnimationsConfig.EyesRandomConfig.bEnabled)
    {
        StopEyesRandomAnimation();
    }
    
    IdleAnimationsConfig.EyesRandomConfig.bEnabled = bEnable;
    
    if (bEnable)
    {
        StartEyesRandomAnimation();
    }
    
    LogIdleAnimation(FString::Printf(TEXT("Eyes random %s"), bEnable ? TEXT("enabled") : TEXT("disabled")));
}

void UVNCharacterIdleAnimationManager::SetIdleAnimationsConfig(const FVNIdleAnimationsConfig& NewConfig)
{
    VN_LOG_DEBUG(TEXT("SetIdleAnimationsConfig: Updating configuration"));
    
    // ИСПРАВЛЕНИЕ: Безопасная остановка только если World доступен
    UWorld* World = GetWorld();
    if (World)
    {
        StopAllIdleAnimations();
    }
    else
    {
        // В редакторе просто обновляем конфиг без остановки анимаций
        VN_LOG_DEBUG(TEXT("SetIdleAnimationsConfig: No World context, updating config only"));
    }
    
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character)
    {
        Character->UpdateSpriteCache();
        VN_LOG_DEBUG(TEXT("SetIdleAnimationsConfig: Cache updated before config change"));
    }
    
    IdleAnimationsConfig = NewConfig;
    
    // Запускаем анимации только если World доступен
    if (World)
    {
        StartAllIdleAnimations();
    }
    
    LogIdleAnimation(TEXT("Config updated"));
}

void UVNCharacterIdleAnimationManager::StopAllIdleAnimations()
{
    VN_LOG_DEBUG(TEXT("StopAllIdleAnimations: Stopping all animations"));
    StopBlinkAnimation();
    StopTalkAnimation();
    StopEyesRandomAnimation();
}

void UVNCharacterIdleAnimationManager::StartAllIdleAnimations()
{
    if (bDisableIdleAnimations) 
    {
        VN_LOG_DEBUG(TEXT("StartAllIdleAnimations: Disabled by flag"));
        return;
    }

    VN_LOG_DEBUG(TEXT("StartAllIdleAnimations: Starting enabled animations"));
    
    if (IdleAnimationsConfig.BlinkConfig.bEnabled) 
    {
        VN_LOG_DEBUG(TEXT("StartAllIdleAnimations: Starting blink"));
        StartBlinkAnimation();
    }
    
    if (IdleAnimationsConfig.TalkConfig.bEnabled) 
    {
        VN_LOG_DEBUG(TEXT("StartAllIdleAnimations: Starting talk"));
        StartTalkAnimation();
    }
    
    if (IdleAnimationsConfig.EyesRandomConfig.bEnabled) 
    {
        VN_LOG_DEBUG(TEXT("StartAllIdleAnimations: Starting eyes"));
        StartEyesRandomAnimation();
    }
}

void UVNCharacterIdleAnimationManager::UpdateSavedSprites()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character) return;

    VN_LOG_DEBUG(TEXT("UpdateSavedSprites: Updating cache"));
    Character->UpdateSpriteCache();
    
    // Дополнительная проверка критичных компонентов
    TSoftObjectPtr<UPaperSprite> CachedEyes = Character->GetCachedSprite(E_VN_ComponentID_Sprite::Eyes);
    if (CachedEyes.IsNull() && Character->Eyes_Sprite && Character->Eyes_Sprite->GetSprite())
    {
        VN_LOG_WARNING(TEXT("UpdateSavedSprites: Eyes cache was empty, fixing..."));
        Character->SetCachedSprite(E_VN_ComponentID_Sprite::Eyes, Character->Eyes_Sprite->GetSprite());
    }
}

// Утилиты
AVNCharacter* UVNCharacterIdleAnimationManager::GetVNCharacterOwner() const
{
    return OwnerCharacter.Get();
}

void UVNCharacterIdleAnimationManager::LogIdleAnimation(const FString& Message, bool bForceLog) const
{
    if (bVerboseLogging || bForceLog)
    {
        VN_LOG_DEBUG(TEXT("IdleAnimManager [%s]: %s"), 
            OwnerCharacter.IsValid() ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
            *Message);
    }
}