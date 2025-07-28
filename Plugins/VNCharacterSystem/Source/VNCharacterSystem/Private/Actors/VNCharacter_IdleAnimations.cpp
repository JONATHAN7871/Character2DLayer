#include "Actors/VNCharacter.h"
#include "Components/VNCharacterIdleAnimationManager.h"
#include "VNCharacterSystemModule.h"

// =====================================================
// IDLE АНИМАЦИИ - ПУБЛИЧНЫЕ МЕТОДЫ
// =====================================================

void AVNCharacter::SetBlinkEnabled(bool bEnable)
{
    if (!IdleAnimationManager)
    {
        VN_LOG_WARNING(TEXT("SetBlinkEnabled: IdleAnimationManager is null"));
        return;
    }

    VN_LOG_DEBUG(TEXT("SetBlinkEnabled: %s"), bEnable ? TEXT("true") : TEXT("false"));
    IdleAnimationManager->SetBlinkEnabled(bEnable);
}

void AVNCharacter::SetTalkEnabled(bool bEnable)
{
    if (!IdleAnimationManager)
    {
        VN_LOG_WARNING(TEXT("SetTalkEnabled: IdleAnimationManager is null"));
        return;
    }

    VN_LOG_DEBUG(TEXT("SetTalkEnabled: %s"), bEnable ? TEXT("true") : TEXT("false"));
    IdleAnimationManager->SetTalkEnabled(bEnable);
}

void AVNCharacter::SetEyesRandomEnabled(bool bEnable)
{
    if (!IdleAnimationManager)
    {
        VN_LOG_WARNING(TEXT("SetEyesRandomEnabled: IdleAnimationManager is null"));
        return;
    }

    VN_LOG_DEBUG(TEXT("SetEyesRandomEnabled: %s"), bEnable ? TEXT("true") : TEXT("false"));
    IdleAnimationManager->SetEyesRandomEnabled(bEnable);
}

void AVNCharacter::SetIdleAnimationsConfig(const FVNIdleAnimationsConfig& NewConfig)
{
    if (!IdleAnimationManager)
    {
        VN_LOG_WARNING(TEXT("SetIdleAnimationsConfig: IdleAnimationManager is null"));
        return;
    }

    VN_LOG_DEBUG(TEXT("SetIdleAnimationsConfig: Updating idle animations config"));
    IdleAnimationManager->SetIdleAnimationsConfig(NewConfig);
}

const FVNIdleAnimationsConfig& AVNCharacter::GetIdleAnimationsConfig() const
{
    if (!IdleAnimationManager)
    {
        VN_LOG_WARNING(TEXT("GetIdleAnimationsConfig: IdleAnimationManager is null"));
        static FVNIdleAnimationsConfig EmptyConfig;
        return EmptyConfig;
    }

    return IdleAnimationManager->GetIdleAnimationsConfig();
}

void AVNCharacter::StopAllIdleAnimations()
{
    if (!IdleAnimationManager)
    {
        VN_LOG_WARNING(TEXT("StopAllIdleAnimations: IdleAnimationManager is null"));
        return;
    }

    VN_LOG_DEBUG(TEXT("StopAllIdleAnimations: Stopping all idle animations"));
    IdleAnimationManager->StopAllIdleAnimations();
}

void AVNCharacter::StartAllIdleAnimations()
{
    if (!IdleAnimationManager)
    {
        VN_LOG_WARNING(TEXT("StartAllIdleAnimations: IdleAnimationManager is null"));
        return;
    }

    VN_LOG_DEBUG(TEXT("StartAllIdleAnimations: Starting all idle animations"));
    IdleAnimationManager->StartAllIdleAnimations();
}

bool AVNCharacter::IsBlinkActive() const
{
    if (!IdleAnimationManager)
    {
        return false;
    }

    return IdleAnimationManager->IsBlinkActive();
}

bool AVNCharacter::IsTalkActive() const
{
    if (!IdleAnimationManager)
    {
        return false;
    }

    return IdleAnimationManager->IsTalkActive();
}

bool AVNCharacter::IsEyesRandomActive() const
{
    if (!IdleAnimationManager)
    {
        return false;
    }

    return IdleAnimationManager->IsEyesRandomActive();
}