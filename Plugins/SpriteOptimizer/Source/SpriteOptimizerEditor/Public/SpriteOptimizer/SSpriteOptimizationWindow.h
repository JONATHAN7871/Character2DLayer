// Copyright 2025, CRAFTCODE, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "SpriteOptimizer/SpriteOptimizer.h"
#include "PaperSprite.h"
#include "Materials/MaterialInterface.h"

class UPaperSprite;
class UMaterialInterface;

struct FSpriteOptimizationRow
{
    TObjectPtr<UPaperSprite> OriginalSprite = nullptr;
    FSpriteOptimizationResult AnalysisResult;
    bool bSelected = true;
    
    FSpriteOptimizationRow()
    {
        bSelected = true;
    }
    
    FSpriteOptimizationRow(UPaperSprite* Sprite)
    {
        OriginalSprite = Sprite;
        bSelected = true;
        if (Sprite)
        {
            AnalysisResult = USpriteOptimizer::AnalyzeSprite(Sprite);
        }
    }
};

class SSpriteOptimizationWindow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSpriteOptimizationWindow) {}
        SLATE_ARGUMENT(TArray<UPaperSprite*>, SpritesToOptimize)
    SLATE_END_ARGS()

    virtual ~SSpriteOptimizationWindow();
    void Construct(const FArguments& InArgs);
    
    static void ShowOptimizationWindow(const TArray<UPaperSprite*>& Sprites);

private:
    // Main data
    TArray<TSharedPtr<FSpriteOptimizationRow>> SpriteRows;
    TArray<FSpriteOptimizationResult> OptimizationResults;
    FSpriteOptimizationSettings CurrentSettings;
    
    // UI widgets
    TSharedPtr<SListView<TSharedPtr<FSpriteOptimizationRow>>> SpriteListView;
    TSharedPtr<STextBlock> SummaryText;
    TSharedPtr<SButton> OptimizeButton;
    
    // Settings widgets
    TSharedPtr<SComboBox<TSharedPtr<FString>>> MaterialComboBox;
    TArray<TSharedPtr<FString>> MaterialOptions;
    TArray<UMaterialInterface*> MaterialAssets;
    TSharedPtr<SSpinBox<float>> PixelsPerUnitSpinBox;
    TSharedPtr<SSpinBox<int32>> PaddingSpinBox;
    TSharedPtr<SCheckBox> CreateBackupCheckBox;
    TSharedPtr<SCheckBox> ReplaceOriginalsCheckBox;
    TSharedPtr<SCheckBox> UseProjectSettingsCheckBox;
    
    // UI creation methods
    TSharedRef<SWidget> CreateHeaderSection();
    TSharedRef<SWidget> CreateCompactSettingsSection();
    TSharedRef<SWidget> CreateCompactSpriteListSection();
    TSharedRef<SWidget> CreateCompactActionSection();
    
    // Table generation
    TSharedRef<ITableRow> GenerateCompactSpriteRow(TSharedPtr<FSpriteOptimizationRow> Item, const TSharedRef<STableViewBase>& OwnerTable);
    void OnSpriteSelectionChanged(TSharedPtr<FSpriteOptimizationRow> Item, ESelectInfo::Type SelectInfo);
    
    // Event handlers
    FReply OnAnalyzeSprites();
    FReply OnOptimizeSprites();
    FReply OnSelectAll();
    FReply OnSelectNone();
    FReply OnSelectOptimal();
    FReply OnOpenProjectSettings();
    FReply OnResetToDefaults();
    
    // Settings handlers
    void OnMaterialComboChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo);
    void OnPixelsPerUnitChanged(float NewValue);
    void OnPaddingChanged(int32 NewValue);
    void OnCreateBackupChanged(ECheckBoxState NewState);
    void OnReplaceOriginalsChanged(ECheckBoxState NewState);
    void OnUseProjectSettingsChanged(ECheckBoxState NewState);
    
    // Helper methods
    void UpdateSummary();
    void UpdateButtonStates();
    TArray<TSharedPtr<FSpriteOptimizationRow>> GetSelectedSprites() const;
    void RefreshAnalysis();
    void LoadSettingsFromProject();
    void InitializeMaterialOptions();
    void ShowNotification(const FText& Message, int32 State = 0);
    
    // Text getters
    FText GetCompactSummaryText() const;
    FText GetSummaryText() const;
};