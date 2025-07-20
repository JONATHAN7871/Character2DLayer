/**
 * VNCharacter_DialogueSystem.cpp
 * 
 * Модуль для работы с диалоговой системой
 * Содержит методы для частичного изменения состояний персонажей
 * и обработки null значений через пресеты.
 */

#include "Actors/VNCharacter.h"
#include "VNCharacterSystemModule.h"

// =====================================================
// ОСНОВНЫЕ МЕТОДЫ ДЛЯ РАБОТЫ С ПРЕСЕТАМИ
// =====================================================

void AVNCharacter::ApplyPartialState(const F_VN_CharacterState& PartialState, float TransitionDuration)
{
    VN_LOG_DEBUG(TEXT("ApplyPartialState: Applying partial state '%s'"), *PartialState.StateID.ToString());

    // =============== ВАЖНОЕ ИСПРАВЛЕНИЕ ===============
    // НЕ валидируем частичное состояние как полное!
    // Вместо этого проверяем только, что есть хотя бы одно изменение
    if (!CanApplyPartialState(PartialState))
    {
        VN_LOG_WARNING(TEXT("ApplyPartialState: No valid changes found in partial state '%s'"), *PartialState.StateID.ToString());
        return;
    }

    // Создаем новое состояние на основе текущего
    F_VN_CharacterState NewState = CurrentState;
    
    // Обновляем только заполненные поля
    if (IsSkeletalBodyConfigFilled(PartialState.BodyConfig))
    {
        NewState.BodyConfig = PartialState.BodyConfig;
        VN_LOG_DEBUG(TEXT("ApplyPartialState: Updated BodyConfig"));
    }
    
    if (IsSkeletalAttachmentConfigFilled(PartialState.ArmsConfig))
    {
        NewState.ArmsConfig = PartialState.ArmsConfig;
        VN_LOG_DEBUG(TEXT("ApplyPartialState: Updated ArmsConfig"));
    }
    
    if (IsSkeletalAttachmentConfigFilled(PartialState.HeadConfig))
    {
        NewState.HeadConfig = PartialState.HeadConfig;
    }
    
    if (IsSkeletalAttachmentConfigFilled(PartialState.Custom01Config))
    {
        NewState.Custom01Config = PartialState.Custom01Config;
    }
    
    if (IsSkeletalAttachmentConfigFilled(PartialState.Custom02Config))
    {
        NewState.Custom02Config = PartialState.Custom02Config;
    }
    
    if (IsSkeletalAttachmentConfigFilled(PartialState.Custom03Config))
    {
        NewState.Custom03Config = PartialState.Custom03Config;
    }
    
    // Обновляем Sprite конфигурации
    if (IsSpriteAttachmentConfigFilled(PartialState.BodySpriteConfig))
    {
        NewState.BodySpriteConfig = PartialState.BodySpriteConfig;
    }
    
    if (IsSpriteAttachmentConfigFilled(PartialState.ArmsSpriteConfig))
    {
        NewState.ArmsSpriteConfig = PartialState.ArmsSpriteConfig;
    }
    
    if (IsSpriteAttachmentConfigFilled(PartialState.HeadSpriteConfig))
    {
        NewState.HeadSpriteConfig = PartialState.HeadSpriteConfig;
    }
    
    if (IsSpriteSimpleConfigFilled(PartialState.EyesSpriteConfig))
    {
        NewState.EyesSpriteConfig = PartialState.EyesSpriteConfig;
        VN_LOG_DEBUG(TEXT("ApplyPartialState: Updated EyesSpriteConfig"));
    }
    
    if (IsSpriteSimpleConfigFilled(PartialState.MouthSpriteConfig))
    {
        NewState.MouthSpriteConfig = PartialState.MouthSpriteConfig;
        VN_LOG_DEBUG(TEXT("ApplyPartialState: Updated MouthSpriteConfig"));
    }
    
    if (IsSpriteSimpleConfigFilled(PartialState.EyebrowSpriteConfig))
    {
        NewState.EyebrowSpriteConfig = PartialState.EyebrowSpriteConfig;
    }
    
    if (IsSpriteSimpleConfigFilled(PartialState.EyelidsSpriteConfig))
    {
        NewState.EyelidsSpriteConfig = PartialState.EyelidsSpriteConfig;
    }
    
    if (IsSpriteSimpleConfigFilled(PartialState.WinkSpriteConfig))
    {
        NewState.WinkSpriteConfig = PartialState.WinkSpriteConfig;
    }
    
    // Обновляем эмоциональные эффекты
    if (IsSpriteSimpleConfigFilled(PartialState.EmotionHeadEffect01SpriteConfig))
    {
        NewState.EmotionHeadEffect01SpriteConfig = PartialState.EmotionHeadEffect01SpriteConfig;
    }
    
    if (IsSpriteSimpleConfigFilled(PartialState.EmotionHeadEffect02SpriteConfig))
    {
        NewState.EmotionHeadEffect02SpriteConfig = PartialState.EmotionHeadEffect02SpriteConfig;
    }
    
    if (IsSpriteSimpleConfigFilled(PartialState.EmotionHeadEffect03SpriteConfig))
    {
        NewState.EmotionHeadEffect03SpriteConfig = PartialState.EmotionHeadEffect03SpriteConfig;
    }
    
    if (IsSpriteAttachmentConfigFilled(PartialState.EmotionBodyEffect01SpriteConfig))
    {
        NewState.EmotionBodyEffect01SpriteConfig = PartialState.EmotionBodyEffect01SpriteConfig;
    }
    
    if (IsSpriteAttachmentConfigFilled(PartialState.EmotionBodyEffect02SpriteConfig))
    {
        NewState.EmotionBodyEffect02SpriteConfig = PartialState.EmotionBodyEffect02SpriteConfig;
    }
    
    if (IsSpriteAttachmentConfigFilled(PartialState.EmotionBodyEffect03SpriteConfig))
    {
        NewState.EmotionBodyEffect03SpriteConfig = PartialState.EmotionBodyEffect03SpriteConfig;
    }
    
    if (IsSpriteAttachmentConfigFilled(PartialState.BodyShadowSpriteConfig))
    {
        NewState.BodyShadowSpriteConfig = PartialState.BodyShadowSpriteConfig;
    }
    
    // Обновляем StateID если указан
    if (!PartialState.StateID.IsNone())
    {
        NewState.StateID = PartialState.StateID;
    }
    
    // Обновляем Description если указан
    if (!PartialState.Description.IsEmpty())
    {
        NewState.Description = PartialState.Description;
    }
    
    // =============== ВАЖНОЕ ИСПРАВЛЕНИЕ ===============
    // Применяем состояние БЕЗ валидации, так как мы уже проверили изменения
    // и это смешанное состояние (старое + новое)
    SetCharacterStateInternal(NewState, TransitionDuration, false); // false = без валидации
}

void AVNCharacter::SetMainPosePreset(const F_VN_CharacterState& NewPosePreset)
{
    VN_LOG_DEBUG(TEXT("SetMainPosePreset: Setting new pose preset '%s'"), *NewPosePreset.StateID.ToString());
    MainPosePreset = NewPosePreset;
}

void AVNCharacter::ReturnToMainPose(float TransitionDuration)
{
    VN_LOG_DEBUG(TEXT("ReturnToMainPose: Returning to main pose"));
    SetCharacterState(MainPosePreset, TransitionDuration);
}

// =====================================================
// МЕТОДЫ ДЛЯ ДИАЛОГОВОЙ СИСТЕМЫ
// =====================================================

void AVNCharacter::SetEyes(TSoftObjectPtr<UPaperSprite> EyesSprite, bool bAnimate, float Duration)
{
    VN_LOG_DEBUG(TEXT("SetEyes: Setting eyes sprite"));
    
    F_VN_CharacterState PartialState;
    PartialState.StateID = TEXT("EyesChange");
    PartialState.EyesSpriteConfig = GetConfigFromPresetOrHidden_Eyes(EyesSprite);
    
    ApplyPartialState(PartialState, bAnimate ? Duration : 0.0f);
}

void AVNCharacter::SetMouth(TSoftObjectPtr<UPaperSprite> MouthSprite, bool bAnimate, float Duration)
{
    VN_LOG_DEBUG(TEXT("SetMouth: Setting mouth sprite"));
    
    F_VN_CharacterState PartialState;
    PartialState.StateID = TEXT("MouthChange");
    PartialState.MouthSpriteConfig = GetConfigFromPresetOrHidden_Mouth(MouthSprite);
    
    ApplyPartialState(PartialState, bAnimate ? Duration : 0.0f);
}

void AVNCharacter::SetEyebrows(TSoftObjectPtr<UPaperSprite> EyebrowSprite, bool bAnimate, float Duration)
{
    VN_LOG_DEBUG(TEXT("SetEyebrows: Setting eyebrow sprite"));
    
    F_VN_CharacterState PartialState;
    PartialState.StateID = TEXT("EyebrowChange");
    PartialState.EyebrowSpriteConfig = GetConfigFromPresetOrHidden_Eyebrows(EyebrowSprite);
    
    ApplyPartialState(PartialState, bAnimate ? Duration : 0.0f);
}

void AVNCharacter::SetBody(TSoftObjectPtr<USkeletalMesh> BodyMesh, bool bAnimate, float Duration)
{
    VN_LOG_DEBUG(TEXT("SetBody: Setting body mesh"));
    
    F_VN_CharacterState PartialState;
    PartialState.StateID = TEXT("BodyChange");
    PartialState.BodyConfig = GetConfigFromPresetOrHidden_Body(BodyMesh);
    
    ApplyPartialState(PartialState, bAnimate ? Duration : 0.0f);
}

void AVNCharacter::SetArms(TSoftObjectPtr<USkeletalMesh> ArmsMesh, bool bAnimate, float Duration)
{
    VN_LOG_DEBUG(TEXT("SetArms: Setting arms mesh"));
    
    F_VN_CharacterState PartialState;
    PartialState.StateID = TEXT("ArmsChange");
    PartialState.ArmsConfig = GetConfigFromPresetOrHidden_Arms(ArmsMesh);
    
    ApplyPartialState(PartialState, bAnimate ? Duration : 0.0f);
}

void AVNCharacter::SetFace(TSoftObjectPtr<UPaperSprite> EyesSprite, TSoftObjectPtr<UPaperSprite> MouthSprite, TSoftObjectPtr<UPaperSprite> EyebrowSprite, bool bAnimate, float Duration)
{
    VN_LOG_DEBUG(TEXT("SetFace: Setting face components"));
    
    F_VN_CharacterState PartialState;
    PartialState.StateID = TEXT("FaceChange");
    
    PartialState.EyesSpriteConfig = GetConfigFromPresetOrHidden_Eyes(EyesSprite);
    PartialState.MouthSpriteConfig = GetConfigFromPresetOrHidden_Mouth(MouthSprite);
    PartialState.EyebrowSpriteConfig = GetConfigFromPresetOrHidden_Eyebrows(EyebrowSprite);
    
    ApplyPartialState(PartialState, bAnimate ? Duration : 0.0f);
}

// =====================================================
// МЕТОДЫ ДЛЯ ОБРАБОТКИ NULL ЗНАЧЕНИЙ
// =====================================================

F_VN_SpriteConfig_Simple AVNCharacter::GetConfigFromPresetOrHidden_Eyes(TSoftObjectPtr<UPaperSprite> Sprite) const
{
    F_VN_SpriteConfig_Simple Config;
    
    if (!Sprite.IsNull())
    {
        // Используем переданный спрайт
        Config.Sprite = Sprite;
        Config.bVisible = true;
        Config.Color = FLinearColor::White;
        Config.Offset = FVector::ZeroVector;
        Config.Scale = 1.0f;
    }
    else
    {
        // Sprite is null - проверяем пресет
        if (!MainPosePreset.EyesSpriteConfig.Sprite.IsNull())
        {
            // Возвращаемся к пресету
            Config = MainPosePreset.EyesSpriteConfig;
            VN_LOG_DEBUG(TEXT("GetConfigFromPresetOrHidden_Eyes: Using preset sprite"));
        }
        else
        {
            // И в пресете пусто - скрываем
            Config.Sprite = nullptr;
            Config.bVisible = false;
            VN_LOG_DEBUG(TEXT("GetConfigFromPresetOrHidden_Eyes: Hiding component"));
        }
    }
    
    return Config;
}

F_VN_SpriteConfig_Simple AVNCharacter::GetConfigFromPresetOrHidden_Mouth(TSoftObjectPtr<UPaperSprite> Sprite) const
{
    F_VN_SpriteConfig_Simple Config;
    
    if (!Sprite.IsNull())
    {
        Config.Sprite = Sprite;
        Config.bVisible = true;
        Config.Color = FLinearColor::White;
        Config.Offset = FVector::ZeroVector;
        Config.Scale = 1.0f;
    }
    else
    {
        if (!MainPosePreset.MouthSpriteConfig.Sprite.IsNull())
        {
            Config = MainPosePreset.MouthSpriteConfig;
            VN_LOG_DEBUG(TEXT("GetConfigFromPresetOrHidden_Mouth: Using preset sprite"));
        }
        else
        {
            Config.Sprite = nullptr;
            Config.bVisible = false;
            VN_LOG_DEBUG(TEXT("GetConfigFromPresetOrHidden_Mouth: Hiding component"));
        }
    }
    
    return Config;
}

F_VN_SpriteConfig_Simple AVNCharacter::GetConfigFromPresetOrHidden_Eyebrows(TSoftObjectPtr<UPaperSprite> Sprite) const
{
    F_VN_SpriteConfig_Simple Config;
    
    if (!Sprite.IsNull())
    {
        Config.Sprite = Sprite;
        Config.bVisible = true;
        Config.Color = FLinearColor::White;
        Config.Offset = FVector::ZeroVector;
        Config.Scale = 1.0f;
    }
    else
    {
        if (!MainPosePreset.EyebrowSpriteConfig.Sprite.IsNull())
        {
            Config = MainPosePreset.EyebrowSpriteConfig;
            VN_LOG_DEBUG(TEXT("GetConfigFromPresetOrHidden_Eyebrows: Using preset sprite"));
        }
        else
        {
            Config.Sprite = nullptr;
            Config.bVisible = false;
            VN_LOG_DEBUG(TEXT("GetConfigFromPresetOrHidden_Eyebrows: Hiding component"));
        }
    }
    
    return Config;
}

F_VN_SkeletalConfig_Body AVNCharacter::GetConfigFromPresetOrHidden_Body(TSoftObjectPtr<USkeletalMesh> Mesh) const
{
    F_VN_SkeletalConfig_Body Config;
    
    if (!Mesh.IsNull())
    {
        Config.SkeletalMesh = Mesh;
        Config.bVisible = true;
        Config.Color = FLinearColor::White;
        Config.Offset = FVector::ZeroVector;
        Config.Scale = 1.0f;
    }
    else
    {
        if (!MainPosePreset.BodyConfig.SkeletalMesh.IsNull())
        {
            Config = MainPosePreset.BodyConfig;
            VN_LOG_DEBUG(TEXT("GetConfigFromPresetOrHidden_Body: Using preset mesh"));
        }
        else
        {
            Config.SkeletalMesh = nullptr;
            Config.bVisible = false;
            VN_LOG_DEBUG(TEXT("GetConfigFromPresetOrHidden_Body: Hiding component"));
        }
    }
    
    return Config;
}

F_VN_SkeletalConfig_Attachment AVNCharacter::GetConfigFromPresetOrHidden_Arms(TSoftObjectPtr<USkeletalMesh> Mesh) const
{
    F_VN_SkeletalConfig_Attachment Config;
    
    if (!Mesh.IsNull())
    {
        Config.SkeletalMesh = Mesh;
        Config.bVisible = true;
        Config.AttachTo = E_SkeletalAttachmentTarget::Body;
        Config.Color = FLinearColor::White;
        Config.Offset = FVector::ZeroVector;
        Config.Scale = 1.0f;
    }
    else
    {
        if (!MainPosePreset.ArmsConfig.SkeletalMesh.IsNull())
        {
            Config = MainPosePreset.ArmsConfig;
            VN_LOG_DEBUG(TEXT("GetConfigFromPresetOrHidden_Arms: Using preset mesh"));
        }
        else
        {
            Config.SkeletalMesh = nullptr;
            Config.bVisible = false;
            VN_LOG_DEBUG(TEXT("GetConfigFromPresetOrHidden_Arms: Hiding component"));
        }
    }
    
    return Config;
}

// =====================================================
// МЕТОДЫ ПРОВЕРКИ ЗАПОЛНЕННОСТИ КОНФИГУРАЦИЙ
// =====================================================

bool AVNCharacter::IsSkeletalBodyConfigFilled(const F_VN_SkeletalConfig_Body& Config) const
{
    return !Config.SkeletalMesh.IsNull() || !Config.bVisible;
}

bool AVNCharacter::IsSkeletalAttachmentConfigFilled(const F_VN_SkeletalConfig_Attachment& Config) const
{
    return !Config.SkeletalMesh.IsNull() || !Config.bVisible;
}

bool AVNCharacter::IsSpriteSimpleConfigFilled(const F_VN_SpriteConfig_Simple& Config) const
{
    return !Config.Sprite.IsNull() || !Config.bVisible;
}

bool AVNCharacter::IsSpriteAttachmentConfigFilled(const F_VN_SpriteConfig_Attachment& Config) const
{
    return !Config.Sprite.IsNull() || !Config.bVisible;
}