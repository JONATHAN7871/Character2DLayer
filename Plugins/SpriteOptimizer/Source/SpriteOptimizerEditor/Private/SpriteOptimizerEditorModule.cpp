// Copyright 2025, CRAFTCODE, All Rights Reserved.

#include "SpriteOptimizerEditorModule.h"
#include "SpriteOptimizer/SpriteOptimizerActions.h"
#include "Settings/SpriteOptimizerSettings.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FSpriteOptimizerEditorModule"

void FSpriteOptimizerEditorModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("SpriteOptimizer: Module starting up"));
	
	// Initialize menu extensions and actions
	InitializeMenuExtensions();
	
	// Register project settings
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
	
	// Cleanup menu extensions
	ShutdownMenuExtensions();
	
	// Unregister settings
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "SpriteOptimizer");
	}
}

void FSpriteOptimizerEditorModule::InitializeMenuExtensions()
{
	// Initialize Content Browser actions
	FSpriteOptimizerActions::Initialize();
}

void FSpriteOptimizerEditorModule::ShutdownMenuExtensions()
{
	// Cleanup Content Browser actions
	FSpriteOptimizerActions::Shutdown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSpriteOptimizerEditorModule, SpriteOptimizerEditor)