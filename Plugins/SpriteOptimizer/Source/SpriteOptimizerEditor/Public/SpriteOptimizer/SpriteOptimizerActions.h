#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "AssetTypeActions_Base.h"
#include "ContentBrowserMenuContexts.h"
#include "ToolMenuSection.h"
#include "PaperSprite.h"

// Команды для оптимизатора спрайтов
class FSpriteOptimizerCommands : public TCommands<FSpriteOptimizerCommands>
{
public:
	FSpriteOptimizerCommands();

	// Команда для оптимизации спрайтов
	TSharedPtr<FUICommandInfo> OptimizeSprites;

	virtual void RegisterCommands() override;
};

// Расширение действий для Paper Sprite в Content Browser
class FSpriteOptimizerActions
{
public:
	static void Initialize();
	static void Shutdown();

private:
	// Добавление пунктов меню в Content Browser
	static void ExtendContentBrowserContextMenu();
    
	// Обработчики команд
	static void ExecuteOptimizeSprites(const struct FToolMenuContext& Context);
    
	// Проверка возможности выполнения команды
	static bool CanExecuteOptimizeSprites(const struct FToolMenuContext& Context);
    
	// Получение выбранных спрайтов из контекста
	static TArray<UPaperSprite*> GetSelectedSpritesFromContext(const struct FToolMenuContext& Context);
    
	// Проверка, является ли ассет спрайтом
	static bool IsAssetSprite(const FAssetData& AssetData);
    
	// Подсчет количества спрайтов среди выбранных ассетов
	static int32 CountSpritesInSelection(const TArray<FAssetData>& SelectedAssets);
    
	// Делегат для расширения меню
	static TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets);
};