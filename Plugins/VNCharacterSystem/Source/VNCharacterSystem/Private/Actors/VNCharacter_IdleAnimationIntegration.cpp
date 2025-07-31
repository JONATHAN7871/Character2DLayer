#include "Actors/VNCharacter.h"
#include "Components/VNCharacterIdleAnimationManager.h"
#include "Data/VNCharacterIdleAnimationDataAsset.h"
#include "Data/VNCharacterDataAsset.h"
#include "VNCharacterSystemModule.h"
#include "Engine/World.h"

void AVNCharacter::ApplyDataAssetWithIdleAnimations(UVNCharacterIdleAnimationDataAsset* IdleAnimationData, bool bAnimate, float Duration)
{
    // Эта функция теперь просто вызывает основной метод.
    ApplyIdleAnimationDataAssetWithEmotionalState(IdleAnimationData, IdleAnimationData->DefaultEmotionalState, true);
}

void AVNCharacter::ApplyIdleAnimationDataAsset(UVNCharacterIdleAnimationDataAsset* IdleAnimationData, bool bRestartAnimations)
{
    // Эта функция тоже просто вызывает основной метод.
    ApplyIdleAnimationDataAssetWithEmotionalState(IdleAnimationData, IdleAnimationData->DefaultEmotionalState, bRestartAnimations);
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

    // НИЧЕГО НЕ ДЕЛАЕМ С IDLE ЗДЕСЬ.
    // Просто вызываем ApplyDataAsset.
    // ApplyDataAsset вызовет SetSprite, а новая версия SetSprite
    // сама позаботится об остановке, сбросе, обновлении кэша и перезапуске Idle.
    
    VN_LOG_DEBUG(TEXT("ApplyDataAssetWithIdleSupport: Delegating all logic to ApplyDataAsset."));
    ApplyDataAsset(CharacterData, bAnimate, Duration);

    // Если DataAsset был применен мгновенно (неанимированно), SetSprite/SetFace
    // уже перезапустили Idle анимации. Если анимированно, они перезапустятся
    // в OnAnimationFinished(Transition), что абсолютно правильно.
    // Больше здесь ничего не нужно.
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