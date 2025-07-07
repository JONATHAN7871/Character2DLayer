#include "ManualSpriteEditorModule.h"

#define LOCTEXT_NAMESPACE "FManualSpriteEditorModule"

void FManualSpriteEditorModule::StartupModule()
{
	// Инициализация Runtime модуля
	UE_LOG(LogTemp, Log, TEXT("ManualSpriteEditor runtime module started"));
}

void FManualSpriteEditorModule::ShutdownModule()
{
	// Очистка Runtime модуля
	UE_LOG(LogTemp, Log, TEXT("ManualSpriteEditor runtime module shutdown"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FManualSpriteEditorModule, ManualSpriteEditor)