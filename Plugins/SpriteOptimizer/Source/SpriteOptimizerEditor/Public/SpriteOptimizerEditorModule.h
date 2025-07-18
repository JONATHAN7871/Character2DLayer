// Copyright 2025, CRAFTCODE, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSpriteOptimizerEditorModule : public IModuleInterface
{
public:
	// IModuleInterface implementation
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	// Initialize UI extensions
	void InitializeMenuExtensions();
	
	// Cleanup UI extensions
	void ShutdownMenuExtensions();
};