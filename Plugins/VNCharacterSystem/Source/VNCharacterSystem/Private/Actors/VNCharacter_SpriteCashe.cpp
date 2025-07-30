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
    
    if (!Component)
    {
        VN_LOG_WARNING(TEXT("RestoreSpriteFromCache: Component not found for ID %d"), (int32)ComponentID);
        return;
    }
    
    // ✅ КРИТИЧЕСКИ ВАЖНО: NULL в кэше - это валидное состояние!
    if (!CachedSprite.IsNull())
    {
        UPaperSprite* LoadedSprite = CachedSprite.LoadSynchronous();
        Component->SetSprite(LoadedSprite);
        VN_LOG_DEBUG(TEXT("RestoreSpriteFromCache: Restored %d to: %s"), 
            (int32)ComponentID, LoadedSprite ? *LoadedSprite->GetName() : TEXT("NULL"));
    }
    else
    {
        // ✅ ИСПРАВЛЕНИЕ: Кэш содержит NULL - компонент должен быть пустым
        Component->SetSprite(nullptr);
        VN_LOG_DEBUG(TEXT("RestoreSpriteFromCache: Restored %d to NULL (cached empty state)"), (int32)ComponentID);
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