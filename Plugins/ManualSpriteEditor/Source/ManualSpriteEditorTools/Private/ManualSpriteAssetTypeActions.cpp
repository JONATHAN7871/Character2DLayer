#include "ManualSpriteAssetTypeActions.h"
#include "ManualSpriteEditorToolkit.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/MessageDialog.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"

#define LOCTEXT_NAMESPACE "ManualSpriteAssetTypeActions"

FText FManualSpriteAssetTypeActions::GetName() const
{
	return LOCTEXT("ManualSpriteAssetTypeActionsName", "Manual Sprite");
}

FColor FManualSpriteAssetTypeActions::GetTypeColor() const
{
	return FColor(129, 196, 115); // Green color to distinguish from regular sprites
}

UClass* FManualSpriteAssetTypeActions::GetSupportedClass() const
{
	return UManualSprite::StaticClass();
}

uint32 FManualSpriteAssetTypeActions::GetCategories()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	return AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Paper2D")), LOCTEXT("Paper2DAssetCategory", "Paper2D"));
}

void FManualSpriteAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (auto* ManualSprite = Cast<UManualSprite>(*ObjIt))
		{
			const TSharedRef<FManualSpriteEditorToolkit> EditorToolkit = MakeShareable(new FManualSpriteEditorToolkit());
			EditorToolkit->InitManualSpriteEditor(Mode, EditWithinLevelEditor, ManualSprite);
		}
	}
}

bool FManualSpriteAssetTypeActions::HasActions(const TArray<UObject*>& InObjects) const
{
	return true;
}

void FManualSpriteAssetTypeActions::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	auto ManualSprites = GetTypedWeakObjectPtrs<UManualSprite>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ManualSprite_ResetGeometry", "Reset to Auto Geometry"),
		LOCTEXT("ManualSprite_ResetGeometryTooltip", "Disable manual geometry and use automatic triangulation"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([this, ManualSprites]()
			{
				ResetToAutoGeometry(ManualSprites);
			}),
			FCanExecuteAction()
		)
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ManualSprite_EnableManualMode", "Enable Manual Mode"),
		LOCTEXT("ManualSprite_EnableManualModeTooltip", "Enable manual geometry editing mode"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([ManualSprites]()
			{
				for (auto& WeakSprite : ManualSprites)
				{
					if (UManualSprite* Sprite = WeakSprite.Get())
					{
						Sprite->bUseManualGeometry = true;
						if (Sprite->ManualGeometry.Vertices.Num() == 0)
						{
							// Generate basic geometry if none exists
							FPropertyChangedEvent DummyEvent(nullptr);
							Sprite->PostEditChangeProperty(DummyEvent);
						}
						(void)Sprite->MarkPackageDirty();
					}
				}
			}),
			FCanExecuteAction()
		)
	);
}

void FManualSpriteAssetTypeActions::ResetToAutoGeometry(TArray<TWeakObjectPtr<UManualSprite>> Objects)
{
	const FText ConfirmText = LOCTEXT("ManualSprite_ResetGeometryConfirm", 
		"This will disable manual geometry and revert to automatic triangulation. Are you sure?");

	if (FMessageDialog::Open(EAppMsgType::YesNo, ConfirmText) == EAppReturnType::Yes)
	{
		for (auto& WeakObject : Objects)
		{
			if (UManualSprite* ManualSprite = WeakObject.Get())
			{
				ManualSprite->bUseManualGeometry = false;
				(void)ManualSprite->MarkPackageDirty();
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE