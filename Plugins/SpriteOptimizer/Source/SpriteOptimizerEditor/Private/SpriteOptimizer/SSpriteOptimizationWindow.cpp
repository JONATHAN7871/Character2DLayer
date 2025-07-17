#include "SpriteOptimizer/SSpriteOptimizationWindow.h"
#include "SpriteOptimizer/SpriteOptimizer.h"
#include "Settings/SpriteOptimizerSettings.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SSpinBox.h"
#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "PropertyCustomizationHelpers.h"
#include "Misc/DateTime.h"
#include "Framework/Docking/TabManager.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Slate/Public/Widgets/Input/SCheckBox.h"
#include "Runtime/SlateCore/Public/Types/SlateEnums.h"
#include "Runtime/SlateCore/Public/Widgets/DeclarativeSyntaxSupport.h"
#include "Runtime/UMG/Public/Components/HorizontalBox.h"

#define LOCTEXT_NAMESPACE "SSpriteOptimizationWindow"

void SSpriteOptimizationWindow::Construct(const FArguments& InArgs)
{
    // Инициализируем настройки по умолчанию
    CurrentSettings.Material = USpriteOptimizer::GetDefaultPaper2DMaterial();
    CurrentSettings.PixelsPerUnit = 1.0f;
    CurrentSettings.Padding = 2;
    CurrentSettings.bCreateBackup = true;
    CurrentSettings.bReplaceOriginals = false;
    CurrentSettings.bUseProjectSettings = true;
    
    // Загружаем настройки из проекта
    LoadSettingsFromProject();
    
    // Инициализируем список материалов
    InitializeMaterialOptions();
    
    // Создаем строки для спрайтов
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
            SNew(SVerticalBox)
            
            // Заголовок
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
            
            // Настройки оптимизации
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                CreateSettingsSection()
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 2)
            [
                SNew(SSeparator)
            ]
            
            // Список спрайтов
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(0, 5)
            [
                CreateSpriteListSection()
            ]
            
            // Сводка
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                CreateSummarySection()
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 2)
            [
                SNew(SSeparator)
            ]
            
            // Действия
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                CreateActionSection()
            ]
        ]
    ];
    
    // Начальное обновление
    RefreshAnalysis();
}

SSpriteOptimizationWindow::~SSpriteOptimizationWindow()
{
    // Принудительно очищаем все ссылки при уничтожении окна
    SpriteRows.Empty();
    OptimizationResults.Empty();
    
    UE_LOG(LogTemp, Log, TEXT("SpriteOptimizationWindow destroyed and cleaned up"));
}

void SSpriteOptimizationWindow::InitializeMaterialOptions()
{
    MaterialOptions.Empty();
    MaterialAssets.Empty();
    
    // Получаем доступные материалы
    TArray<UMaterialInterface*> AvailableMaterials = USpriteOptimizer::GetAvailablePaper2DMaterials();
    
    for (UMaterialInterface* Material : AvailableMaterials)
    {
        if (Material)
        {
            MaterialAssets.Add(Material);
            MaterialOptions.Add(MakeShared<FString>(Material->GetName()));
        }
    }
    
    // Добавляем опцию "Default"
    MaterialAssets.Insert(nullptr, 0);
    MaterialOptions.Insert(MakeShared<FString>(TEXT("Default Paper2D Material")), 0);
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

void SSpriteOptimizationWindow::ShowOptimizationWindow(const TArray<UPaperSprite*>& Sprites)
{
    if (Sprites.Num() == 0)
    {
        return;
    }
    
    TSharedRef<SWindow> OptimizationWindow = SNew(SWindow)
        .Title(FText::Format(LOCTEXT("OptimizationWindowTitle", "Sprite Optimization - {0} sprites"), Sprites.Num()))
        .SizingRule(ESizingRule::UserSized)
        .ClientSize(FVector2D(1200, 800))  // Уменьшена высота, так как убрали секцию атласа
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
            .Text(LOCTEXT("OptimizationTitle", "🚀 Sprite Optimization Tool"))
            .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
            .Justification(ETextJustify::Center)
        ]
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 5)
        [
            SNew(STextBlock)
            .Text(FText::Format(LOCTEXT("OptimizationSubtitle", 
                "Optimize {0} selected sprites by removing transparent areas and creating efficient assets.\n"
                "💡 For combining sprites into atlases, use the separate 'Create Atlas' option."), 
                SpriteRows.Num()))
            .Justification(ETextJustify::Center)
            .AutoWrapText(true)
        ];
}

TSharedRef<SWidget> SSpriteOptimizationWindow::CreateSettingsSection()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
        .Padding(10)
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
                    .Text(LOCTEXT("SettingsSection", "⚙️ Optimization Settings"))
                    .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(5, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("ProjectSettings", "Project Settings"))
                    .OnClicked(this, &SSpriteOptimizationWindow::OnOpenProjectSettings)
                    .ToolTipText(LOCTEXT("ProjectSettingsTooltip", "Open project settings for Sprite Optimizer"))
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(5, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("ResetDefaults", "Reset"))
                    .OnClicked(this, &SSpriteOptimizationWindow::OnResetToDefaults)
                    .ToolTipText(LOCTEXT("ResetDefaultsTooltip", "Reset settings to default values"))
                ]
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10)
            [
                SAssignNew(UseProjectSettingsCheckBox, SCheckBox)
                .IsChecked(CurrentSettings.bUseProjectSettings ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                .OnCheckStateChanged(this, &SSpriteOptimizationWindow::OnUseProjectSettingsChanged)
                .Content()
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("UseProjectSettingsLabel", "Use Project Settings as Default"))
                    .ToolTipText(LOCTEXT("UseProjectSettingsTooltip", "Load default values from project settings"))
                ]
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10)
            [
                SNew(SUniformGridPanel)
                .SlotPadding(FMargin(5, 2))
                
                // Материал
                + SUniformGridPanel::Slot(0, 0)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("MaterialLabel", "Material:"))
                    .ToolTipText(LOCTEXT("MaterialTooltip", "Material to use for optimized sprites"))
                ]

                + SUniformGridPanel::Slot(1, 0)
                [
                    SAssignNew(MaterialComboBox, SComboBox<TSharedPtr<FString>>)
                    .OptionsSource(&MaterialOptions)
                    .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                    {
                        return SNew(STextBlock).Text(FText::FromString(*Item));
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
                            return LOCTEXT("NoMaterial", "Select Material");
                        })
                    ]
                    .ToolTipText(LOCTEXT("MaterialTooltip", "Material to use for optimized sprites"))
                ]
                
                // Pixels Per Unit
                + SUniformGridPanel::Slot(0, 1)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("PixelsPerUnitLabel", "Pixels Per Unit:"))
                    .ToolTipText(LOCTEXT("PixelsPerUnitTooltip", "How many pixels in the sprite correspond to one Unreal unit"))
                ]
                
                + SUniformGridPanel::Slot(1, 1)
                [
                    SAssignNew(PixelsPerUnitSpinBox, SSpinBox<float>)
                    .Value(CurrentSettings.PixelsPerUnit)
                    .MinValue(0.1f)
                    .MaxValue(100.0f)
                    .Delta(0.1f)
                    .OnValueChanged(this, &SSpriteOptimizationWindow::OnPixelsPerUnitChanged)
                    .ToolTipText(LOCTEXT("PixelsPerUnitTooltip", "How many pixels in the sprite correspond to one Unreal unit"))
                ]
                
                // Padding
                + SUniformGridPanel::Slot(0, 2)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("PaddingLabel", "Padding:"))
                    .ToolTipText(LOCTEXT("PaddingTooltip", "Extra pixels to add around the content"))
                ]
                
                + SUniformGridPanel::Slot(1, 2)
                [
                    SAssignNew(PaddingSpinBox, SSpinBox<int32>)
                    .Value(CurrentSettings.Padding)
                    .MinValue(0)
                    .MaxValue(20)
                    .OnValueChanged(this, &SSpriteOptimizationWindow::OnPaddingChanged)
                    .ToolTipText(LOCTEXT("PaddingTooltip", "Extra pixels to add around the content"))
                ]
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SNew(SHorizontalBox)
                
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SAssignNew(CreateBackupCheckBox, SCheckBox)
                    .IsChecked(CurrentSettings.bCreateBackup ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                    .OnCheckStateChanged(this, &SSpriteOptimizationWindow::OnCreateBackupChanged)
                    .Content()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("CreateBackupLabel", "Create Backup"))
                        .ToolTipText(LOCTEXT("CreateBackupTooltip", "Create backup copies of original assets"))
                    ]
                ]
                
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SAssignNew(ReplaceOriginalsCheckBox, SCheckBox)
                    .IsChecked(CurrentSettings.bReplaceOriginals ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                    .OnCheckStateChanged(this, &SSpriteOptimizationWindow::OnReplaceOriginalsChanged)
                    .Content()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("ReplaceOriginalsLabel", "Replace Originals"))
                        .ToolTipText(LOCTEXT("ReplaceOriginalsTooltip", "Replace original sprites with optimized versions"))
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> SSpriteOptimizationWindow::CreateSpriteListSection()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
        .Padding(10)
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
                    .Text(LOCTEXT("SpritesSection", "📋 Sprites to Optimize"))
                    .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(5, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("SelectAll", "All"))
                    .OnClicked(this, &SSpriteOptimizationWindow::OnSelectAll)
                    .ToolTipText(LOCTEXT("SelectAllTooltip", "Select all sprites"))
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(5, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("SelectNone", "None"))
                    .OnClicked(this, &SSpriteOptimizationWindow::OnSelectNone)
                    .ToolTipText(LOCTEXT("SelectNoneTooltip", "Deselect all sprites"))
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(5, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("SelectOptimal", "Optimal"))
                    .OnClicked(this, &SSpriteOptimizationWindow::OnSelectOptimal)
                    .ToolTipText(LOCTEXT("SelectOptimalTooltip", "Select only sprites that will benefit from optimization"))
                ]
            ]
            
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(0, 5)
            [
                SAssignNew(SpriteListView, SListView<TSharedPtr<FSpriteOptimizationRow>>)
                .ListItemsSource(&SpriteRows)
                .OnGenerateRow(this, &SSpriteOptimizationWindow::GenerateSpriteRow)
                .OnSelectionChanged(this, &SSpriteOptimizationWindow::OnSpriteSelectionChanged)
                .SelectionMode(ESelectionMode::Multi)
                .HeaderRow
                (
                    SNew(SHeaderRow)
                    
                    + SHeaderRow::Column("Selected")
                    .DefaultLabel(LOCTEXT("SelectedHeader", "✓"))
                    .FixedWidth(30)
                    
                    + SHeaderRow::Column("SpriteName")
                    .DefaultLabel(LOCTEXT("SpriteNameHeader", "Sprite Name"))
                    .FillWidth(0.25f)
                    
                    + SHeaderRow::Column("OriginalSize")
                    .DefaultLabel(LOCTEXT("OriginalSizeHeader", "Original Size"))
                    .FillWidth(0.15f)
                    
                    + SHeaderRow::Column("UsedArea")
                    .DefaultLabel(LOCTEXT("UsedAreaHeader", "Used Area"))
                    .FillWidth(0.15f)
                    
                    + SHeaderRow::Column("OptimizedSize")
                    .DefaultLabel(LOCTEXT("OptimizedSizeHeader", "Optimized Size"))
                    .FillWidth(0.15f)
                    
                    + SHeaderRow::Column("Savings")
                    .DefaultLabel(LOCTEXT("SavingsHeader", "Savings"))
                    .FillWidth(0.15f)
                    
                    + SHeaderRow::Column("Status")
                    .DefaultLabel(LOCTEXT("StatusHeader", "Status"))
                    .FillWidth(0.15f)
                )
            ]
        ];
}

TSharedRef<SWidget> SSpriteOptimizationWindow::CreateSummarySection()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
        .Padding(10)
        [
            SNew(SVerticalBox)
            
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(LOCTEXT("SummarySection", "📊 Optimization Summary"))
                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SAssignNew(SummaryText, STextBlock)
                .Text(GetSummaryText())
                .AutoWrapText(true)
            ]
        ];
}

TSharedRef<SWidget> SSpriteOptimizationWindow::CreateActionSection()
{
    return SNew(SHorizontalBox)
        
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(5)
        [
            SNew(SButton)
            .Text(LOCTEXT("RefreshAnalysis", "🔄 Refresh Analysis"))
            .OnClicked(this, &SSpriteOptimizationWindow::OnAnalyzeSprites)
            .HAlign(HAlign_Center)
            .ToolTipText(LOCTEXT("RefreshAnalysisTooltip", "Re-analyze all sprites with current settings"))
        ]
        
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(5)
        [
            SAssignNew(PreviewButton, SButton)
            .Text(LOCTEXT("PreviewOptimization", "👁️ Preview"))
            .OnClicked(this, &SSpriteOptimizationWindow::OnPreviewOptimization)
            .HAlign(HAlign_Center)
            .ToolTipText(LOCTEXT("PreviewTooltip", "Preview optimization results"))
        ]
        
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(5)
        [
            SAssignNew(OptimizeButton, SButton)
            .Text(LOCTEXT("OptimizeSprites", "🚀 Optimize Selected"))
            .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
            .OnClicked(this, &SSpriteOptimizationWindow::OnOptimizeSprites)
            .HAlign(HAlign_Center)
            .ToolTipText(LOCTEXT("OptimizeTooltip", "Create optimized versions of selected sprites"))
        ];
}

TSharedRef<ITableRow> SSpriteOptimizationWindow::GenerateSpriteRow(TSharedPtr<FSpriteOptimizationRow> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(STableRow<TSharedPtr<FSpriteOptimizationRow>>, OwnerTable)
        [
            SNew(SHorizontalBox)
            
            // Checkbox
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .HAlign(HAlign_Center)
            .Padding(5, 0)
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
            
            // Sprite Name
            + SHorizontalBox::Slot()
            .FillWidth(0.25f)
            .VAlign(VAlign_Center)
            .Padding(5, 2)
            [
                SNew(STextBlock)
                .Text(FText::FromString(Item->OriginalSprite ? Item->OriginalSprite->GetName() : TEXT("Unknown")))
                .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
            ]
            
            // Original Size
            + SHorizontalBox::Slot()
            .FillWidth(0.15f)
            .VAlign(VAlign_Center)
            .Padding(5, 2)
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
            ]
            
            // Used Area
            + SHorizontalBox::Slot()
            .FillWidth(0.15f)
            .VAlign(VAlign_Center)
            .Padding(5, 2)
            [
                SNew(STextBlock)
                .Text_Lambda([Item]()
                {
                    if (Item->AnalysisResult.bSuccess)
                    {
                        return FText::FromString(FString::Printf(TEXT("%.1f%%"), Item->AnalysisResult.UsagePercent));
                    }
                    return FText::FromString(TEXT("N/A"));
                })
                .ColorAndOpacity_Lambda([Item]()
                {
                    if (!Item->AnalysisResult.bSuccess) return FSlateColor(FLinearColor::Gray);
                    if (Item->AnalysisResult.UsagePercent < 30.0f) return FSlateColor(FLinearColor::Red);
                    if (Item->AnalysisResult.UsagePercent < 60.0f) return FSlateColor(FLinearColor::Yellow);
                    return FSlateColor(FLinearColor::Green);
                })
            ]
            
            // Optimized Size
            + SHorizontalBox::Slot()
            .FillWidth(0.15f)
            .VAlign(VAlign_Center)
            .Padding(5, 2)
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
            ]
            
            // Savings
            + SHorizontalBox::Slot()
            .FillWidth(0.15f)
            .VAlign(VAlign_Center)
            .Padding(5, 2)
            [
                SNew(STextBlock)
                .Text_Lambda([Item]()
                {
                    if (Item->AnalysisResult.bSuccess)
                    {
                        return FText::FromString(FString::Printf(TEXT("%.1f%%"), Item->AnalysisResult.SavingsPercent));
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
            ]
            
            // Status
            + SHorizontalBox::Slot()
            .FillWidth(0.15f)
            .VAlign(VAlign_Center)
            .Padding(5, 2)
            [
                SNew(STextBlock)
                .Text_Lambda([Item]()
                {
                    if (!Item->AnalysisResult.bSuccess)
                    {
                        return FText::FromString(TEXT("❌ Error"));
                    }
                    else if (Item->AnalysisResult.SavingsPercent > 50.0f)
                    {
                        return FText::FromString(TEXT("🟢 Good"));
                    }
                    else if (Item->AnalysisResult.SavingsPercent > 20.0f)
                    {
                        return FText::FromString(TEXT("🟡 Fair"));
                    }
                    else
                    {
                        return FText::FromString(TEXT("🔴 Poor"));
                    }
                })
                .ToolTipText_Lambda([Item]()
                {
                    if (!Item->AnalysisResult.bSuccess)
                    {
                        return FText::FromString(Item->AnalysisResult.ErrorMessage);
                    }
                    return FText::FromString(TEXT("Optimization potential based on savings percentage"));
                })
            ]
        ];
}

void SSpriteOptimizationWindow::OnSpriteSelectionChanged(TSharedPtr<FSpriteOptimizationRow> Item, ESelectInfo::Type SelectInfo)
{
    UpdateSummary();
    UpdateButtonStates();
}

FReply SSpriteOptimizationWindow::OnSelectOptimal()
{
    const USpriteOptimizerSettings* ProjectSettings = GetDefault<USpriteOptimizerSettings>();
    float MinSavings = ProjectSettings ? ProjectSettings->MinimumSavingsForAutoSelect : 30.0f;
    
    for (auto& Row : SpriteRows)
    {
        // Выбираем только спрайты с экономией больше порога
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
    
    // Сбрасываем MaterialComboBox
    if (MaterialComboBox.IsValid())
    {
        MaterialComboBox->SetSelectedItem(MaterialOptions[0]); // Default материал
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
    bool bHasOptimizationResults = OptimizationResults.Num() > 0;
    
    if (OptimizeButton.IsValid())
    {
        OptimizeButton->SetEnabled(bHasSelection);
    }
    
    if (PreviewButton.IsValid())
    {
        PreviewButton->SetEnabled(bHasOptimizationResults);
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

void SSpriteOptimizationWindow::LoadSettingsFromProject()
{
    if (CurrentSettings.bUseProjectSettings)
    {
        CurrentSettings.LoadFromProjectSettings();
        
        // Обновляем MaterialComboBox
        if (MaterialComboBox.IsValid() && CurrentSettings.Material)
        {
            // Находим соответствующий элемент в списке
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

void SSpriteOptimizationWindow::OnMaterialChanged(const FAssetData& AssetData)
{
    CurrentSettings.Material = Cast<UMaterialInterface>(AssetData.GetAsset());
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
        "⚡ Performance improvement: ~{5}x faster loading\n"
        "💡 Tip: Use 'Create Atlas' to combine multiple sprites for even better performance!"),
        ValidSprites,
        FText::AsNumber(TotalOriginalMB, &FNumberFormattingOptions::DefaultWithGrouping()),
        FText::AsNumber(TotalOptimizedMB, &FNumberFormattingOptions::DefaultWithGrouping()),
        FText::AsNumber(TotalSavingsMB, &FNumberFormattingOptions::DefaultWithGrouping()),
        FText::AsNumber(SavingsPercent, &FNumberFormattingOptions::DefaultWithGrouping()),
        FMath::Max(1, FMath::RoundToInt(SavingsPercent / 15.0f))
    );
}

FText SSpriteOptimizationWindow::GetSelectedSpritesText() const
{
    TArray<TSharedPtr<FSpriteOptimizationRow>> SelectedSprites = GetSelectedSprites();
    return FText::Format(LOCTEXT("SelectedSpritesFormat", "{0} sprites selected"), SelectedSprites.Num());
}

FText SSpriteOptimizationWindow::GetPreviewText() const
{
    if (OptimizationResults.Num() == 0)
    {
        return LOCTEXT("NoResultsToPreview", "No optimization results to preview");
    }
    
    FString PreviewText = TEXT("Optimization Results:\n\n");
    
    float TotalOriginalMB = 0.0f;
    float TotalOptimizedMB = 0.0f;
    int32 SuccessCount = 0;
    
    // Безопасная итерация по результатам
    for (int32 i = 0; i < OptimizationResults.Num(); i++)
    {
        const FSpriteOptimizationResult& Result = OptimizationResults[i];
        
        // Проверяем валидность имени спрайта
        FString SpriteName = Result.SpriteName.IsEmpty() ? TEXT("Unknown Sprite") : Result.SpriteName;
        PreviewText += FString::Printf(TEXT("• %s: "), *SpriteName);
        
        if (Result.bSuccess)
        {
            PreviewText += FString::Printf(TEXT("✅ %.1f%% savings (%.1fMB → %.1fMB)\n"), 
                                         Result.SavingsPercent, Result.OriginalSizeMB, Result.OptimizedSizeMB);
            
            // Безопасная проверка путей
            FString TexturePath = Result.OptimizedTexturePath.IsEmpty() ? TEXT("Unknown Path") : Result.OptimizedTexturePath;
            FString SpritePath = Result.OptimizedSpritePath.IsEmpty() ? TEXT("Unknown Path") : Result.OptimizedSpritePath;
            
            PreviewText += FString::Printf(TEXT("  📁 Texture: %s\n"), *TexturePath);
            PreviewText += FString::Printf(TEXT("  🎨 Sprite: %s\n\n"), *SpritePath);
            
            TotalOriginalMB += Result.OriginalSizeMB;
            TotalOptimizedMB += Result.OptimizedSizeMB;
            SuccessCount++;
        }
        else
        {
            FString ErrorMsg = Result.ErrorMessage.IsEmpty() ? TEXT("Unknown Error") : Result.ErrorMessage;
            PreviewText += FString::Printf(TEXT("❌ Failed: %s\n\n"), *ErrorMsg);
        }
    }
    
    if (SuccessCount > 0)
    {
        float TotalSavingsMB = TotalOriginalMB - TotalOptimizedMB;
        float SavingsPercent = TotalOriginalMB > 0.0f ? (TotalSavingsMB / TotalOriginalMB) * 100.0f : 0.0f;
        
        PreviewText += FString::Printf(TEXT("📊 Summary:\n"));
        PreviewText += FString::Printf(TEXT("Successfully optimized: %d/%d sprites\n"), SuccessCount, OptimizationResults.Num());
        PreviewText += FString::Printf(TEXT("Total savings: %.1f MB (%.1f%%)\n"), TotalSavingsMB, SavingsPercent);
        PreviewText += FString::Printf(TEXT("Performance improvement: ~%dx faster loading\n\n"), 
                                     FMath::Max(1, FMath::RoundToInt(SavingsPercent / 15.0f)));
        PreviewText += TEXT("💡 Optimized assets are saved in the same directory as originals with '_Optimized' suffix.\n");
        PreviewText += TEXT("🔄 Content Browser will be automatically refreshed to show new assets.\n");
        PreviewText += TEXT("🎨 To combine sprites into atlases, use the separate 'Create Atlas' option.");
    }
    
    return FText::FromString(PreviewText);
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

void SSpriteOptimizationWindow::ShowOptimizationPreview()
{
    // Дополнительная проверка
    if (OptimizationResults.Num() == 0)
    {
        ShowNotification(LOCTEXT("NoOptimizationResults", "No optimization results to preview. Run optimization first."), 2);
        return;
    }

    // Получаем текст превью заранее
    FText PreviewTextContent = GetPreviewText();
    
    // Создаем окно с минимальной сложностью
    TSharedPtr<SWindow> PreviewWindow = SNew(SWindow)
        .Title(LOCTEXT("OptimizationPreviewTitle", "Optimization Results Preview"))
        .SizingRule(ESizingRule::UserSized)
        .ClientSize(FVector2D(800, 600))
        .SupportsMaximize(false)
        .SupportsMinimize(false)
        .IsTopmostWindow(false);

    if (!PreviewWindow.IsValid())
    {
        ShowNotification(LOCTEXT("PreviewWindowError", "Failed to create preview window"), 2);
        return;
    }

    // Создаем содержимое окна
    TSharedPtr<SWidget> WindowContent = SNew(SVerticalBox)
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("PreviewTitle", "Optimization Results"))
            .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
            .Justification(ETextJustify::Center)
        ]
        
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        .Padding(10)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
            .Padding(10)
            [
                SNew(SScrollBox)
                + SScrollBox::Slot()
                [
                    SNew(STextBlock)
                    .Text(PreviewTextContent)
                    .AutoWrapText(true)
                    .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
                ]
            ]
        ]
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SButton)
                .Text(LOCTEXT("ClosePreview", "Close"))
                .OnClicked_Static(&SSpriteOptimizationWindow::ClosePreviewWindow, PreviewWindow)
                .HAlign(HAlign_Center)
            ]
            
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
        ];

    if (!WindowContent.IsValid())
    {
        ShowNotification(LOCTEXT("PreviewContentError", "Failed to create preview content"), 2);
        return;
    }

    PreviewWindow->SetContent(WindowContent.ToSharedRef());
    
    // Показываем окно
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().AddWindow(PreviewWindow.ToSharedRef());
    }
    else
    {
        ShowNotification(LOCTEXT("SlateNotInitialized", "UI system not ready"), 2);
    }
}

FReply SSpriteOptimizationWindow::ClosePreviewWindow(TSharedPtr<SWindow> WindowToClose)
{
    if (WindowToClose.IsValid())
    {
        WindowToClose->RequestDestroyWindow();
    }
    return FReply::Handled();
}

FReply SSpriteOptimizationWindow::OnAnalyzeSprites()
{
    ShowNotification(LOCTEXT("AnalyzingSprites", "Analyzing sprites..."), 0);
    
    // Повторно анализируем все спрайты
    for (auto& Row : SpriteRows)
    {
        if (Row->OriginalSprite)
        {
            Row->AnalysisResult = USpriteOptimizer::AnalyzeSprite(Row->OriginalSprite.Get());
        }
    }
    
    // Обновляем UI
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
    
    // Собираем спрайты для оптимизации
    TArray<UPaperSprite*> SpritesToOptimize;
    for (const auto& Row : SelectedSprites)
    {
        if (Row->OriginalSprite && Row->AnalysisResult.bSuccess)
        {
            SpritesToOptimize.Add(Row->OriginalSprite.Get());
        }
    }
    
    // Выполняем оптимизацию
    OptimizationResults = USpriteOptimizer::OptimizeSprites(SpritesToOptimize, CurrentSettings);
    
    // Подсчитываем успешные результаты
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
    
    // ВАЖНО: Закрываем окно после завершения оптимизации
    if (SuccessCount > 0)
    {
        // Даем время показать уведомление, затем закрываем окно
        FTimerHandle TimerHandle;
        FTimerDelegate TimerDelegate;
        TimerDelegate.BindLambda([this]()
        {
            // Очищаем все ссылки на спрайты
            SpriteRows.Empty();
            OptimizationResults.Empty();
            
            // Закрываем окно
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

FReply SSpriteOptimizationWindow::OnPreviewOptimization()
{
    // Простая проверка
    if (OptimizationResults.Num() == 0)
    {
        ShowNotification(LOCTEXT("NoOptimizationResults", "No optimization results to preview. Run optimization first."), 2);
        return FReply::Handled();
    }
    
    // Простой вызов без try-catch для лучшей диагностики
    ShowOptimizationPreview();
    
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

#undef LOCTEXT_NAMESPACE