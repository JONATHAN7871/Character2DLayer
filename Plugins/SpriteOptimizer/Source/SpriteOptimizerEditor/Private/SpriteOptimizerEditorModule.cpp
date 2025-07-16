#include "SpriteOptimizerEditorModule.h"
#include "SpriteOptimizer/SpriteOptimizerActions.h"
#include "Settings/SpriteOptimizerSettings.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FSpriteOptimizerEditorModule"

void FSpriteOptimizerEditorModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("SpriteOptimizer: Module starting up"));
	
	// Инициализируем действия и расширения меню
	InitializeMenuExtensions();
	
	// Регистрируем настройки проекта
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "SpriteOptimizer",
			LOCTEXT("SpriteOptimizerSettingsName", "Sprite Optimizer"),
			LOCTEXT("SpriteOptimizerSettingsDescription", "Configure sprite optimization settings"),
			GetMutableDefault<USpriteOptimizerSettings>()
		);
	}
}

void FSpriteOptimizerEditorModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("SpriteOptimizer: Module shutting down"));
	
	// Очищаем расширения меню
	ShutdownMenuExtensions();
	
	// Отменяем регистрацию настроек
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "SpriteOptimizer");
	}
}

void FSpriteOptimizerEditorModule::InitializeMenuExtensions()
{
	// Инициализируем действия для Content Browser
	FSpriteOptimizerActions::Initialize();
}

void FSpriteOptimizerEditorModule::ShutdownMenuExtensions()
{
	// Очищаем действия Content Browser
	FSpriteOptimizerActions::Shutdown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSpriteOptimizerEditorModule, SpriteOptimizerEditor)