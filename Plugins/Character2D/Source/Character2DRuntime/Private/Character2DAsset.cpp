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

    const bool bHasSprites        = HasValidSpriteConfiguration();
    const bool bHasSkeletalMeshes = HasValidSkeletalConfiguration();

    if (!bHasSprites && !bHasSkeletalMeshes)
    {
        UE_LOG(
            LogCharacter2D,
            Warning,
            TEXT("Invalid configuration: no sprites or skeletal meshes in asset \"%s\""),
            *GetName()
        );
    }
}

bool UCharacter2DAsset::HasValidSpriteConfiguration() const
{
    const auto& SpriteStruct = SpriteStructure;
    return (SpriteStruct.Body.Sprite || 
            SpriteStruct.Arms.Sprite || 
            SpriteStruct.Head.Head.Sprite || 
            SpriteStruct.Head.Eyes.Sprite || 
            SpriteStruct.Head.Eyebrows.Sprite || 
            SpriteStruct.Head.Eyelids.Sprite || 
            SpriteStruct.Head.Mouth.Sprite ||
            SpriteStruct.Shadow.Sprite);
}

bool UCharacter2DAsset::HasValidSkeletalConfiguration() const
{
    return (Body.Mesh || Arms.Mesh || Head.Mesh);
}

bool UCharacter2DAsset::IsValidForRuntime() const
{
    return (HasValidSpriteConfiguration() || HasValidSkeletalConfiguration());
}

#if WITH_EDITOR
void UCharacter2DAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (FProperty* Property = PropertyChangedEvent.Property)
    {
        const FString PropertyName = Property->GetName();

        if (PropertyName.Contains(TEXT("Head")) ||
            PropertyName.Contains(TEXT("Eyebrow")) ||
            PropertyName.Contains(TEXT("Eyes")) ||
            PropertyName.Contains(TEXT("Eyelids")) ||
            PropertyName.Contains(TEXT("Mouth")) ||
            PropertyName.Contains(TEXT("Blink")) ||
            PropertyName.Contains(TEXT("Talk")) ||
            PropertyName.Contains(TEXT("Effect")) ||
            PropertyName.Contains(TEXT("Shadow")) ||
            PropertyName.Contains(TEXT("Arms")) ||
            PropertyName.Contains(TEXT("Skeletal")))
        {
            ValidateHeadHierarchy();
        }

        const bool bHasSprites  = HasValidSpriteConfiguration();
        const bool bHasSkeletal = HasValidSkeletalConfiguration();

        if (!bHasSprites && !bHasSkeletal)
        {
            UE_LOG(
                LogCharacter2D,
                Warning,
                TEXT("Invalid configuration: no sprites or skeletal meshes in asset \"%s\""),
                *GetName()
            );
        }
    }
}

void UCharacter2DAsset::ValidateHeadHierarchy()
{
    const auto& HeadStruct = SpriteStructure.Head;
    
    // Validate head hierarchy
    if (!HeadStruct.Head.Sprite)
    {
        bool bHasAnyChildSprites = (HeadStruct.Eyebrows.Sprite || 
                                    HeadStruct.Eyes.Sprite || 
                                    HeadStruct.Eyelids.Sprite || 
                                    HeadStruct.Mouth.Sprite || 
                                    HeadStruct.EffectLayer1.Sprite || 
                                    HeadStruct.EffectLayer2.Sprite || 
                                    HeadStruct.EffectLayer3.Sprite);
        if (bHasAnyChildSprites)
        {
            UE_LOG(LogCharacter2D, Warning, 
                   TEXT("Character2D Asset '%s': Facial/Effect sprites configured but Head root sprite is missing. Elements may not position correctly."), 
                   *GetName());
        }
    }
    
    // Validate animation settings
    if (HeadStruct.BlinkSettings.ClosedEyelidsFlipbook && !HeadStruct.Eyelids.Sprite)
    {
        UE_LOG(LogCharacter2D, Warning, 
               TEXT("Character2D Asset '%s': Closed eyelids flipbook configured but static Eyelids sprite is missing."), 
               *GetName());
    }
    
    if (HeadStruct.TalkSettings.TalkFlipbook && !HeadStruct.Mouth.Sprite)
    {
        UE_LOG(LogCharacter2D, Warning, 
               TEXT("Character2D Asset '%s': Talk flipbook configured but static Mouth sprite is missing."), 
               *GetName());
    }
    
    // Validate head attachment
    if (HeadStruct.Head.AttachmentTarget != ECharacter2DAttachmentTarget::None && HeadStruct.Head.SocketName == NAME_None)
    {
        UE_LOG(LogCharacter2D, Warning, 
               TEXT("Character2D Asset '%s': Head attachment target set but socket name is empty."), 
               *GetName());
    }
    
    // Validate shadow layer attachment
    const auto& ShadowStruct = SpriteStructure.Shadow;
    if (ShadowStruct.AttachmentTarget != ECharacter2DAttachmentTarget::None && ShadowStruct.SocketName == NAME_None)
    {
        UE_LOG(LogCharacter2D, Warning, 
               TEXT("Character2D Asset '%s': Shadow attachment target set but socket name is empty."), 
               *GetName());
    }
    
    // Validate skeletal arms attachment
    if (Arms.AttachmentTarget != ECharacter2DSkeletalAttachmentTarget::None && Arms.SocketName == NAME_None)
    {
        UE_LOG(LogCharacter2D, Warning, 
               TEXT("Character2D Asset '%s': Arms attachment target set but socket name is empty."), 
               *GetName());
    }
    
    // Validate skeletal head attachment
    if (Head.AttachmentTarget != ECharacter2DSkeletalAttachmentTarget::None && Head.SocketName == NAME_None)
    {
        UE_LOG(LogCharacter2D, Warning, 
               TEXT("Character2D Asset '%s': Head attachment target set but socket name is empty."), 
               *GetName());
    }
    
    // Validate that attachment targets have valid meshes
    if ((Arms.AttachmentTarget == ECharacter2DSkeletalAttachmentTarget::Body || 
         Head.AttachmentTarget == ECharacter2DSkeletalAttachmentTarget::Body) && 
         !Body.Mesh)
    {
        UE_LOG(LogCharacter2D, Warning, 
               TEXT("Character2D Asset '%s': Arms or Head is set to attach to Body, but Body mesh is not configured."), 
               *GetName());
    }
}

void UCharacter2DAsset::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
    Super::GetAssetRegistryTags(Context);

    auto GetRenderingModeDescription = [this]()
    {
        const bool bHasSprites  = HasValidSpriteConfiguration();
        const bool bHasSkeletal = HasValidSkeletalConfiguration();

        if (bHasSprites && bHasSkeletal)
        {
            return TEXT("Dual Rendering");
        }
        if (bHasSprites)
        {
            return TEXT("Sprite Only");
        }
        if (bHasSkeletal)
        {
            return TEXT("Skeletal Mesh Only");
        }
        return TEXT("Invalid Configuration");
    };

    Context.AddTag(
        FAssetRegistryTag(
            TEXT("RenderingMode"),
            GetRenderingModeDescription(),
            FAssetRegistryTag::TT_Alphabetical
        )
    );
    Context.AddTag(
        FAssetRegistryTag(
            TEXT("HasSprites"),
            HasValidSpriteConfiguration() ? TEXT("True") : TEXT("False"),
            FAssetRegistryTag::TT_Alphabetical
        )
    );
    Context.AddTag(
        FAssetRegistryTag(
            TEXT("HasSkeletalMeshes"),
            HasValidSkeletalConfiguration() ? TEXT("True") : TEXT("False"),
            FAssetRegistryTag::TT_Alphabetical
        )
    );
    Context.AddTag(
        FAssetRegistryTag(
            TEXT("HasShadowLayer"),
            SpriteStructure.Shadow.Sprite ? TEXT("True") : TEXT("False"),
            FAssetRegistryTag::TT_Alphabetical
        )
    );
    
    // Add skeletal attachment info
    FString AttachmentInfo = TEXT("None");
    if (Arms.AttachmentTarget != ECharacter2DSkeletalAttachmentTarget::None || 
        Head.AttachmentTarget != ECharacter2DSkeletalAttachmentTarget::None)
    {
        TArray<FString> AttachedParts;
        if (Arms.AttachmentTarget != ECharacter2DSkeletalAttachmentTarget::None)
        {
            AttachedParts.Add(TEXT("Arms"));
        }
        if (Head.AttachmentTarget != ECharacter2DSkeletalAttachmentTarget::None)
        {
            AttachedParts.Add(TEXT("Head"));
        }
        AttachmentInfo = FString::Join(AttachedParts, TEXT(", "));
    }
    
    Context.AddTag(
        FAssetRegistryTag(
            TEXT("SkeletalAttachments"),
            AttachmentInfo,
            FAssetRegistryTag::TT_Alphabetical
        )
    );
}
#endif // WITH_EDITOR