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
        VN_LOG_ERROR(TEXT("ApplyDataAsset FAILED: Received NULL CharacterDataAsset!"));
        return;
    }

    VN_LOG_DEBUG(TEXT("ApplyDataAsset: Starting application of DataAsset %s"), *CharacterData->GetName());

    // --- ШАГ 1: ПОДГОТОВКА И СБРОС ---
    if (AnimationManager && AnimationManager->IsAnimating() && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::Transition)
    {
        VN_LOG_WARNING(TEXT("ApplyDataAsset: Forcing completion of ongoing transition."));
        AnimationManager->ClearAnimationQueue();
        FinalizeCurrentTransition();
    }
    
    StopAndResetIdleAnimations();
    FadingInComponents.Empty();
    FadingOutComponents.Empty();
    
    // --- ШАГ 2: ПРИМЕНЕНИЕ ГЛОБАЛЬНЫХ ТРАНСФОРМАЦИЙ ---
    if (CharacterData->bOverrideGlobalTransforms)
    {
        VN_LOG_DEBUG(TEXT("ApplyDataAsset: Overriding global transforms."));
        GlobalSkeletalOffset = CharacterData->GlobalSkeletalOffset;
        GlobalSkeletalScale = CharacterData->GlobalSkeletalScale;
        GlobalSpriteOffset = CharacterData->GlobalSpriteOffset;
        GlobalSpriteScale = CharacterData->GlobalSpriteScale;
    }
    
    // --- ШАГ 3: ОБРАБОТКА АССЕТОВ ---
    ProcessSkeletalComponentChange(E_VN_ComponentID_Skeletal::Body, CharacterData->BodyConfig.SkeletalMesh, bAnimate);
    ProcessSkeletalComponentChange(E_VN_ComponentID_Skeletal::Arms, CharacterData->ArmsConfig.SkeletalMesh, bAnimate);
    ProcessSkeletalComponentChange(E_VN_ComponentID_Skeletal::Head, CharacterData->HeadConfig.SkeletalMesh, bAnimate);
    ProcessSkeletalComponentChange(E_VN_ComponentID_Skeletal::Custom01, CharacterData->Custom01Config.SkeletalMesh, bAnimate);
    ProcessSkeletalComponentChange(E_VN_ComponentID_Skeletal::Custom02, CharacterData->Custom02Config.SkeletalMesh, bAnimate);
    ProcessSkeletalComponentChange(E_VN_ComponentID_Skeletal::Custom03, CharacterData->Custom03Config.SkeletalMesh, bAnimate);

    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::Body, CharacterData->BodySpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::Arms, CharacterData->ArmsSpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::Head, CharacterData->HeadSpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::Eyebrow, CharacterData->EyebrowSpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::Eyes, CharacterData->EyesSpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::Eyelids, CharacterData->EyelidsSpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::Wink, CharacterData->WinkSpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::Mouth, CharacterData->MouthSpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::EmotionBody_01, CharacterData->EmotionBodyEffect01SpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::EmotionBody_02, CharacterData->EmotionBodyEffect02SpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::EmotionBody_03, CharacterData->EmotionBodyEffect03SpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::EmotionHead_01, CharacterData->EmotionHeadEffect01SpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::EmotionHead_02, CharacterData->EmotionHeadEffect02SpriteConfig.Sprite, bAnimate);
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::EmotionHead_03, CharacterData->EmotionHeadEffect03SpriteConfig.Sprite, bAnimate);
    
    // --- КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: ОБРАБОТКА ТЕНИ С ТРАНСФОРМАЦИЯМИ ---
    // Shadow теперь обрабатывается автоматически через ProcessSpriteComponentChange
    ProcessSpriteComponentChange(E_VN_ComponentID_Sprite::BodyShadow, CharacterData->BodyShadowSpriteConfig.Sprite, bAnimate);
    
    // Применяем свойства Shadow (трансформации, цвет и т.д.)
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::BodyShadow, CharacterData->BodyShadowSpriteConfig);
    
    // Shadow остаётся скрытым - его видимость управляется только анимациями Appear/Disappear
    if (BodyShadow_Sprite)
    {
        BodyShadow_Sprite->SetVisibility(false);
        VN_LOG_DEBUG(TEXT("ApplyDataAsset: Shadow configured from DataAsset - Offset: %s, Scale: %.2f"), 
            *CharacterData->BodyShadowSpriteConfig.Offset.ToString(), CharacterData->BodyShadowSpriteConfig.Scale);
    }
    
    // --- ШАГ 4: ПРИМЕНЕНИЕ ОСТАЛЬНЫХ СВОЙСТВ ---
    ApplySkeletalConfigProperties(E_VN_ComponentID_Skeletal::Body, CharacterData->BodyConfig);
    ApplySkeletalConfigProperties(E_VN_ComponentID_Skeletal::Arms, CharacterData->ArmsConfig);
    ApplySkeletalConfigProperties(E_VN_ComponentID_Skeletal::Head, CharacterData->HeadConfig);
    ApplySkeletalConfigProperties(E_VN_ComponentID_Skeletal::Custom01, CharacterData->Custom01Config);
    ApplySkeletalConfigProperties(E_VN_ComponentID_Skeletal::Custom02, CharacterData->Custom02Config);
    ApplySkeletalConfigProperties(E_VN_ComponentID_Skeletal::Custom03, CharacterData->Custom03Config);
    
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::Body, CharacterData->BodySpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::Arms, CharacterData->ArmsSpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::Head, CharacterData->HeadSpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::Eyebrow, CharacterData->EyebrowSpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::Eyes, CharacterData->EyesSpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::Eyelids, CharacterData->EyelidsSpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::Wink, CharacterData->WinkSpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::Mouth, CharacterData->MouthSpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::EmotionBody_01, CharacterData->EmotionBodyEffect01SpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::EmotionBody_02, CharacterData->EmotionBodyEffect02SpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::EmotionBody_03, CharacterData->EmotionBodyEffect03SpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::EmotionHead_01, CharacterData->EmotionHeadEffect01SpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::EmotionHead_02, CharacterData->EmotionHeadEffect02SpriteConfig);
    ApplySpriteConfigProperties(E_VN_ComponentID_Sprite::EmotionHead_03, CharacterData->EmotionHeadEffect03SpriteConfig);

    // --- ШАГ 5: ЗАПУСК АНИМАЦИИ И ПЕРЕЗАПУСК IDLE ---
    bool bHasAnimatedChanges = bAnimate && Duration > 0.0f && AnimationManager && (FadingInComponents.Num() > 0 || FadingOutComponents.Num() > 0);
    if (bHasAnimatedChanges)
    {
        VN_LOG_DEBUG(TEXT("ApplyDataAsset: Requesting batched transition commit."));
        RequestTransitionCommit(Duration);
    }
    else
    {
        VN_LOG_DEBUG(TEXT("ApplyDataAsset: No animation needed. Finalizing state immediately."));
        HideAllFadeComponents();
        if (IdleAnimationManager)
        {
            IdleAnimationManager->StartAllIdleAnimations();
        }
    }

    VN_LOG_DEBUG(TEXT("ApplyDataAsset: Completed with proper Shadow transform handling."));
}

// =====================================================
// ОБНОВЛЕННЫЕ HELPER ФУНКЦИИ ДЛЯ ОБРАБОТКИ ИЗМЕНЕНИЙ
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
    // СПЕЦИАЛЬНАЯ ОБРАБОТКА ДЛЯ SHADOW - он не участвует в анимациях переходов
    if (ID == E_VN_ComponentID_Sprite::BodyShadow)
    {
        if (BodyShadow_Sprite)
        {
            // Просто устанавливаем спрайт без анимации - Shadow управляется только через Appear/Disappear
            ValidateAndSetupSpriteComponent(BodyShadow_Sprite, NewSprite);
            VN_LOG_DEBUG(TEXT("ProcessSpriteComponentChange: Shadow sprite updated directly (no transition animation)"));
        }
        return;
    }

    // Обычная обработка для всех остальных компонентов
    UPaperSpriteComponent* MainComp = GetSpriteComponent(ID);
    if (!MainComp) return;
    
    // Обновляем кэш, объявляя этот спрайт новым "базовым"
    UpdateCacheForComponent(ID, NewSprite);
    
    const UPaperSprite* CurrentSprite = MainComp->GetSprite();
    bool bAssetChanged = false;
    
    if ((!CurrentSprite && !NewSprite.IsNull()) || (CurrentSprite && NewSprite.IsNull()) || (CurrentSprite && !NewSprite.IsNull() && CurrentSprite->GetPathName() != NewSprite.ToString()))
    {
        bAssetChanged = true;
    }
    
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
        if (!MaterialOverride.Value.IsNull()) Comp->SetMaterial(MaterialOverride.Key, MaterialOverride.Value.LoadSynchronous());
    }
    
    UpdateComponentTransform(Comp, Config.Offset, Config.Scale);
    
    // === ИСПРАВЛЕНИЕ: КЭШИРУЕМ БАЗОВЫЙ ЦВЕТ ИЗ КОНФИГА ===
    CacheComponentBaseColor(Comp, Config.Color);
    // Применяем цвет с учетом фокуса
    ApplyComponentColorWithFocus(Comp);
    
    if (!Config.SkeletalMesh.IsNull()) Comp->SetVisibility(Config.bVisible);
}

void AVNCharacter::ApplySpriteConfigProperties(E_VN_ComponentID_Sprite ID, const F_VN_SpriteConfig_Attachment& Config)
{
    UPaperSpriteComponent* Comp = GetSpriteComponent(ID);
    if (!Comp) return;

    // Обрабатываем прикрепление
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
    
    // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: Применяем трансформации для ВСЕХ компонентов, включая Shadow
    UpdateComponentTransform(Comp, Config.Offset, Config.Scale);
    
    // Кэшируем и применяем цвет
    CacheComponentBaseColor(Comp, Config.Color);
    ApplyComponentColorWithFocus(Comp);
    
    // Специальная логика видимости для Shadow
    if (ID != E_VN_ComponentID_Sprite::BodyShadow)
    {
        Comp->SetVisibility(!Config.Sprite.IsNull());
    }
    
    VN_LOG_DEBUG(TEXT("ApplySpriteConfigProperties (Attachment): %s - Offset: %s, Scale: %.2f"), 
        *Comp->GetName(), *Config.Offset.ToString(), Config.Scale);
}

void AVNCharacter::ApplyShadowTransform(const FVector& Offset, float Scale)
{
    if (!BodyShadow_Sprite)
    {
        VN_LOG_WARNING(TEXT("ApplyShadowTransform: BodyShadow_Sprite is null"));
        return;
    }

    // Применяем трансформации с учётом глобальных настроек
    UpdateComponentTransform(BodyShadow_Sprite, Offset, Scale);
    
    VN_LOG_DEBUG(TEXT("ApplyShadowTransform: Applied offset %s and scale %.2f to shadow"), 
        *Offset.ToString(), Scale);
}

void AVNCharacter::GetShadowTransform(FVector& OutOffset, float& OutScale) const
{
    if (!BodyShadow_Sprite)
    {
        OutOffset = FVector::ZeroVector;
        OutScale = 1.0f;
        return;
    }

    OutOffset = BodyShadow_Sprite->GetRelativeLocation();
    OutScale = BodyShadow_Sprite->GetRelativeScale3D().X; // Используем X компонент как uniform scale
}

void AVNCharacter::ApplySpriteConfigProperties(E_VN_ComponentID_Sprite ID, const F_VN_SpriteConfig_Simple& Config)
{
    UPaperSpriteComponent* Comp = GetSpriteComponent(ID);
    if (!Comp) return;
    
    // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: Применяем трансформации для ВСЕХ компонентов, включая Shadow
    UpdateComponentTransform(Comp, Config.Offset, Config.Scale);
    
    // Кэшируем и применяем цвет
    CacheComponentBaseColor(Comp, Config.Color);
    ApplyComponentColorWithFocus(Comp);
    
    // Специальная логика видимости для Shadow - он управляется отдельно
    if (ID != E_VN_ComponentID_Sprite::BodyShadow)
    {
        Comp->SetVisibility(!Config.Sprite.IsNull());
    }
    
    VN_LOG_DEBUG(TEXT("ApplySpriteConfigProperties (Simple): %s - Offset: %s, Scale: %.2f"), 
        *Comp->GetName(), *Config.Offset.ToString(), Config.Scale);
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

void AVNCharacter::ApplySkeletalConfigProperties(E_VN_ComponentID_Skeletal ID, const F_VN_SkeletalConfig_Attachment& Config)
{
    USkeletalMeshComponent* Comp = GetSkeletalComponent(ID);
    if (!Comp) return;
    
    if (Config.AnimInstanceClass) Comp->SetAnimInstanceClass(Config.AnimInstanceClass);
    for (const auto& MaterialOverride : Config.MaterialOverrides)
    {
        if (!MaterialOverride.Value.IsNull()) Comp->SetMaterial(MaterialOverride.Key, MaterialOverride.Value.LoadSynchronous());
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
    
    // === ИСПРАВЛЕНИЕ: КЭШИРУЕМ БАЗОВЫЙ ЦВЕТ ИЗ КОНФИГА ===
    CacheComponentBaseColor(Comp, Config.Color);
    // Применяем цвет с учетом фокуса
    ApplyComponentColorWithFocus(Comp);
    
    if (!Config.SkeletalMesh.IsNull()) Comp->SetVisibility(Config.bVisible);
}