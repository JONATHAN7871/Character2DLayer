#include "Actors/VNCharacter.h"
#include "Data/VNCharacterDataAsset.h"
#include "Components/VNCharacterAnimationManager.h"
#include "Components/VNCharacterIdleAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "Materials/MaterialInterface.h"

void AVNCharacter::ApplyDataAsset(UVNCharacterDataAsset* CharacterData, bool bAnimate, float Duration)
{
    if (!CharacterData)
    {
        VN_LOG_WARNING(TEXT("ApplyDataAsset: CharacterData is null"));
        return;
    }

    VN_LOG_DEBUG(TEXT("ApplyDataAsset: Starting application of DataAsset %s (animate=%s, duration=%.2f)"), 
        *CharacterData->GetName(), bAnimate ? TEXT("true") : TEXT("false"), Duration);

    // --- КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: ПРИНУДИТЕЛЬНОЕ ЗАВЕРШЕНИЕ ТЕКУЩЕЙ АНИМАЦИИ ---
    if (AnimationManager && AnimationManager->IsAnimating() && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::Transition)
    {
        VN_LOG_WARNING(TEXT("ApplyDataAsset: Forcing completion of ongoing transition to prevent conflicts"));
        AnimationManager->ClearAnimationQueue();
        FinalizeCurrentTransition();
    }

    // --- STEP 2: ИНИЦИАЛИЗАЦИЯ - ОЧИСТКА СПИСКОВ ---
    FadingInComponents.Empty();
    FadingOutComponents.Empty();

    // --- STEP 3: ПРИНУДИТЕЛЬНОЕ ПОКАЗАНИЕ ВСЕХ MAIN КОМПОНЕНТОВ ---
    // Это предотвращает проблемы с HiddenInGame при быстрой смене
    TArray<USceneComponent*> AllMainComponents = GetAllMainComponents();
    for (USceneComponent* Component : AllMainComponents)
    {
        if (Component)
        {
            Component->SetHiddenInGame(false);
        }
    }

    // --- STEP 4: ГЛОБАЛЬНЫЕ ТРАНСФОРМАЦИИ ---
    if (CharacterData->bOverrideGlobalTransforms)
    {
        VN_LOG_DEBUG(TEXT("ApplyDataAsset: Overriding global transforms"));
        GlobalSkeletalOffset = CharacterData->GlobalSkeletalOffset;
        GlobalSkeletalScale = CharacterData->GlobalSkeletalScale;
        GlobalSpriteOffset = CharacterData->GlobalSpriteOffset;
        GlobalSpriteScale = CharacterData->GlobalSpriteScale;
    }
    
    // --- STEP 5: СБОРКА ИЗМЕНЕНИЙ - ПРОХОД ПО ВСЕМ КОМПОНЕНТАМ ---
    
    // === SKELETAL КОМПОНЕНТЫ ===
    ProcessSkeletalComponentChange(E_VN_ComponentID_Skeletal::Body, CharacterData->BodyConfig.SkeletalMesh, bAnimate);
    ProcessSkeletalComponentChange(E_VN_ComponentID_Skeletal::Arms, CharacterData->ArmsConfig.SkeletalMesh, bAnimate);
    ProcessSkeletalComponentChange(E_VN_ComponentID_Skeletal::Head, CharacterData->HeadConfig.SkeletalMesh, bAnimate);
    ProcessSkeletalComponentChange(E_VN_ComponentID_Skeletal::Custom01, CharacterData->Custom01Config.SkeletalMesh, bAnimate);
    ProcessSkeletalComponentChange(E_VN_ComponentID_Skeletal::Custom02, CharacterData->Custom02Config.SkeletalMesh, bAnimate);
    ProcessSkeletalComponentChange(E_VN_ComponentID_Skeletal::Custom03, CharacterData->Custom03Config.SkeletalMesh, bAnimate);

    // === SPRITE КОМПОНЕНТЫ ===
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::Body, CharacterData->BodySpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::Arms, CharacterData->ArmsSpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::BodyShadow, CharacterData->BodyShadowSpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::Head, CharacterData->HeadSpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::EmotionBody_01, CharacterData->EmotionBodyEffect01SpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::EmotionBody_02, CharacterData->EmotionBodyEffect02SpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::EmotionBody_03, CharacterData->EmotionBodyEffect03SpriteConfig.Sprite, bAnimate);
    
    // === FACIAL SPRITE КОМПОНЕНТЫ ===
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::Eyebrow, CharacterData->EyebrowSpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::Eyes, CharacterData->EyesSpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::Eyelids, CharacterData->EyelidsSpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::Wink, CharacterData->WinkSpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::Mouth, CharacterData->MouthSpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::EmotionHead_01, CharacterData->EmotionHeadEffect01SpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::EmotionHead_02, CharacterData->EmotionHeadEffect02SpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::EmotionHead_03, CharacterData->EmotionHeadEffect03SpriteConfig.Sprite, bAnimate);

    // --- ПРИМЕНЕНИЕ ОСТАЛЬНЫХ СВОЙСТВ КОНФИГУРАЦИЙ ---
    ApplySkeletalConfigProperties(E_VN_ComponentID_Skeletal::Body, CharacterData->BodyConfig);
    ApplySkeletalConfigProperties(E_VN_ComponentID_Skeletal::Arms, CharacterData->ArmsConfig);
    ApplySkeletalConfigProperties(E_VN_ComponentID_Skeletal::Head, CharacterData->HeadConfig);
    ApplySkeletalConfigProperties(E_VN_ComponentID_Skeletal::Custom01, CharacterData->Custom01Config);
    ApplySkeletalConfigProperties(E_VN_ComponentID_Skeletal::Custom02, CharacterData->Custom02Config);
    ApplySkeletalConfigProperties(E_VN_ComponentID_Skeletal::Custom03, CharacterData->Custom03Config);
    
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::Body, CharacterData->BodySpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::Arms, CharacterData->ArmsSpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::BodyShadow, CharacterData->BodyShadowSpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::Head, CharacterData->HeadSpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::EmotionBody_01, CharacterData->EmotionBodyEffect01SpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::EmotionBody_02, CharacterData->EmotionBodyEffect02SpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::EmotionBody_03, CharacterData->EmotionBodyEffect03SpriteConfig);
    
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::Eyebrow, CharacterData->EyebrowSpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::Eyes, CharacterData->EyesSpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::Eyelids, CharacterData->EyelidsSpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::Wink, CharacterData->WinkSpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::Mouth, CharacterData->MouthSpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::EmotionHead_01, CharacterData->EmotionHeadEffect01SpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::EmotionHead_02, CharacterData->EmotionHeadEffect02SpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::EmotionHead_03, CharacterData->EmotionHeadEffect03SpriteConfig);

    // --- СКРЫТИЕ ТЕНЕВЫХ КОМПОНЕНТОВ ---
    if (BodyShadow_Sprite) BodyShadow_Sprite->SetVisibility(false);
    if (BodyShadow_Sprite_Fade) BodyShadow_Sprite_Fade->SetVisibility(false);

    // --- ЗАПУСК АНИМАЦИИ ---
    VN_LOG_DEBUG(TEXT("ApplyDataAsset: FadingIn components: %d, FadingOut components: %d"), 
        FadingInComponents.Num(), FadingOutComponents.Num());

    if (bAnimate && Duration > 0.0f && AnimationManager && (FadingInComponents.Num() > 0 || FadingOutComponents.Num() > 0))
    {
        VN_LOG_DEBUG(TEXT("ApplyDataAsset: Requesting batched transition commit with duration %.2f"), Duration);
        RequestTransitionCommit(Duration);
    }
    else
    {
        VN_LOG_DEBUG(TEXT("ApplyDataAsset: No animation needed or animation disabled, finalizing state immediately"));
        HideAllFadeComponents();
    }

    VN_LOG_DEBUG(TEXT("ApplyDataAsset: Completed application of DataAsset %s"), *CharacterData->GetName());
}

// =====================================================
// НОВЫЕ HELPER ФУНКЦИИ ДЛЯ ОБРАБОТКИ ИЗМЕНЕНИЙ
// =====================================================

void AVNCharacter::ProcessSkeletalComponentChange(E_VN_ComponentID_Skeletal ID, TSoftObjectPtr<USkeletalMesh> NewMesh, bool bAnimate)
{
    USkeletalMeshComponent* MainComp = GetSkeletalComponent(ID);
    if (!MainComp) return;
    
    const USkeletalMesh* CurrentMesh = MainComp->GetSkeletalMeshAsset();
    bool bAssetChanged = false;
    
    if (!CurrentMesh && !NewMesh.IsNull()) bAssetChanged = true;
    else if (CurrentMesh && NewMesh.IsNull()) bAssetChanged = true;
    else if (CurrentMesh && !NewMesh.IsNull()) bAssetChanged = (CurrentMesh->GetPathName() != NewMesh.ToString());
    
    if (bAssetChanged && bAnimate)
    {
        if (USkeletalMeshComponent* FadeComp = GetSkeletalFadeComponent(ID))
        {
            PrepareSkeletalTransition(MainComp, FadeComp, NewMesh);
        }
    }
    else if (bAssetChanged)
    {
        ValidateAndSetupSkeletalComponent(MainComp, NewMesh);
    }
}

void AVNCharacter::ProcessSpriteComponentChange(E_VN_ComponentID_Sprite ID, TSoftObjectPtr<UPaperSprite> NewSprite, bool bAnimate)
{
    UPaperSpriteComponent* MainComp = GetSpriteComponent(ID);
    if (!MainComp) return;
    
    const UPaperSprite* CurrentSprite = MainComp->GetSprite();
    bool bAssetChanged = false;
    
    if (!CurrentSprite && !NewSprite.IsNull()) bAssetChanged = true;
    else if (CurrentSprite && NewSprite.IsNull()) bAssetChanged = true;
    else if (CurrentSprite && !NewSprite.IsNull()) bAssetChanged = (CurrentSprite->GetPathName() != NewSprite.ToString());
    
    if (bAssetChanged && bAnimate)
    {
        if (UPaperSpriteComponent* FadeComp = GetSpriteFadeComponent(ID))
        {
            PrepareSpriteTransition(MainComp, FadeComp, NewSprite);
        }
    }
    else if (bAssetChanged)
    {
        ValidateAndSetupSpriteComponent(MainComp, NewSprite);
    }
}

// =====================================================
// ПРИМЕНЕНИЕ СВОЙСТВ КОНФИГУРАЦИИ (БЕЗ ИЗМЕНЕНИЯ АССЕТОВ)
// =====================================================

void AVNCharacter::ApplySkeletalConfigProperties(E_VN_ComponentID_Skeletal ID, const F_VN_SkeletalConfig_Body& Config)
{
    USkeletalMeshComponent* Comp = GetSkeletalComponent(ID);
    if (!Comp) return;
    
    if (Config.AnimInstanceClass) Comp->SetAnimInstanceClass(Config.AnimInstanceClass);
    
    for (const auto& MaterialOverride : Config.MaterialOverrides)
    {
        if (!MaterialOverride.Value.IsNull()) 
        {
            Comp->SetMaterial(MaterialOverride.Key, MaterialOverride.Value.LoadSynchronous());
        }
    }
    
    ResetComponentAttachmentToDefault(Comp);
    UpdateComponentTransform(Comp, Config.Offset, Config.Scale);
    
    // ИСПРАВЛЕНИЕ ФОКУСА: Применяем цвет из конфига, но с учетом фокуса
    FLinearColor ConfigColor = Config.Color;
    FLinearColor FinalColor = bIsInFocus ? ConfigColor : ConfigColor * DimColorMultiplier;
    SetComponentColor(Comp, FinalColor);
    
    if (!Config.SkeletalMesh.IsNull())
    {
        Comp->SetVisibility(Config.bVisible);
    }
}

void AVNCharacter::ApplySkeletalConfigProperties(E_VN_ComponentID_Skeletal ID, const F_VN_SkeletalConfig_Attachment& Config)
{
    USkeletalMeshComponent* Comp = GetSkeletalComponent(ID);
    if (!Comp) return;
    
    if (Config.AnimInstanceClass) Comp->SetAnimInstanceClass(Config.AnimInstanceClass);
    
    for (const auto& MaterialOverride : Config.MaterialOverrides)
    {
        if (!MaterialOverride.Value.IsNull()) 
        {
            Comp->SetMaterial(MaterialOverride.Key, MaterialOverride.Value.LoadSynchronous());
        }
    }
    
    if (Config.AttachTo != E_SkeletalAttachmentTarget::None)
    {
        if (USkeletalMeshComponent* AttachTarget = (Config.AttachTo == E_SkeletalAttachmentTarget::Body) ? Body_Skeletal : nullptr)
        {
            Comp->AttachToComponent(AttachTarget, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Config.SocketName);
        }
    }
    else
    {
        ResetComponentAttachmentToDefault(Comp);
    }
    
    UpdateComponentTransform(Comp, Config.Offset, Config.Scale);
    
    // ИСПРАВЛЕНИЕ ФОКУСА: Применяем цвет из конфига, но с учетом фокуса
    FLinearColor ConfigColor = Config.Color;
    FLinearColor FinalColor = bIsInFocus ? ConfigColor : ConfigColor * DimColorMultiplier;
    SetComponentColor(Comp, FinalColor);
    
    if (!Config.SkeletalMesh.IsNull())
    {
        Comp->SetVisibility(Config.bVisible);
    }
}

void AVNCharacter::ApplySpriteConfigProperties(E_VN_ComponentID_Sprite ID, const F_VN_SpriteConfig_Attachment& Config)
{
    UPaperSpriteComponent* Comp = GetSpriteComponent(ID);
    if (!Comp) return;
    
    if (Config.AttachTo != E_SpriteAttachmentTarget::None)
    {
        if (USkeletalMeshComponent* AttachTarget = GetSkeletalComponentBySpriteTarget(Config.AttachTo))
        {
            Comp->AttachToComponent(AttachTarget, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Config.SocketName);
        }
    }
    else
    {
        ResetComponentAttachmentToDefault(Comp);
    }
    
    UpdateComponentTransform(Comp, Config.Offset, Config.Scale);
    
    // ИСПРАВЛЕНИЕ ФОКУСА: Применяем цвет из конфига, но с учетом фокуса
    FLinearColor ConfigColor = Config.Color;
    FLinearColor FinalColor = bIsInFocus ? ConfigColor : ConfigColor * DimColorMultiplier;
    Comp->SetSpriteColor(FinalColor);
    
    if (!Config.Sprite.IsNull())
    {
        Comp->SetVisibility(Config.bVisible);
    }
}

void AVNCharacter::ApplySpriteConfigProperties(E_VN_ComponentID_Sprite ID, const F_VN_SpriteConfig_Simple& Config)
{
    UPaperSpriteComponent* Comp = GetSpriteComponent(ID);
    if (!Comp) return;
    
    UpdateComponentTransform(Comp, Config.Offset, Config.Scale);
    
    // ИСПРАВЛЕНИЕ ФОКУСА: Применяем цвет из конфига, но с учетом фокуса
    FLinearColor ConfigColor = Config.Color;
    FLinearColor FinalColor = bIsInFocus ? ConfigColor : ConfigColor * DimColorMultiplier;
    Comp->SetSpriteColor(FinalColor);
    
    if (!Config.Sprite.IsNull())
    {
        Comp->SetVisibility(Config.bVisible);
    }
}

// =====================================================
// UTILITY FUNCTION
// =====================================================

USkeletalMeshComponent* AVNCharacter::GetSkeletalComponentBySpriteTarget(E_SpriteAttachmentTarget Target)
{
    switch(Target)
    {
    case E_SpriteAttachmentTarget::Body_Skeletal: return Body_Skeletal;
    case E_SpriteAttachmentTarget::Arms_Skeletal: return Arms_Skeletal;
    case E_SpriteAttachmentTarget::Head_Skeletal: return Head_Skeletal;
    case E_SpriteAttachmentTarget::Custom01_Skeletal: return Custom01_Skeletal;
    case E_SpriteAttachmentTarget::Custom02_Skeletal: return Custom02_Skeletal;
    case E_SpriteAttachmentTarget::Custom03_Skeletal: return Custom03_Skeletal;
    default: return nullptr;
    }
}

void AVNCharacter::ApplyDataAssetWithIdleAnimations(UVNCharacterDataAsset* CharacterData, bool bAnimate, float Duration)
{
    if (!CharacterData)
    {
        VN_LOG_WARNING(TEXT("ApplyDataAssetWithIdleAnimations: CharacterData is null"));
        return;
    }

    // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: Останавливаем idle анимации ПЕРЕД применением DataAsset
    if (IdleAnimationManager)
    {
        VN_LOG_DEBUG(TEXT("ApplyDataAssetWithIdleAnimations: Stopping idle animations"));
        IdleAnimationManager->StopAllIdleAnimations();
    }

    // Применяем обычный DataAsset
    ApplyDataAsset(CharacterData, bAnimate, Duration);
    
    // ИСПРАВЛЕНИЕ: Применяем idle анимации ПОСЛЕ завершения основной анимации
    if (IdleAnimationManager)
    {
        if (bAnimate && Duration > 0.0f)
        {
            // Запускаем idle анимации с задержкой
            FTimerHandle DelayedIdleTimer;
            GetWorld()->GetTimerManager().SetTimer(
                DelayedIdleTimer,
                [this, CharacterData]()
                {
                    ApplyIdleAnimationsFromDataAsset(CharacterData);
                },
                Duration + 0.1f, // Небольшая задержка
                false
            );
        }
        else
        {
            // Мгновенное применение
            ApplyIdleAnimationsFromDataAsset(CharacterData);
        }
    }
}