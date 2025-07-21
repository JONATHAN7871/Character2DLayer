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

    if (CharacterData->bOverrideGlobalTransforms)
    {
        GlobalSkeletalOffset = CharacterData->GlobalSkeletalOffset;
        GlobalSkeletalScale = CharacterData->GlobalSkeletalScale;
        GlobalSpriteOffset = CharacterData->GlobalSpriteOffset;
        GlobalSpriteScale = CharacterData->GlobalSpriteScale;
    }
    
    ApplyAllComponentConfigurationsFromDataAsset(CharacterData, bAnimate);

    if (BodyShadow_Sprite) BodyShadow_Sprite->SetVisibility(false);
    if (BodyShadow_Sprite_Fade) BodyShadow_Sprite_Fade->SetVisibility(false);

    if (bAnimate && Duration > 0.0f && AnimationManager)
    {
        AnimationManager->PlayTransition(Duration);
    }
    else
    {
        HideAllFadeComponents();
    }
}

void AVNCharacter::ApplyAllComponentConfigurationsFromDataAsset(const UVNCharacterDataAsset* CharacterData, bool bAnimate)
{
    if (!CharacterData) return;

    // --- Skeletal Mesh Application ---
    
    // Handle Body separately as it has a different config struct (no attachment properties)
    if (auto* Comp = GetSkeletalComponent(E_VN_ComponentID_Skeletal::Body))
    {
        const auto& Config = CharacterData->BodyConfig;
        SetSkeletalMesh(E_VN_ComponentID_Skeletal::Body, Config.SkeletalMesh, bAnimate, 0.f);
        if (Config.AnimInstanceClass) Comp->SetAnimInstanceClass(Config.AnimInstanceClass);
        for (const auto& Elem : Config.MaterialOverrides) {
            if (!Elem.Value.IsNull()) Comp->SetMaterial(Elem.Key, Elem.Value.LoadSynchronous());
        }
        ResetComponentAttachmentToDefault(Comp);
        UpdateComponentTransform(Comp, Config.Offset, Config.Scale);
        SetComponentColor(Comp, Config.Color);
        if (!Config.SkeletalMesh.IsNull()) Comp->SetVisibility(Config.bVisible);
    }

    // Handle Attachments using a lambda
    auto ApplySkeletalAttachConfig = [&](E_VN_ComponentID_Skeletal ID, const F_VN_SkeletalConfig_Attachment& Config) {
        if (auto* Comp = GetSkeletalComponent(ID)) {
            SetSkeletalMesh(ID, Config.SkeletalMesh, bAnimate, 0.f);
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
    ApplySkeletalAttachConfig(E_VN_ComponentID_Skeletal::Arms, CharacterData->ArmsConfig);
    ApplySkeletalAttachConfig(E_VN_ComponentID_Skeletal::Head, CharacterData->HeadConfig);
    ApplySkeletalAttachConfig(E_VN_ComponentID_Skeletal::Custom01, CharacterData->Custom01Config);
    ApplySkeletalAttachConfig(E_VN_ComponentID_Skeletal::Custom02, CharacterData->Custom02Config);
    ApplySkeletalAttachConfig(E_VN_ComponentID_Skeletal::Custom03, CharacterData->Custom03Config);

    // --- Sprite (Attachment) Application ---
    auto ApplySpriteAttachConfig = [&](E_VN_ComponentID_Sprite ID, const F_VN_SpriteConfig_Attachment& Config) {
        if (auto* Comp = GetSpriteComponent(ID)) {
             SetSprite(ID, Config.Sprite, bAnimate, 0.f);
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
    ApplySpriteAttachConfig(E_VN_ComponentID_Sprite::Body, CharacterData->BodySpriteConfig);
    ApplySpriteAttachConfig(E_VN_ComponentID_Sprite::Arms, CharacterData->ArmsSpriteConfig);
    ApplySpriteAttachConfig(E_VN_ComponentID_Sprite::BodyShadow, CharacterData->BodyShadowSpriteConfig);
    ApplySpriteAttachConfig(E_VN_ComponentID_Sprite::Head, CharacterData->HeadSpriteConfig);
    ApplySpriteAttachConfig(E_VN_ComponentID_Sprite::EmotionBody_01, CharacterData->EmotionBodyEffect01SpriteConfig);
    ApplySpriteAttachConfig(E_VN_ComponentID_Sprite::EmotionBody_02, CharacterData->EmotionBodyEffect02SpriteConfig);
    ApplySpriteAttachConfig(E_VN_ComponentID_Sprite::EmotionBody_03, CharacterData->EmotionBodyEffect03SpriteConfig);
    
    // --- Sprite (Simple) Application ---
    auto ApplySpriteSimpleConfig = [&](E_VN_ComponentID_Sprite ID, const F_VN_SpriteConfig_Simple& Config) {
        if (auto* Comp = GetSpriteComponent(ID)) {
            SetSprite(ID, Config.Sprite, bAnimate, 0.f);
            UpdateComponentTransform(Comp, Config.Offset, Config.Scale);
            Comp->SetSpriteColor(Config.Color);
            if (!Config.Sprite.IsNull()) Comp->SetVisibility(Config.bVisible);
        }
    };
    ApplySpriteSimpleConfig(E_VN_ComponentID_Sprite::Eyebrow, CharacterData->EyebrowSpriteConfig);
    ApplySpriteSimpleConfig(E_VN_ComponentID_Sprite::Eyes, CharacterData->EyesSpriteConfig);
    ApplySpriteSimpleConfig(E_VN_ComponentID_Sprite::Eyelids, CharacterData->EyelidsSpriteConfig);
    ApplySpriteSimpleConfig(E_VN_ComponentID_Sprite::Wink, CharacterData->WinkSpriteConfig);
    ApplySpriteSimpleConfig(E_VN_ComponentID_Sprite::Mouth, CharacterData->MouthSpriteConfig);
    ApplySpriteSimpleConfig(E_VN_ComponentID_Sprite::EmotionHead_01, CharacterData->EmotionHeadEffect01SpriteConfig);
    ApplySpriteSimpleConfig(E_VN_ComponentID_Sprite::EmotionHead_02, CharacterData->EmotionHeadEffect02SpriteConfig);
    ApplySpriteSimpleConfig(E_VN_ComponentID_Sprite::EmotionHead_03, CharacterData->EmotionHeadEffect03SpriteConfig);
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