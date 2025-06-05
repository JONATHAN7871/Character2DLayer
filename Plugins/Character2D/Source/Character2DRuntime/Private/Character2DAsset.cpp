#include "Character2DAsset.h"
#include "Engine/World.h"
#include "AssetRegistry/AssetRegistryTagsContext.h"

void UCharacter2DAsset::PostLoad()
{
	Super::PostLoad();
	
	// Migration support for legacy assets
	MigrateLegacyData();
}

void UCharacter2DAsset::MigrateLegacyData()
{
	// Check if we need to migrate from old bUseSpriteInsteadOfMesh system
	// This would be detected by checking for the presence of old data structures
	
	// Example migration logic:
	// If old asset had bUseSpriteInsteadOfMesh = true, 
	// we now set bEnableDualRendering = false and ensure sprites are visible
	
	// Initialize Visual Novel Settings if they're empty (for legacy assets)
	if (VisualNovelSettings.DefaultAppearanceTransition.Duration == 0.0f)
	{
		// Set up default values for visual novel settings
		VisualNovelSettings.DefaultAppearanceTransition.Duration = 1.0f;
		VisualNovelSettings.DefaultDisappearanceTransition.Duration = 1.0f;
		VisualNovelSettings.DefaultEmotionSettings.Duration = 2.0f;
		VisualNovelSettings.DefaultEmotionSettings.Intensity = 0.5f;
		VisualNovelSettings.DefaultEntranceTransition = ECharacter2DTransitionType::FadeIn;
		VisualNovelSettings.bAutoPlayEntranceTransition = false;
	}
	
	// Ensure backwards compatibility for existing sprite configurations
	// If sprites are configured but skeletal meshes are not, disable dual rendering
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
	else if (bHasSprites && bHasSkeletalMeshes)
	{
		// User can choose - default to dual rendering for maximum compatibility
		// but this could be configurable
		bEnableDualRendering = true;
	}
}

bool UCharacter2DAsset::HasValidSpriteConfiguration() const
{
	const FCharacter2DSpriteStructure& Sprites = SpriteStructure;
	
	return (Sprites.Body.Sprite != nullptr ||
	        Sprites.Arms.Sprite != nullptr ||
	        Sprites.Head.Sprite != nullptr ||
	        Sprites.Eyes.Sprite != nullptr ||
	        Sprites.Eyebrow.Sprite != nullptr ||
	        Sprites.Eyelids.Sprite != nullptr ||
	        Sprites.Mouth.Sprite != nullptr);
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
