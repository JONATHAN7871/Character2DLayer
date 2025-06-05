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

#if WITH_EDITOR
void UCharacter2DAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	FProperty* Property = PropertyChangedEvent.Property;
	if (Property)
	{
		FString PropertyName = Property->GetName();
		
		// Handle property changes that might affect rendering mode
		if (PropertyName == TEXT("bEnableDualRendering"))
		{
			// Validate dual rendering settings
			bool bHasSprites = HasValidSpriteConfiguration();
			bool bHasSkeletalMeshes = HasValidSkeletalConfiguration();
			
			// Warn if dual rendering is enabled but only one type is configured
			if (bEnableDualRendering && (!bHasSprites || !bHasSkeletalMeshes))
			{
				UE_LOG(LogTemp, Warning, TEXT("Dual rendering enabled but missing sprites or skeletal meshes in %s"), *GetName());
			}
		}
		
		// Automatically update animation settings based on sprite configuration
		if (PropertyName.Contains(TEXT("Sprite")) || PropertyName.Contains(TEXT("Flipbook")))
		{
			ValidateAnimationSettings();
		}
	}
}

void UCharacter2DAsset::ValidateAnimationSettings()
{
	// Validate blink settings
	if (bAutoBlink)
	{
		const auto& BlinkSettings = SpriteStructure.EyelidsBlinkSettings;
		if (!BlinkSettings.BlinkFlipbook)
		{
			UE_LOG(LogTemp, Warning, TEXT("Auto blink enabled but no blink flipbook set in %s"), *GetName());
		}
		if (!SpriteStructure.Eyelids.Sprite)
		{
			UE_LOG(LogTemp, Warning, TEXT("Auto blink enabled but no eyelids sprite set in %s"), *GetName());
		}
	}
	
	// Validate talk settings
	if (bAutoTalk)
	{
		const auto& TalkSettings = SpriteStructure.MouthTalkSettings;
		if (!TalkSettings.TalkFlipbook)
		{
			UE_LOG(LogTemp, Warning, TEXT("Auto talk enabled but no talk flipbook set in %s"), *GetName());
		}
		if (!SpriteStructure.Mouth.Sprite)
		{
			UE_LOG(LogTemp, Warning, TEXT("Auto talk enabled but no mouth sprite set in %s"), *GetName());
		}
	}
}

FString UCharacter2DAsset::GetRenderingModeDescription() const
{
	bool bHasSprites = HasValidSpriteConfiguration();
	bool bHasSkeletalMeshes = HasValidSkeletalConfiguration();
	
	if (bEnableDualRendering && bHasSprites && bHasSkeletalMeshes)
	{
		return TEXT("Dual Rendering (Sprites + Skeletal)");
	}
	else if (bHasSprites && !bHasSkeletalMeshes)
	{
		return TEXT("Sprite Only");
	}
	else if (!bHasSprites && bHasSkeletalMeshes)
	{
		return TEXT("Skeletal Mesh Only");
	}
	else if (bEnableDualRendering)
	{
		return TEXT("Dual Rendering (Incomplete Configuration)");
	}
	else
	{
		return TEXT("No Valid Configuration");
	}
}

TArray<FString> UCharacter2DAsset::GetConfigurationWarnings() const
{
	TArray<FString> Warnings;
	
	bool bHasSprites = HasValidSpriteConfiguration();
	bool bHasSkeletalMeshes = HasValidSkeletalConfiguration();
	
	// Check for missing configurations
	if (!bHasSprites && !bHasSkeletalMeshes)
	{
		Warnings.Add(TEXT("No sprites or skeletal meshes configured"));
	}
	
	if (bEnableDualRendering && (!bHasSprites || !bHasSkeletalMeshes))
	{
		if (!bHasSprites)
		{
			Warnings.Add(TEXT("Dual rendering enabled but no sprites configured"));
		}
		if (!bHasSkeletalMeshes)
		{
			Warnings.Add(TEXT("Dual rendering enabled but no skeletal meshes configured"));
		}
	}
	
	// Check animation settings
	if (bAutoBlink && !SpriteStructure.EyelidsBlinkSettings.BlinkFlipbook)
	{
		Warnings.Add(TEXT("Auto blink enabled but no blink flipbook set"));
	}
	
	if (bAutoTalk && !SpriteStructure.MouthTalkSettings.TalkFlipbook)
	{
		Warnings.Add(TEXT("Auto talk enabled but no talk flipbook set"));
	}
	
	// Check visual novel settings
	if (VisualNovelSettings.bAutoPlayEntranceTransition && 
	    VisualNovelSettings.DefaultEntranceTransition == ECharacter2DTransitionType::None)
	{
		Warnings.Add(TEXT("Auto entrance transition enabled but transition type is None"));
	}
	
	return Warnings;
}

bool UCharacter2DAsset::IsValidForRuntime() const
{
	bool bHasSprites = HasValidSpriteConfiguration();
	bool bHasSkeletalMeshes = HasValidSkeletalConfiguration();
	
	// At least one rendering method must be available
	return (bHasSprites || bHasSkeletalMeshes);
}

void UCharacter2DAsset::GetAssetRegistryTags(FAssetRegistryTagsContext& Context) const
{
        Super::GetAssetRegistryTags(Context);

        // Add custom tags for asset browser filtering using the new context API
        Context.AddTag(TEXT("RenderingMode"), GetRenderingModeDescription(), FAssetRegistryTag::TT_Alphabetical);

        Context.AddTag(TEXT("HasSprites"),
                HasValidSpriteConfiguration() ? TEXT("True") : TEXT("False"),
                FAssetRegistryTag::TT_Alphabetical);

        Context.AddTag(TEXT("HasSkeletalMeshes"),
                HasValidSkeletalConfiguration() ? TEXT("True") : TEXT("False"),
                FAssetRegistryTag::TT_Alphabetical);

        Context.AddTag(TEXT("SupportsBlinking"),
                (bAutoBlink && SpriteStructure.EyelidsBlinkSettings.BlinkFlipbook) ? TEXT("True") : TEXT("False"),
                FAssetRegistryTag::TT_Alphabetical);

        Context.AddTag(TEXT("SupportsTalking"),
                (bAutoTalk && SpriteStructure.MouthTalkSettings.TalkFlipbook) ? TEXT("True") : TEXT("False"),
                FAssetRegistryTag::TT_Alphabetical);
}
#endif // WITH_EDITOR
