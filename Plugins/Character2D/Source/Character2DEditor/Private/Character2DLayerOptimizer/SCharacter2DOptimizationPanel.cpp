#include "Character2DLayerOptimizer/SCharacter2DOptimizationPanel.h"
#include "Character2DLayerOptimizer/Character2DLayerOptimizer.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Layout/SSeparator.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "SCharacter2DOptimizationPanel"

void SCharacter2DOptimizationPanel::Construct(const FArguments& InArgs)
{
    CharacterAsset = InArgs._CharacterAsset;
    OnOptimizationComplete = InArgs._OnOptimizationComplete;
    
    ChildSlot
    [
        SNew(SVerticalBox)
        
        // Заголовок
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("OptimizationTitle", "Character2D Layer Optimization"))
            .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
            .Justification(ETextJustify::Center)
        ]
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5, 2)
        [
            SNew(SSeparator)
        ]
        
        // Секция анализа
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            CreateAnalysisSection()
        ]
        
        // Секция результатов
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        .Padding(5)
        [
            CreateResultsSection()
        ]
        
        // Секция итогов
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            CreateSummarySection()
        ]
        
        // Секция действий
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            CreateActionsSection()
        ]
    ];
    
    // Начальное обновление
    UpdateButtonStates();
}

TSharedRef<SWidget> SCharacter2DOptimizationPanel::CreateAnalysisSection()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(10)
        [
            SNew(SVerticalBox)
            
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(LOCTEXT("AnalysisSection", "Analysis"))
                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("AnalysisInfo", "Analyze current layer textures to find optimization opportunities"))
                .AutoWrapText(true)
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SNew(SButton)
                .Text(LOCTEXT("AnalyzeLayers", "🔍 Analyze Current Layers"))
                .OnClicked(this, &SCharacter2DOptimizationPanel::OnAnalyzeLayers)
                .IsEnabled_Lambda([this]() { return CharacterAsset.IsValid(); })
                .HAlign(HAlign_Center)
            ]
        ];
}

TSharedRef<SWidget> SCharacter2DOptimizationPanel::CreateResultsSection()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(10)
        [
            SNew(SVerticalBox)
            
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(LOCTEXT("ResultsSection", "Optimization Results"))
                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
            ]
            
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(0, 5)
            [
                SAssignNew(LayerAnalysisTable, SListView<TSharedPtr<FLayerAnalysisRow>>)
                .ListItemsSource(&LayerAnalysisData)
                .OnGenerateRow(this, &SCharacter2DOptimizationPanel::GenerateLayerAnalysisRow)
                .OnSelectionChanged(this, &SCharacter2DOptimizationPanel::OnLayerSelectionChanged)
                .SelectionMode(ESelectionMode::Multi)
                .HeaderRow
                (
                    SNew(SHeaderRow)
                    
                    + SHeaderRow::Column("LayerName")
                    .DefaultLabel(LOCTEXT("LayerName", "Layer"))
                    .FillWidth(0.2f)
                    
                    + SHeaderRow::Column("OriginalSize")
                    .DefaultLabel(LOCTEXT("OriginalSize", "Original Size"))
                    .FillWidth(0.15f)
                    
                    + SHeaderRow::Column("UsedArea")
                    .DefaultLabel(LOCTEXT("UsedArea", "Used Area"))
                    .FillWidth(0.15f)
                    
                    + SHeaderRow::Column("OptimizedSize")
                    .DefaultLabel(LOCTEXT("OptimizedSize", "Optimized Size"))
                    .FillWidth(0.15f)
                    
                    + SHeaderRow::Column("Savings")
                    .DefaultLabel(LOCTEXT("Savings", "Savings"))
                    .FillWidth(0.15f)
                    
                    + SHeaderRow::Column("Region")
                    .DefaultLabel(LOCTEXT("Region", "Content Region"))
                    .FillWidth(0.2f)
                )
            ]
        ];
}

TSharedRef<SWidget> SCharacter2DOptimizationPanel::CreateSummarySection()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(10)
        [
            SNew(SVerticalBox)
            
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(LOCTEXT("SummarySection", "Summary"))
                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SAssignNew(OptimizationSummary, STextBlock)
                .Text(LOCTEXT("OptimizationSummary", "Run analysis to see optimization potential"))
                .AutoWrapText(true)
            ]
        ];
}

TSharedRef<SWidget> SCharacter2DOptimizationPanel::CreateActionsSection()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(10)
        [
            SNew(SVerticalBox)
            
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(LOCTEXT("ActionsSection", "Actions"))
                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SNew(SHorizontalBox)
                
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(2)
                [
                    SAssignNew(OptimizeButton, SButton)
                    .Text(LOCTEXT("OptimizeSelected", "Optimize Selected"))
                    .OnClicked(this, &SCharacter2DOptimizationPanel::OnOptimizeSelected)
                    .IsEnabled(false)
                    .HAlign(HAlign_Center)
                ]
                
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(2)
                [
                    SAssignNew(PreviewButton, SButton)
                    .Text(LOCTEXT("PreviewOptimization", "Preview"))
                    .OnClicked(this, &SCharacter2DOptimizationPanel::OnPreviewOptimization)
                    .IsEnabled(false)
                    .HAlign(HAlign_Center)
                ]
                
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(2)
                [
                    SAssignNew(ApplyButton, SButton)
                    .Text(LOCTEXT("ApplyOptimization", "Apply"))
                    .OnClicked(this, &SCharacter2DOptimizationPanel::OnApplyOptimization)
                    .IsEnabled(false)
                    .HAlign(HAlign_Center)
                ]
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SNew(SSeparator)
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SNew(SButton)
                .Text(LOCTEXT("AutoOptimizeEverything", "🚀 Auto-Optimize Everything"))
                .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
                .OnClicked(this, &SCharacter2DOptimizationPanel::OnAutoOptimizeEverything)
                .IsEnabled_Lambda([this]() { return CharacterAsset.IsValid(); })
                .HAlign(HAlign_Center)
            ]
        ];
}

TSharedRef<ITableRow> SCharacter2DOptimizationPanel::GenerateLayerAnalysisRow(TSharedPtr<FLayerAnalysisRow> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(STableRow<TSharedPtr<FLayerAnalysisRow>>, OwnerTable)
        [
            SNew(SHorizontalBox)
            
            // Layer Name
            + SHorizontalBox::Slot()
            .FillWidth(0.2f)
            .VAlign(VAlign_Center)
            .Padding(5, 2)
            [
                SNew(STextBlock)
                .Text(FText::FromString(Item->LayerName))
                .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
            ]
            
            // Original Size
            + SHorizontalBox::Slot()
            .FillWidth(0.15f)
            .VAlign(VAlign_Center)
            .Padding(5, 2)
            [
                SNew(STextBlock)
                .Text(FText::FromString(FString::Printf(TEXT("%.1f MB"), Item->OriginalSizeMB)))
            ]
            
            // Used Area
            + SHorizontalBox::Slot()
            .FillWidth(0.15f)
            .VAlign(VAlign_Center)
            .Padding(5, 2)
            [
                SNew(STextBlock)
                .Text(FText::FromString(FString::Printf(TEXT("%.1f%%"), Item->UsagePercent)))
                .ColorAndOpacity_Lambda([Item]()
                {
                    if (Item->UsagePercent < 30.0f) return FSlateColor(FLinearColor::Red);
                    if (Item->UsagePercent < 60.0f) return FSlateColor(FLinearColor::Yellow);
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
                .Text(FText::FromString(FString::Printf(TEXT("%.1f MB"), Item->OptimizedSizeMB)))
            ]
            
            // Savings
            + SHorizontalBox::Slot()
            .FillWidth(0.15f)
            .VAlign(VAlign_Center)
            .Padding(5, 2)
            [
                SNew(STextBlock)
                .Text(FText::FromString(FString::Printf(TEXT("%.1f%%"), Item->SavingsPercent)))
                .ColorAndOpacity_Lambda([Item]()
                {
                    if (Item->SavingsPercent > 70.0f) return FSlateColor(FLinearColor::Green);
                    if (Item->SavingsPercent > 40.0f) return FSlateColor(FLinearColor::Yellow);
                    return FSlateColor(FLinearColor::White);
                })
            ]
            
            // Content Region
            + SHorizontalBox::Slot()
            .FillWidth(0.2f)
            .VAlign(VAlign_Center)
            .Padding(5, 2)
            [
                SNew(STextBlock)
                .Text(FText::FromString(FString::Printf(TEXT("%dx%d"), 
                    Item->UsedBounds.Width(), Item->UsedBounds.Height())))
            ]
        ];
}

void SCharacter2DOptimizationPanel::OnLayerSelectionChanged(TSharedPtr<FLayerAnalysisRow> Item, ESelectInfo::Type SelectInfo)
{
    UpdateButtonStates();
}

FReply SCharacter2DOptimizationPanel::OnAnalyzeLayers()
{
    if (!CharacterAsset.IsValid())
    {
        return FReply::Handled();
    }
    
    // Очищаем предыдущие данные
    LayerAnalysisData.Empty();
    CurrentOptimizationResults.Empty();
    
    // ИСПРАВЛЕНО: Функция анализа спрайта
    auto AnalyzeSprite = [this](UPaperSprite* Sprite, const FString& LayerName)
    {
        if (Sprite)
        {
            UTexture2D* SourceTexture = Sprite->GetSourceTexture();
            if (SourceTexture)
            {
                // ИСПРАВЛЕНО: Используем MakeShared вместо MakeShareable(new ...)
                TSharedPtr<FLayerAnalysisRow> Row = MakeShared<FLayerAnalysisRow>();
                Row->LayerName = LayerName;
                Row->OriginalTexture = SourceTexture;
                Row->UsedBounds = UCharacter2DLayerOptimizer::FindUsedBounds(SourceTexture);
                Row->CalculateStats();
                LayerAnalysisData.Add(Row);
            }
        }
    };
    
    UCharacter2DAsset* Asset = CharacterAsset.Get();
    AnalyzeSprite(Asset->SpriteStructure.Body.Sprite, TEXT("Body"));
    AnalyzeSprite(Asset->SpriteStructure.Arms.Sprite, TEXT("Arms"));
    AnalyzeSprite(Asset->SpriteStructure.Head.Head.Sprite, TEXT("Head"));
    AnalyzeSprite(Asset->SpriteStructure.Head.Eyes.Sprite, TEXT("Eyes"));
    AnalyzeSprite(Asset->SpriteStructure.Head.Eyebrows.Sprite, TEXT("Eyebrows"));
    AnalyzeSprite(Asset->SpriteStructure.Head.Eyelids.Sprite, TEXT("Eyelids"));
    AnalyzeSprite(Asset->SpriteStructure.Head.Mouth.Sprite, TEXT("Mouth"));
    AnalyzeSprite(Asset->SpriteStructure.Shadow.Sprite, TEXT("Shadow"));
    
    // Обновляем таблицу
    LayerAnalysisTable->RequestListRefresh();
    
    // Показываем общую статистику
    UpdateOptimizationSummary();
    UpdateButtonStates();
    
    return FReply::Handled();
}

FReply SCharacter2DOptimizationPanel::OnOptimizeSelected()
{
    TArray<TSharedPtr<FLayerAnalysisRow>> SelectedLayers = GetSelectedLayers();
    
    if (SelectedLayers.Num() == 0)
    {
        return FReply::Handled();
    }
    
    CurrentOptimizationResults.Empty();
    
    // Оптимизируем выбранные слои
    for (const auto& Layer : SelectedLayers)
    {
        if (Layer->OriginalTexture)
        {
            // Находим соответствующий спрайт в ассете
            UPaperSprite* LayerSprite = nullptr;
            UCharacter2DAsset* Asset = CharacterAsset.Get();
            
            if (Layer->LayerName == TEXT("Body")) LayerSprite = Asset->SpriteStructure.Body.Sprite;
            else if (Layer->LayerName == TEXT("Arms")) LayerSprite = Asset->SpriteStructure.Arms.Sprite;
            else if (Layer->LayerName == TEXT("Head")) LayerSprite = Asset->SpriteStructure.Head.Head.Sprite;
            else if (Layer->LayerName == TEXT("Eyes")) LayerSprite = Asset->SpriteStructure.Head.Eyes.Sprite;
            else if (Layer->LayerName == TEXT("Eyebrows")) LayerSprite = Asset->SpriteStructure.Head.Eyebrows.Sprite;
            else if (Layer->LayerName == TEXT("Eyelids")) LayerSprite = Asset->SpriteStructure.Head.Eyelids.Sprite;
            else if (Layer->LayerName == TEXT("Mouth")) LayerSprite = Asset->SpriteStructure.Head.Mouth.Sprite;
            else if (Layer->LayerName == TEXT("Shadow")) LayerSprite = Asset->SpriteStructure.Shadow.Sprite;
            
            if (LayerSprite)
            {
                FLayerOptimizationResult Result = UCharacter2DLayerOptimizer::OptimizeLayer(LayerSprite, Layer->LayerName);
                CurrentOptimizationResults.Add(Result);
            }
        }
    }
    
    UpdateButtonStates();
    
    return FReply::Handled();
}

FReply SCharacter2DOptimizationPanel::OnApplyOptimization()
{
    if (CurrentOptimizationResults.Num() == 0 || !CharacterAsset.IsValid())
    {
        return FReply::Handled();
    }
    
    // Применяем оптимизацию к ассету
    UCharacter2DLayerOptimizer::ApplyOptimizationToAsset(CharacterAsset.Get(), CurrentOptimizationResults);
    
    // Вызываем callback если он задан
    if (OnOptimizationComplete.IsBound())
    {
        OnOptimizationComplete.Execute(CurrentOptimizationResults);
    }
    
    // Очищаем результаты
    CurrentOptimizationResults.Empty();
    
    // Обновляем анализ
    OnAnalyzeLayers();
    
    return FReply::Handled();
}

FReply SCharacter2DOptimizationPanel::OnPreviewOptimization()
{
    ShowOptimizationPreview();
    return FReply::Handled();
}

FReply SCharacter2DOptimizationPanel::OnAutoOptimizeEverything()
{
    if (!CharacterAsset.IsValid())
    {
        return FReply::Handled();
    }
    
    // Сначала анализируем
    OnAnalyzeLayers();
    
    // Выбираем все слои
    LayerAnalysisTable->SetItemSelection(LayerAnalysisData, true);
    
    // Оптимизируем их
    OnOptimizeSelected();
    
    // Применяем оптимизацию
    OnApplyOptimization();
    
    return FReply::Handled();
}

void SCharacter2DOptimizationPanel::UpdateOptimizationSummary()
{
    float TotalOriginalMB = 0;
    float TotalOptimizedMB = 0;
    int32 LayerCount = 0;
    
    for (const auto& Row : LayerAnalysisData)
    {
        TotalOriginalMB += Row->OriginalSizeMB;
        TotalOptimizedMB += Row->OptimizedSizeMB;
        LayerCount++;
    }
    
    if (LayerCount == 0)
    {
        OptimizationSummary->SetText(LOCTEXT("OptimizationSummaryEmpty", "Run analysis to see optimization potential"));
        return;
    }
    
    float TotalSavingsMB = TotalOriginalMB - TotalOptimizedMB;
    float SavingsPercent = TotalOriginalMB > 0 ? (TotalSavingsMB / TotalOriginalMB) * 100.0f : 0.0f;
    
    FText SummaryText = FText::Format(LOCTEXT("OptimizationSummaryFormat",
        "📊 Optimization Analysis:\n"
        "Layers analyzed: {0}\n"
        "Current total size: {1:.1f} MB\n"
        "Optimized total size: {2:.1f} MB\n"
        "Potential savings: {3:.1f} MB ({4:.1f}%)\n\n"
        "🚀 Expected performance improvement: {5}x faster loading"),
        LayerCount,
        TotalOriginalMB,
        TotalOptimizedMB,
        TotalSavingsMB,
        SavingsPercent,
        FMath::Max(1, FMath::RoundToInt(SavingsPercent / 15.0f))
    );
    
    OptimizationSummary->SetText(SummaryText);
}

void SCharacter2DOptimizationPanel::UpdateButtonStates()
{
    TArray<TSharedPtr<FLayerAnalysisRow>> SelectedLayers = GetSelectedLayers();
    bool bHasSelection = SelectedLayers.Num() > 0;
    bool bHasOptimizationResults = CurrentOptimizationResults.Num() > 0;
    
    if (OptimizeButton.IsValid())
    {
        OptimizeButton->SetEnabled(bHasSelection);
    }
    
    if (PreviewButton.IsValid())
    {
        PreviewButton->SetEnabled(bHasOptimizationResults);
    }
    
    if (ApplyButton.IsValid())
    {
        ApplyButton->SetEnabled(bHasOptimizationResults);
    }
}

TArray<TSharedPtr<FLayerAnalysisRow>> SCharacter2DOptimizationPanel::GetSelectedLayers() const
{
    TArray<TSharedPtr<FLayerAnalysisRow>> SelectedItems;
    if (LayerAnalysisTable.IsValid())
    {
        SelectedItems = LayerAnalysisTable->GetSelectedItems();
    }
    return SelectedItems;
}

void SCharacter2DOptimizationPanel::ShowOptimizationPreview()
{
    // Создаем окно предпросмотра
    TSharedRef<SWindow> PreviewWindow = SNew(SWindow)
        .Title(LOCTEXT("OptimizationPreview", "Optimization Preview"))
        .SizingRule(ESizingRule::UserSized)
        .ClientSize(FVector2D(800, 600))
        .SupportsMaximize(false)
        .SupportsMinimize(false)
        .Content()
        [
            SNew(SBorder)
            .Padding(20)
            [
                SNew(SVerticalBox)
                
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 10)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("PreviewInfo", "Optimization Preview - Comparing Original vs Optimized"))
                    .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                    .Justification(ETextJustify::Center)
                ]
                
                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                .Padding(0, 10)
                [
                    SNew(SHorizontalBox)
                    
                    // Левая панель - статистика ДО
                    + SHorizontalBox::Slot()
                    .FillWidth(0.5f)
                    .Padding(5)
                    [
                        SNew(SBorder)
                        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                        .Padding(10)
                        [
                            SNew(SVerticalBox)
                            
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("OriginalStats", "Original (Before Optimization)"))
                                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                                .Justification(ETextJustify::Center)
                            ]
                            
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0, 10)
                            [
                                SNew(STextBlock)
                                .Text(GetOriginalStatsText())
                                .AutoWrapText(true)
                            ]
                        ]
                    ]
                    
                    // Правая панель - статистика ПОСЛЕ
                    + SHorizontalBox::Slot()
                    .FillWidth(0.5f)
                    .Padding(5)
                    [
                        SNew(SBorder)
                        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                        .Padding(10)
                        [
                            SNew(SVerticalBox)
                            
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("OptimizedStats", "Optimized (After Optimization)"))
                                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                                .Justification(ETextJustify::Center)
                            ]
                            
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0, 10)
                            [
                                SNew(STextBlock)
                                .Text(GetOptimizedStatsText())
                                .AutoWrapText(true)
                            ]
                        ]
                    ]
                ]
                
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 10)
                [
                    SNew(SHorizontalBox)
                    
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .Padding(5)
                    [
                        SNew(SButton)
                        .Text(LOCTEXT("ClosePreview", "Close"))
                        .OnClicked(FOnClicked::CreateLambda([PreviewWindow]() -> FReply
                        {
                            // ИСПРАВЛЕНО: Используем простой способ закрытия без WeakPtr
                            PreviewWindow->RequestDestroyWindow();
                            return FReply::Handled();
                        }))
                        .HAlign(HAlign_Center)
                    ]
                ]
            ]
        ];
    
    // Показываем окно
    FSlateApplication::Get().AddWindow(PreviewWindow);
}

void SCharacter2DOptimizationPanel::RefreshAnalysis()
{
    OnAnalyzeLayers();
}

void SCharacter2DOptimizationPanel::SetCharacterAsset(UCharacter2DAsset* NewAsset)
{
    CharacterAsset = NewAsset;
    
    // Очищаем данные
    LayerAnalysisData.Empty();
    CurrentOptimizationResults.Empty();
    
    if (LayerAnalysisTable.IsValid())
    {
        LayerAnalysisTable->RequestListRefresh();
    }
    
    UpdateOptimizationSummary();
    UpdateButtonStates();
}

FText SCharacter2DOptimizationPanel::GetOriginalStatsText() const
{
    float TotalOriginalMB = 0;
    int32 LayerCount = 0;
    
    for (const auto& Row : LayerAnalysisData)
    {
        TotalOriginalMB += Row->OriginalSizeMB;
        LayerCount++;
    }
    
    if (LayerCount == 0)
    {
        return LOCTEXT("NoDataOriginal", "No data available");
    }
    
    FString StatsText = FString::Printf(TEXT("Total Layers: %d\nTotal Size: %.1f MB\n\nBreakdown:\n"), 
                                       LayerCount, TotalOriginalMB);
    
    for (const auto& Row : LayerAnalysisData)
    {
        StatsText += FString::Printf(TEXT("• %s: %.1f MB\n"), 
                                   *Row->LayerName, Row->OriginalSizeMB);
    }
    
    return FText::FromString(StatsText);
}

FText SCharacter2DOptimizationPanel::GetOptimizedStatsText() const
{
    float TotalOptimizedMB = 0;
    float TotalOriginalMB = 0;
    int32 LayerCount = 0;
    
    for (const auto& Row : LayerAnalysisData)
    {
        TotalOptimizedMB += Row->OptimizedSizeMB;
        TotalOriginalMB += Row->OriginalSizeMB;
        LayerCount++;
    }
    
    if (LayerCount == 0)
    {
        return LOCTEXT("NoDataOptimized", "No data available");
    }
    
    float TotalSavingsMB = TotalOriginalMB - TotalOptimizedMB;
    float SavingsPercent = TotalOriginalMB > 0 ? (TotalSavingsMB / TotalOriginalMB) * 100.0f : 0.0f;
    
    FString StatsText = FString::Printf(TEXT("Total Layers: %d\nOptimized Size: %.1f MB\nSavings: %.1f MB (%.1f%%)\n\nBreakdown:\n"), 
                                       LayerCount, TotalOptimizedMB, TotalSavingsMB, SavingsPercent);
    
    for (const auto& Row : LayerAnalysisData)
    {
        StatsText += FString::Printf(TEXT("• %s: %.1f MB (%.1f%% saved)\n"), 
                                   *Row->LayerName, Row->OptimizedSizeMB, Row->SavingsPercent);
    }
    
    return FText::FromString(StatsText);
}

#undef LOCTEXT_NAMESPACE