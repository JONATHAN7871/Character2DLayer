// VNCharacterIdle_Core.cpp - Основной функционал idle анимаций

#include "Components/VNCharacterIdleAnimationManager.h"
#include "Actors/VNCharacter.h"
#include "VNCharacterSystemModule.h"
#include "PaperSpriteComponent.h"
#include "Engine/World.h"

// =====================================================
// БАЗОВЫЕ МЕТОДЫ IDLE АНИМАЦИЙ
// =====================================================

UVNCharacterIdleAnimationManager::UVNCharacterIdleAnimationManager()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    
    bDisableIdleAnimations = false;
    bVerboseLogging = false;
    bIsBlinkAnimationPlaying = false;
    bIsEyesRandomAnimationPlaying = false;
    CurrentBlinkState = EBlinkState::WaitingForBlink;
    bPendingDoubleBlink = false;
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
    StartAllIdleAnimations();
}

void UVNCharacterIdleAnimationManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    CheckForSpriteChanges();
}

// =====================================================
// СИСТЕМА ОТСЛЕЖИВАНИЯ ИЗМЕНЕНИЙ СПРАЙТОВ
// =====================================================

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
            // Спрайт был изменен извне - обновляем сохраненный
            OriginalEyelidsSprite = CurrentSprite;
            VN_LOG_DEBUG(TEXT("External eyelids change detected during blink"));
        }
    }

    // Проверяем рот во время разговора
    if (IdleAnimationsConfig.TalkConfig.bEnabled && Character->Mouth_Sprite)
    {
        UPaperSprite* CurrentSprite = Character->Mouth_Sprite->GetSprite();
        if (CurrentSprite && !IsCurrentSpritePartOfTalkAnimation(CurrentSprite))
        {
            // Рот изменен извне - обновляем сохраненный
            OriginalMouthSprite = CurrentSprite;
            VN_LOG_DEBUG(TEXT("External mouth change detected during talk"));
        }
    }

    // Проверяем глаза во время движения
    if (bIsEyesRandomAnimationPlaying && Character->Eyes_Sprite)
    {
        UPaperSprite* CurrentSprite = Character->Eyes_Sprite->GetSprite();
        if (CurrentSprite && !IsAnimationSprite(Character->Eyes_Sprite, CurrentSprite))
        {
            // Глаза изменены извне - обновляем сохраненные
            OriginalEyesSprite = CurrentSprite;
            VN_LOG_DEBUG(TEXT("External eyes change detected during movement"));
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

    // Проверяем несколько точек времени
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
// ПУБЛИЧНЫЕ МЕТОДЫ API
// =====================================================

void UVNCharacterIdleAnimationManager::SetBlinkEnabled(bool bEnable)
{
    if (IdleAnimationsConfig.BlinkConfig.bEnabled == bEnable) return;
    
    IdleAnimationsConfig.BlinkConfig.bEnabled = bEnable;
    bEnable ? StartBlinkAnimation() : StopBlinkAnimation();
    LogIdleAnimation(FString::Printf(TEXT("Blink %s"), bEnable ? TEXT("enabled") : TEXT("disabled")));
}

void UVNCharacterIdleAnimationManager::SetTalkEnabled(bool bEnable)
{
    if (IdleAnimationsConfig.TalkConfig.bEnabled == bEnable) return;
    
    IdleAnimationsConfig.TalkConfig.bEnabled = bEnable;
    bEnable ? StartTalkAnimation() : StopTalkAnimation();
    LogIdleAnimation(FString::Printf(TEXT("Talk %s"), bEnable ? TEXT("enabled") : TEXT("disabled")));
}

void UVNCharacterIdleAnimationManager::SetEyesRandomEnabled(bool bEnable)
{
    if (IdleAnimationsConfig.EyesRandomConfig.bEnabled == bEnable) return;
    
    IdleAnimationsConfig.EyesRandomConfig.bEnabled = bEnable;
    bEnable ? StartEyesRandomAnimation() : StopEyesRandomAnimation();
    LogIdleAnimation(FString::Printf(TEXT("Eyes random %s"), bEnable ? TEXT("enabled") : TEXT("disabled")));
}

void UVNCharacterIdleAnimationManager::SetIdleAnimationsConfig(const FVNIdleAnimationsConfig& NewConfig)
{
    StopAllIdleAnimations();
    IdleAnimationsConfig = NewConfig;
    StartAllIdleAnimations();
    LogIdleAnimation(TEXT("Config updated"));
}

void UVNCharacterIdleAnimationManager::StopAllIdleAnimations()
{
    StopBlinkAnimation();
    StopTalkAnimation();
    StopEyesRandomAnimation();
}

void UVNCharacterIdleAnimationManager::StartAllIdleAnimations()
{
    if (bDisableIdleAnimations) return;

    if (IdleAnimationsConfig.BlinkConfig.bEnabled) StartBlinkAnimation();
    if (IdleAnimationsConfig.TalkConfig.bEnabled) StartTalkAnimation();
    if (IdleAnimationsConfig.EyesRandomConfig.bEnabled) StartEyesRandomAnimation();
}

void UVNCharacterIdleAnimationManager::UpdateSavedSprites()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character) return;

    if (Character->Eyelids_Sprite)
    {
        SaveCurrentSprite(Character->Eyelids_Sprite, OriginalEyelidsSprite);
    }
    if (Character->Mouth_Sprite)
    {
        SaveCurrentSprite(Character->Mouth_Sprite, OriginalMouthSprite);
    }
    if (Character->Eyes_Sprite)
    {
        SaveCurrentSprite(Character->Eyes_Sprite, OriginalEyesSprite);
    }
}

// =====================================================
// УТИЛИТЫ
// =====================================================

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

void UVNCharacterIdleAnimationManager::SaveCurrentSprite(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite>& SavedSprite)
{
    if (!Component) return;
    SavedSprite = Component->GetSprite();
}

void UVNCharacterIdleAnimationManager::RestoreCurrentSprite(UPaperSpriteComponent* Component, const TSoftObjectPtr<UPaperSprite>& SavedSprite)
{
    if (!Component) return;
    
    if (!SavedSprite.IsNull())
    {
        UPaperSprite* LoadedSprite = SavedSprite.LoadSynchronous();
        if (LoadedSprite)
        {
            Component->SetSprite(LoadedSprite);
        }
    }
    else
    {
        Component->SetSprite(nullptr);
    }
}