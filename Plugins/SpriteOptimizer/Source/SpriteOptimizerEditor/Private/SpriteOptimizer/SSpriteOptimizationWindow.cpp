// Copyright 2025, CRAFTCODE, All Rights Reserved.

#include "SpriteOptimizer/SSpriteOptimizationWindow.h"
#include "SpriteOptimizer/SpriteOptimizer.h"
#include "Settings/SpriteOptimizerSettings.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SSpinBox.h"
#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Slate/Public/Widgets/Input/SCheckBox.h"

#define LOCTEXT_NAMESPACE "SSpriteOptimizationWindow"

void SSpriteOptimizationWindow::Construct(const FArguments& InArgs)
{
    // Initialize default settings
    CurrentSettings.Material = USpriteOptimizer::GetDefaultPaper2DMaterial();
    CurrentSettings.PixelsPerUnit = 1.0f;
    CurrentSettings.Padding = 2;
    CurrentSettings.bCreateBackup = true;
    CurrentSettings.bReplaceOriginals = false;
    CurrentSettings.bUseProjectSettings = true;
    
    LoadSettingsFromProject();
    InitializeMaterialOptions();
    
    // Create sprite rows
    for (UPaperSprite* Sprite : InArgs._SpritesToOptimize)
    {
        if (Sprite)
        {
            TSharedPtr<FSpriteOptimizationRow> Row = MakeShared<FSpriteOptimizationRow>(Sprite);
            SpriteRows.Add(Row);
        }
    }
    
    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(10)
        [
            SNew(SScrollBox)
            .Orientation(Orient_Vertical)
            
            + SScrollBox::Slot()
            [
                SNew(SVerticalBox)
                
                // Header
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    CreateHeaderSection()
                ]
                
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 2)
                [
                    SNew(SSeparator)
                ]
                
                // Settings
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    CreateCompactSettingsSection()
                ]
                
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 2)
                [
                    SNew(SSeparator)
                ]
                
                // Sprite list
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    CreateCompactSpriteListSection()
                ]
                
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 2)
                [
                    SNew(SSeparator)
                ]
                
                // Actions
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    CreateCompactActionSection()
                ]
            ]
        ]
    ];
    
    RefreshAnalysis();
}

SSpriteOptimizationWindow::~SSpriteOptimizationWindow()
{
    SpriteRows.Empty();
    OptimizationResults.Empty();
    UE_LOG(LogTemp, Log, TEXT("SpriteOptimizationWindow destroyed"));
}

void SSpriteOptimizationWindow::ShowOptimizationWindow(const TArray<UPaperSprite*>& Sprites)
{
    if (Sprites.Num() == 0)
    {
        return;
    }
    
    TSharedRef<SWindow> OptimizationWindow = SNew(SWindow)
        .Title(FText::Format(LOCTEXT("OptimizationWindowTitle", "Sprite Optimization - {0} sprites"), Sprites.Num()))
        .SizingRule(ESizingRule::UserSized)
        .ClientSize(FVector2D(1200, 800))
        .SupportsMaximize(true)
        .SupportsMinimize(false)
        .Content()
        [
            SNew(SSpriteOptimizationWindow)
            .SpritesToOptimize(Sprites)
        ];
    
    FSlateApplication::Get().AddWindow(OptimizationWindow);
}

TSharedRef<SWidget> SSpriteOptimizationWindow::CreateHeaderSection()
{
    return SNew(SVerticalBox)
        
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(STextBlock)
            .Text(LOCTEXT("OptimizationTitle", "🚀 Sprite Optimization"))
            .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
            .Justification(ETextJustify::Center)
        ]
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 5)
        [
            SNew(STextBlock)
            .Text(FText::Format(LOCTEXT("OptimizationSubtitle", 
                "Remove transparent areas from {0} sprites to reduce memory usage"), 
                SpriteRows.Num()))
            .Justification(ETextJustify::Center)
            .AutoWrapText(true)
            .Font(FAppStyle::GetFontStyle("SmallFont"))
        ];
}

TSharedRef<SWidget> SSpriteOptimizationWindow::CreateCompactSettingsSection()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
        .Padding(8)
        [
            SNew(SVerticalBox)
            
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(LOCTEXT("SettingsSection", "⚙️ Settings"))
                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 8)
            [
                SNew(SHorizontalBox)
                
                // Material
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(0, 0, 5, 0)
                [
                    SNew(SVerticalBox)
                    
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("MaterialLabel", "Material:"))
                        .Font(FAppStyle::GetFontStyle("SmallFont"))
                    ]
                    
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 2)
                    [
                        SAssignNew(MaterialComboBox, SComboBox<TSharedPtr<FString>>)
                        .OptionsSource(&MaterialOptions)
                        .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                        {
                            return SNew(STextBlock).Text(FText::FromString(*Item)).Font(FAppStyle::GetFontStyle("SmallFont"));
                        })
                        .OnSelectionChanged(this, &SSpriteOptimizationWindow::OnMaterialComboChanged)
                        .Content()
                        [
                            SNew(STextBlock)
                            .Text_Lambda([this]()
                            {
                                if (CurrentSettings.Material)
                                {
                                    return FText::FromString(CurrentSettings.Material->GetName());
                                }
                                return LOCTEXT("DefaultMaterial", "Default");
                            })
                            .Font(FAppStyle::GetFontStyle("SmallFont"))
                        ]
                    ]
                ]
                
                // Pixels Per Unit
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(5, 0)
                [
                    SNew(SVerticalBox)
                    
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("PixelsPerUnitLabel", "Pixels Per Unit:"))
                        .Font(FAppStyle::GetFontStyle("SmallFont"))
                    ]
                    
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 2)
                    [
                        SAssignNew(PixelsPerUnitSpinBox, SSpinBox<float>)
                        .Value(CurrentSettings.PixelsPerUnit)
                        .MinValue(0.1f)
                        .MaxValue(100.0f)
                        .Delta(0.1f)
                        .OnValueChanged(this, &SSpriteOptimizationWindow::OnPixelsPerUnitChanged)
                    ]
                ]
                
                // Padding
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(5, 0, 0, 0)
                [
                    SNew(SVerticalBox)
                    
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("PaddingLabel", "Padding:"))
                        .Font(FAppStyle::GetFontStyle("SmallFont"))
                    ]
                    
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 2)
                    [
                        SAssignNew(PaddingSpinBox, SSpinBox<int32>)
                        .Value(CurrentSettings.Padding)
                        .MinValue(0)
                        .MaxValue(20)
                        .OnValueChanged(this, &SSpriteOptimizationWindow::OnPaddingChanged)
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> SSpriteOptimizationWindow::CreateCompactSpriteListSection()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
        .Padding(8)
        [
            SNew(SVerticalBox)
            
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SHorizontalBox)
                
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("SpritesSection", "📋 Sprites"))
                    .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(3, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("SelectAll", "All"))
                    .OnClicked(this, &SSpriteOptimizationWindow::OnSelectAll)
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(3, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("SelectNone", "None"))
                    .OnClicked(this, &SSpriteOptimizationWindow::OnSelectNone)
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(3, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("SelectOptimal", "Best"))
                    .OnClicked(this, &SSpriteOptimizationWindow::OnSelectOptimal)
                    .ToolTipText(LOCTEXT("SelectOptimalTooltip", "Select sprites with good optimization potential"))
                ]
            ]
            
            // Summary
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SAssignNew(SummaryText, STextBlock)
                .Text(GetCompactSummaryText())
                .AutoWrapText(true)
                .Font(FAppStyle::GetFontStyle("SmallFont"))
                .ColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)))
            ]
            
            // Sprite list
            + SVerticalBox::Slot()
            .MaxHeight(200.0f)
            .Padding(0, 5)
            [
                SAssignNew(SpriteListView, SListView<TSharedPtr<FSpriteOptimizationRow>>)
                .ListItemsSource(&SpriteRows)
                .OnGenerateRow(this, &SSpriteOptimizationWindow::GenerateCompactSpriteRow)
                .OnSelectionChanged(this, &SSpriteOptimizationWindow::OnSpriteSelectionChanged)
                .SelectionMode(ESelectionMode::Multi)
                .HeaderRow
                (
                    SNew(SHeaderRow)
                    
                    + SHeaderRow::Column("Selected")
                    .DefaultLabel(LOCTEXT("SelectedHeader", ""))
                    .FixedWidth(25)
                    
                    + SHeaderRow::Column("SpriteName")
                    .DefaultLabel(LOCTEXT("SpriteNameHeader", "Sprite"))
                    .FillWidth(0.4f)
                    
                    + SHeaderRow::Column("OriginalSize")
                    .DefaultLabel(LOCTEXT("OriginalSizeHeader", "Original"))
                    .FillWidth(0.2f)
                    
                    + SHeaderRow::Column("OptimizedSize")
                    .DefaultLabel(LOCTEXT("OptimizedSizeHeader", "Optimized"))
                    .FillWidth(0.2f)
                    
                    + SHeaderRow::Column("Savings")
                    .DefaultLabel(LOCTEXT("SavingsHeader", "Savings"))
                    .FillWidth(0.2f)
                )
            ]
        ];
}

TSharedRef<SWidget> SSpriteOptimizationWindow::CreateCompactActionSection()
{
    return SNew(SHorizontalBox)
        
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(3)
        [
            SNew(SButton)
            .Text(LOCTEXT("RefreshAnalysis", "🔄 Refresh"))
            .OnClicked(this, &SSpriteOptimizationWindow::OnAnalyzeSprites)
            .HAlign(HAlign_Center)
            .ToolTipText(LOCTEXT("RefreshAnalysisTooltip", "Re-analyze all sprites"))
        ]
        
        + SHorizontalBox::Slot()
        .FillWidth(2.0f)
        .Padding(3)
        [
            SAssignNew(OptimizeButton, SButton)
            .Text(LOCTEXT("OptimizeSprites", "🚀 Optimize Selected"))
            .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
            .OnClicked(this, &SSpriteOptimizationWindow::OnOptimizeSprites)
            .HAlign(HAlign_Center)
            .ToolTipText(LOCTEXT("OptimizeTooltip", "Create optimized versions of selected sprites"))
        ];
}

TSharedRef<ITableRow> SSpriteOptimizationWindow::GenerateCompactSpriteRow(TSharedPtr<FSpriteOptimizationRow> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(STableRow<TSharedPtr<FSpriteOptimizationRow>>, OwnerTable)
        [
            SNew(SHorizontalBox)
            
            // Checkbox
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .HAlign(HAlign_Center)
            .Padding(3, 0)
            [
                SNew(SCheckBox)
                .IsChecked(Item->bSelected ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                .OnCheckStateChanged_Lambda([Item, this](ECheckBoxState NewState)
                {
                    Item->bSelected = (NewState == ECheckBoxState::Checked);
                    UpdateSummary();
                    UpdateButtonStates();
                })
            ]
            
            // Sprite name
            + SHorizontalBox::Slot()
            .FillWidth(0.4f)
            .VAlign(VAlign_Center)
            .Padding(3, 2)
            [
                SNew(STextBlock)
                .Text_Lambda([Item]()
                {
                    if (Item->OriginalSprite)
                    {
                        FString Name = Item->OriginalSprite->GetName();
                        Name = Name.Replace(TEXT("T_"), TEXT(""));
                        Name = Name.Replace(TEXT("_Sprite"), TEXT(""));
                        if (Name.Len() > 12)
                        {
                            return FText::FromString(Name.Left(9) + TEXT("..."));
                        }
                        return FText::FromString(Name);
                    }
                    return FText::FromString(TEXT("Unknown"));
                })
                .Font(FAppStyle::GetFontStyle("SmallFont"))
                .ToolTipText_Lambda([Item]()
                {
                    return Item->OriginalSprite ? FText::FromString(Item->OriginalSprite->GetName()) : FText::FromString(TEXT("Unknown sprite"));
                })
            ]
            
            // Original size
            + SHorizontalBox::Slot()
            .FillWidth(0.2f)
            .VAlign(VAlign_Center)
            .Padding(3, 2)
            [
                SNew(STextBlock)
                .Text_Lambda([Item]()
                {
                    if (Item->AnalysisResult.bSuccess)
                    {
                        return FText::FromString(FString::Printf(TEXT("%.0fx%.0f"), 
                            Item->AnalysisResult.OriginalSize.X, Item->AnalysisResult.OriginalSize.Y));
                    }
                    return FText::FromString(TEXT("N/A"));
                })
                .Font(FAppStyle::GetFontStyle("SmallFont"))
            ]
            
            // Optimized size
            + SHorizontalBox::Slot()
            .FillWidth(0.2f)
            .VAlign(VAlign_Center)
            .Padding(3, 2)
            [
                SNew(STextBlock)
                .Text_Lambda([Item]()
                {
                    if (Item->AnalysisResult.bSuccess)
                    {
                        return FText::FromString(FString::Printf(TEXT("%.0fx%.0f"), 
                            Item->AnalysisResult.OptimizedSize.X, Item->AnalysisResult.OptimizedSize.Y));
                    }
                    return FText::FromString(TEXT("N/A"));
                })
                .Font(FAppStyle::GetFontStyle("SmallFont"))
                .ColorAndOpacity(FSlateColor(FLinearColor::Green))
            ]
            
            // Savings
            + SHorizontalBox::Slot()
            .FillWidth(0.2f)
            .VAlign(VAlign_Center)
            .Padding(3, 2)
            [
                SNew(SHorizontalBox)
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(STextBlock)
                    .Text_Lambda([Item]()
                    {
                        if (Item->AnalysisResult.bSuccess)
                        {
                            return FText::FromString(FString::Printf(TEXT("%.0f%%"), Item->AnalysisResult.SavingsPercent));
                        }
                        return FText::FromString(TEXT("N/A"));
                    })
                    .ColorAndOpacity_Lambda([Item]()
                    {
                        if (!Item->AnalysisResult.bSuccess) return FSlateColor(FLinearColor::Gray);
                        if (Item->AnalysisResult.SavingsPercent > 70.0f) return FSlateColor(FLinearColor::Green);
                        if (Item->AnalysisResult.SavingsPercent > 40.0f) return FSlateColor(FLinearColor::Yellow);
                        return FSlateColor(FLinearColor::White);
                    })
                    .Font(FAppStyle::GetFontStyle("SmallFont"))
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(3, 0, 0, 0)
                [
                    SNew(STextBlock)
                    .Text_Lambda([Item]()
                    {
                        if (!Item->AnalysisResult.bSuccess)
                        {
                            return FText::FromString(TEXT("ERR"));
                        }
                        else if (Item->AnalysisResult.SavingsPercent > 70.0f)
                        {
                            return FText::FromString(TEXT("GREAT"));
                        }
                        else if (Item->AnalysisResult.SavingsPercent > 40.0f)
                        {
                            return FText::FromString(TEXT("GOOD"));
                        }
                        else
                        {
                            return FText::FromString(TEXT("POOR"));
                        }
                    })
                    .ColorAndOpacity_Lambda([Item]()
                    {
                        if (!Item->AnalysisResult.bSuccess) return FSlateColor(FLinearColor::Red);
                        if (Item->AnalysisResult.SavingsPercent > 70.0f) return FSlateColor(FLinearColor::Green);
                        if (Item->AnalysisResult.SavingsPercent > 40.0f) return FSlateColor(FLinearColor::Yellow);
                        return FSlateColor(FLinearColor::Red);
                    })
                    .Font(FAppStyle::GetFontStyle("SmallFont"))
                ]
            ]
        ];
}

void SSpriteOptimizationWindow::OnSpriteSelectionChanged(TSharedPtr<FSpriteOptimizationRow> Item, ESelectInfo::Type SelectInfo)
{
    UpdateSummary();
    UpdateButtonStates();
}

void SSpriteOptimizationWindow::InitializeMaterialOptions()
{
    MaterialOptions.Empty();
    MaterialAssets.Empty();
    
    TArray<UMaterialInterface*> AvailableMaterials = USpriteOptimizer::GetAvailablePaper2DMaterials();
    
    for (UMaterialInterface* Material : AvailableMaterials)
    {
        if (Material)
        {
            MaterialAssets.Add(Material);
            MaterialOptions.Add(MakeShared<FString>(Material->GetName()));
        }
    }
    
    MaterialAssets.Insert(nullptr, 0);
    MaterialOptions.Insert(MakeShared<FString>(TEXT("Default Paper2D Material")), 0);
}

void SSpriteOptimizationWindow::LoadSettingsFromProject()
{
    if (CurrentSettings.bUseProjectSettings)
    {
        CurrentSettings.LoadFromProjectSettings();
        
        if (MaterialComboBox.IsValid() && CurrentSettings.Material)
        {
            for (int32 i = 0; i < MaterialAssets.Num(); i++)
            {
                if (MaterialAssets[i] == CurrentSettings.Material)
                {
                    MaterialComboBox->SetSelectedItem(MaterialOptions[i]);
                    break;
                }
            }
        }
        
        if (PixelsPerUnitSpinBox.IsValid())
        {
            PixelsPerUnitSpinBox->SetValue(CurrentSettings.PixelsPerUnit);
        }
        if (PaddingSpinBox.IsValid())
        {
            PaddingSpinBox->SetValue(CurrentSettings.Padding);
        }
        if (CreateBackupCheckBox.IsValid())
        {
            CreateBackupCheckBox->SetIsChecked(CurrentSettings.bCreateBackup ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
        }
        if (ReplaceOriginalsCheckBox.IsValid())
        {
            ReplaceOriginalsCheckBox->SetIsChecked(CurrentSettings.bReplaceOriginals ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
        }
    }
}

FReply SSpriteOptimizationWindow::OnAnalyzeSprites()
{
    ShowNotification(LOCTEXT("AnalyzingSprites", "Analyzing sprites..."), 0);
    
    // Re-analyze all sprites
    for (auto& Row : SpriteRows)
    {
        if (Row->OriginalSprite)
        {
            Row->AnalysisResult = USpriteOptimizer::AnalyzeSprite(Row->OriginalSprite.Get());
        }
    }
    
    if (SpriteListView.IsValid())
    {
        SpriteListView->RequestListRefresh();
    }
    
    UpdateSummary();
    UpdateButtonStates();
    
    ShowNotification(LOCTEXT("AnalysisComplete", "Analysis complete!"), 1);
    
    return FReply::Handled();
}

FReply SSpriteOptimizationWindow::OnOptimizeSprites()
{
    TArray<TSharedPtr<FSpriteOptimizationRow>> SelectedSprites = GetSelectedSprites();
    
    if (SelectedSprites.Num() == 0)
    {
        ShowNotification(LOCTEXT("NoSpritesSelected", "Please select sprites to optimize"), 2);
        return FReply::Handled();
    }
    
    FText ProcessingText = FText::Format(LOCTEXT("OptimizingSprites", "Optimizing {0} sprites..."), SelectedSprites.Num());
    ShowNotification(ProcessingText, 0);
    
    // Collect sprites for optimization
    TArray<UPaperSprite*> SpritesToOptimize;
    for (const auto& Row : SelectedSprites)
    {
        if (Row->OriginalSprite && Row->AnalysisResult.bSuccess)
        {
            SpritesToOptimize.Add(Row->OriginalSprite.Get());
        }
    }
    
    // Perform optimization
    OptimizationResults = USpriteOptimizer::OptimizeSprites(SpritesToOptimize, CurrentSettings);
    
    // Count successful results
    int32 SuccessCount = 0;
    for (const auto& Result : OptimizationResults)
    {
        if (Result.bSuccess)
        {
            SuccessCount++;
        }
    }
    
    UpdateButtonStates();
    
    FText CompletionText = FText::Format(LOCTEXT("OptimizationComplete", "✅ Successfully optimized {0} out of {1} sprites"), 
                                        SuccessCount, OptimizationResults.Num());
    ShowNotification(CompletionText, 1);
    
    // Close window after successful optimization
    if (SuccessCount > 0)
    {
        FTimerHandle TimerHandle;
        FTimerDelegate TimerDelegate;
        TimerDelegate.BindLambda([this]()
        {
            SpriteRows.Empty();
            OptimizationResults.Empty();
            
            TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
            if (ParentWindow.IsValid())
            {
                ParentWindow->RequestDestroyWindow();
            }
        });
        
        GEditor->GetTimerManager()->SetTimer(TimerHandle, TimerDelegate, 2.0f, false);
    }
    
    return FReply::Handled();
}

FReply SSpriteOptimizationWindow::OnSelectAll()
{
    for (auto& Row : SpriteRows)
    {
        Row->bSelected = true;
    }
    
    if (SpriteListView.IsValid())
    {
        SpriteListView->RequestListRefresh();
    }
    
    UpdateSummary();
    UpdateButtonStates();
    
    return FReply::Handled();
}

FReply SSpriteOptimizationWindow::OnSelectNone()
{
    for (auto& Row : SpriteRows)
    {
        Row->bSelected = false;
    }
    
    if (SpriteListView.IsValid())
    {
        SpriteListView->RequestListRefresh();
    }
    
    UpdateSummary();
    UpdateButtonStates();
    
    return FReply::Handled();
}

FReply SSpriteOptimizationWindow::OnSelectOptimal()
{
    const USpriteOptimizerSettings* ProjectSettings = GetDefault<USpriteOptimizerSettings>();
    float MinSavings = ProjectSettings ? ProjectSettings->MinimumSavingsForAutoSelect : 30.0f;
    
    for (auto& Row : SpriteRows)
    {
        Row->bSelected = Row->AnalysisResult.bSuccess && Row->AnalysisResult.SavingsPercent > MinSavings;
    }
    
    if (SpriteListView.IsValid())
    {
        SpriteListView->RequestListRefresh();
    }
    
    UpdateSummary();
    UpdateButtonStates();
    
    return FReply::Handled();
}

FReply SSpriteOptimizationWindow::OnOpenProjectSettings()
{
    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
    {
        SettingsModule->ShowViewer("Project", "Plugins", "SpriteOptimizer");
    }
    return FReply::Handled();
}

FReply SSpriteOptimizationWindow::OnResetToDefaults()
{
    CurrentSettings = FSpriteOptimizationSettings();
    CurrentSettings.Material = USpriteOptimizer::GetDefaultPaper2DMaterial();
    
    if (MaterialComboBox.IsValid())
    {
        MaterialComboBox->SetSelectedItem(MaterialOptions[0]);
    }
    
    if (PixelsPerUnitSpinBox.IsValid())
    {
        PixelsPerUnitSpinBox->SetValue(CurrentSettings.PixelsPerUnit);
    }
    if (PaddingSpinBox.IsValid())
    {
        PaddingSpinBox->SetValue(CurrentSettings.Padding);
    }
    if (CreateBackupCheckBox.IsValid())
    {
        CreateBackupCheckBox->SetIsChecked(CurrentSettings.bCreateBackup ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
    }
    if (ReplaceOriginalsCheckBox.IsValid())
    {
        ReplaceOriginalsCheckBox->SetIsChecked(CurrentSettings.bReplaceOriginals ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
    }
    if (UseProjectSettingsCheckBox.IsValid())
    {
        UseProjectSettingsCheckBox->SetIsChecked(CurrentSettings.bUseProjectSettings ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
    }
    
    RefreshAnalysis();
    return FReply::Handled();
}

void SSpriteOptimizationWindow::OnMaterialComboChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo)
{
    if (SelectedItem.IsValid())
    {
        int32 Index = MaterialOptions.IndexOfByPredicate([SelectedItem](const TSharedPtr<FString>& Item)
        {
            return Item.IsValid() && *Item == *SelectedItem;
        });
        
        if (Index != INDEX_NONE && MaterialAssets.IsValidIndex(Index))
        {
            CurrentSettings.Material = MaterialAssets[Index];
            if (!CurrentSettings.Material)
            {
                CurrentSettings.Material = USpriteOptimizer::GetDefaultPaper2DMaterial();
            }
        }
    }
}

void SSpriteOptimizationWindow::OnPixelsPerUnitChanged(float NewValue)
{
    CurrentSettings.PixelsPerUnit = NewValue;
}

void SSpriteOptimizationWindow::OnPaddingChanged(int32 NewValue)
{
    CurrentSettings.Padding = NewValue;
    RefreshAnalysis();
}

void SSpriteOptimizationWindow::OnCreateBackupChanged(ECheckBoxState NewState)
{
    CurrentSettings.bCreateBackup = (NewState == ECheckBoxState::Checked);
}

void SSpriteOptimizationWindow::OnReplaceOriginalsChanged(ECheckBoxState NewState)
{
    CurrentSettings.bReplaceOriginals = (NewState == ECheckBoxState::Checked);
}

void SSpriteOptimizationWindow::OnUseProjectSettingsChanged(ECheckBoxState NewState)
{
    CurrentSettings.bUseProjectSettings = (NewState == ECheckBoxState::Checked);
    if (CurrentSettings.bUseProjectSettings)
    {
        LoadSettingsFromProject();
    }
}

void SSpriteOptimizationWindow::UpdateSummary()
{
    if (SummaryText.IsValid())
    {
        SummaryText->SetText(GetSummaryText());
    }
}

void SSpriteOptimizationWindow::UpdateButtonStates()
{
    TArray<TSharedPtr<FSpriteOptimizationRow>> SelectedSprites = GetSelectedSprites();
    bool bHasSelection = SelectedSprites.Num() > 0;
    
    if (OptimizeButton.IsValid())
    {
        OptimizeButton->SetEnabled(bHasSelection);
    }
}

TArray<TSharedPtr<FSpriteOptimizationRow>> SSpriteOptimizationWindow::GetSelectedSprites() const
{
    TArray<TSharedPtr<FSpriteOptimizationRow>> SelectedSprites;
    
    for (const auto& Row : SpriteRows)
    {
        if (Row->bSelected)
        {
            SelectedSprites.Add(Row);
        }
    }
    
    return SelectedSprites;
}

void SSpriteOptimizationWindow::RefreshAnalysis()
{
    OnAnalyzeSprites();
}

void SSpriteOptimizationWindow::ShowNotification(const FText& Message, int32 State)
{
    FString StateString;
    switch (State)
    {
        case 0: StateString = TEXT("PENDING"); break;
        case 1: StateString = TEXT("SUCCESS"); break;
        case 2: StateString = TEXT("FAIL"); break;
        default: StateString = TEXT("INFO"); break;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Sprite Optimizer [%s]: %s"), *StateString, *Message.ToString());
}

FText SSpriteOptimizationWindow::GetCompactSummaryText() const
{
    TArray<TSharedPtr<FSpriteOptimizationRow>> SelectedSprites = GetSelectedSprites();
    
    if (SelectedSprites.Num() == 0)
    {
        return LOCTEXT("NoSelection", "Select sprites to optimize");
    }
    
    float TotalSavingsMB = 0;
    int32 ValidSprites = 0;
    
    for (const auto& Row : SelectedSprites)
    {
        if (Row->AnalysisResult.bSuccess)
        {
            TotalSavingsMB += (Row->AnalysisResult.OriginalSizeMB - Row->AnalysisResult.OptimizedSizeMB);
            ValidSprites++;
        }
    }
    
    return FText::Format(LOCTEXT("CompactSummaryFormat",
        "{0} selected • {1} MB savings • Ready to optimize"),
        ValidSprites,
        FText::AsNumber(TotalSavingsMB, &FNumberFormattingOptions::DefaultWithGrouping())
    );
}

FText SSpriteOptimizationWindow::GetSummaryText() const
{
    TArray<TSharedPtr<FSpriteOptimizationRow>> SelectedSprites = GetSelectedSprites();
    
    if (SelectedSprites.Num() == 0)
    {
        return LOCTEXT("NoSelection", "Select sprites to see optimization summary");
    }
    
    float TotalOriginalMB = 0;
    float TotalOptimizedMB = 0;
    int32 ValidSprites = 0;
    
    for (const auto& Row : SelectedSprites)
    {
        if (Row->AnalysisResult.bSuccess)
        {
            TotalOriginalMB += Row->AnalysisResult.OriginalSizeMB;
            TotalOptimizedMB += Row->AnalysisResult.OptimizedSizeMB;
            ValidSprites++;
        }
    }
    
    if (ValidSprites == 0)
    {
        return LOCTEXT("NoValidSprites", "No valid sprites selected for optimization");
    }
    
    float TotalSavingsMB = TotalOriginalMB - TotalOptimizedMB;
    float SavingsPercent = TotalOriginalMB > 0 ? (TotalSavingsMB / TotalOriginalMB) * 100.0f : 0.0f;
    
    return FText::Format(LOCTEXT("SummaryFormat",
        "📊 Selected: {0} sprites\n"
        "💾 Current total size: {1} MB\n"
        "🚀 Optimized total size: {2} MB\n"
        "💰 Potential savings: {3} MB ({4}%)\n"
        "⚡ Performance improvement: ~{5}x faster loading"),
        ValidSprites,
        FText::AsNumber(TotalOriginalMB, &FNumberFormattingOptions::DefaultWithGrouping()),
        FText::AsNumber(TotalOptimizedMB, &FNumberFormattingOptions::DefaultWithGrouping()),
        FText::AsNumber(TotalSavingsMB, &FNumberFormattingOptions::DefaultWithGrouping()),
        FText::AsNumber(SavingsPercent, &FNumberFormattingOptions::DefaultWithGrouping()),
        FMath::Max(1, FMath::RoundToInt(SavingsPercent / 15.0f))
    );
}

#undef LOCTEXT_NAMESPACE