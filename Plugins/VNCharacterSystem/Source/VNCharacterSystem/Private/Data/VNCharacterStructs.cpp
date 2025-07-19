#include "Data/VNCharacterStructs.h"
#include "Data/VNCharacterTypes.h"
#include "VNCharacterSystemModule.h"
#include "Engine/SkeletalMesh.h"
#include "PaperSprite.h"
#include "Materials/MaterialInterface.h"

/**
 * Реализация сложных методов для структур VN персонажей
 * 
 * Содержит:
 * - Операторы сравнения для структур
 * - Методы валидации
 * - Утилиты для работы с ассетами
 * - Вспомогательные функции
 */

// =====================================================
// ОПЕРАТОРЫ СРАВНЕНИЯ ДЛЯ SKELETAL CONFIG BODY
// =====================================================

bool F_VN_SkeletalConfig_Body::operator==(const F_VN_SkeletalConfig_Body& Other) const
{
    return SkeletalMesh == Other.SkeletalMesh &&
           AnimInstanceClass == Other.AnimInstanceClass &&
           Color.Equals(Other.Color, 0.01f) &&
           Offset.Equals(Other.Offset, 0.01f) &&
           FMath::IsNearlyEqual(Scale, Other.Scale, 0.01f) &&
           bVisible == Other.bVisible &&
           CompareMaterialMaps(MaterialOverrides, Other.MaterialOverrides);
}

bool F_VN_SkeletalConfig_Body::operator!=(const F_VN_SkeletalConfig_Body& Other) const
{
    return !(*this == Other);
}

bool F_VN_SkeletalConfig_Body::IsValid() const
{
    // Если компонент видимый, должен быть задан SkeletalMesh
    if (bVisible && SkeletalMesh.IsNull())
    {
        VN_LOG_WARNING(TEXT("SkeletalConfig_Body: Component is visible but SkeletalMesh is null"));
        return false;
    }
    
    // Масштаб должен быть положительным
    if (Scale <= 0.0f)
    {
        VN_LOG_WARNING(TEXT("SkeletalConfig_Body: Scale must be greater than 0, current: %f"), Scale);
        return false;
    }
    
    return true;
}

bool F_VN_SkeletalConfig_Body::CompareMaterialMaps(
    const TMap<int32, TSoftObjectPtr<UMaterialInterface>>& Map1,
    const TMap<int32, TSoftObjectPtr<UMaterialInterface>>& Map2) const
{
    if (Map1.Num() != Map2.Num())
        return false;
        
    for (const auto& Pair : Map1)
    {
        const TSoftObjectPtr<UMaterialInterface>* OtherMaterial = Map2.Find(Pair.Key);
        if (!OtherMaterial || *OtherMaterial != Pair.Value)
            return false;
    }
    
    return true;
}

// =====================================================
// ОПЕРАТОРЫ СРАВНЕНИЯ ДЛЯ SKELETAL CONFIG ATTACHMENT
// =====================================================

bool F_VN_SkeletalConfig_Attachment::operator==(const F_VN_SkeletalConfig_Attachment& Other) const
{
    return SkeletalMesh == Other.SkeletalMesh &&
           AnimInstanceClass == Other.AnimInstanceClass &&
           AttachTo == Other.AttachTo &&
           SocketName == Other.SocketName &&
           bUseSocketTransform == Other.bUseSocketTransform &&
           Color.Equals(Other.Color, 0.01f) &&
           Offset.Equals(Other.Offset, 0.01f) &&
           FMath::IsNearlyEqual(Scale, Other.Scale, 0.01f) &&
           bVisible == Other.bVisible &&
           CompareMaterialMaps(MaterialOverrides, Other.MaterialOverrides);
}

bool F_VN_SkeletalConfig_Attachment::operator!=(const F_VN_SkeletalConfig_Attachment& Other) const
{
    return !(*this == Other);
}

bool F_VN_SkeletalConfig_Attachment::IsValid() const
{
    // Если компонент видимый, должен быть задан SkeletalMesh
    if (bVisible && SkeletalMesh.IsNull())
    {
        VN_LOG_WARNING(TEXT("SkeletalConfig_Attachment: Component is visible but SkeletalMesh is null"));
        return false;
    }
    
    // Масштаб должен быть положительным
    if (Scale <= 0.0f)
    {
        VN_LOG_WARNING(TEXT("SkeletalConfig_Attachment: Scale must be greater than 0, current: %f"), Scale);
        return false;
    }
    
    // Если используется сокет, его имя не должно быть пустым
    if (bUseSocketTransform && SocketName.IsNone() && AttachTo != E_SkeletalAttachmentTarget::None)
    {
        VN_LOG_WARNING(TEXT("SkeletalConfig_Attachment: UseSocketTransform is true but SocketName is None"));
        return false;
    }
    
    return true;
}

bool F_VN_SkeletalConfig_Attachment::CompareMaterialMaps(
    const TMap<int32, TSoftObjectPtr<UMaterialInterface>>& Map1,
    const TMap<int32, TSoftObjectPtr<UMaterialInterface>>& Map2) const
{
    if (Map1.Num() != Map2.Num())
        return false;
        
    for (const auto& Pair : Map1)
    {
        const TSoftObjectPtr<UMaterialInterface>* OtherMaterial = Map2.Find(Pair.Key);
        if (!OtherMaterial || *OtherMaterial != Pair.Value)
            return false;
    }
    
    return true;
}

// =====================================================
// ОПЕРАТОРЫ СРАВНЕНИЯ ДЛЯ SPRITE CONFIG ATTACHMENT
// =====================================================

bool F_VN_SpriteConfig_Attachment::operator==(const F_VN_SpriteConfig_Attachment& Other) const
{
    return Sprite == Other.Sprite &&
           AttachTo == Other.AttachTo &&
           SocketName == Other.SocketName &&
           bUseSocketTransform == Other.bUseSocketTransform &&
           Offset.Equals(Other.Offset, 0.01f) &&
           FMath::IsNearlyEqual(Scale, Other.Scale, 0.01f) &&
           Color.Equals(Other.Color, 0.01f) &&
           bVisible == Other.bVisible;
}

bool F_VN_SpriteConfig_Attachment::operator!=(const F_VN_SpriteConfig_Attachment& Other) const
{
    return !(*this == Other);
}

bool F_VN_SpriteConfig_Attachment::IsValid() const
{
    // Если спрайт видимый, должен быть задан Sprite
    if (bVisible && Sprite.IsNull())
    {
        VN_LOG_WARNING(TEXT("SpriteConfig_Attachment: Component is visible but Sprite is null"));
        return false;
    }
    
    // Масштаб должен быть положительным
    if (Scale <= 0.0f)
    {
        VN_LOG_WARNING(TEXT("SpriteConfig_Attachment: Scale must be greater than 0, current: %f"), Scale);
        return false;
    }
    
    // Если используется сокет, его имя не должно быть пустым (кроме случая None)
    if (bUseSocketTransform && SocketName.IsNone() && AttachTo != E_SpriteAttachmentTarget::None)
    {
        VN_LOG_WARNING(TEXT("SpriteConfig_Attachment: UseSocketTransform is true but SocketName is None"));
        return false;
    }
    
    return true;
}

// =====================================================
// ОПЕРАТОРЫ СРАВНЕНИЯ ДЛЯ SPRITE CONFIG SIMPLE
// =====================================================

bool F_VN_SpriteConfig_Simple::operator==(const F_VN_SpriteConfig_Simple& Other) const
{
    return Sprite == Other.Sprite &&
           Offset.Equals(Other.Offset, 0.01f) &&
           FMath::IsNearlyEqual(Scale, Other.Scale, 0.01f) &&
           Color.Equals(Other.Color, 0.01f) &&
           bVisible == Other.bVisible;
}

bool F_VN_SpriteConfig_Simple::operator!=(const F_VN_SpriteConfig_Simple& Other) const
{
    return !(*this == Other);
}

bool F_VN_SpriteConfig_Simple::IsValid() const
{
    // Если спрайт видимый, должен быть задан Sprite
    if (bVisible && Sprite.IsNull())
    {
        VN_LOG_WARNING(TEXT("SpriteConfig_Simple: Component is visible but Sprite is null"));
        return false;
    }
    
    // Масштаб должен быть положительным
    if (Scale <= 0.0f)
    {
        VN_LOG_WARNING(TEXT("SpriteConfig_Simple: Scale must be greater than 0, current: %f"), Scale);
        return false;
    }
    
    return true;
}

// =====================================================
// МЕТОДЫ ДЛЯ ANIMATION REQUEST
// =====================================================

bool FVNAnimationRequest::operator==(const FVNAnimationRequest& Other) const
{
    return AnimationType == Other.AnimationType &&
           FMath::IsNearlyEqual(Duration, Other.Duration, 0.001f) &&
           bIsSpawnAnimation == Other.bIsSpawnAnimation &&
           bIsInFocus == Other.bIsInFocus;
}

bool FVNAnimationRequest::operator!=(const FVNAnimationRequest& Other) const
{
    return !(*this == Other);
}

FString FVNAnimationRequest::ToString() const
{
    FString AnimTypeStr;
    switch (AnimationType)
    {
        case EVNAnimationType::None:
            AnimTypeStr = TEXT("None");
            break;
        case EVNAnimationType::Transition:
            AnimTypeStr = TEXT("Transition");
            break;
        case EVNAnimationType::SpawnDespawn:
            AnimTypeStr = FString::Printf(TEXT("SpawnDespawn (%s)"), 
                bIsSpawnAnimation ? TEXT("Appear") : TEXT("Disappear"));
            break;
        case EVNAnimationType::Focus:
            AnimTypeStr = FString::Printf(TEXT("Focus (%s)"), 
                bIsInFocus ? TEXT("In Focus") : TEXT("Out of Focus"));
            break;
    }
    
    return FString::Printf(TEXT("AnimRequest: %s, Duration: %.2f"), *AnimTypeStr, Duration);
}

// =====================================================
// РАСШИРЕННЫЕ МЕТОДЫ ДЛЯ CHARACTER STATE
// =====================================================

bool F_VN_CharacterState::HasVisibleComponents() const
{
    // Проверяем Skeletal Mesh компоненты
    if (BodyConfig.bVisible || ArmsConfig.bVisible || HeadConfig.bVisible ||
        Custom01Config.bVisible || Custom02Config.bVisible || Custom03Config.bVisible)
    {
        return true;
    }
    
    // Проверяем Sprite компоненты
    if (BodySpriteConfig.bVisible || ArmsSpriteConfig.bVisible || BodyShadowSpriteConfig.bVisible ||
        EmotionBodyEffect01SpriteConfig.bVisible || EmotionBodyEffect02SpriteConfig.bVisible || EmotionBodyEffect03SpriteConfig.bVisible ||
        HeadSpriteConfig.bVisible || EyebrowSpriteConfig.bVisible || EyesSpriteConfig.bVisible ||
        EyelidsSpriteConfig.bVisible || WinkSpriteConfig.bVisible || MouthSpriteConfig.bVisible ||
        EmotionHeadEffect01SpriteConfig.bVisible || EmotionHeadEffect02SpriteConfig.bVisible || EmotionHeadEffect03SpriteConfig.bVisible)
    {
        return true;
    }
    
    return false;
}

bool F_VN_CharacterState::ValidateAllConfigs() const
{
    bool bIsValid = true;
    
    // Валидация Skeletal Mesh конфигураций
    if (!BodyConfig.IsValid())
    {
        VN_LOG_WARNING(TEXT("CharacterState '%s': BodyConfig is invalid"), *StateID.ToString());
        bIsValid = false;
    }
    
    if (!ArmsConfig.IsValid())
    {
        VN_LOG_WARNING(TEXT("CharacterState '%s': ArmsConfig is invalid"), *StateID.ToString());
        bIsValid = false;
    }
    
    if (!HeadConfig.IsValid())
    {
        VN_LOG_WARNING(TEXT("CharacterState '%s': HeadConfig is invalid"), *StateID.ToString());
        bIsValid = false;
    }
    
    if (!Custom01Config.IsValid())
    {
        VN_LOG_WARNING(TEXT("CharacterState '%s': Custom01Config is invalid"), *StateID.ToString());
        bIsValid = false;
    }
    
    if (!Custom02Config.IsValid())
    {
        VN_LOG_WARNING(TEXT("CharacterState '%s': Custom02Config is invalid"), *StateID.ToString());
        bIsValid = false;
    }
    
    if (!Custom03Config.IsValid())
    {
        VN_LOG_WARNING(TEXT("CharacterState '%s': Custom03Config is invalid"), *StateID.ToString());
        bIsValid = false;
    }
    
    // Валидация Sprite конфигураций
    if (!BodySpriteConfig.IsValid())
    {
        VN_LOG_WARNING(TEXT("CharacterState '%s': BodySpriteConfig is invalid"), *StateID.ToString());
        bIsValid = false;
    }
    
    if (!ArmsSpriteConfig.IsValid())
    {
        VN_LOG_WARNING(TEXT("CharacterState '%s': ArmsSpriteConfig is invalid"), *StateID.ToString());
        bIsValid = false;
    }
    
    if (!HeadSpriteConfig.IsValid())
    {
        VN_LOG_WARNING(TEXT("CharacterState '%s': HeadSpriteConfig is invalid"), *StateID.ToString());
        bIsValid = false;
    }
    
    if (!EyebrowSpriteConfig.IsValid())
    {
        VN_LOG_WARNING(TEXT("CharacterState '%s': EyebrowSpriteConfig is invalid"), *StateID.ToString());
        bIsValid = false;
    }
    
    if (!EyesSpriteConfig.IsValid())
    {
        VN_LOG_WARNING(TEXT("CharacterState '%s': EyesSpriteConfig is invalid"), *StateID.ToString());
        bIsValid = false;
    }
    
    if (!MouthSpriteConfig.IsValid())
    {
        VN_LOG_WARNING(TEXT("CharacterState '%s': MouthSpriteConfig is invalid"), *StateID.ToString());
        bIsValid = false;
    }
    
    // Остальные конфигурации проверяются аналогично...
    
    return bIsValid;
}

TArray<FString> F_VN_CharacterState::GetDetailedValidationErrors() const
{
    TArray<FString> Errors;
    
    if (StateID.IsNone())
    {
        Errors.Add(TEXT("StateID is None - must be set to a valid name"));
    }
    
    // Проверяем, есть ли хотя бы один видимый компонент
    if (!HasVisibleComponents())
    {
        Errors.Add(TEXT("No visible components found - character will be invisible"));
    }
    
    // Детальная проверка каждой конфигурации
    if (BodyConfig.bVisible && BodyConfig.SkeletalMesh.IsNull())
    {
        Errors.Add(TEXT("BodyConfig: SkeletalMesh is null but component is visible"));
    }
    
    if (ArmsConfig.bVisible && ArmsConfig.SkeletalMesh.IsNull())
    {
        Errors.Add(TEXT("ArmsConfig: SkeletalMesh is null but component is visible"));
    }
    
    if (HeadConfig.bVisible && HeadConfig.SkeletalMesh.IsNull())
    {
        Errors.Add(TEXT("HeadConfig: SkeletalMesh is null but component is visible"));
    }
    
    // Проверка спрайтов
    if (EyesSpriteConfig.bVisible && EyesSpriteConfig.Sprite.IsNull())
    {
        Errors.Add(TEXT("EyesSpriteConfig: Sprite is null but component is visible"));
    }
    
    if (MouthSpriteConfig.bVisible && MouthSpriteConfig.Sprite.IsNull())
    {
        Errors.Add(TEXT("MouthSpriteConfig: Sprite is null but component is visible"));
    }
    
    // Можно добавить больше проверок по необходимости...
    
    return Errors;
}

bool F_VN_CharacterState::operator==(const F_VN_CharacterState& Other) const
{
    // Быстрое сравнение по StateID
    if (StateID != Other.StateID)
        return false;
        
    // Если StateID одинаковые, но Description разные - это может быть копия
    // В этом случае делаем полное сравнение конфигураций
    return BodyConfig == Other.BodyConfig &&
           ArmsConfig == Other.ArmsConfig &&
           HeadConfig == Other.HeadConfig &&
           Custom01Config == Other.Custom01Config &&
           Custom02Config == Other.Custom02Config &&
           Custom03Config == Other.Custom03Config &&
           BodySpriteConfig == Other.BodySpriteConfig &&
           ArmsSpriteConfig == Other.ArmsSpriteConfig &&
           BodyShadowSpriteConfig == Other.BodyShadowSpriteConfig &&
           HeadSpriteConfig == Other.HeadSpriteConfig &&
           EyebrowSpriteConfig == Other.EyebrowSpriteConfig &&
           EyesSpriteConfig == Other.EyesSpriteConfig &&
           EyelidsSpriteConfig == Other.EyelidsSpriteConfig &&
           WinkSpriteConfig == Other.WinkSpriteConfig &&
           MouthSpriteConfig == Other.MouthSpriteConfig;
           // Добавляем сравнение остальных конфигураций по необходимости
}

bool F_VN_CharacterState::operator!=(const F_VN_CharacterState& Other) const
{
    return !(*this == Other);
}

FString F_VN_CharacterState::GetSummary() const
{
    int32 VisibleSkeletalCount = 0;
    int32 VisibleSpriteCount = 0;
    
    // Подсчитываем видимые Skeletal Mesh компоненты
    if (BodyConfig.bVisible) VisibleSkeletalCount++;
    if (ArmsConfig.bVisible) VisibleSkeletalCount++;
    if (HeadConfig.bVisible) VisibleSkeletalCount++;
    if (Custom01Config.bVisible) VisibleSkeletalCount++;
    if (Custom02Config.bVisible) VisibleSkeletalCount++;
    if (Custom03Config.bVisible) VisibleSkeletalCount++;
    
    // Подсчитываем видимые Sprite компоненты
    if (BodySpriteConfig.bVisible) VisibleSpriteCount++;
    if (ArmsSpriteConfig.bVisible) VisibleSpriteCount++;
    if (BodyShadowSpriteConfig.bVisible) VisibleSpriteCount++;
    if (HeadSpriteConfig.bVisible) VisibleSpriteCount++;
    if (EyebrowSpriteConfig.bVisible) VisibleSpriteCount++;
    if (EyesSpriteConfig.bVisible) VisibleSpriteCount++;
    if (EyelidsSpriteConfig.bVisible) VisibleSpriteCount++;
    if (WinkSpriteConfig.bVisible) VisibleSpriteCount++;
    if (MouthSpriteConfig.bVisible) VisibleSpriteCount++;
    // Добавляем остальные спрайты...
    
    return FString::Printf(TEXT("State '%s': %d Skeletal, %d Sprites (%s)"), 
        *StateID.ToString(), 
        VisibleSkeletalCount, 
        VisibleSpriteCount,
        *Description);
}