// --- START OF FILE VNCharacter_Initialization.cpp ---

#include "Actors/VNCharacter.h"
#include "Data/VNCharacterDataAsset.h"
#include "Data/VNCharacterIdleAnimationDataAsset.h"
#include "Components/VNCharacterAnimationManager.h"
#include "Components/VNCharacterIdleAnimationManager.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "VNCharacterSystemModule.h"

void AVNCharacter::CharacterSpawn(bool bIsNarrator, UVNCharacterDataAsset* InCharacterData, UVNCharacterIdleAnimationDataAsset* InIdleData, bool bAnimate, float Duration)
{
    VN_LOG_DEBUG(TEXT("CharacterSpawn: Initializing character '%s'. Narrator: %s"), 
        *CharacterName, bIsNarrator ? TEXT("true") : TEXT("false"));

    // Останавливаем все текущие анимации, чтобы избежать конфликтов
    if (AnimationManager)
    {
        AnimationManager->ClearAnimationQueue();
    }
    if (IdleAnimationManager)
    {
        IdleAnimationManager->StopAllIdleAnimations();
    }

    // --- ОБРАБОТКА СЛУЧАЯ "РАССКАЗЧИК" ---
    if (bIsNarrator)
    {
        VN_LOG_DEBUG(TEXT("CharacterSpawn: Configuring as Narrator. Clearing all visuals."));
        ClearAllSettings(); // Очищает все спрайты, меши и кэши
        SetActorHiddenInGame(true); // Скрываем самого актора
        PrimaryActorTick.SetTickFunctionEnable(false); // Отключаем Tick за ненадобностью
        return;
    }

    // --- СТАНДАРТНАЯ ИНИЦИАЛИЗАЦИЯ ПЕРСОНАЖА ---
    
    // Убеждаемся, что актор видим
    SetActorHiddenInGame(false);
    PrimaryActorTick.SetTickFunctionEnable(true); // Включаем Tick, если он был выключен

    // Применяем Character DataAsset, если он указан
    if (InCharacterData)
    {
        VN_LOG_DEBUG(TEXT("CharacterSpawn: Applying Character DataAsset '%s'"), *InCharacterData->GetName());
        ApplyDataAsset(InCharacterData, bAnimate, Duration);
    }
    else
    {
        VN_LOG_WARNING(TEXT("CharacterSpawn: Character DataAsset is not provided. Character may be empty."));
        // Если DataAsset не указан, все равно очищаем персонажа, чтобы не осталось старых частей
        ClearAllSettings();
    }

    // Применяем Idle Animation DataAsset, если он указан
    if (InIdleData)
    {
        VN_LOG_DEBUG(TEXT("CharacterSpawn: Applying Idle DataAsset '%s'"), *InIdleData->GetName());

        // Если спавн анимированный, ждем завершения основной анимации
        if (bAnimate && Duration > 0.f)
        {
            FTimerHandle IdleApplyTimer;
            GetWorld()->GetTimerManager().SetTimer(
                IdleApplyTimer,
                [this, InIdleData]() 
                {
                    if (this && InIdleData) 
                    {
                        // Применяем Idle DataAsset с перезапуском анимаций
                        this->ApplyIdleAnimationDataAsset(InIdleData, true);
                        VN_LOG_DEBUG(TEXT("CharacterSpawn: Idle animations applied after delay."));
                    }
                }, 
                Duration + 0.1f, // Небольшая задержка после завершения основной анимации
                false);
        }
        else
        {
            // Применяем немедленно
            ApplyIdleAnimationDataAsset(InIdleData, true);
        }
    }
    else
    {
        VN_LOG_DEBUG(TEXT("CharacterSpawn: Idle DataAsset is not provided. No idle animations will be set."));
    }
}