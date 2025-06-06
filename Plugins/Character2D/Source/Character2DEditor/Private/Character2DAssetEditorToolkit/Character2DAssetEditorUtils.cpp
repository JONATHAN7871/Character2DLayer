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

void FCharacter2DAssetEditorUtils::AddAssetRegistryTags(const UCharacter2DAsset* Asset, TArray<FAssetRegistryTag>& OutTags)
{
    if (!Asset) return;

    auto GetRenderingModeDescription = [Asset]()
    {
        const bool bHasSprites = Asset->HasValidSpriteConfiguration();
        const bool bHasSkeletal = Asset->HasValidSkeletalConfiguration();

        if (Asset->bEnableDualRendering && bHasSprites && bHasSkeletal)
        {
            return TEXT("Dual Rendering (Sprites + Skeletal)");
        }
        if (bHasSprites && !bHasSkeletal)
        {
            return TEXT("Sprite Only");
        }
        if (!bHasSprites && bHasSkeletal)
        {
            return TEXT("Skeletal Mesh Only");
        }
        if (Asset->bEnableDualRendering)
        {
            return TEXT("Dual Rendering (Incomplete Configuration)");
        }
        return TEXT("No Valid Configuration");
    };

    OutTags.Add(FAssetRegistryTag(TEXT("RenderingMode"), GetRenderingModeDescription(), FAssetRegistryTag::TT_Alphabetical));
    OutTags.Add(FAssetRegistryTag(TEXT("HasSprites"), Asset->HasValidSpriteConfiguration() ? TEXT("True") : TEXT("False"), FAssetRegistryTag::TT_Alphabetical));
    OutTags.Add(FAssetRegistryTag(TEXT("HasSkeletalMeshes"), Asset->HasValidSkeletalConfiguration() ? TEXT("True") : TEXT("False"), FAssetRegistryTag::TT_Alphabetical));
    OutTags.Add(FAssetRegistryTag(TEXT("SupportsBlinking"), (Asset->bAutoBlink && Asset->SpriteStructure.EyelidsBlinkSettings.BlinkFlipbook) ? TEXT("True") : TEXT("False"), FAssetRegistryTag::TT_Alphabetical));
    OutTags.Add(FAssetRegistryTag(TEXT("SupportsTalking"), (Asset->bAutoTalk && Asset->SpriteStructure.MouthTalkSettings.TalkFlipbook) ? TEXT("True") : TEXT("False"), FAssetRegistryTag::TT_Alphabetical));
}
#endif // WITH_EDITOR
