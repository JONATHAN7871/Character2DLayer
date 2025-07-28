// VNCharacter_SpriteFix.cpp - Исправление проблемы с исчезающими глазами

#include "Actors/VNCharacter.h"
#include "Components/VNCharacterIdleAnimationManager.h"
#include "Data/VNCharacterIdleAnimationDataAsset.h"
#include "VNCharacterSystemModule.h"
#include "Engine/World.h"

// =====================================================
// НОВЫЕ МЕТОДЫ ДЛЯ РАБОТЫ С IDLE ANIMATION DATAASSET
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

    // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: Останавливаем анимации и СОХРАНЯЕМ текущие спрайты
    IdleAnimationManager->StopAllIdleAnimations();
    
    // ИСПРАВЛЕНИЕ ГЛАЗ: Принудительно обновляем все сохраненные спрайты до применения
    IdleAnimationManager->UpdateSavedSprites();

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
    
    // ИСПРАВЛЕНИЕ: Обновляем сохраненные спрайты
    IdleAnimationManager->UpdateSavedSprites();

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

void AVNCharacter::SetBlinkAnimationSettings(UPaperFlipbook* BlinkFlipbook, bool bEnabled, float MinInterval, float MaxInterval, float Duration, float DoubleBlinkChance)
{
    if (!IdleAnimationManager) return;

    // Останавливаем моргание
    IdleAnimationManager->SetBlinkEnabled(false);
    
    // ИСПРАВЛЕНИЕ: Обновляем сохраненные спрайты
    IdleAnimationManager->UpdateSavedSprites();

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

    VN_LOG_DEBUG(TEXT("SetBlinkAnimationSettings: Updated blink settings"));
}

void AVNCharacter::SetTalkAnimationSettings(UPaperFlipbook* TalkFlipbook, bool bEnabled, float TalkSpeed)
{
    if (!IdleAnimationManager) return;

    // Останавливаем разговор
    IdleAnimationManager->SetTalkEnabled(false);
    
    // ИСПРАВЛЕНИЕ: Обновляем сохраненные спрайты
    IdleAnimationManager->UpdateSavedSprites();

    // Обновляем только настройки разговора
    FVNIdleAnimationsConfig CurrentConfig = IdleAnimationManager->GetIdleAnimationsConfig();
    
    CurrentConfig.TalkConfig.TalkFlipbook = TalkFlipbook;
    CurrentConfig.TalkConfig.bEnabled = bEnabled;
    CurrentConfig.TalkConfig.TalkSpeed = FMath::Clamp(TalkSpeed, 0.1f, 10.0f);

    // Применяем обновленную конфигурацию
    IdleAnimationManager->SetIdleAnimationsConfig(CurrentConfig);

    VN_LOG_DEBUG(TEXT("SetTalkAnimationSettings: Updated talk settings"));
}

void AVNCharacter::SetEyesAnimationSettings(UPaperFlipbook* EyesFlipbook, bool bEnabled, float MinLookDuration, float MaxLookDuration, float MinWaitDuration, float MaxWaitDuration)
{
    if (!IdleAnimationManager) return;

    // Останавливаем движения глаз
    IdleAnimationManager->SetEyesRandomEnabled(false);
    
    // ИСПРАВЛЕНИЕ: Обновляем сохраненные спрайты
    IdleAnimationManager->UpdateSavedSprites();

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

    VN_LOG_DEBUG(TEXT("SetEyesAnimationSettings: Updated eyes settings"));
}

// =====================================================
// ИСПРАВЛЕНИЕ DATAASSET БЕЗ IDLE АНИМАЦИЙ
// =====================================================

void AVNCharacter::ApplyDataAssetFixed(UVNCharacterDataAsset* CharacterData, bool bAnimate, float Duration)
{
    if (!CharacterData)
    {
        VN_LOG_WARNING(TEXT("ApplyDataAssetFixed: CharacterData is null"));
        return;
    }

    // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: Сохраняем текущие спрайты ПЕРЕД применением DataAsset
    if (IdleAnimationManager)
    {
        VN_LOG_DEBUG(TEXT("ApplyDataAssetFixed: Updating saved sprites before DataAsset application"));
        IdleAnimationManager->UpdateSavedSprites();
    }

    // Применяем обычный DataAsset (теперь БЕЗ idle анимаций)
    ApplyDataAsset(CharacterData, bAnimate, Duration);

    // ИСПРАВЛЕНИЕ ГЛАЗ: После применения DataAsset обновляем сохраненные спрайты снова
    if (IdleAnimationManager)
    {
        if (bAnimate && Duration > 0.0f)
        {
            // Ждем завершения анимации и обновляем сохраненные спрайты
            FTimerHandle FixTimer;
            GetWorld()->GetTimerManager().SetTimer(
                FixTimer,
                [this]()
                {
                    if (IdleAnimationManager)
                    {
                        IdleAnimationManager->UpdateSavedSprites();
                        VN_LOG_DEBUG(TEXT("ApplyDataAssetFixed: Updated saved sprites after animation"));
                    }
                },
                Duration + 0.1f,
                false
            );
        }
        else
        {
            // Мгновенное обновление
            IdleAnimationManager->UpdateSavedSprites();
        }
    }
}

// =====================================================
// ЭКСТРЕННОЕ ИСПРАВЛЕНИЕ ПРОПАВШИХ СПРАЙТОВ
// =====================================================

void AVNCharacter::FixMissingSprites()
{
    VN_LOG_WARNING(TEXT("FixMissingSprites: Emergency sprite restoration"));
    
    // Проверяем и исправляем основные компоненты лица
    bool bNeedsSpriteUpdate = false;

    if (Eyes_Sprite && !Eyes_Sprite->GetSprite())
    {
        VN_LOG_WARNING(TEXT("FixMissingSprites: Eyes sprite missing"));
        bNeedsSpriteUpdate = true;
    }

    if (Mouth_Sprite && !Mouth_Sprite->GetSprite())
    {
        VN_LOG_WARNING(TEXT("FixMissingSprites: Mouth sprite missing"));
        bNeedsSpriteUpdate = true;
    }
    
    if (Eyebrow_Sprite && !Eyebrow_Sprite->GetSprite())
    {
        VN_LOG_WARNING(TEXT("FixMissingSprites: Eyebrow sprite missing"));
        bNeedsSpriteUpdate = true;
    }

    // Если IdleAnimationManager есть и обнаружены проблемы - обновляем
    if (bNeedsSpriteUpdate && IdleAnimationManager)
    {
        VN_LOG_WARNING(TEXT("FixMissingSprites: Updating saved sprites in IdleAnimationManager"));
        IdleAnimationManager->UpdateSavedSprites();
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

    VN_LOG_DEBUG(TEXT("FixMissingSprites: Emergency fix completed"));
}

// =====================================================
// УТИЛИТЫ ДЛЯ ПРОВЕРКИ СОСТОЯНИЯ
// =====================================================

bool AVNCharacter::HasMissingSprites() const
{
    // Проверяем основные компоненты лица
    if (Eyes_Sprite && !Eyes_Sprite->GetSprite()) return true;
    if (Mouth_Sprite && !Mouth_Sprite->GetSprite()) return true;
    if (Eyebrow_Sprite && !Eyebrow_Sprite->GetSprite()) return true;
    
    return false;
}

FString AVNCharacter::GetSpritesStatusReport() const
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