#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Character2DLayerOptimizer.h"
#include "Character2DAsset.h"

class UCharacter2DAsset;

struct FLayerAnalysisRow
{
    FString LayerName;
    TObjectPtr<UTexture2D> OriginalTexture = nullptr;
    FIntRect UsedBounds;
    float OriginalSizeMB = 0.0f;
    float OptimizedSizeMB = 0.0f;
    float SavingsPercent = 0.0f;
    float UsagePercent = 0.0f;
    bool bSelected = false;
    
    void CalculateStats()
    {
        if (OriginalTexture)
        {
            int32 OriginalPixels = OriginalTexture->GetSizeX() * OriginalTexture->GetSizeY();
            int32 UsedPixels = UsedBounds.Width() * UsedBounds.Height();
            
            OriginalSizeMB = (OriginalPixels * 4) / (1024.0f * 1024.0f);
            OptimizedSizeMB = (UsedPixels * 4) / (1024.0f * 1024.0f);
            SavingsPercent = OriginalSizeMB > 0 ? ((OriginalSizeMB - OptimizedSizeMB) / OriginalSizeMB) * 100.0f : 0.0f;
            UsagePercent = OriginalPixels > 0 ? (static_cast<float>(UsedPixels) / OriginalPixels) * 100.0f : 0.0f;
        }
    }
};

DECLARE_DELEGATE_OneParam(FOnOptimizationComplete, const TArray<FLayerOptimizationResult>&);

class SCharacter2DOptimizationPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCharacter2DOptimizationPanel) {}
        SLATE_ARGUMENT(UCharacter2DAsset*, CharacterAsset)
        SLATE_EVENT(FOnOptimizationComplete, OnOptimizationComplete)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    
    // Обновление данных
    void RefreshAnalysis();
    void SetCharacterAsset(UCharacter2DAsset* NewAsset);

private:
    // Данные
    TWeakObjectPtr<UCharacter2DAsset> CharacterAsset;
    TArray<TSharedPtr<FLayerAnalysisRow>> LayerAnalysisData;
    TArray<FLayerOptimizationResult> CurrentOptimizationResults;
    FOnOptimizationComplete OnOptimizationComplete;
    
    // Виджеты
    TSharedPtr<SListView<TSharedPtr<FLayerAnalysisRow>>> LayerAnalysisTable;
    TSharedPtr<STextBlock> OptimizationSummary;
    TSharedPtr<SButton> OptimizeButton;
    TSharedPtr<SButton> ApplyButton;
    TSharedPtr<SButton> PreviewButton;
    
    // Обработчики событий
    FReply OnAnalyzeLayers();
    FReply OnOptimizeSelected();
    FReply OnApplyOptimization();
    FReply OnPreviewOptimization();
    FReply OnAutoOptimizeEverything();
    
    // Создание UI элементов
    TSharedRef<SWidget> CreateAnalysisSection();
    TSharedRef<SWidget> CreateResultsSection();
    TSharedRef<SWidget> CreateActionsSection();
    TSharedRef<SWidget> CreateSummarySection();
    
    // Работа с таблицей
    TSharedRef<ITableRow> GenerateLayerAnalysisRow(TSharedPtr<FLayerAnalysisRow> Item, const TSharedRef<STableViewBase>& OwnerTable);
    void OnLayerSelectionChanged(TSharedPtr<FLayerAnalysisRow> Item, ESelectInfo::Type SelectInfo);
    
    // Вспомогательные функции
    void UpdateOptimizationSummary();
    void UpdateButtonStates();
    TArray<TSharedPtr<FLayerAnalysisRow>> GetSelectedLayers() const;
    
    // Создание окна предпросмотра
    void ShowOptimizationPreview();
    
    // Получение текста статистики до оптимизации
    FText GetOriginalStatsText() const;
    
    // Получение текста статистики после оптимизации  
    FText GetOptimizedStatsText() const;
};