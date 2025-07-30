#include "Actors/VNCharacter.h"
#include "Data/VNCharacterDataAsset.h"
#include "Data/VNCharacterIdleAnimationDataAsset.h"
#include "VNCharacterSystemModule.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Components/VNCharacterAnimationManager.h"
#include "Components/VNCharacterIdleAnimationManager.h"

// =====================================================
// РЕАЛИЗАЦИЯ МЕТОДОВ АВТОИНИЦИАЛИЗАЦИИ
// =====================================================

void AVNCharacter::PerformAutoInitialization()
{
    VN_LOG_DEBUG(TEXT("PerformAutoInitialization: Starting auto-initialization"));

    bool bHasCharacterData = (AutoInitCharacterData != nullptr);
    bool bHasIdleData = (AutoInitIdleData != nullptr);

    if (!bHasCharacterData && !bHasIdleData)
    {
        VN_LOG_WARNING(TEXT("PerformAutoInitialization: No DataAssets set for auto-initialization"));
        return;
    }

    // Применяем Character DataAsset если есть
    if (bHasCharacterData)
    {
        VN_LOG_DEBUG(TEXT("PerformAutoInitialization: Applying Character DataAsset: %s"), 
            *AutoInitCharacterData->GetName());
        
        if (bHasIdleData)
        {
            // Если есть оба DataAsset'а, используем интегрированный метод
            ApplyDataAssetWithIdleSupport(AutoInitCharacterData, bAutoInitWithAnimation, AutoInitAnimationDuration);
        }
        else
        {
            // Только Character DataAsset
            ApplyDataAsset(AutoInitCharacterData, bAutoInitWithAnimation, AutoInitAnimationDuration);
        }
    }

    // Применяем Idle DataAsset если есть (и если не применили выше)
    if (bHasIdleData && !bHasCharacterData)
    {
        VN_LOG_DEBUG(TEXT("PerformAutoInitialization: Applying Idle DataAsset: %s"), 
            *AutoInitIdleData->GetName());
        
        ApplyIdleAnimationDataAsset(AutoInitIdleData, bAutoStartIdleAnimations);
    }
    else if (bHasIdleData && bHasCharacterData)
    {
        // Если применили Character DataAsset выше, дополнительно настраиваем idle
        VN_LOG_DEBUG(TEXT("PerformAutoInitialization: Applying additional Idle settings"));
        
        if (bAutoInitWithAnimation)
        {
            // Ждем завершения анимации Character DataAsset, потом запускаем idle
            FTimerHandle DelayedIdleTimer;
            GetWorld()->GetTimerManager().SetTimer(
                DelayedIdleTimer,
                [this]()
                {
                    if (AutoInitIdleData && bAutoStartIdleAnimations)
                    {
                        ApplyIdleAnimationDataAssetWithEmotionalState(
                            AutoInitIdleData, 
                            AutoInitIdleData->DefaultEmotionalState, 
                            true
                        );
                    }
                },
                AutoInitAnimationDuration + 0.1f,
                false
            );
        }
        else if (bAutoStartIdleAnimations)
        {
            // Сразу запускаем idle анимации
            ApplyIdleAnimationDataAssetWithEmotionalState(
                AutoInitIdleData, 
                AutoInitIdleData->DefaultEmotionalState, 
                true
            );
        }
    }

    VN_LOG_DEBUG(TEXT("PerformAutoInitialization: Auto-initialization completed"));
}

void AVNCharacter::ApplyAutoInitSettings()
{
    if (!AutoInitCharacterData && !AutoInitIdleData)
    {
        VN_LOG_WARNING(TEXT("ApplyAutoInitSettings: No DataAssets set for initialization"));
        return;
    }

    VN_LOG_DEBUG(TEXT("ApplyAutoInitSettings: Manually triggered auto-initialization"));

    // Останавливаем текущие анимации если есть
    if (IsAnimating())
    {
        if (AnimationManager)
        {
            AnimationManager->ClearAnimationQueue();
        }
    }

    // Выполняем инициализацию
    PerformAutoInitialization();
}

void AVNCharacter::ClearAllSettings()
{
    VN_LOG_DEBUG(TEXT("ClearAllSettings: Clearing all character settings"));

    // Останавливаем все анимации
    if (AnimationManager)
    {
        AnimationManager->ClearAnimationQueue();
    }
    
    if (IdleAnimationManager)
    {
        IdleAnimationManager->StopAllIdleAnimations();
    }

    // Очищаем все Skeletal Mesh компоненты
    TArray<E_VN_ComponentID_Skeletal> SkeletalIDs = {
        E_VN_ComponentID_Skeletal::Body,
        E_VN_ComponentID_Skeletal::Arms,
        E_VN_ComponentID_Skeletal::Head,
        E_VN_ComponentID_Skeletal::Custom01,
        E_VN_ComponentID_Skeletal::Custom02,
        E_VN_ComponentID_Skeletal::Custom03
    };

    for (E_VN_ComponentID_Skeletal ID : SkeletalIDs)
    {
        SetSkeletalMesh(ID, nullptr, false);
    }

    // Очищаем все Sprite компоненты
    TArray<E_VN_ComponentID_Sprite> SpriteIDs = {
        E_VN_ComponentID_Sprite::Body,
        E_VN_ComponentID_Sprite::Arms,
        E_VN_ComponentID_Sprite::Head,
        E_VN_ComponentID_Sprite::Eyebrow,
        E_VN_ComponentID_Sprite::Eyes,
        E_VN_ComponentID_Sprite::Eyelids,
        E_VN_ComponentID_Sprite::Wink,
        E_VN_ComponentID_Sprite::Mouth,
        E_VN_ComponentID_Sprite::EmotionHead_01,
        E_VN_ComponentID_Sprite::EmotionHead_02,
        E_VN_ComponentID_Sprite::EmotionHead_03,
        E_VN_ComponentID_Sprite::EmotionBody_01,
        E_VN_ComponentID_Sprite::EmotionBody_02,
        E_VN_ComponentID_Sprite::EmotionBody_03
    };

    for (E_VN_ComponentID_Sprite ID : SpriteIDs)
    {
        SetSprite(ID, nullptr, false);
    }

    // Очищаем кэш
    UpdateSpriteCache();

    // Сбрасываем фокус и видимость
    SetFocus(true, 0.0f);
    SetActorHiddenInGame(false);

    VN_LOG_DEBUG(TEXT("ClearAllSettings: All settings cleared"));
}

void AVNCharacter::ApplyCharacterDataAssetOnly(bool bAnimate, float Duration)
{
    if (!AutoInitCharacterData)
    {
        VN_LOG_WARNING(TEXT("ApplyCharacterDataAssetOnly: No Character DataAsset set"));
        return;
    }

    VN_LOG_DEBUG(TEXT("ApplyCharacterDataAssetOnly: Applying %s"), *AutoInitCharacterData->GetName());
    ApplyDataAsset(AutoInitCharacterData, bAnimate, Duration);
}

void AVNCharacter::ApplyIdleDataAssetOnly(bool bRestartAnimations)
{
    if (!AutoInitIdleData)
    {
        VN_LOG_WARNING(TEXT("ApplyIdleDataAssetOnly: No Idle DataAsset set"));
        return;
    }

    VN_LOG_DEBUG(TEXT("ApplyIdleDataAssetOnly: Applying %s"), *AutoInitIdleData->GetName());
    ApplyIdleAnimationDataAsset(AutoInitIdleData, bRestartAnimations);
}

void AVNCharacter::SetAutoInitDataAssets(UVNCharacterDataAsset* CharacterData, UVNCharacterIdleAnimationDataAsset* IdleData)
{
    AutoInitCharacterData = CharacterData;
    AutoInitIdleData = IdleData;

    VN_LOG_DEBUG(TEXT("SetAutoInitDataAssets: Character DataAsset: %s, Idle DataAsset: %s"),
        CharacterData ? *CharacterData->GetName() : TEXT("None"),
        IdleData ? *IdleData->GetName() : TEXT("None"));
}

// =====================================================
// ДОПОЛНИТЬ PostEditChangeProperty В VNCharacter.cpp
// =====================================================

#if WITH_EDITOR
void AVNCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (!PropertyChangedEvent.Property)
        return;

    FName PropertyName = PropertyChangedEvent.Property->GetFName();

    // Обработка изменений DataAsset'ов для предпросмотра в редакторе
    if (PropertyName == GET_MEMBER_NAME_CHECKED(AVNCharacter, AutoInitCharacterData))
    {
        VN_LOG_DEBUG(TEXT("PostEditChangeProperty: AutoInitCharacterData changed"));
        
        if (AutoInitCharacterData && AnimationManager)
        {
            ApplyDataAsset(AutoInitCharacterData, false, 0.0f);
            VN_LOG_DEBUG(TEXT("PostEditChangeProperty: Applied Character DataAsset for preview"));
        }
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(AVNCharacter, AutoInitIdleData))
    {
        VN_LOG_DEBUG(TEXT("PostEditChangeProperty: AutoInitIdleData changed"));
        
        // ИСПРАВЛЕНИЕ: Безопасное применение без таймеров
        if (AutoInitIdleData && IdleAnimationManager)
        {
            // Не останавливаем анимации в редакторе - просто применяем конфиг
            const FVNIdleAnimationsConfig& NewConfig = AutoInitIdleData->GetIdleAnimationsConfig();
            IdleAnimationManager->SetIdleAnimationsConfig(NewConfig);
            
            VN_LOG_DEBUG(TEXT("PostEditChangeProperty: Applied Idle DataAsset config for preview"));
        }
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(AVNCharacter, bAutoApplyOnBeginPlay) ||
             PropertyName == GET_MEMBER_NAME_CHECKED(AVNCharacter, bAutoInitWithAnimation) ||
             PropertyName == GET_MEMBER_NAME_CHECKED(AVNCharacter, AutoInitAnimationDuration) ||
             PropertyName == GET_MEMBER_NAME_CHECKED(AVNCharacter, AutoInitDelay) ||
             PropertyName == GET_MEMBER_NAME_CHECKED(AVNCharacter, bAutoStartIdleAnimations))
    {
        // Простое логирование изменений настроек автоинициализации
        VN_LOG_DEBUG(TEXT("PostEditChangeProperty: Auto-init setting changed: %s"), *PropertyName.ToString());
    }

    // Примечание: Для корректного обновления в редакторе при изменении Global transforms
    // теперь рекомендуется повторно применить DataAsset, так как логика трансформации
    // централизована и применяется при вызове ApplyDataAsset.
}
#endif