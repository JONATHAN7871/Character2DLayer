#include "Character2DAsset.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#if WITH_EDITOR
#include "UObject/AssetRegistryTagsContext.h"
#include "UObject/UObjectGlobals.h"
#include "Logging/LogMacros.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogCharacter2D, Log, All);

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
    
    // ═══ NEW: Migrate to hierarchical head structure ═══
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
    // ═══ NEW: Check new hierarchical structure ═══
    const auto& SpriteStruct = SpriteStructure;
    
    return (SpriteStruct.Body.Sprite != nullptr ||
            SpriteStruct.Arms.Sprite != nullptr ||
            SpriteStruct.Head.Head.Sprite != nullptr ||
            SpriteStruct.Head.Eyes.Sprite != nullptr ||
            SpriteStruct.Head.Eyebrows.Sprite != nullptr ||  // Updated name
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

        // ═══ Handle head hierarchy changes ═══
        if (PropertyName.Contains(TEXT("Head")) || PropertyName.Contains(TEXT("Eyebrow")) || 
            PropertyName.Contains(TEXT("Eyes")) || PropertyName.Contains(TEXT("Eyelids")) || 
            PropertyName.Contains(TEXT("Mouth")) || PropertyName.Contains(TEXT("Blink")) || 
            PropertyName.Contains(TEXT("Talk")))
        {
            // Validate head hierarchy settings
            ValidateHeadHierarchy();
        }

        if (PropertyName == TEXT("bEnableDualRendering"))
        {
            const bool bHasSprites = HasValidSpriteConfiguration();
            const bool bHasSkeletal = HasValidSkeletalConfiguration();
            if (bEnableDualRendering && (!bHasSprites || !bHasSkeletal))
            {
                UE_LOG(LogCharacter2D, Warning, TEXT("Dual rendering enabled but missing sprites or skeletal meshes in %s"), *GetName());
            }
        }

        // ═══ Handle head transform changes (cascade to children) ═══
        if (PropertyName.Contains(TEXT("Head")) && 
            (PropertyName.Contains(TEXT("Offset")) || PropertyName.Contains(TEXT("Scale")) || PropertyName.Contains(TEXT("bVisible"))))
        {
            // Head transform changes affect all children automatically through the new structure
            UE_LOG(LogCharacter2D, Log, TEXT("Head transform changed in %s - children will inherit automatically"), *GetName());
        }
    }
}

void UCharacter2DAsset::ValidateHeadHierarchy()
{
    const auto& HeadStruct = SpriteStructure.Head;
    
    // Warn if head children have sprites but head root doesn't
    if (!HeadStruct.Head.Sprite)
    {
        bool bHasFacialSprites = (HeadStruct.Eyebrows.Sprite || HeadStruct.Eyes.Sprite || 
                                 HeadStruct.Eyelids.Sprite || HeadStruct.Mouth.Sprite);
        if (bHasFacialSprites)
        {
            UE_LOG(LogCharacter2D, Warning, TEXT("Character2D Asset '%s': Facial sprites configured but Head root sprite is missing. Facial elements may not position correctly."), *GetName());
        }
    }

    // Warn if animations are configured but corresponding static sprites aren't
    if (HeadStruct.BlinkSettings.BlinkFlipbook && !HeadStruct.Eyelids.Sprite)
    {
        UE_LOG(LogCharacter2D, Warning, TEXT("Character2D Asset '%s': Blink animation configured but static Eyelids sprite is missing."), *GetName());
    }

    if (HeadStruct.TalkSettings.TalkFlipbook && !HeadStruct.Mouth.Sprite)
    {
        UE_LOG(LogCharacter2D, Warning, TEXT("Character2D Asset '%s': Talk animation configured but static Mouth sprite is missing."), *GetName());
    }

    // Validate attachment target (only head should have it)
    if (HeadStruct.Head.AttachmentTarget != ECharacter2DAttachmentTarget::None)
    {
        if (HeadStruct.Head.SocketName == NAME_None)
        {
            UE_LOG(LogCharacter2D, Warning, TEXT("Character2D Asset '%s': Head attachment target set but socket name is empty."), *GetName());
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

    auto GetHeadHierarchyInfo = [this]()
    {
        const auto& HeadStruct = SpriteStructure.Head;
        int32 FacialElementCount = 0;
        if (HeadStruct.Eyebrows.Sprite) FacialElementCount++;
        if (HeadStruct.Eyes.Sprite) FacialElementCount++;
        if (HeadStruct.Eyelids.Sprite) FacialElementCount++;
        if (HeadStruct.Mouth.Sprite) FacialElementCount++;
        
        return FString::Printf(TEXT("Head: %s, Facial Elements: %d"), 
                              HeadStruct.Head.Sprite ? TEXT("Yes") : TEXT("No"), 
                              FacialElementCount);
    };

    Context.AddTag(FAssetRegistryTag(TEXT("RenderingMode"), GetRenderingModeDescription(), FAssetRegistryTag::TT_Alphabetical));
    Context.AddTag(FAssetRegistryTag(TEXT("HasSprites"), HasValidSpriteConfiguration() ? TEXT("True") : TEXT("False"), FAssetRegistryTag::TT_Alphabetical));
    Context.AddTag(FAssetRegistryTag(TEXT("HasSkeletalMeshes"), HasValidSkeletalConfiguration() ? TEXT("True") : TEXT("False"), FAssetRegistryTag::TT_Alphabetical));
    Context.AddTag(FAssetRegistryTag(TEXT("HeadHierarchy"), GetHeadHierarchyInfo(), FAssetRegistryTag::TT_Alphabetical));
    Context.AddTag(FAssetRegistryTag(TEXT("SupportsBlinking"), (bAutoBlink && SpriteStructure.Head.BlinkSettings.BlinkFlipbook) ? TEXT("True") : TEXT("False"), FAssetRegistryTag::TT_Alphabetical));
    Context.AddTag(FAssetRegistryTag(TEXT("SupportsTalking"), (bAutoTalk && SpriteStructure.Head.TalkSettings.TalkFlipbook) ? TEXT("True") : TEXT("False"), FAssetRegistryTag::TT_Alphabetical));
    
    // ═══ NEW: Add head attachment info ═══
    const auto& HeadRoot = SpriteStructure.Head.Head;
    if (HeadRoot.AttachmentTarget != ECharacter2DAttachmentTarget::None)
    {
        FString AttachmentInfo = FString::Printf(TEXT("%s:%s"), 
                                               *UEnum::GetValueAsString(HeadRoot.AttachmentTarget),
                                               *HeadRoot.SocketName.ToString());
        Context.AddTag(FAssetRegistryTag(TEXT("HeadAttachment"), AttachmentInfo, FAssetRegistryTag::TT_Alphabetical));
    }
    else
    {
        Context.AddTag(FAssetRegistryTag(TEXT("HeadAttachment"), TEXT("None"), FAssetRegistryTag::TT_Alphabetical));
    }
}
#endif // WITH_EDITOR