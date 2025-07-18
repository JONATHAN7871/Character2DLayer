// Copyright 2025, CRAFTCODE, All Rights Reserved.

#include "SpriteOptimizer/SpriteOptimizerActions.h"
#include "SpriteOptimizer/SSpriteOptimizationWindow.h"
#include "SpriteOptimizer/SAtlasCreationWindow.h"
#include "Framework/Commands/UICommandList.h"
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
    // Register commands
    FSpriteOptimizerCommands::Register();
    
    // Extend Content Browser menu
    ExtendContentBrowserContextMenu();
}

void FSpriteOptimizerActions::Shutdown()
{
    // Unregister commands
    FSpriteOptimizerCommands::Unregister();
}

void FSpriteOptimizerActions::ExtendContentBrowserContextMenu()
{
    // Get Content Browser menu
    UToolMenus* ToolMenus = UToolMenus::Get();
    if (!ToolMenus)
    {
        return;
    }
    
    // Extend context menu for assets
    UToolMenu* Menu = ToolMenus->ExtendMenu("ContentBrowser.AssetContextMenu");
    if (!Menu)
    {
        return;
    }
    
    // Add section at the top of menu
    FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
    
    // Add sprite optimization action
    Section.AddDynamicEntry("SpriteOptimizerDynamic", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
    {
        // Check context
        if (const UContentBrowserAssetContextMenuContext* Context = InSection.FindContext<UContentBrowserAssetContextMenuContext>())
        {
            // Check for selected sprites
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
            
            // === SPRITE OPTIMIZATION ===
            if (SpriteCount > 0)
            {
                FToolUIAction OptimizeUIAction;
                OptimizeUIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&FSpriteOptimizerActions::ExecuteOptimizeSprites);
                OptimizeUIAction.CanExecuteAction = FToolMenuCanExecuteAction::CreateStatic(&FSpriteOptimizerActions::CanExecuteOptimizeSprites);
                
                FText OptimizeMenuLabel = SpriteCount == 1 ? 
                    LOCTEXT("OptimizeSpritesSingle", "🚀 Optimize Sprite") :
                    FText::Format(LOCTEXT("OptimizeSpriteMultiple", "🚀 Optimize {0} Sprites"), SpriteCount);
                
                InSection.AddMenuEntry(
                    "OptimizeSprites",
                    OptimizeMenuLabel,
                    LOCTEXT("OptimizeSpritesTooltip", "Open the sprite optimization tool to reduce texture memory usage by removing transparent areas"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.AssetActions.Edit"),
                    OptimizeUIAction,
                    EUserInterfaceActionType::Button
                );
            }
            
            // === ATLAS CREATION ===
            if (SpriteCount > 1)
            {
                FToolUIAction AtlasUIAction;
                AtlasUIAction.ExecuteAction = FToolMenuExecuteAction::CreateLambda([SelectedSprites](const FToolMenuContext&)
                {
                    SAtlasCreationWindow::ShowAtlasCreationWindow(SelectedSprites);
                });
                
                AtlasUIAction.CanExecuteAction = FToolMenuCanExecuteAction::CreateLambda([SpriteCount](const FToolMenuContext&) -> bool
                {
                    return SpriteCount > 1;
                });
                
                InSection.AddMenuEntry(
                    "CreateSpriteAtlas",
                    FText::Format(LOCTEXT("CreateAtlasMultiple", "🎨 Create Atlas from {0} Sprites"), SpriteCount),
                    LOCTEXT("CreateAtlasTooltip", "Open advanced atlas creation window to combine selected sprites into a single optimized texture"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.AssetActions.Create"),
                    AtlasUIAction,
                    EUserInterfaceActionType::Button
                );
                
                // === QUICK ATLAS CREATION ===
                FToolUIAction QuickAtlasUIAction;
                QuickAtlasUIAction.ExecuteAction = FToolMenuExecuteAction::CreateLambda([SelectedSprites](const FToolMenuContext&)
                {
                    // Quick atlas creation with default settings
                    FSpriteAtlasSettings DefaultSettings;
                    DefaultSettings.bOptimizeSpritesFirst = true;
                    DefaultSettings.bCreateIndividualSprites = true;
                    DefaultSettings.MaxAtlasSize = FIntPoint(2048, 2048);
                    DefaultSettings.SpritePadding = 2;
                    DefaultSettings.PackingAlgorithm = EAtlasPackingAlgorithm::BestFit;
                    
                    FString AtlasName = FString::Printf(TEXT("QuickAtlas_%d_Sprites"), SelectedSprites.Num());
                    
                    // Show start notification
                    USpriteOptimizer::ShowOptimizationNotification(
                        FText::Format(LOCTEXT("QuickAtlasStarted", "🔄 Creating quick atlas from {0} sprites..."), SelectedSprites.Num()), 
                        true
                    );
                    
                    FSpriteAtlasResult Result = USpriteOptimizer::CreateSpriteAtlas(SelectedSprites, DefaultSettings, AtlasName);
                    
                    if (Result.bSuccess)
                    {
                        USpriteOptimizer::ShowOptimizationNotification(
                            FText::Format(LOCTEXT("QuickAtlasSuccess", 
                                "✅ Quick Atlas created successfully!\n"
                                "📐 Size: {0}x{1}\n"
                                "📊 Efficiency: {2}%\n"
                                "🎨 Individual sprites: {3}"),
                                Result.AtlasSize.X, Result.AtlasSize.Y,
                                FText::AsNumber(Result.PackingEfficiency),
                                Result.CreatedSprites.Num()
                            ), 
                            true
                        );
                        USpriteOptimizer::RefreshContentBrowser();
                    }
                    else
                    {
                        USpriteOptimizer::ShowOptimizationNotification(
                            FText::Format(LOCTEXT("QuickAtlasError", "❌ Quick Atlas creation failed: {0}"), 
                                         FText::FromString(Result.ErrorMessage)), 
                            false
                        );
                    }
                });
                
                InSection.AddMenuEntry(
                    "QuickCreateSpriteAtlas",
                    FText::Format(LOCTEXT("QuickCreateAtlas", "⚡ Quick Atlas ({0} sprites)"), SpriteCount),
                    LOCTEXT("QuickCreateAtlasTooltip", "Instantly create an atlas with default settings - optimize first, then pack with BestFit algorithm"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.AssetActions.Reimport"),
                    QuickAtlasUIAction,
                    EUserInterfaceActionType::Button
                );
            }
            
            // === SEPARATOR ===
            if (SpriteCount > 0)
            {
                InSection.AddSeparator("SpriteOptimizerSeparator");
            }
        }
    }));
}

void FSpriteOptimizerActions::ExecuteOptimizeSprites(const FToolMenuContext& Context)
{
    TArray<UPaperSprite*> SelectedSprites = GetSelectedSpritesFromContext(Context);
    
    if (SelectedSprites.Num() > 0)
    {
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
    
    // Check for sprites in selection
    int32 SpriteCount = CountSpritesInSelection(SelectedAssets);
    
    if (SpriteCount > 0)
    {
        Extender->AddMenuExtension(
            "GetAssetActions",
            EExtensionHook::After,
            nullptr,
            FMenuExtensionDelegate::CreateLambda([SelectedAssets, SpriteCount](FMenuBuilder& MenuBuilder)
            {
                // Collect sprites
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
                    // === OPTIMIZATION MENU ===
                    FText OptimizeMenuLabel = Sprites.Num() == 1 ? 
                        LOCTEXT("OptimizeSpritesSingle", "🚀 Optimize Sprite") :
                        FText::Format(LOCTEXT("OptimizeSpriteMultiple", "🚀 Optimize {0} Sprites"), Sprites.Num());
                    
                    MenuBuilder.AddMenuEntry(
                        OptimizeMenuLabel,
                        LOCTEXT("OptimizeSpritesTooltip", "Open the sprite optimization tool to reduce texture memory usage by removing transparent areas"),
                        FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.AssetActions.Edit"),
                        FUIAction(FExecuteAction::CreateLambda([Sprites]()
                        {
                            SSpriteOptimizationWindow::ShowOptimizationWindow(Sprites);
                        }))
                    );
                    
                    // === ATLAS MENU ===
                    if (Sprites.Num() > 1)
                    {
                        MenuBuilder.AddMenuEntry(
                            FText::Format(LOCTEXT("CreateAtlasAdvanced", "🎨 Create Atlas ({0} sprites)"), Sprites.Num()),
                            LOCTEXT("CreateAtlasAdvancedTooltip", "Open advanced atlas creation window with detailed settings"),
                            FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.AssetActions.Create"),
                            FUIAction(FExecuteAction::CreateLambda([Sprites]()
                            {
                                SAtlasCreationWindow::ShowAtlasCreationWindow(Sprites);
                            }))
                        );
                    }
                }
            })
        );
    }
    
    return Extender;
}

#undef LOCTEXT_NAMESPACE