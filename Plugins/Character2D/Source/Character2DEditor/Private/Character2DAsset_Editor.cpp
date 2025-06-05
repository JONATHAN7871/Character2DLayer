#include "Character2DAsset.h"
#include "Character2DAssetEditorToolkit/Character2DAssetEditorUtils.h"

#if WITH_EDITOR
void UCharacter2DAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    FCharacter2DAssetEditorUtils::HandlePostEditChange(this, PropertyChangedEvent);
}

void UCharacter2DAsset::GetAssetRegistryTags(FAssetRegistryTagsContext& Context) const
{
    Super::GetAssetRegistryTags(Context);
    FCharacter2DAssetEditorUtils::AddAssetRegistryTags(this, Context);
}
#endif // WITH_EDITOR
