#include "Character2DAssetEditorToolkit/FAssetTypeActions_Character2DAsset.h"
#include "Character2DAssetEditorToolkit/FCharacter2DAssetEditorToolkit.h"

void FAssetTypeActions_Character2DAsset::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Obj : InObjects)
	{
		if (UCharacter2DAsset* Asset = Cast<UCharacter2DAsset>(Obj))
		{
			TSharedRef<FCharacter2DAssetEditorToolkit> Editor = MakeShareable(new FCharacter2DAssetEditorToolkit());
			Editor->InitEditor(EToolkitMode::Standalone, EditWithinLevelEditor, Asset);
		}
	}
}
