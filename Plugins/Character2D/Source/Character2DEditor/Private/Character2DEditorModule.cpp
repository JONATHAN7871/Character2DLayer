#include "Character2DEditorModule.h"
#include "AssetToolsModule.h"
#include "ToolMenus.h"
#include "Character2DBuilderWindow/Slate/SCharacter2DBuilderWindow.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenuSection.h"
#include "ToolMenuEntry.h"
#include "IAssetTypeActions.h"
#include "Character2DAssetEditorToolkit/FAssetTypeActions_Character2DAsset.h"

static const FName Character2DTabName("Character2DBuilder");

#define LOCTEXT_NAMESPACE "FCharacter2DEditorModule"

void FCharacter2DEditorModule::StartupModule()
{
	RegisterMenus();

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	Character2DAssetCategory = AssetTools.RegisterAdvancedAssetCategory(
		FName("Character2D"),
		LOCTEXT("Character2DCategory", "Character2D")
	);

	Character2DAssetActions = MakeShared<FAssetTypeActions_Character2DAsset>(Character2DAssetCategory);
	AssetTools.RegisterAssetTypeActions(
		StaticCastSharedRef<IAssetTypeActions>(Character2DAssetActions.ToSharedRef())
	);
}

void FCharacter2DEditorModule::ShutdownModule()
{
	UToolMenus::UnregisterOwner(this);

	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		if (Character2DAssetActions.IsValid())
		{
			AssetTools.UnregisterAssetTypeActions(
				StaticCastSharedRef<IAssetTypeActions>(Character2DAssetActions.ToSharedRef())
			);
		}
	}
	Character2DAssetActions.Reset();
}

void FCharacter2DEditorModule::RegisterMenus()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FCharacter2DEditorModule::RegisterMenus_Internal)
	);
}

void FCharacter2DEditorModule::RegisterMenus_Internal()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");

	Section.AddMenuEntry(
		"OpenCharacter2DBuilder",
		LOCTEXT("Character2DBuilderLabel", "Character2D Editor"),
		LOCTEXT("Character2DBuilderTooltip", "Open the 2D Character Builder tool."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FCharacter2DEditorModule::OpenCharacter2DBuilder))
	);
}

void FCharacter2DEditorModule::OpenCharacter2DBuilder()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(Character2DTabName,
		FOnSpawnTab::CreateLambda([](const FSpawnTabArgs& Args)
		{
			return SNew(SDockTab)
				.TabRole(ETabRole::NomadTab)
				[
					SNew(SCharacter2DBuilderWindow)
				];
		}))
		.SetDisplayName(LOCTEXT("Character2DTabTitle", "Character2D Editor"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	FGlobalTabmanager::Get()->TryInvokeTab(Character2DTabName);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCharacter2DEditorModule, Character2DEditor)
