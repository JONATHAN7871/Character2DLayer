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

    // Если ни спрайтов, ни скелетных мешей, выводим предупреждение
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
    return (SpriteStruct.Body.Sprite || SpriteStruct.Arms.Sprite || SpriteStruct.Head.Head.Sprite || SpriteStruct.Head.Eyes.Sprite || SpriteStruct.Head.Eyebrows.Sprite || SpriteStruct.Head.Eyelids.Sprite || SpriteStruct.Head.Mouth.Sprite);
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

        // При изменении любой части головы — проверяем иерархию
        if (PropertyName.Contains(TEXT("Head")) ||
            PropertyName.Contains(TEXT("Eyebrow")) ||
            PropertyName.Contains(TEXT("Eyes")) ||
            PropertyName.Contains(TEXT("Eyelids")) ||
            PropertyName.Contains(TEXT("Mouth")) ||
            PropertyName.Contains(TEXT("Blink")) ||
            PropertyName.Contains(TEXT("Talk")) ||
            PropertyName.Contains(TEXT("Effect")))
        {
            ValidateHeadHierarchy();
        }

        // Проверяем конфигурацию сразу после изменения любого свойства
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
    if (!HeadStruct.Head.Sprite)
    {
        bool bHasAnyChildSprites = (HeadStruct.Eyebrows.Sprite || HeadStruct.Eyes.Sprite || HeadStruct.Eyelids.Sprite || HeadStruct.Mouth.Sprite || HeadStruct.EffectLayer1.Sprite || HeadStruct.EffectLayer2.Sprite || HeadStruct.EffectLayer3.Sprite);
        if (bHasAnyChildSprites)
        {
            UE_LOG(LogCharacter2D, Warning, TEXT("Character2D Asset '%s': Facial/Effect sprites configured but Head root sprite is missing. Elements may not position correctly."), *GetName());
        }
    }
    if (HeadStruct.BlinkSettings.ClosedEyelidsFlipbook && !HeadStruct.Eyelids.Sprite)
    {
        UE_LOG(LogCharacter2D, Warning, TEXT("Character2D Asset '%s': Closed eyelids flipbook configured but static Eyelids sprite is missing."), *GetName());
    }
    if (HeadStruct.TalkSettings.TalkFlipbook && !HeadStruct.Mouth.Sprite)
    {
        UE_LOG(LogCharacter2D, Warning, TEXT("Character2D Asset '%s': Talk flipbook configured but static Mouth sprite is missing."), *GetName());
    }
    if (HeadStruct.Head.AttachmentTarget != ECharacter2DAttachmentTarget::None && HeadStruct.Head.SocketName == NAME_None)
    {
        UE_LOG(LogCharacter2D, Warning, TEXT("Character2D Asset '%s': Head attachment target set but socket name is empty."), *GetName());
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
            return TEXT("Dual Rendering");            // или TEXT("Sprite + Skeletal")
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
}
#endif // WITH_EDITOR