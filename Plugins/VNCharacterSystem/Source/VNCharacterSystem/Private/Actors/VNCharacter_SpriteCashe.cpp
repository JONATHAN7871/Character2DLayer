#include "Actors/VNCharacter.h"
#include "Components/VNCharacterIdleAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "PaperSpriteComponent.h"

TSoftObjectPtr<UPaperSprite> AVNCharacter::GetCachedSprite(E_VN_ComponentID_Sprite ComponentID) const
{
    switch (ComponentID)
    {
        case E_VN_ComponentID_Sprite::Eyes:
            return CachedEyesSprite;
        case E_VN_ComponentID_Sprite::Mouth:
            return CachedMouthSprite;
        case E_VN_ComponentID_Sprite::Eyebrow:
            return CachedEyebrowSprite;
        case E_VN_ComponentID_Sprite::Eyelids:
            return CachedEyelidsSprite;
        case E_VN_ComponentID_Sprite::Wink:
            return CachedWinkSprite;
        default:
            return nullptr;
    }
}

void AVNCharacter::SetCachedSprite(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> Sprite)
{
    // Универсальная логика для всех компонентов, чтобы не сломать остальное
    switch (ComponentID)
    {
    case E_VN_ComponentID_Sprite::Eyes:    CachedEyesSprite = Sprite; break;
    case E_VN_ComponentID_Sprite::Mouth:   CachedMouthSprite = Sprite; break;
    case E_VN_ComponentID_Sprite::Eyebrow: CachedEyebrowSprite = Sprite; break;
    case E_VN_ComponentID_Sprite::Eyelids: CachedEyelidsSprite = Sprite; break;
    case E_VN_ComponentID_Sprite::Wink:    CachedWinkSprite = Sprite; break;
    default: break;
    }
    
    // --- ОСОБОЕ ЛОГИРОВАНИЕ ТОЛЬКО ДЛЯ РТА ---
    if (ComponentID == E_VN_ComponentID_Sprite::Mouth)
    {
        FString SpriteName = Sprite.IsNull() ? TEXT("NULL") : Sprite.ToString();
        UE_LOG(LogTemp, Error, TEXT("!!! SET CACHED MOUTH SPRITE -> %s"), *SpriteName);
    }
}

void AVNCharacter::UpdateSpriteCache()
{
    VN_LOG_DEBUG(TEXT("UpdateSpriteCache: Updating all cached sprites to current component state"));
    
    // ИСПРАВЛЕНИЕ: Кэшируем ВСЕ состояния, включая nullptr
    if (Eyes_Sprite)
    {
        UPaperSprite* CurrentSprite = Eyes_Sprite->GetSprite();
        CachedEyesSprite = CurrentSprite; // Может быть nullptr
        VN_LOG_DEBUG(TEXT("UpdateSpriteCache: Eyes updated to: %s"), 
            CurrentSprite ? *CurrentSprite->GetName() : TEXT("NULL"));
    }
    
    if (Mouth_Sprite)
    {
        UPaperSprite* CurrentSprite = Mouth_Sprite->GetSprite();
        CachedMouthSprite = CurrentSprite; // Может быть nullptr
        VN_LOG_DEBUG(TEXT("UpdateSpriteCache: Mouth updated to: %s"), 
            CurrentSprite ? *CurrentSprite->GetName() : TEXT("NULL"));
    }
    
    if (Eyebrow_Sprite)
    {
        UPaperSprite* CurrentSprite = Eyebrow_Sprite->GetSprite();
        CachedEyebrowSprite = CurrentSprite; // Может быть nullptr
        VN_LOG_DEBUG(TEXT("UpdateSpriteCache: Eyebrow updated to: %s"), 
            CurrentSprite ? *CurrentSprite->GetName() : TEXT("NULL"));
    }
    
    if (Eyelids_Sprite)
    {
        UPaperSprite* CurrentSprite = Eyelids_Sprite->GetSprite();
        CachedEyelidsSprite = CurrentSprite; // Может быть nullptr - это нормально!
        VN_LOG_DEBUG(TEXT("UpdateSpriteCache: Eyelids updated to: %s"), 
            CurrentSprite ? *CurrentSprite->GetName() : TEXT("NULL"));
    }
    
    if (Wink_Sprite)
    {
        UPaperSprite* CurrentSprite = Wink_Sprite->GetSprite();
        CachedWinkSprite = CurrentSprite; // Может быть nullptr
        VN_LOG_DEBUG(TEXT("UpdateSpriteCache: Wink updated to: %s"), 
            CurrentSprite ? *CurrentSprite->GetName() : TEXT("NULL"));
    }
}

void AVNCharacter::RestoreSpriteFromCache(E_VN_ComponentID_Sprite ComponentID)
{
    TSoftObjectPtr<UPaperSprite> CachedSprite = GetCachedSprite(ComponentID);
    UPaperSpriteComponent* Component = GetSpriteComponent(ComponentID);
    if(Component) 
    {
        Component->SetSprite(CachedSprite.LoadSynchronous());
    }

    // --- ОСОБОЕ ЛОГИРОВАНИЕ ТОЛЬКО ДЛЯ РТА ---
    if (ComponentID == E_VN_ComponentID_Sprite::Mouth)
    {
        FString CachedSpriteName = CachedSprite.IsNull() ? TEXT("NULL") : CachedSprite.ToString();
        UE_LOG(LogTemp, Error, TEXT("!!! RESTORE MOUTH FROM CACHE -> Attempting to restore to: %s"), *CachedSpriteName);
        if(Component && Component->GetSprite())
        {
            UE_LOG(LogTemp, Error, TEXT("!!!                        Success. Component now has: %s"), *Component->GetSprite()->GetName());
        }
        else if (Component)
        {
            UE_LOG(LogTemp, Error, TEXT("!!!                        Success. Component now has: NULL"));
        }
    }
}

void AVNCharacter::CacheSpriteOnSet(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> Sprite)
{
    SetCachedSprite(ComponentID, Sprite);
}

void AVNCharacter::SynchronizeIdleAnimationStates()
{
    if (!IdleAnimationManager) return;

    VN_LOG_DEBUG(TEXT("SynchronizeIdleAnimationStates: Starting synchronization"));
    
    // Обновляем кэш всех состояний (включая пустые)
    UpdateSpriteCache();
    
    VN_LOG_DEBUG(TEXT("SynchronizeIdleAnimationStates: Sprite cache updated for idle animations"));
}