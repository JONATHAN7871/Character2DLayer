#include "Character2DAsset.h"
#include "Engine/World.h"
#if WITH_EDITOR
#include "UObject/AssetRegistryTagsContext.h"
#endif

void UCharacter2DAsset::PostLoad()
{
    Super::PostLoad();
    
    // Migration support for legacy assets
    MigrateLegacyData();
}

void UCharacter2DAsset::MigrateLegacyData()
{
    // Initialize Visual Novel Settings if they're empty (for legacy assets)
    if (VisualNovelSettings.DefaultFadeDuration == 0.0f)
    {
        // Set up default values for visual novel settings
        VisualNovelSettings.DefaultFadeDuration = 1.0f;
        VisualNovelSettings.DefaultEmotionSettings.Duration = 2.0f;
        VisualNovelSettings.DefaultEmotionSettings.Intensity = 0.5f;
        VisualNovelSettings.DefaultMovementSettings.Duration = 1.0f;
    }
    
    // Migrate sprite structure to new format
    SpriteStructure.MigrateFromLegacyStructure();
    
    // Ensure backwards compatibility for existing sprite configurations
    bool bHasSprites = HasValidSpriteConfiguration();
    bool bHasSkeletalMeshes = HasValidSkeletalConfiguration();
    
    if (bHasSprites && !bHasSkeletalMeshes)
    {
        bEnableDualRendering = false; // Show only sprites
    }
    else if (!bHasSprites && bHasSkeletalMeshes)
    {
        bEnableDualRendering = false; // Show only skeletal meshes
    }
    // If both are configured, leave dual rendering as-is (user preference)
}

bool UCharacter2DAsset::HasValidSpriteConfiguration() const
{
    // Check new structure first
    const auto& SpriteStruct = SpriteStructure;
    
    return (SpriteStruct.Body.Sprite != nullptr ||
            SpriteStruct.Arms.Sprite != nullptr ||
            SpriteStruct.Head.Head.Sprite != nullptr ||
            SpriteStruct.Head.Eyes.Sprite != nullptr ||
            SpriteStruct.Head.Eyebrow.Sprite != nullptr ||
            SpriteStruct.Head.Eyelids.Sprite != nullptr ||
            SpriteStruct.Head.Mouth.Sprite != nullptr);
}

bool UCharacter2DAsset::HasValidSkeletalConfiguration() const
{
    return (Body.Mesh != nullptr ||
            Arms.Mesh != nullptr ||
            Head.Mesh != nullptr);
}

bool UCharacter2DAsset::IsValidForRuntime() const
{
    bool bHasSprites = HasValidSpriteConfiguration();
    bool bHasSkeletalMeshes = HasValidSkeletalConfiguration();

    // At least one rendering method must be available
    return (bHasSprites || bHasSkeletalMeshes);
}

#if WITH_EDITOR
void UCharacter2DAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    FProperty* Property = PropertyChangedEvent.Property;
    if (Property)
    {
        const FString PropertyName = Property->GetName();

        if (PropertyName == TEXT("bEnableDualRendering"))
        {
            const bool bHasSprites = HasValidSpriteConfiguration();
            const bool bHasSkeletal = HasValidSkeletalConfiguration();
            if (bEnableDualRendering && (!bHasSprites || !bHasSkeletal))
            {
                UE_LOG(LogTemp, Warning, TEXT("Dual rendering enabled but missing sprites or skeletal meshes in %s"), *GetName());
            }
        }
    }
}

void UCharacter2DAsset::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
    Super::GetAssetRegistryTags(Context);
    auto GetRenderingModeDescription = [this]()
    {
        const bool bHasSprites = HasValidSpriteConfiguration();
        const bool bHasSkeletal = HasValidSkeletalConfiguration();
        if (bEnableDualRendering && bHasSprites && bHasSkeletal)
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
        if (bEnableDualRendering)
        {
            return TEXT("Dual Rendering (Incomplete Configuration)");
        }
        return TEXT("No Valid Configuration");
    };

    Context.AddTag(FAssetRegistryTag(TEXT("RenderingMode"), GetRenderingModeDescription(), FAssetRegistryTag::TT_Alphabetical));
    Context.AddTag(FAssetRegistryTag(TEXT("HasSprites"), HasValidSpriteConfiguration() ? TEXT("True") : TEXT("False"), FAssetRegistryTag::TT_Alphabetical));
    Context.AddTag(FAssetRegistryTag(TEXT("HasSkeletalMeshes"), HasValidSkeletalConfiguration() ? TEXT("True") : TEXT("False"), FAssetRegistryTag::TT_Alphabetical));
    Context.AddTag(FAssetRegistryTag(TEXT("SupportsBlinking"), (bAutoBlink && SpriteStructure.Head.EyelidsBlinkSettings.BlinkFlipbook) ? TEXT("True") : TEXT("False"), FAssetRegistryTag::TT_Alphabetical));
    Context.AddTag(FAssetRegistryTag(TEXT("SupportsTalking"), (bAutoTalk && SpriteStructure.Head.MouthTalkSettings.TalkFlipbook) ? TEXT("True") : TEXT("False"), FAssetRegistryTag::TT_Alphabetical));
}
#endif // WITH_EDITOR