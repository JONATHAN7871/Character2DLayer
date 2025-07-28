// VNCharacter_IdleAnimationIntegration.cpp - Интеграция с системой idle-анимаций

#include "Actors/VNCharacter.h"
#include "Components/VNCharacterIdleAnimationManager.h"
#include "Data/VNCharacterIdleAnimationDataAsset.h"
#include "Data/VNCharacterDataAsset.h"  // ДОБАВЛЕНО: для корректной компиляции
#include "VNCharacterSystemModule.h"
#include "Engine/World.h"

// =====================================================
// ИНТЕГРАЦИЯ С IDLE ANIMATION DATAASSET
// =====================================================

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

    // Останавливаем текущие анимации и синхронизируем состояния
    IdleAnimationManager->StopAllIdleAnimations();
    SynchronizeIdleAnimationStates();

    // Применяем новую конфигурацию
    FVNIdleAnimationsConfig NewConfig = IdleAnimationData->GetIdleAnimationsConfig();
    IdleAnimationManager->SetIdleAnimationsConfig(NewConfig);

    // Перезапускаем анимации если нужно
    if (bRestartAnimations)
    {
        VN_LOG_DEBUG(TEXT("ApplyIdleAnimationDataAsset: Restarting animations"));
        IdleAnimationManager->StartAllIdleAnimations();
    }

    VN_LOG_DEBUG(TEXT("ApplyIdleAnimationDataAsset: Applied successfully - %s"), 
        *IdleAnimationData->GetConfigSummary());
}

void AVNCharacter::ApplyIdleAnimationDataAssetSmooth(UVNCharacterIdleAnimationDataAsset* IdleAnimationData, float DelayBeforeRestart)
{
    if (!IdleAnimationData || !IdleAnimationManager) return;

    // Останавливаем текущие анимации
    IdleAnimationManager->StopAllIdleAnimations();
    SynchronizeIdleAnimationStates();

    // Применяем конфигурацию с задержкой
    FTimerHandle DelayedTimer;
    GetWorld()->GetTimerManager().SetTimer(
        DelayedTimer,
        [this, IdleAnimationData]()
        {
            if (IdleAnimationManager)
            {
                FVNIdleAnimationsConfig NewConfig = IdleAnimationData->GetIdleAnimationsConfig();
                IdleAnimationManager->SetIdleAnimationsConfig(NewConfig);
                IdleAnimationManager->StartAllIdleAnimations();
            }
        },
        DelayBeforeRestart,
        false
    );
}

// =====================================================
// КОНФИГУРАЦИЯ ОТДЕЛЬНЫХ АНИМАЦИЙ
// =====================================================

void AVNCharacter::ConfigureBlinkAnimation(UPaperFlipbook* BlinkFlipbook, bool bEnabled, float MinInterval, float MaxInterval, float Duration, float DoubleBlinkChance)
{
    if (!IdleAnimationManager) return;

    // Останавливаем текущее моргание
    IdleAnimationManager->SetBlinkEnabled(false);
    SynchronizeIdleAnimationStates();

    // Получаем текущую конфигурацию и обновляем только моргание
    FVNIdleAnimationsConfig CurrentConfig = IdleAnimationManager->GetIdleAnimationsConfig();
    
    CurrentConfig.BlinkConfig.BlinkFlipbook = BlinkFlipbook;
    CurrentConfig.BlinkConfig.bEnabled = bEnabled;
    CurrentConfig.BlinkConfig.MinBlinkInterval = FMath::Clamp(MinInterval, 0.5f, 10.0f);
    CurrentConfig.BlinkConfig.MaxBlinkInterval = FMath::Clamp(MaxInterval, MinInterval, 15.0f);
    CurrentConfig.BlinkConfig.BlinkDuration = FMath::Clamp(Duration, 0.05f, 0.5f);
    CurrentConfig.BlinkConfig.DoubleBlinkChance = FMath::Clamp(DoubleBlinkChance, 0.0f, 1.0f);

    // Применяем обновленную конфигурацию
    IdleAnimationManager->SetIdleAnimationsConfig(CurrentConfig);

    VN_LOG_DEBUG(TEXT("ConfigureBlinkAnimation: Updated blink settings"));
}

void AVNCharacter::ConfigureTalkAnimation(UPaperFlipbook* TalkFlipbook, bool bEnabled, float TalkSpeed)
{
    if (!IdleAnimationManager) return;

    // Останавливаем текущий разговор
    IdleAnimationManager->SetTalkEnabled(false);
    SynchronizeIdleAnimationStates();

    // Обновляем только настройки разговора
    FVNIdleAnimationsConfig CurrentConfig = IdleAnimationManager->GetIdleAnimationsConfig();
    
    CurrentConfig.TalkConfig.TalkFlipbook = TalkFlipbook;
    CurrentConfig.TalkConfig.bEnabled = bEnabled;
    CurrentConfig.TalkConfig.TalkSpeed = FMath::Clamp(TalkSpeed, 0.1f, 10.0f);

    // Применяем обновленную конфигурацию
    IdleAnimationManager->SetIdleAnimationsConfig(CurrentConfig);

    VN_LOG_DEBUG(TEXT("ConfigureTalkAnimation: Updated talk settings"));
}

void AVNCharacter::ConfigureEyesAnimation(UPaperFlipbook* EyesFlipbook, bool bEnabled, float MinLookDuration, float MaxLookDuration, float MinWaitDuration, float MaxWaitDuration)
{
    if (!IdleAnimationManager) return;

    // Останавливаем движения глаз
    IdleAnimationManager->SetEyesRandomEnabled(false);
    SynchronizeIdleAnimationStates();

    // Обновляем только настройки глаз
    FVNIdleAnimationsConfig CurrentConfig = IdleAnimationManager->GetIdleAnimationsConfig();
    
    CurrentConfig.EyesRandomConfig.EyesDirectionsFlipbook = EyesFlipbook;
    CurrentConfig.EyesRandomConfig.bEnabled = bEnabled;
    CurrentConfig.EyesRandomConfig.MinLookDuration = FMath::Clamp(MinLookDuration, 0.1f, 5.0f);
    CurrentConfig.EyesRandomConfig.MaxLookDuration = FMath::Clamp(MaxLookDuration, MinLookDuration, 10.0f);
    CurrentConfig.EyesRandomConfig.MinWaitDuration = FMath::Clamp(MinWaitDuration, 0.1f, 10.0f);
    CurrentConfig.EyesRandomConfig.MaxWaitDuration = FMath::Clamp(MaxWaitDuration, MinWaitDuration, 15.0f);

    // Применяем обновленную конфигурацию
    IdleAnimationManager->SetIdleAnimationsConfig(CurrentConfig);

    VN_LOG_DEBUG(TEXT("ConfigureEyesAnimation: Updated eyes settings"));
}

// =====================================================
// УЛУЧШЕННАЯ ИНТЕГРАЦИЯ С DATAASSET
// =====================================================

void AVNCharacter::ApplyDataAssetWithIdleSupport(UVNCharacterDataAsset* CharacterData, bool bAnimate, float Duration)
{
    if (!CharacterData)
    {
        VN_LOG_WARNING(TEXT("ApplyDataAssetWithIdleSupport: CharacterData is null"));
        return;
    }

    // Синхронизируем состояния ПЕРЕД применением DataAsset
    if (IdleAnimationManager)
    {
        VN_LOG_DEBUG(TEXT("ApplyDataAssetWithIdleSupport: Synchronizing states before DataAsset application"));
        SynchronizeIdleAnimationStates();
    }

    // Применяем DataAsset (который теперь содержит только спрайты и мешы)
    ApplyDataAsset(CharacterData, bAnimate, Duration);

    // После применения DataAsset синхронизируем состояния снова
    if (IdleAnimationManager)
    {
        if (bAnimate && Duration > 0.0f)
        {
            // Ждем завершения анимации и синхронизируем состояния
            FTimerHandle SyncTimer;
            GetWorld()->GetTimerManager().SetTimer(
                SyncTimer,
                [this]()
                {
                    if (IdleAnimationManager)
                    {
                        SynchronizeIdleAnimationStates();
                        VN_LOG_DEBUG(TEXT("ApplyDataAssetWithIdleSupport: States synchronized after animation"));
                    }
                },
                Duration + 0.1f,
                false
            );
        }
        else
        {
            // Мгновенная синхронизация
            SynchronizeIdleAnimationStates();
        }
    }
}

void AVNCharacter::ApplyIdleAnimationsFromDataAsset(UVNCharacterDataAsset* CharacterData)
{
    // УСТАРЕЛО: Этот метод больше не нужен, так как idle анимации 
    // теперь в отдельном UVNCharacterIdleAnimationDataAsset
    
    VN_LOG_WARNING(TEXT("ApplyIdleAnimationsFromDataAsset: This method is deprecated. Use ApplyIdleAnimationDataAsset with UVNCharacterIdleAnimationDataAsset instead."));
    
    // Просто синхронизируем состояния
    if (IdleAnimationManager)
    {
        SynchronizeIdleAnimationStates();
    }
}

// =====================================================
// СИСТЕМА ВАЛИДАЦИИ И ВОССТАНОВЛЕНИЯ СОСТОЯНИЙ
// =====================================================

void AVNCharacter::RestoreComponentStates()
{
    VN_LOG_WARNING(TEXT("RestoreComponentStates: Restoring component states"));
    
    // Проверяем и восстанавливаем основные компоненты лица
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

    // Если IdleAnimationManager есть и обнаружены проблемы - синхронизируем
    if (bNeedsStateSync && IdleAnimationManager)
    {
        VN_LOG_WARNING(TEXT("RestoreComponentStates: Synchronizing states in IdleAnimationManager"));
        SynchronizeIdleAnimationStates();
    }
    
    // Принудительно показываем все основные компоненты
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
    // Проверяем основные компоненты лица
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

// =====================================================
// ВНУТРЕННИЕ ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ
// =====================================================

void AVNCharacter::SynchronizeIdleAnimationStates()
{
    if (!IdleAnimationManager) return;

    // Обновляем все сохраненные спрайты до текущего состояния
    IdleAnimationManager->UpdateSavedSprites();
    
    VN_LOG_DEBUG(TEXT("SynchronizeIdleAnimationStates: Idle animation states synchronized"));
}