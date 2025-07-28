#include "Actors/VNCharacter.h"
#include "Components/VNCharacterIdleAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "PaperSpriteComponent.h"

// Система кэширования спрайтов для idle анимаций

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
    switch (ComponentID)
    {
    case E_VN_ComponentID_Sprite::Eyes:
        CachedEyesSprite = Sprite;
        VN_LOG_DEBUG(TEXT("SetCachedSprite: Eyes cached: %s"), 
            Sprite.IsNull() ? TEXT("NULL") : *Sprite.ToString());
        break;
    case E_VN_ComponentID_Sprite::Mouth:
        CachedMouthSprite = Sprite;
        VN_LOG_DEBUG(TEXT("SetCachedSprite: Mouth cached: %s"), 
            Sprite.IsNull() ? TEXT("NULL") : *Sprite.ToString());
        break;
    case E_VN_ComponentID_Sprite::Eyebrow:
        CachedEyebrowSprite = Sprite;
        VN_LOG_DEBUG(TEXT("SetCachedSprite: Eyebrow cached: %s"), 
            Sprite.IsNull() ? TEXT("NULL") : *Sprite.ToString());
        break;
    case E_VN_ComponentID_Sprite::Eyelids:
        CachedEyelidsSprite = Sprite;
        VN_LOG_DEBUG(TEXT("SetCachedSprite: Eyelids cached: %s"), 
            Sprite.IsNull() ? TEXT("NULL") : *Sprite.ToString());
        break;
    case E_VN_ComponentID_Sprite::Wink:
        CachedWinkSprite = Sprite;
        VN_LOG_DEBUG(TEXT("SetCachedSprite: Wink cached: %s"), 
            Sprite.IsNull() ? TEXT("NULL") : *Sprite.ToString());
        break;
    default:
        VN_LOG_WARNING(TEXT("SetCachedSprite: Unsupported component ID: %d"), (int32)ComponentID);
        break;
    }
}

void AVNCharacter::UpdateSpriteCache()
{
    VN_LOG_DEBUG(TEXT("UpdateSpriteCache: Updating all cached sprites to current component state"));
    
    if (Eyes_Sprite)
    {
        CachedEyesSprite = Eyes_Sprite->GetSprite();
        VN_LOG_DEBUG(TEXT("UpdateSpriteCache: Eyebrow updated to: %s"), 
            CachedEyebrowSprite.IsNull() ? TEXT("NULL") : *CachedEyebrowSprite.ToString());
    }
    
    if (Eyelids_Sprite)
    {
        CachedEyelidsSprite = Eyelids_Sprite->GetSprite();
        VN_LOG_DEBUG(TEXT("UpdateSpriteCache: Eyelids updated to: %s"), 
            CachedEyelidsSprite.IsNull() ? TEXT("NULL") : *CachedEyelidsSprite.ToString());
    }
    
    if (Wink_Sprite)
    {
        CachedWinkSprite = Wink_Sprite->GetSprite();
        VN_LOG_DEBUG(TEXT("UpdateSpriteCache: Wink updated to: %s"), 
            CachedWinkSprite.IsNull() ? TEXT("NULL") : *CachedWinkSprite.ToString());
    }
}

void AVNCharacter::RestoreSpriteFromCache(E_VN_ComponentID_Sprite ComponentID)
{
    TSoftObjectPtr<UPaperSprite> CachedSprite = GetCachedSprite(ComponentID);
    UPaperSpriteComponent* Component = GetSpriteComponent(ComponentID);
    
    if (!Component)
    {
        VN_LOG_WARNING(TEXT("RestoreSpriteFromCache: Component not found for ID %d"), (int32)ComponentID);
        return;
    }
    
    if (!CachedSprite.IsNull())
    {
        UPaperSprite* LoadedSprite = CachedSprite.LoadSynchronous();
        Component->SetSprite(LoadedSprite);
        VN_LOG_DEBUG(TEXT("RestoreSpriteFromCache: Restored %d to: %s"), 
            (int32)ComponentID, LoadedSprite ? *LoadedSprite->GetName() : TEXT("NULL"));
    }
    else
    {
        Component->SetSprite(nullptr);
        VN_LOG_DEBUG(TEXT("RestoreSpriteFromCache: Restored %d to NULL"), (int32)ComponentID);
    }
}

void AVNCharacter::CacheSpriteOnSet(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> Sprite)
{
    SetCachedSprite(ComponentID, Sprite);
}

void AVNCharacter::SynchronizeIdleAnimationStates()
{
    if (!IdleAnimationManager) return;

    UpdateSpriteCache();
    
    VN_LOG_DEBUG(TEXT("SynchronizeIdleAnimationStates: Sprite cache updated for idle animations"));
}