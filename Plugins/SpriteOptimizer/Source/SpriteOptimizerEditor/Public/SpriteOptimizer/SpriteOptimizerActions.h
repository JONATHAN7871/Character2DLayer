// Copyright 2025, CRAFTCODE, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "ContentBrowserMenuContexts.h"
#include "PaperSprite.h"
#include "ToolMenuSection.h"
#include "Framework/Commands/Commands.h"

class FUICommandInfo;

// Commands for sprite optimizer
class FSpriteOptimizerCommands : public TCommands<FSpriteOptimizerCommands>
{
public:
	FSpriteOptimizerCommands();

	// Command for sprite optimization
	TSharedPtr<FUICommandInfo> OptimizeSprites;

	virtual void RegisterCommands() override;
};

// Content Browser context menu extension for Paper Sprites
class FSpriteOptimizerActions
{
public:
	static void Initialize();
	static void Shutdown();

private:
	// Content Browser menu extension
	static void ExtendContentBrowserContextMenu();
    
	// Optimization command handlers
	static void ExecuteOptimizeSprites(const struct FToolMenuContext& Context);
	static bool CanExecuteOptimizeSprites(const struct FToolMenuContext& Context);
    
	// Helper methods
	static TArray<UPaperSprite*> GetSelectedSpritesFromContext(const struct FToolMenuContext& Context);
	static bool IsAssetSprite(const FAssetData& AssetData);
	static int32 CountSpritesInSelection(const TArray<FAssetData>& SelectedAssets);
	static TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets);
};