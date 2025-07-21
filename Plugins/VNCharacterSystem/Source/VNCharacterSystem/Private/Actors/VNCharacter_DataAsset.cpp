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
        VN_LOG_WARNING(TEXT("ApplyDataAsset: CharacterData is null"));
        return;
    }

    VN_LOG_DEBUG(TEXT("ApplyDataAsset: Starting application of DataAsset %s (animate=%s, duration=%.2f)"), 
        *CharacterData->GetName(), bAnimate ? TEXT("true") : TEXT("false"), Duration);

    // --- ЛОГИКА ПРЕРЫВАНИЯ АНИМАЦИИ ---
    if (AnimationManager && AnimationManager->IsAnimating() && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::Transition)
    {
        VN_LOG_WARNING(TEXT("ApplyDataAsset: Interrupted an ongoing transition. Finalizing..."));
        // Эта команда остановит анимацию и вызовет OnAnimationFinished, который, в свою очередь,
        // вызовет FinalizeCurrentTransition(), очищая систему для нового перехода.
        AnimationManager->ClearAnimationQueue(); 
    }

    if (CharacterData->bOverrideGlobalTransforms)
    {
        VN_LOG_DEBUG(TEXT("ApplyDataAsset: Overriding global transforms"));
        GlobalSkeletalOffset = CharacterData->GlobalSkeletalOffset;
        GlobalSkeletalScale = CharacterData->GlobalSkeletalScale;
        GlobalSpriteOffset = CharacterData->GlobalSpriteOffset;
        GlobalSpriteScale = CharacterData->GlobalSpriteScale;
    }
    
    // Новая централизованная функция делает всё сама
    ApplyAllComponentConfigurationsFromDataAsset(CharacterData, bAnimate, Duration);

    VN_LOG_DEBUG(TEXT("ApplyDataAsset: Completed application of DataAsset %s"), *CharacterData->GetName());
}

void AVNCharacter::ApplyAllComponentConfigurationsFromDataAsset(const UVNCharacterDataAsset* CharacterData, bool bAnimate, float Duration)
{
    if (!CharacterData) return;

    VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurationsFromDataAsset: Starting two-stage application process"));

    // --- ЭТАП 1: ПОДГОТОВКА АНИМАЦИИ ТОЛЬКО ДЛЯ ИЗМЕНИВШИХСЯ КОМПОНЕНТОВ ---
    VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: STAGE 1 - Preparing transitions for changed components"));
    
    int32 TransitionsCount = 0;

    // Skeletal Meshes
    auto PrepareSkeletalTransitionIfNeeded = [&](E_VN_ComponentID_Skeletal ID, const TSoftObjectPtr<USkeletalMesh>& NewMesh, const FString& ComponentName) {
        if (auto* Comp = GetSkeletalComponent(ID)) {
            const USkeletalMesh* CurrentMesh = Comp->GetSkeletalMeshAsset();
            bool bAssetChanged = false;
            
            if (!CurrentMesh && !NewMesh.IsNull()) {
                bAssetChanged = true; // Был пустым, стал непустым
            } else if (CurrentMesh && NewMesh.IsNull()) {
                bAssetChanged = true; // Был непустым, стал пустым
            } else if (CurrentMesh && !NewMesh.IsNull()) {
                bAssetChanged = (CurrentMesh->GetPathName() != NewMesh.ToString()); // Сравниваем пути
            }
            
            if (bAnimate && bAssetChanged) {
                VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: Skeletal asset changed for %s: [%s] -> [%s]"), 
                    *ComponentName,
                    CurrentMesh ? *CurrentMesh->GetName() : TEXT("None"),
                    NewMesh.IsNull() ? TEXT("None") : *NewMesh.ToString());
                    
                if (auto* FadeComp = GetSkeletalFadeComponent(ID)) {
                    PrepareSkeletalTransition(Comp, FadeComp, NewMesh);
                    TransitionsCount++;
                } else {
                    VN_LOG_WARNING(TEXT("ApplyAllComponentConfigurations: No fade component found for skeletal %s"), *ComponentName);
                }
            } else {
                VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: Skeletal asset unchanged for %s"), *ComponentName);
            }
        }
    };
    
    PrepareSkeletalTransitionIfNeeded(E_VN_ComponentID_Skeletal::Body, CharacterData->BodyConfig.SkeletalMesh, TEXT("Body"));
    PrepareSkeletalTransitionIfNeeded(E_VN_ComponentID_Skeletal::Arms, CharacterData->ArmsConfig.SkeletalMesh, TEXT("Arms"));
    PrepareSkeletalTransitionIfNeeded(E_VN_ComponentID_Skeletal::Head, CharacterData->HeadConfig.SkeletalMesh, TEXT("Head"));
    PrepareSkeletalTransitionIfNeeded(E_VN_ComponentID_Skeletal::Custom01, CharacterData->Custom01Config.SkeletalMesh, TEXT("Custom01"));
    PrepareSkeletalTransitionIfNeeded(E_VN_ComponentID_Skeletal::Custom02, CharacterData->Custom02Config.SkeletalMesh, TEXT("Custom02"));
    PrepareSkeletalTransitionIfNeeded(E_VN_ComponentID_Skeletal::Custom03, CharacterData->Custom03Config.SkeletalMesh, TEXT("Custom03"));

    // Sprites
    auto PrepareSpriteTransitionIfNeeded = [&](E_VN_ComponentID_Sprite ID, const TSoftObjectPtr<UPaperSprite>& NewSprite, const FString& ComponentName) {
        if (auto* Comp = GetSpriteComponent(ID)) {
            const UPaperSprite* CurrentSprite = Comp->GetSprite();
            bool bAssetChanged = false;
            
            if (!CurrentSprite && !NewSprite.IsNull()) {
                bAssetChanged = true; // Был пустым, стал непустым
            } else if (CurrentSprite && NewSprite.IsNull()) {
                bAssetChanged = true; // Был непустым, стал пустым
            } else if (CurrentSprite && !NewSprite.IsNull()) {
                bAssetChanged = (CurrentSprite->GetPathName() != NewSprite.ToString()); // Сравниваем пути
            }
            
            if (bAnimate && bAssetChanged) {
                VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: Sprite asset changed for %s: [%s] -> [%s]"), 
                    *ComponentName,
                    CurrentSprite ? *CurrentSprite->GetName() : TEXT("None"),
                    NewSprite.IsNull() ? TEXT("None") : *NewSprite.ToString());
                    
                if (auto* FadeComp = GetSpriteFadeComponent(ID)) {
                    PrepareSpriteTransition(Comp, FadeComp, NewSprite);
                    TransitionsCount++;
                } else {
                    VN_LOG_WARNING(TEXT("ApplyAllComponentConfigurations: No fade component found for sprite %s"), *ComponentName);
                }
            } else {
                VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: Sprite asset unchanged for %s"), *ComponentName);
            }
        }
    };
    
    PrepareSpriteTransitionIfNeeded(E_VN_ComponentID_Sprite::Body, CharacterData->BodySpriteConfig.Sprite, TEXT("Body_Sprite"));
    PrepareSpriteTransitionIfNeeded(E_VN_ComponentID_Sprite::Arms, CharacterData->ArmsSpriteConfig.Sprite, TEXT("Arms_Sprite"));
    PrepareSpriteTransitionIfNeeded(E_VN_ComponentID_Sprite::BodyShadow, CharacterData->BodyShadowSpriteConfig.Sprite, TEXT("BodyShadow_Sprite"));
    PrepareSpriteTransitionIfNeeded(E_VN_ComponentID_Sprite::Head, CharacterData->HeadSpriteConfig.Sprite, TEXT("Head_Sprite"));
    PrepareSpriteTransitionIfNeeded(E_VN_ComponentID_Sprite::EmotionBody_01, CharacterData->EmotionBodyEffect01SpriteConfig.Sprite, TEXT("EmotionBody01"));
    PrepareSpriteTransitionIfNeeded(E_VN_ComponentID_Sprite::EmotionBody_02, CharacterData->EmotionBodyEffect02SpriteConfig.Sprite, TEXT("EmotionBody02"));
    PrepareSpriteTransitionIfNeeded(E_VN_ComponentID_Sprite::EmotionBody_03, CharacterData->EmotionBodyEffect03SpriteConfig.Sprite, TEXT("EmotionBody03"));
    PrepareSpriteTransitionIfNeeded(E_VN_ComponentID_Sprite::Eyebrow, CharacterData->EyebrowSpriteConfig.Sprite, TEXT("Eyebrow"));
    PrepareSpriteTransitionIfNeeded(E_VN_ComponentID_Sprite::Eyes, CharacterData->EyesSpriteConfig.Sprite, TEXT("Eyes"));
    PrepareSpriteTransitionIfNeeded(E_VN_ComponentID_Sprite::Eyelids, CharacterData->EyelidsSpriteConfig.Sprite, TEXT("Eyelids"));
    PrepareSpriteTransitionIfNeeded(E_VN_ComponentID_Sprite::Wink, CharacterData->WinkSpriteConfig.Sprite, TEXT("Wink"));
    PrepareSpriteTransitionIfNeeded(E_VN_ComponentID_Sprite::Mouth, CharacterData->MouthSpriteConfig.Sprite, TEXT("Mouth"));
    PrepareSpriteTransitionIfNeeded(E_VN_ComponentID_Sprite::EmotionHead_01, CharacterData->EmotionHeadEffect01SpriteConfig.Sprite, TEXT("EmotionHead01"));
    PrepareSpriteTransitionIfNeeded(E_VN_ComponentID_Sprite::EmotionHead_02, CharacterData->EmotionHeadEffect02SpriteConfig.Sprite, TEXT("EmotionHead02"));
    PrepareSpriteTransitionIfNeeded(E_VN_ComponentID_Sprite::EmotionHead_03, CharacterData->EmotionHeadEffect03SpriteConfig.Sprite, TEXT("EmotionHead03"));

    VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: STAGE 1 complete - Prepared %d transitions"), TransitionsCount);

    // --- ЭТАП 2: ПРИМЕНЕНИЕ ВСЕХ СВОЙСТВ ---
    VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: STAGE 2 - Applying properties to all components"));
    
    // Skeletal Meshes
    auto ApplySkeletalProperties = [&](E_VN_ComponentID_Skeletal ID, const F_VN_SkeletalConfig_Body& Config, const FString& ComponentName) {
        if (auto* Comp = GetSkeletalComponent(ID)) {
            VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: Applying skeletal properties to %s"), *ComponentName);
            
            // Применяем ассет только если он не в списке на анимацию
            if (!FadingInComponents.Contains(Comp)) {
                ValidateAndSetupSkeletalComponent(Comp, Config.SkeletalMesh);
            } else {
                VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: Skipping asset setup for %s (in animation)"), *ComponentName);
            }
            
            if (Config.AnimInstanceClass) Comp->SetAnimInstanceClass(Config.AnimInstanceClass);
            for (const auto& Elem : Config.MaterialOverrides) {
                if (!Elem.Value.IsNull()) Comp->SetMaterial(Elem.Key, Elem.Value.LoadSynchronous());
            }
            ResetComponentAttachmentToDefault(Comp);
            UpdateComponentTransform(Comp, Config.Offset, Config.Scale);
            SetComponentColor(Comp, Config.Color);
            if (!Config.SkeletalMesh.IsNull()) Comp->SetVisibility(Config.bVisible);
        }
    };

    auto ApplySkeletalPropertiesWithAttach = [&](E_VN_ComponentID_Skeletal ID, const F_VN_SkeletalConfig_Attachment& Config, const FString& ComponentName) {
        if (auto* Comp = GetSkeletalComponent(ID)) {
            VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: Applying skeletal attachment properties to %s"), *ComponentName);
            
            // Применяем ассет только если он не в списке на анимацию
            if (!FadingInComponents.Contains(Comp)) {
                ValidateAndSetupSkeletalComponent(Comp, Config.SkeletalMesh);
            } else {
                VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: Skipping asset setup for %s (in animation)"), *ComponentName);
            }
            
            if (Config.AnimInstanceClass) Comp->SetAnimInstanceClass(Config.AnimInstanceClass);
            for (const auto& Elem : Config.MaterialOverrides) {
                if (!Elem.Value.IsNull()) Comp->SetMaterial(Elem.Key, Elem.Value.LoadSynchronous());
            }
            
            if (Config.AttachTo != E_SkeletalAttachmentTarget::None) {
                if(USkeletalMeshComponent* AttachTarget = (Config.AttachTo == E_SkeletalAttachmentTarget::Body) ? Body_Skeletal : nullptr) {
                    Comp->AttachToComponent(AttachTarget, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Config.SocketName);
                }
            } else {
                ResetComponentAttachmentToDefault(Comp);
            }
            
            UpdateComponentTransform(Comp, Config.Offset, Config.Scale);
            SetComponentColor(Comp, Config.Color);
            if (!Config.SkeletalMesh.IsNull()) Comp->SetVisibility(Config.bVisible);
        }
    };
    
    ApplySkeletalProperties(E_VN_ComponentID_Skeletal::Body, CharacterData->BodyConfig, TEXT("Body"));
    ApplySkeletalPropertiesWithAttach(E_VN_ComponentID_Skeletal::Arms, CharacterData->ArmsConfig, TEXT("Arms"));
    ApplySkeletalPropertiesWithAttach(E_VN_ComponentID_Skeletal::Head, CharacterData->HeadConfig, TEXT("Head"));
    ApplySkeletalPropertiesWithAttach(E_VN_ComponentID_Skeletal::Custom01, CharacterData->Custom01Config, TEXT("Custom01"));
    ApplySkeletalPropertiesWithAttach(E_VN_ComponentID_Skeletal::Custom02, CharacterData->Custom02Config, TEXT("Custom02"));
    ApplySkeletalPropertiesWithAttach(E_VN_ComponentID_Skeletal::Custom03, CharacterData->Custom03Config, TEXT("Custom03"));

    // Sprites
    auto ApplySpritePropertiesWithAttach = [&](E_VN_ComponentID_Sprite ID, const F_VN_SpriteConfig_Attachment& Config, const FString& ComponentName) {
        if (auto* Comp = GetSpriteComponent(ID)) {
            VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: Applying sprite attachment properties to %s"), *ComponentName);
            
            // Применяем ассет только если он не в списке на анимацию
            if (!FadingInComponents.Contains(Comp)) {
                ValidateAndSetupSpriteComponent(Comp, Config.Sprite);
            } else {
                VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: Skipping asset setup for %s (in animation)"), *ComponentName);
            }
            
            if (Config.AttachTo != E_SpriteAttachmentTarget::None) {
                if(USkeletalMeshComponent* AttachTarget = GetSkeletalComponentBySpriteTarget(Config.AttachTo)) {
                    Comp->AttachToComponent(AttachTarget, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Config.SocketName);
                }
            } else {
                ResetComponentAttachmentToDefault(Comp);
            }
            
            UpdateComponentTransform(Comp, Config.Offset, Config.Scale);
            Comp->SetSpriteColor(Config.Color);
            if (!Config.Sprite.IsNull()) Comp->SetVisibility(Config.bVisible);
        }
    };

    auto ApplySpritePropertiesSimple = [&](E_VN_ComponentID_Sprite ID, const F_VN_SpriteConfig_Simple& Config, const FString& ComponentName) {
        if (auto* Comp = GetSpriteComponent(ID)) {
            VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: Applying sprite simple properties to %s"), *ComponentName);
            
            // Применяем ассет только если он не в списке на анимацию
            if (!FadingInComponents.Contains(Comp)) {
                ValidateAndSetupSpriteComponent(Comp, Config.Sprite);
            } else {
                VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: Skipping asset setup for %s (in animation)"), *ComponentName);
            }
            
            UpdateComponentTransform(Comp, Config.Offset, Config.Scale);
            Comp->SetSpriteColor(Config.Color);
            if (!Config.Sprite.IsNull()) Comp->SetVisibility(Config.bVisible);
        }
    };
    
    ApplySpritePropertiesWithAttach(E_VN_ComponentID_Sprite::Body, CharacterData->BodySpriteConfig, TEXT("Body_Sprite"));
    ApplySpritePropertiesWithAttach(E_VN_ComponentID_Sprite::Arms, CharacterData->ArmsSpriteConfig, TEXT("Arms_Sprite"));
    ApplySpritePropertiesWithAttach(E_VN_ComponentID_Sprite::BodyShadow, CharacterData->BodyShadowSpriteConfig, TEXT("BodyShadow_Sprite"));
    ApplySpritePropertiesWithAttach(E_VN_ComponentID_Sprite::Head, CharacterData->HeadSpriteConfig, TEXT("Head_Sprite"));
    ApplySpritePropertiesWithAttach(E_VN_ComponentID_Sprite::EmotionBody_01, CharacterData->EmotionBodyEffect01SpriteConfig, TEXT("EmotionBody01"));
    ApplySpritePropertiesWithAttach(E_VN_ComponentID_Sprite::EmotionBody_02, CharacterData->EmotionBodyEffect02SpriteConfig, TEXT("EmotionBody02"));
    ApplySpritePropertiesWithAttach(E_VN_ComponentID_Sprite::EmotionBody_03, CharacterData->EmotionBodyEffect03SpriteConfig, TEXT("EmotionBody03"));
    ApplySpritePropertiesSimple(E_VN_ComponentID_Sprite::Eyebrow, CharacterData->EyebrowSpriteConfig, TEXT("Eyebrow"));
    ApplySpritePropertiesSimple(E_VN_ComponentID_Sprite::Eyes, CharacterData->EyesSpriteConfig, TEXT("Eyes"));
    ApplySpritePropertiesSimple(E_VN_ComponentID_Sprite::Eyelids, CharacterData->EyelidsSpriteConfig, TEXT("Eyelids"));
    ApplySpritePropertiesSimple(E_VN_ComponentID_Sprite::Wink, CharacterData->WinkSpriteConfig, TEXT("Wink"));
    ApplySpritePropertiesSimple(E_VN_ComponentID_Sprite::Mouth, CharacterData->MouthSpriteConfig, TEXT("Mouth"));
    ApplySpritePropertiesSimple(E_VN_ComponentID_Sprite::EmotionHead_01, CharacterData->EmotionHeadEffect01SpriteConfig, TEXT("EmotionHead01"));
    ApplySpritePropertiesSimple(E_VN_ComponentID_Sprite::EmotionHead_02, CharacterData->EmotionHeadEffect02SpriteConfig, TEXT("EmotionHead02"));
    ApplySpritePropertiesSimple(E_VN_ComponentID_Sprite::EmotionHead_03, CharacterData->EmotionHeadEffect03SpriteConfig, TEXT("EmotionHead03"));

    VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: STAGE 2 complete - Applied properties to all components"));

    // --- ЭТАП 3: ЗАПУСК АНИМАЦИИ, ЕСЛИ БЫЛИ ИЗМЕНЕНИЯ ---
    VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: STAGE 3 - Finalizing animation"));
    
    if (BodyShadow_Sprite) BodyShadow_Sprite->SetVisibility(false);
    if (BodyShadow_Sprite_Fade) BodyShadow_Sprite_Fade->SetVisibility(false);

    if (bAnimate && Duration > 0.0f && AnimationManager && FadingInComponents.Num() > 0)
    {
        VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: Starting transition animation with %d components (duration=%.2f)"), 
            FadingInComponents.Num(), Duration);
        AnimationManager->PlayTransition(Duration);
    }
    else
    {
        if (FadingInComponents.Num() == 0)
        {
            VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: No components changed, skipping animation"));
        }
        else
        {
            VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: Animation disabled, applying changes instantly"));
        }
        
        // Если анимация не нужна, или ничего не изменилось, просто скрываем fade-компоненты
        HideAllFadeComponents();
    }

    VN_LOG_DEBUG(TEXT("ApplyAllComponentConfigurations: Two-stage application process complete"));
}

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