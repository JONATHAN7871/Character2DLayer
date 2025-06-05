#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetRegistryTagsContext.h"
#include "UObject/Object.h"
#include "Character2DAsset.h"

/** Utility helpers for UCharacter2DAsset used only in the editor module. */
class CHARACTER2DEDITOR_API FCharacter2DAssetEditorUtils
{
public:
    static void HandlePostEditChange(UCharacter2DAsset* Asset, FPropertyChangedEvent& PropertyChangedEvent);
    static void AddAssetRegistryTags(const UCharacter2DAsset* Asset, FAssetRegistryTagsContext& Context);
};
