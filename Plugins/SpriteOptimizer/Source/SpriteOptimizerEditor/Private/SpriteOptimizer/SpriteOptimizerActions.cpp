#include "SpriteOptimizer/SpriteOptimizerActions.h"
#include "SpriteOptimizer/SSpriteOptimizationWindow.h"
#include "Framework/Commands/UICommandList.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ToolMenus.h"
#include "PaperSprite.h"
#include "AssetRegistry/AssetData.h"
#include "ContentBrowserMenuContexts.h"

#define LOCTEXT_NAMESPACE "SpriteOptimizerActions"

FSpriteOptimizerCommands::FSpriteOptimizerCommands()
    : TCommands<FSpriteOptimizerCommands>(
        TEXT("SpriteOptimizer"),
        NSLOCTEXT("Contexts", "SpriteOptimizer", "Sprite Optimizer"),
        NAME_None,
        FAppStyle::GetAppStyleSetName())
{
}

void FSpriteOptimizerCommands::RegisterCommands()
{
    UI_COMMAND(OptimizeSprites, "Optimize Sprites", "Open sprite optimization tool for selected sprites", EUserInterfaceActionType::Button, FInputChord());
}

void FSpriteOptimizerActions::Initialize()
{
    // Регистрируем команды
    FSpriteOptimizerCommands::Register();
    
    // Расширяем Content Browser меню
    ExtendContentBrowserContextMenu();
}

void FSpriteOptimizerActions::Shutdown()
{
    // Отменяем регистрацию команд
    FSpriteOptimizerCommands::Unregister();
}

void FSpriteOptimizerActions::ExtendContentBrowserContextMenu()
{
    // Получаем меню Content Browser
    UToolMenus* ToolMenus = UToolMenus::Get();
    if (!ToolMenus)
    {
        return;
    }
    
    // Расширяем контекстное меню для ассетов
    UToolMenu* Menu = ToolMenus->ExtendMenu("ContentBrowser.AssetContextMenu");
    if (!Menu)
    {
        return;
    }
    
    // Добавляем секцию в самый верх меню
    FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
    
    // Добавляем действие оптимизации спрайтов
    Section.AddDynamicEntry("OptimizeSpritesDynamic", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
    {
        // Проверяем контекст
        if (const UContentBrowserAssetContextMenuContext* Context = InSection.FindContext<UContentBrowserAssetContextMenuContext>())
        {
            // Проверяем есть ли выбранные спрайты
            TArray<UPaperSprite*> SelectedSprites;
            int32 SpriteCount = 0;
            
            for (const FAssetData& AssetData : Context->SelectedAssets)
            {
                if (IsAssetSprite(AssetData))
                {
                    SpriteCount++;
                    if (UPaperSprite* Sprite = Cast<UPaperSprite>(AssetData.GetAsset()))
                    {
                        SelectedSprites.Add(Sprite);
                    }
                }
            }
            
            // Если есть спрайты, добавляем пункт меню
            if (SpriteCount > 0)
            {
                FToolUIAction UIAction;
                UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&FSpriteOptimizerActions::ExecuteOptimizeSprites);
                UIAction.CanExecuteAction = FToolMenuCanExecuteAction::CreateStatic(&FSpriteOptimizerActions::CanExecuteOptimizeSprites);
                
                FText MenuLabel = SpriteCount == 1 ? 
                    LOCTEXT("OptimizeSpritesSingle", "🚀 Optimize Sprite") :
                    FText::Format(LOCTEXT("OptimizeSpriteMultiple", "🚀 Optimize {0} Sprites"), SpriteCount);
                
                InSection.AddMenuEntry(
                    "OptimizeSprites",
                    MenuLabel,
                    LOCTEXT("OptimizeSpritesTooltip", "Open the sprite optimization tool to reduce texture memory usage by removing transparent areas"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.AssetActions.Edit"),
                    UIAction,
                    EUserInterfaceActionType::Button
                );
            }
        }
    }));
}

void FSpriteOptimizerActions::ExecuteOptimizeSprites(const FToolMenuContext& Context)
{
    TArray<UPaperSprite*> SelectedSprites = GetSelectedSpritesFromContext(Context);
    
    if (SelectedSprites.Num() > 0)
    {
        // Открываем окно оптимизации
        SSpriteOptimizationWindow::ShowOptimizationWindow(SelectedSprites);
    }
}

bool FSpriteOptimizerActions::CanExecuteOptimizeSprites(const FToolMenuContext& Context)
{
    TArray<UPaperSprite*> SelectedSprites = GetSelectedSpritesFromContext(Context);
    return SelectedSprites.Num() > 0;
}

TArray<UPaperSprite*> FSpriteOptimizerActions::GetSelectedSpritesFromContext(const FToolMenuContext& Context)
{
    TArray<UPaperSprite*> SelectedSprites;
    
    if (const UContentBrowserAssetContextMenuContext* AssetContext = Context.FindContext<UContentBrowserAssetContextMenuContext>())
    {
        for (const FAssetData& AssetData : AssetContext->SelectedAssets)
        {
            if (IsAssetSprite(AssetData))
            {
                if (UPaperSprite* Sprite = Cast<UPaperSprite>(AssetData.GetAsset()))
                {
                    SelectedSprites.Add(Sprite);
                }
            }
        }
    }
    
    return SelectedSprites;
}

bool FSpriteOptimizerActions::IsAssetSprite(const FAssetData& AssetData)
{
    return AssetData.AssetClassPath == UPaperSprite::StaticClass()->GetClassPathName();
}

int32 FSpriteOptimizerActions::CountSpritesInSelection(const TArray<FAssetData>& SelectedAssets)
{
    int32 SpriteCount = 0;
    for (const FAssetData& AssetData : SelectedAssets)
    {
        if (IsAssetSprite(AssetData))
        {
            SpriteCount++;
        }
    }
    return SpriteCount;
}

TSharedRef<FExtender> FSpriteOptimizerActions::OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets)
{
    TSharedRef<FExtender> Extender = MakeShared<FExtender>();
    
    // Проверяем, есть ли среди выбранных ассетов спрайты
    int32 SpriteCount = CountSpritesInSelection(SelectedAssets);
    
    if (SpriteCount > 0)
    {
        Extender->AddMenuExtension(
            "GetAssetActions",
            EExtensionHook::After,
            nullptr,
            FMenuExtensionDelegate::CreateLambda([SelectedAssets, SpriteCount](FMenuBuilder& MenuBuilder)
            {
                // Собираем спрайты
                TArray<UPaperSprite*> Sprites;
                for (const FAssetData& AssetData : SelectedAssets)
                {
                    if (IsAssetSprite(AssetData))
                    {
                        if (UPaperSprite* Sprite = Cast<UPaperSprite>(AssetData.GetAsset()))
                        {
                            Sprites.Add(Sprite);
                        }
                    }
                }
                
                if (Sprites.Num() > 0)
                {
                    FText MenuLabel = Sprites.Num() == 1 ? 
                        LOCTEXT("OptimizeSpritesSingle", "🚀 Optimize Sprite") :
                        FText::Format(LOCTEXT("OptimizeSpriteMultiple", "🚀 Optimize {0} Sprites"), Sprites.Num());
                    
                    MenuBuilder.AddMenuEntry(
                        MenuLabel,
                        LOCTEXT("OptimizeSpritesTooltip", "Open the sprite optimization tool to reduce texture memory usage by removing transparent areas"),
                        FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.AssetActions.Edit"),
                        FUIAction(FExecuteAction::CreateLambda([Sprites]()
                        {
                            SSpriteOptimizationWindow::ShowOptimizationWindow(Sprites);
                        }))
                    );
                }
            })
        );
    }
    
    return Extender;
}

#undef LOCTEXT_NAMESPACE