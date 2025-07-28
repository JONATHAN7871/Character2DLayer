#include "Actors/VNCharacter.h"
#include "Components/VNCharacterIdleAnimationManager.h"
#include "Data/VNCharacterIdleAnimationDataAsset.h"
#include "Data/VNCharacterDataAsset.h"
#include "VNCharacterSystemModule.h"
#include "Engine/World.h"

void AVNCharacter::ApplyDataAssetWithIdleAnimations(UVNCharacterIdleAnimationDataAsset* IdleAnimationData, bool bAnimate, float Duration)
{
    if (!IdleAnimationData)
    {
        VN_LOG_WARNING(TEXT("ApplyDataAssetWithIdleAnimations: IdleAnimationData is null"));
        return;
    }

    if (!IdleAnimationManager)
    {
        VN_LOG_WARNING(TEXT("ApplyDataAssetWithIdleAnimations: IdleAnimationManager is null"));
        return;
    }

    VN_LOG_DEBUG(TEXT("ApplyDataAssetWithIdleAnimations: Applying IdleAnimationDataAsset %s"), *IdleAnimationData->GetName());

    IdleAnimationManager->StopAllIdleAnimations();
    SynchronizeIdleAnimationStates();

    ApplyIdleAnimationDataAssetWithEmotionalState(IdleAnimationData, IdleAnimationData->DefaultEmotionalState, true);

    VN_LOG_DEBUG(TEXT("ApplyDataAssetWithIdleAnimations: Applied successfully"));
}

void AVNCharacter::ApplyIdleAnimationDataAsset(UVNCharacterIdleAnimationDataAsset* IdleAnimationData, bool bRestartAnimations)
{
    if (!IdleAnimationData)
    {
        VN_LOG_WARNING(TEXT("ApplyIdleAnimationDataAsset: IdleAnimationData is null"));
        return;
    }

    if (!IdleAnimationManager)
    {
        VN_LOG_WARNING(TEXT("ApplyIdleAnimationDataAsset: IdleAnimationManager is null"));
        return;
    }

    VN_LOG_DEBUG(TEXT("ApplyIdleAnimationDataAsset: Applying %s"), *IdleAnimationData->GetName());

    IdleAnimationManager->StopAllIdleAnimations();
    SynchronizeIdleAnimationStates();

    ApplyIdleAnimationDataAssetWithEmotionalState(IdleAnimationData, IdleAnimationData->DefaultEmotionalState, bRestartAnimations);

    VN_LOG_DEBUG(TEXT("ApplyIdleAnimationDataAsset: Applied successfully - %s"), 
        *IdleAnimationData->GetConfigSummary());
}

void AVNCharacter::ApplyIdleAnimationDataAssetSmooth(UVNCharacterIdleAnimationDataAsset* IdleAnimationData, float DelayBeforeRestart)
{
    if (!IdleAnimationData || !IdleAnimationManager) return;

    IdleAnimationManager->StopAllIdleAnimations();
    SynchronizeIdleAnimationStates();

    FTimerHandle DelayedTimer;
    GetWorld()->GetTimerManager().SetTimer(
        DelayedTimer,
        [this, IdleAnimationData]()
        {
            if (IdleAnimationManager)
            {
                ApplyIdleAnimationDataAssetWithEmotionalState(IdleAnimationData, IdleAnimationData->DefaultEmotionalState, true);
            }
        },
        DelayBeforeRestart,
        false
    );
}

void AVNCharacter::ConfigureBlinkAnimation(UPaperFlipbook* BlinkFlipbook, bool bEnabled, float MinInterval, float MaxInterval, float Duration, float DoubleBlinkChance)
{
    if (!IdleAnimationManager) return;

    IdleAnimationManager->SetBlinkEnabled(false);
    SynchronizeIdleAnimationStates();

    FVNIdleAnimationsConfig CurrentConfig = IdleAnimationManager->GetIdleAnimationsConfig();
    
    CurrentConfig.BlinkConfig.BlinkFlipbook = BlinkFlipbook;
    CurrentConfig.BlinkConfig.bEnabled = bEnabled;
    CurrentConfig.BlinkConfig.MinBlinkInterval = FMath::Clamp(MinInterval, 0.5f, 10.0f);
    CurrentConfig.BlinkConfig.MaxBlinkInterval = FMath::Clamp(MaxInterval, MinInterval, 15.0f);
    CurrentConfig.BlinkConfig.BlinkDuration = FMath::Clamp(Duration, 0.05f, 0.5f);
    CurrentConfig.BlinkConfig.DoubleBlinkChance = FMath::Clamp(DoubleBlinkChance, 0.0f, 1.0f);

    IdleAnimationManager->SetIdleAnimationsConfig(CurrentConfig);

    VN_LOG_DEBUG(TEXT("ConfigureBlinkAnimation: Updated blink settings"));
}

void AVNCharacter::ConfigureTalkAnimation(UPaperFlipbook* TalkFlipbook, bool bEnabled, float TalkSpeed)
{
    if (!IdleAnimationManager) return;

    IdleAnimationManager->SetTalkEnabled(false);
    SynchronizeIdleAnimationStates();

    FVNIdleAnimationsConfig CurrentConfig = IdleAnimationManager->GetIdleAnimationsConfig();
    
    CurrentConfig.TalkConfig.TalkFlipbook = TalkFlipbook;
    CurrentConfig.TalkConfig.bEnabled = bEnabled;
    CurrentConfig.TalkConfig.TalkSpeed = FMath::Clamp(TalkSpeed, 0.1f, 10.0f);

    IdleAnimationManager->SetIdleAnimationsConfig(CurrentConfig);

    VN_LOG_DEBUG(TEXT("ConfigureTalkAnimation: Updated talk settings"));
}

void AVNCharacter::ConfigureEyesAnimation(UPaperFlipbook* EyesFlipbook, bool bEnabled, float MinLookDuration, float MaxLookDuration, float MinWaitDuration, float MaxWaitDuration)
{
    if (!IdleAnimationManager) return;

    IdleAnimationManager->SetEyesRandomEnabled(false);
    SynchronizeIdleAnimationStates();

    FVNIdleAnimationsConfig CurrentConfig = IdleAnimationManager->GetIdleAnimationsConfig();
    
    CurrentConfig.EyesRandomConfig.EyesDirectionsFlipbook = EyesFlipbook;
    CurrentConfig.EyesRandomConfig.bEnabled = bEnabled;
    CurrentConfig.EyesRandomConfig.MinLookDuration = FMath::Clamp(MinLookDuration, 0.1f, 5.0f);
    CurrentConfig.EyesRandomConfig.MaxLookDuration = FMath::Clamp(MaxLookDuration, MinLookDuration, 10.0f);
    CurrentConfig.EyesRandomConfig.MinWaitDuration = FMath::Clamp(MinWaitDuration, 0.1f, 10.0f);
    CurrentConfig.EyesRandomConfig.MaxWaitDuration = FMath::Clamp(MaxWaitDuration, MinWaitDuration, 15.0f);

    IdleAnimationManager->SetIdleAnimationsConfig(CurrentConfig);

    VN_LOG_DEBUG(TEXT("ConfigureEyesAnimation: Updated eyes settings"));
}

void AVNCharacter::ApplyDataAssetWithIdleSupport(UVNCharacterDataAsset* CharacterData, bool bAnimate, float Duration)
{
    if (!CharacterData)
    {
        VN_LOG_WARNING(TEXT("ApplyDataAssetWithIdleSupport: CharacterData is null"));
        return;
    }

    if (IdleAnimationManager)
    {
        VN_LOG_DEBUG(TEXT("ApplyDataAssetWithIdleSupport: Synchronizing states before DataAsset application"));
        IdleAnimationManager->StopAllIdleAnimations();
        SynchronizeIdleAnimationStates();
    }

    ApplyDataAsset(CharacterData, bAnimate, Duration);

    if (IdleAnimationManager)
    {
        if (bAnimate && Duration > 0.0f)
        {
            FTimerHandle SyncTimer;
            GetWorld()->GetTimerManager().SetTimer(
                SyncTimer,
                [this]()
                {
                    if (IdleAnimationManager)
                    {
                        // Принудительно обновляем все кэшированные спрайты
                        UpdateSpriteCache();
                        
                        IdleAnimationManager->StartAllIdleAnimations();
                        
                        VN_LOG_DEBUG(TEXT("ApplyDataAssetWithIdleSupport: States synchronized and idle animations restarted"));
                    }
                },
                Duration + 0.1f,
                false
            );
        }
        else
        {
            UpdateSpriteCache();
            IdleAnimationManager->StartAllIdleAnimations();
            VN_LOG_DEBUG(TEXT("ApplyDataAssetWithIdleSupport: States synchronized immediately"));
        }
    }
}

void AVNCharacter::RestoreComponentStates()
{
    VN_LOG_WARNING(TEXT("RestoreComponentStates: Restoring component states"));
    
    bool bNeedsStateSync = false;

    if (Eyes_Sprite && !Eyes_Sprite->GetSprite())
    {
        VN_LOG_WARNING(TEXT("RestoreComponentStates: Eyes sprite missing"));
        bNeedsStateSync = true;
    }

    if (Mouth_Sprite && !Mouth_Sprite->GetSprite())
    {
        VN_LOG_WARNING(TEXT("RestoreComponentStates: Mouth sprite missing"));
        bNeedsStateSync = true;
    }
    
    if (Eyebrow_Sprite && !Eyebrow_Sprite->GetSprite())
    {
        VN_LOG_WARNING(TEXT("RestoreComponentStates: Eyebrow sprite missing"));
        bNeedsStateSync = true;
    }

    if (bNeedsStateSync && IdleAnimationManager)
    {
        VN_LOG_WARNING(TEXT("RestoreComponentStates: Synchronizing states in IdleAnimationManager"));
        SynchronizeIdleAnimationStates();
    }
    
    TArray<USceneComponent*> AllMainComponents = GetAllMainComponents();
    for (USceneComponent* Component : AllMainComponents)
    {
        if (Component && Component != BodyShadow_Sprite)
        {
            Component->SetHiddenInGame(false);
            Component->SetVisibility(true);
        }
    }

    VN_LOG_DEBUG(TEXT("RestoreComponentStates: Component state restoration completed"));
}

bool AVNCharacter::ValidateComponentStates() const
{
    if (Eyes_Sprite && !Eyes_Sprite->GetSprite()) return false;
    if (Mouth_Sprite && !Mouth_Sprite->GetSprite()) return false;
    if (Eyebrow_Sprite && !Eyebrow_Sprite->GetSprite()) return false;
    
    return true;
}

FString AVNCharacter::GetComponentStatusReport() const
{
    TArray<FString> StatusLines;
    
    StatusLines.Add(FString::Printf(TEXT("Eyes: %s"), 
        (Eyes_Sprite && Eyes_Sprite->GetSprite()) ? *Eyes_Sprite->GetSprite()->GetName() : TEXT("MISSING")));
    
    StatusLines.Add(FString::Printf(TEXT("Mouth: %s"), 
        (Mouth_Sprite && Mouth_Sprite->GetSprite()) ? *Mouth_Sprite->GetSprite()->GetName() : TEXT("MISSING")));
    
    StatusLines.Add(FString::Printf(TEXT("Eyebrows: %s"), 
        (Eyebrow_Sprite && Eyebrow_Sprite->GetSprite()) ? *Eyebrow_Sprite->GetSprite()->GetName() : TEXT("MISSING")));

    StatusLines.Add(FString::Printf(TEXT("Eyelids: %s"), 
        (Eyelids_Sprite && Eyelids_Sprite->GetSprite()) ? *Eyelids_Sprite->GetSprite()->GetName() : TEXT("MISSING")));

    return FString::Join(StatusLines, TEXT("\n"));
}