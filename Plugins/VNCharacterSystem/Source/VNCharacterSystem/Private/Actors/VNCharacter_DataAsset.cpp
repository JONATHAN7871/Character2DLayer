#include "Actors/VNCharacter.h"
#include "Data/VNCharacterDataAsset.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "Materials/MaterialInterface.h"

void AVNCharacter::ApplyDataAsset(UVNCharacterDataAsset* CharacterData, bool bAnimate, float Duration)
{
    if (!CharacterData)
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyDataAsset: CharacterData is null"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("ApplyDataAsset: Starting application of DataAsset %s (animate=%s, duration=%.2f)"), 
        *CharacterData->GetName(), bAnimate ? TEXT("true") : TEXT("false"), Duration);

    // --- ПРЕРЫВАНИЕ ТЕКУЩЕЙ АНИМАЦИИ ---
    if (AnimationManager && AnimationManager->IsAnimating() && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::Transition)
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyDataAsset: Interrupted ongoing transition. Finalizing..."));
        AnimationManager->ClearAnimationQueue(); 
    }

    // --- ГЛОБАЛЬНЫЕ ТРАНСФОРМАЦИИ ---
    if (CharacterData->bOverrideGlobalTransforms)
    {
        UE_LOG(LogTemp, Log, TEXT("ApplyDataAsset: Overriding global transforms"));
        GlobalSkeletalOffset = CharacterData->GlobalSkeletalOffset;
        GlobalSkeletalScale = CharacterData->GlobalSkeletalScale;
        GlobalSpriteOffset = CharacterData->GlobalSpriteOffset;
        GlobalSpriteScale = CharacterData->GlobalSpriteScale;
    }
    
    // --- ПРИМЕНЕНИЕ КОНФИГУРАЦИЙ ---
    UE_LOG(LogTemp, Log, TEXT("ApplyDataAsset: Applying skeletal configurations..."));
    ApplySkeletalConfig(E_VN_ComponentID_Skeletal::Body, CharacterData->BodyConfig, bAnimate);
    ApplySkeletalConfig(E_VN_ComponentID_Skeletal::Arms, CharacterData->ArmsConfig, bAnimate);
    ApplySkeletalConfig(E_VN_ComponentID_Skeletal::Head, CharacterData->HeadConfig, bAnimate);
    ApplySkeletalConfig(E_VN_ComponentID_Skeletal::Custom01, CharacterData->Custom01Config, bAnimate);
    ApplySkeletalConfig(E_VN_ComponentID_Skeletal::Custom02, CharacterData->Custom02Config, bAnimate);
    ApplySkeletalConfig(E_VN_ComponentID_Skeletal::Custom03, CharacterData->Custom03Config, bAnimate);

    UE_LOG(LogTemp, Log, TEXT("ApplyDataAsset: Applying sprite configurations..."));
    ApplySpriteConfig(E_VN_ComponentID_Sprite::Body, CharacterData->BodySpriteConfig, bAnimate);
    ApplySpriteConfig(E_VN_ComponentID_Sprite::Arms, CharacterData->ArmsSpriteConfig, bAnimate);
    ApplySpriteConfig(E_VN_ComponentID_Sprite::BodyShadow, CharacterData->BodyShadowSpriteConfig, bAnimate);
    ApplySpriteConfig(E_VN_ComponentID_Sprite::Head, CharacterData->HeadSpriteConfig, bAnimate);
    ApplySpriteConfig(E_VN_ComponentID_Sprite::EmotionBody_01, CharacterData->EmotionBodyEffect01SpriteConfig, bAnimate);
    ApplySpriteConfig(E_VN_ComponentID_Sprite::EmotionBody_02, CharacterData->EmotionBodyEffect02SpriteConfig, bAnimate);
    ApplySpriteConfig(E_VN_ComponentID_Sprite::EmotionBody_03, CharacterData->EmotionBodyEffect03SpriteConfig, bAnimate);

    UE_LOG(LogTemp, Log, TEXT("ApplyDataAsset: Applying facial sprite configurations..."));
    ApplySpriteConfig(E_VN_ComponentID_Sprite::Eyebrow, CharacterData->EyebrowSpriteConfig, bAnimate);
    ApplySpriteConfig(E_VN_ComponentID_Sprite::Eyes, CharacterData->EyesSpriteConfig, bAnimate);
    ApplySpriteConfig(E_VN_ComponentID_Sprite::Eyelids, CharacterData->EyelidsSpriteConfig, bAnimate);
    ApplySpriteConfig(E_VN_ComponentID_Sprite::Wink, CharacterData->WinkSpriteConfig, bAnimate);
    ApplySpriteConfig(E_VN_ComponentID_Sprite::Mouth, CharacterData->MouthSpriteConfig, bAnimate);
    ApplySpriteConfig(E_VN_ComponentID_Sprite::EmotionHead_01, CharacterData->EmotionHeadEffect01SpriteConfig, bAnimate);
    ApplySpriteConfig(E_VN_ComponentID_Sprite::EmotionHead_02, CharacterData->EmotionHeadEffect02SpriteConfig, bAnimate);
    ApplySpriteConfig(E_VN_ComponentID_Sprite::EmotionHead_03, CharacterData->EmotionHeadEffect03SpriteConfig, bAnimate);

    // --- СКРЫТИЕ ТЕНЕВЫХ КОМПОНЕНТОВ ---
    if (BodyShadow_Sprite) BodyShadow_Sprite->SetVisibility(false);
    if (BodyShadow_Sprite_Fade) BodyShadow_Sprite_Fade->SetVisibility(false);

    // --- ЗАПУСК АНИМАЦИИ ---
    UE_LOG(LogTemp, Warning, TEXT("ApplyDataAsset: FadingIn components: %d, FadingOut components: %d"), 
        FadingInComponents.Num(), FadingOutComponents.Num());

    if (bAnimate && Duration > 0.0f && AnimationManager && (FadingInComponents.Num() > 0 || FadingOutComponents.Num() > 0))
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyDataAsset: Starting transition animation with %d components (duration=%.2f)"), 
            FadingInComponents.Num() + FadingOutComponents.Num(), Duration);
        AnimationManager->PlayTransition(Duration);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("ApplyDataAsset: No animation needed, hiding fade components"));
        HideAllFadeComponents();
    }

    UE_LOG(LogTemp, Warning, TEXT("ApplyDataAsset: Completed application of DataAsset %s"), *CharacterData->GetName());
}

// =====================================================
// УНИВЕРСАЛЬНЫЕ SKELETAL CONFIG FUNCTIONS
// =====================================================

void AVNCharacter::ApplySkeletalConfig(E_VN_ComponentID_Skeletal ID, const F_VN_SkeletalConfig_Body& Config, bool bAnimate)
{
    USkeletalMeshComponent* Comp = GetSkeletalComponent(ID);
    if (!Comp) 
    {
        UE_LOG(LogTemp, Error, TEXT("ApplySkeletalConfig: Component not found for ID %d"), (int32)ID);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("ApplySkeletalConfig: Processing skeletal component %s"), *Comp->GetName());

    // --- ПРОВЕРКА ИЗМЕНЕНИЯ АССЕТА ---
    const USkeletalMesh* CurrentMesh = Comp->GetSkeletalMeshAsset();
    bool bAssetChanged = false;
    
    if (!CurrentMesh && !Config.SkeletalMesh.IsNull())
    {
        bAssetChanged = true;
        UE_LOG(LogTemp, Warning, TEXT("ApplySkeletalConfig: %s changed from NULL to %s"), 
            *Comp->GetName(), *Config.SkeletalMesh.ToString());
    }
    else if (CurrentMesh && Config.SkeletalMesh.IsNull())
    {
        bAssetChanged = true;
        UE_LOG(LogTemp, Warning, TEXT("ApplySkeletalConfig: %s changed from %s to NULL"), 
            *Comp->GetName(), *CurrentMesh->GetName());
    }
    else if (CurrentMesh && !Config.SkeletalMesh.IsNull())
    {
        bAssetChanged = (CurrentMesh->GetPathName() != Config.SkeletalMesh.ToString());
        if (bAssetChanged)
        {
            UE_LOG(LogTemp, Warning, TEXT("ApplySkeletalConfig: %s changed from %s to %s"), 
                *Comp->GetName(), *CurrentMesh->GetName(), *Config.SkeletalMesh.ToString());
        }
    }

    // --- ПРИМЕНЕНИЕ АССЕТА С УНИВЕРСАЛЬНОЙ СИСТЕМОЙ ПЕРЕХОДОВ ---
    if (bAnimate && bAssetChanged)
    {
        if (USkeletalMeshComponent* FadeComp = GetSkeletalFadeComponent(ID))
        {
            UE_LOG(LogTemp, Warning, TEXT("ApplySkeletalConfig: Preparing universal transition for %s"), *Comp->GetName());
            PrepareSkeletalTransition(Comp, FadeComp, Config.SkeletalMesh);
            
            // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: НЕ применяем свойства для случая "Content → Empty"
            // Свойства применятся после анимации в FinalizeCurrentTransition
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("ApplySkeletalConfig: Fade component not found for %s"), *Comp->GetName());
            ValidateAndSetupSkeletalComponent(Comp, Config.SkeletalMesh);
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("ApplySkeletalConfig: Applying %s instantly"), *Comp->GetName());
        ValidateAndSetupSkeletalComponent(Comp, Config.SkeletalMesh);
    }

    // --- ПРИМЕНЕНИЕ ОСТАЛЬНЫХ СВОЙСТВ (ТОЛЬКО ЕСЛИ НЕ В ПРОЦЕССЕ АНИМАЦИИ) ---
    // Если компонент не исчезает, применяем свойства
    if (!FadingOutComponents.Contains(Comp))
    {
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
        SetComponentColor(Comp, Config.Color);
        
        // --- ВИДИМОСТЬ ---
        if (!Config.SkeletalMesh.IsNull())
        {
            Comp->SetVisibility(Config.bVisible);
            UE_LOG(LogTemp, Log, TEXT("ApplySkeletalConfig: Set visibility for %s to %s"), 
                *Comp->GetName(), Config.bVisible ? TEXT("true") : TEXT("false"));
        }
        else if (!bAnimate || !bAssetChanged)
        {
            // Скрываем только если не идет анимация исчезновения
            Comp->SetVisibility(false);
            UE_LOG(LogTemp, Log, TEXT("ApplySkeletalConfig: Hidden %s (null mesh)"), *Comp->GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("ApplySkeletalConfig: Skipping properties for %s (component is fading out)"), *Comp->GetName());
    }
}

void AVNCharacter::ApplySkeletalConfig(E_VN_ComponentID_Skeletal ID, const F_VN_SkeletalConfig_Attachment& Config, bool bAnimate)
{
    USkeletalMeshComponent* Comp = GetSkeletalComponent(ID);
    if (!Comp) 
    {
        UE_LOG(LogTemp, Error, TEXT("ApplySkeletalConfig: Attachment component not found for ID %d"), (int32)ID);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("ApplySkeletalConfig: Processing skeletal attachment component %s"), *Comp->GetName());

    // --- ПРОВЕРКА ИЗМЕНЕНИЯ АССЕТА ---
    const USkeletalMesh* CurrentMesh = Comp->GetSkeletalMeshAsset();
    bool bAssetChanged = false;
    
    if (!CurrentMesh && !Config.SkeletalMesh.IsNull())
    {
        bAssetChanged = true;
        UE_LOG(LogTemp, Warning, TEXT("ApplySkeletalConfig: Attachment %s changed from NULL to %s"), 
            *Comp->GetName(), *Config.SkeletalMesh.ToString());
    }
    else if (CurrentMesh && Config.SkeletalMesh.IsNull())
    {
        bAssetChanged = true;
        UE_LOG(LogTemp, Warning, TEXT("ApplySkeletalConfig: Attachment %s changed from %s to NULL"), 
            *Comp->GetName(), *CurrentMesh->GetName());
    }
    else if (CurrentMesh && !Config.SkeletalMesh.IsNull())
    {
        bAssetChanged = (CurrentMesh->GetPathName() != Config.SkeletalMesh.ToString());
        if (bAssetChanged)
        {
            UE_LOG(LogTemp, Warning, TEXT("ApplySkeletalConfig: Attachment %s changed from %s to %s"), 
                *Comp->GetName(), *CurrentMesh->GetName(), *Config.SkeletalMesh.ToString());
        }
    }

    // --- ПРИМЕНЕНИЕ АССЕТА С УНИВЕРСАЛЬНОЙ СИСТЕМОЙ ПЕРЕХОДОВ ---
    if (bAnimate && bAssetChanged)
    {
        if (USkeletalMeshComponent* FadeComp = GetSkeletalFadeComponent(ID))
        {
            UE_LOG(LogTemp, Warning, TEXT("ApplySkeletalConfig: Preparing universal transition for attachment %s"), *Comp->GetName());
            PrepareSkeletalTransition(Comp, FadeComp, Config.SkeletalMesh);
            
            // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: НЕ применяем свойства для случая "Content → Empty"
            // Свойства применятся после анимации в FinalizeCurrentTransition
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("ApplySkeletalConfig: Fade component not found for attachment %s"), *Comp->GetName());
            ValidateAndSetupSkeletalComponent(Comp, Config.SkeletalMesh);
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("ApplySkeletalConfig: Applying attachment %s instantly"), *Comp->GetName());
        ValidateAndSetupSkeletalComponent(Comp, Config.SkeletalMesh);
    }

    // --- ПРИМЕНЕНИЕ ОСТАЛЬНЫХ СВОЙСТВ (ТОЛЬКО ЕСЛИ НЕ В ПРОЦЕССЕ АНИМАЦИИ) ---
    // Если компонент не исчезает, применяем свойства
    if (!FadingOutComponents.Contains(Comp))
    {
        if (Config.AnimInstanceClass) Comp->SetAnimInstanceClass(Config.AnimInstanceClass);
        
        for (const auto& MaterialOverride : Config.MaterialOverrides)
        {
            if (!MaterialOverride.Value.IsNull()) 
            {
                Comp->SetMaterial(MaterialOverride.Key, MaterialOverride.Value.LoadSynchronous());
            }
        }
        
        // --- ATTACHMENT ---
        if (Config.AttachTo != E_SkeletalAttachmentTarget::None)
        {
            if (USkeletalMeshComponent* AttachTarget = (Config.AttachTo == E_SkeletalAttachmentTarget::Body) ? Body_Skeletal : nullptr)
            {
                Comp->AttachToComponent(AttachTarget, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Config.SocketName);
                UE_LOG(LogTemp, Log, TEXT("ApplySkeletalConfig: Attached %s to %s"), *Comp->GetName(), *AttachTarget->GetName());
            }
        }
        else
        {
            ResetComponentAttachmentToDefault(Comp);
        }
        
        UpdateComponentTransform(Comp, Config.Offset, Config.Scale);
        SetComponentColor(Comp, Config.Color);
        
        // --- ВИДИМОСТЬ ---
        if (!Config.SkeletalMesh.IsNull())
        {
            Comp->SetVisibility(Config.bVisible);
            UE_LOG(LogTemp, Log, TEXT("ApplySkeletalConfig: Set visibility for attachment %s to %s"), 
                *Comp->GetName(), Config.bVisible ? TEXT("true") : TEXT("false"));
        }
        else if (!bAnimate || !bAssetChanged)
        {
            // Скрываем только если не идет анимация исчезновения
            Comp->SetVisibility(false);
            UE_LOG(LogTemp, Log, TEXT("ApplySkeletalConfig: Hidden attachment %s (null mesh)"), *Comp->GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("ApplySkeletalConfig: Skipping properties for attachment %s (component is fading out)"), *Comp->GetName());
    }
}

// =====================================================
// УНИВЕРСАЛЬНЫЕ SPRITE CONFIG FUNCTIONS
// =====================================================

void AVNCharacter::ApplySpriteConfig(E_VN_ComponentID_Sprite ID, const F_VN_SpriteConfig_Attachment& Config, bool bAnimate)
{
    UPaperSpriteComponent* Comp = GetSpriteComponent(ID);
    if (!Comp) 
    {
        UE_LOG(LogTemp, Error, TEXT("ApplySpriteConfig: Component not found for ID %d"), (int32)ID);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("ApplySpriteConfig: Processing sprite attachment component %s"), *Comp->GetName());

    // --- ПРОВЕРКА ИЗМЕНЕНИЯ АССЕТА ---
    const UPaperSprite* CurrentSprite = Comp->GetSprite();
    bool bAssetChanged = false;
    
    if (!CurrentSprite && !Config.Sprite.IsNull())
    {
        bAssetChanged = true;
        UE_LOG(LogTemp, Warning, TEXT("ApplySpriteConfig: %s changed from NULL to %s"), 
            *Comp->GetName(), *Config.Sprite.ToString());
    }
    else if (CurrentSprite && Config.Sprite.IsNull())
    {
        bAssetChanged = true;
        UE_LOG(LogTemp, Warning, TEXT("ApplySpriteConfig: %s changed from %s to NULL"), 
            *Comp->GetName(), *CurrentSprite->GetName());
    }
    else if (CurrentSprite && !Config.Sprite.IsNull())
    {
        bAssetChanged = (CurrentSprite->GetPathName() != Config.Sprite.ToString());
        if (bAssetChanged)
        {
            UE_LOG(LogTemp, Warning, TEXT("ApplySpriteConfig: %s changed from %s to %s"), 
                *Comp->GetName(), *CurrentSprite->GetName(), *Config.Sprite.ToString());
        }
    }

    // --- ПРИМЕНЕНИЕ АССЕТА С УНИВЕРСАЛЬНОЙ СИСТЕМОЙ ПЕРЕХОДОВ ---
    if (bAnimate && bAssetChanged)
    {
        if (UPaperSpriteComponent* FadeComp = GetSpriteFadeComponent(ID))
        {
            UE_LOG(LogTemp, Warning, TEXT("ApplySpriteConfig: Preparing universal transition for %s"), *Comp->GetName());
            PrepareSpriteTransition(Comp, FadeComp, Config.Sprite);
            
            // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: НЕ применяем свойства для случая "Content → Empty"
            // Свойства применятся после анимации в FinalizeCurrentTransition
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("ApplySpriteConfig: Fade component not found for %s"), *Comp->GetName());
            ValidateAndSetupSpriteComponent(Comp, Config.Sprite);
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("ApplySpriteConfig: Applying %s instantly"), *Comp->GetName());
        ValidateAndSetupSpriteComponent(Comp, Config.Sprite);
    }

    // --- ПРИМЕНЕНИЕ ОСТАЛЬНЫХ СВОЙСТВ (ТОЛЬКО ЕСЛИ НЕ В ПРОЦЕССЕ АНИМАЦИИ) ---
    // Если компонент не исчезает, применяем свойства
    if (!FadingOutComponents.Contains(Comp))
    {
        // --- ATTACHMENT ---
        if (Config.AttachTo != E_SpriteAttachmentTarget::None)
        {
            if (USkeletalMeshComponent* AttachTarget = GetSkeletalComponentBySpriteTarget(Config.AttachTo))
            {
                Comp->AttachToComponent(AttachTarget, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Config.SocketName);
                UE_LOG(LogTemp, Log, TEXT("ApplySpriteConfig: Attached sprite %s to %s"), *Comp->GetName(), *AttachTarget->GetName());
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("ApplySpriteConfig: Failed to find attachment target for %s"), *Comp->GetName());
            }
        }
        else
        {
            ResetComponentAttachmentToDefault(Comp);
        }
        
        UpdateComponentTransform(Comp, Config.Offset, Config.Scale);
        Comp->SetSpriteColor(Config.Color);
        
        // --- ВИДИМОСТЬ ---
        if (!Config.Sprite.IsNull())
        {
            Comp->SetVisibility(Config.bVisible);
            UE_LOG(LogTemp, Log, TEXT("ApplySpriteConfig: Set visibility for %s to %s"), 
                *Comp->GetName(), Config.bVisible ? TEXT("true") : TEXT("false"));
        }
        else if (!bAnimate || !bAssetChanged)
        {
            // Скрываем только если не идет анимация исчезновения
            Comp->SetVisibility(false);
            UE_LOG(LogTemp, Log, TEXT("ApplySpriteConfig: Hidden %s (null sprite)"), *Comp->GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("ApplySpriteConfig: Skipping properties for %s (component is fading out)"), *Comp->GetName());
    }
}

void AVNCharacter::ApplySpriteConfig(E_VN_ComponentID_Sprite ID, const F_VN_SpriteConfig_Simple& Config, bool bAnimate)
{
    UPaperSpriteComponent* Comp = GetSpriteComponent(ID);
    if (!Comp) 
    {
        UE_LOG(LogTemp, Error, TEXT("ApplySpriteConfig: Simple component not found for ID %d"), (int32)ID);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("ApplySpriteConfig: Processing simple sprite component %s"), *Comp->GetName());

    // --- ПРОВЕРКА ИЗМЕНЕНИЯ АССЕТА ---
    const UPaperSprite* CurrentSprite = Comp->GetSprite();
    bool bAssetChanged = false;
    
    if (!CurrentSprite && !Config.Sprite.IsNull())
    {
        bAssetChanged = true;
        UE_LOG(LogTemp, Warning, TEXT("ApplySpriteConfig: Simple %s changed from NULL to %s"), 
            *Comp->GetName(), *Config.Sprite.ToString());
    }
    else if (CurrentSprite && Config.Sprite.IsNull())
    {
        bAssetChanged = true;
        UE_LOG(LogTemp, Warning, TEXT("ApplySpriteConfig: Simple %s changed from %s to NULL"), 
            *Comp->GetName(), *CurrentSprite->GetName());
    }
    else if (CurrentSprite && !Config.Sprite.IsNull())
    {
        bAssetChanged = (CurrentSprite->GetPathName() != Config.Sprite.ToString());
        if (bAssetChanged)
        {
            UE_LOG(LogTemp, Warning, TEXT("ApplySpriteConfig: Simple %s changed from %s to %s"), 
                *Comp->GetName(), *CurrentSprite->GetName(), *Config.Sprite.ToString());
        }
    }

    // --- ПРИМЕНЕНИЕ АССЕТА С УНИВЕРСАЛЬНОЙ СИСТЕМОЙ ПЕРЕХОДОВ ---
    if (bAnimate && bAssetChanged)
    {
        if (UPaperSpriteComponent* FadeComp = GetSpriteFadeComponent(ID))
        {
            UE_LOG(LogTemp, Warning, TEXT("ApplySpriteConfig: Preparing universal transition for simple %s"), *Comp->GetName());
            PrepareSpriteTransition(Comp, FadeComp, Config.Sprite);
            
            // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: НЕ применяем свойства для случая "Content → Empty"
            // Свойства применятся после анимации в FinalizeCurrentTransition
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("ApplySpriteConfig: Fade component not found for simple %s"), *Comp->GetName());
            ValidateAndSetupSpriteComponent(Comp, Config.Sprite);
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("ApplySpriteConfig: Applying simple %s instantly"), *Comp->GetName());
        ValidateAndSetupSpriteComponent(Comp, Config.Sprite);
    }

    // --- ПРИМЕНЕНИЕ ОСТАЛЬНЫХ СВОЙСТВ (ТОЛЬКО ЕСЛИ НЕ В ПРОЦЕССЕ АНИМАЦИИ) ---
    // Если компонент не исчезает, применяем свойства
    if (!FadingOutComponents.Contains(Comp))
    {
        UpdateComponentTransform(Comp, Config.Offset, Config.Scale);
        Comp->SetSpriteColor(Config.Color);
        
        // --- ВИДИМОСТЬ ---
        if (!Config.Sprite.IsNull())
        {
            Comp->SetVisibility(Config.bVisible);
            UE_LOG(LogTemp, Log, TEXT("ApplySpriteConfig: Set visibility for simple %s to %s"), 
                *Comp->GetName(), Config.bVisible ? TEXT("true") : TEXT("false"));
        }
        else if (!bAnimate || !bAssetChanged)
        {
            // Скрываем только если не идет анимация исчезновения
            Comp->SetVisibility(false);
            UE_LOG(LogTemp, Log, TEXT("ApplySpriteConfig: Hidden simple %s (null sprite)"), *Comp->GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("ApplySpriteConfig: Skipping properties for simple %s (component is fading out)"), *Comp->GetName());
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