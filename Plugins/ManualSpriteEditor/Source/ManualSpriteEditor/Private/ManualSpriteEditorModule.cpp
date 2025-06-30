#include "ManualSpriteEditorModule.h"
#include "ManualSpriteAssetTypeActions.h"
#include "ManualSpriteEditorCommands.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"

#define LOCTEXT_NAMESPACE "FManualSpriteEditorModule"

void FManualSpriteEditorModule::StartupModule()
{
	// Регистрируем команды для Undo/Redo и горячих клавиш
	FManualSpriteEditorCommands::Register();

	// Регистрируем типы ассетов
	RegisterAssetTypeActions();

	UE_LOG(LogTemp, Log, TEXT("ManualSpriteEditor module started with Undo/Redo support"));
}

void FManualSpriteEditorModule::ShutdownModule()
{
	// Отменяем регистрацию команд
	FManualSpriteEditorCommands::Unregister();

	// Отменяем регистрацию типов ассетов
	UnregisterAssetTypeActions();

	UE_LOG(LogTemp, Log, TEXT("ManualSpriteEditor module shutdown"));
}

void FManualSpriteEditorModule::RegisterAssetTypeActions()
{
	// Получаем модуль Asset Tools
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	// Создаём и регистрируем action для Manual Sprite
	ManualSpriteAssetTypeActions = MakeShareable(new FManualSpriteAssetTypeActions);
	AssetTools.RegisterAssetTypeActions(ManualSpriteAssetTypeActions.ToSharedRef());
}

void FManualSpriteEditorModule::UnregisterAssetTypeActions()
{
	// Получаем модуль Asset Tools (если он ещё загружен)
	FAssetToolsModule* AssetToolsModule = FModuleManager::GetModulePtr<FAssetToolsModule>("AssetTools");
	
	if (AssetToolsModule != nullptr)
	{
		IAssetTools& AssetTools = AssetToolsModule->Get();
		
		if (ManualSpriteAssetTypeActions.IsValid())
		{
			AssetTools.UnregisterAssetTypeActions(ManualSpriteAssetTypeActions.ToSharedRef());
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FManualSpriteEditorModule, ManualSpriteEditor)