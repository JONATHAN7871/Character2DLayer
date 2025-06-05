#include "Character2DAssetEditorToolkit/Character2DAssetEditorUtils.h"

#if WITH_EDITOR
#include "Character2DAsset.h"

void FCharacter2DAssetEditorUtils::HandlePostEditChange(UCharacter2DAsset* Asset, FPropertyChangedEvent& PropertyChangedEvent)
{
    if (!Asset) return;

    FProperty* Property = PropertyChangedEvent.Property;
    if (Property)
    {
        const FString PropertyName = Property->GetName();

        if (PropertyName == TEXT("bEnableDualRendering"))
        {
            const bool bHasSprites = Asset->HasValidSpriteConfiguration();
            const bool bHasSkeletal = Asset->HasValidSkeletalConfiguration();
            if (Asset->bEnableDualRendering && (!bHasSprites || !bHasSkeletal))
            {
                UE_LOG(LogTemp, Warning, TEXT("Dual rendering enabled but missing sprites or skeletal meshes in %s"), *Asset->GetName());
            }
        }
    }
}

void FCharacter2DAssetEditorUtils::AddAssetRegistryTags(const UCharacter2DAsset* Asset, FAssetRegistryTagsContext& Context)
{
    if (!Asset) return;

    Context.AddTag(TEXT("RenderingMode"), Asset->HasValidSpriteConfiguration() && Asset->HasValidSkeletalConfiguration()
                                          ? TEXT("Dual") : (Asset->HasValidSpriteConfiguration() ? TEXT("Sprite") : TEXT("Skeletal")),
                   FAssetRegistryTag::TT_Alphabetical);
}
#endif // WITH_EDITOR
