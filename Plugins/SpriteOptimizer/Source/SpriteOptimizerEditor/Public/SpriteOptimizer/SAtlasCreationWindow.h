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
#include "Widgets/Input/SEditableTextBox.h"
#include "SpriteOptimizer/SpriteOptimizer.h"
#include "PaperSprite.h"

class UPaperSprite;
class SAtlasPreviewWindow;

struct FAtlasSpriteInfo
{
    TObjectPtr<UPaperSprite> Sprite = nullptr;
    FSpriteOptimizationResult AnalysisResult;
    bool bIncludeInAtlas = true;
    FIntPoint OptimizedSize;
    
    FAtlasSpriteInfo()
    {
        bIncludeInAtlas = true;
        OptimizedSize = FIntPoint::ZeroValue;
    }
    
    FAtlasSpriteInfo(UPaperSprite* InSprite)
    {
        Sprite = InSprite;
        bIncludeInAtlas = true;
        OptimizedSize = FIntPoint::ZeroValue;
        
        if (Sprite)
        {
            AnalysisResult = USpriteOptimizer::AnalyzeSprite(Sprite);
            OptimizedSize = FIntPoint(AnalysisResult.OptimizedSize.X, AnalysisResult.OptimizedSize.Y);
        }
    }
};

class SAtlasCreationWindow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SAtlasCreationWindow) {}
        SLATE_ARGUMENT(TArray<UPaperSprite*>, SourceSprites)
    SLATE_END_ARGS()

    virtual ~SAtlasCreationWindow();
    void Construct(const FArguments& InArgs);
    
    static void ShowAtlasCreationWindow(const TArray<UPaperSprite*>& Sprites);

private:
    // Main data
    TArray<TSharedPtr<FAtlasSpriteInfo>> SpriteInfos;
    FSpriteAtlasSettings AtlasSettings;
    FSpriteAtlasResult LastAnalysisResult;
    TArray<TSharedPtr<FString>> PackingAlgorithmOptions;
    FString CurrentAtlasName;
    
    // UI widgets - atlas settings
    TSharedPtr<SEditableTextBox> AtlasNameTextBox;
    TSharedPtr<SSpinBox<int32>> MaxWidthSpinBox;
    TSharedPtr<SSpinBox<int32>> MaxHeightSpinBox;
    TSharedPtr<SSpinBox<int32>> SpritePaddingSpinBox;
    TSharedPtr<SComboBox<TSharedPtr<FString>>> PackingAlgorithmComboBox;
    TSharedPtr<SEditableTextBox> AtlasSuffixTextBox;
    
    // UI widgets - options
    TSharedPtr<SCheckBox> OptimizeSpritesFirstCheckBox;
    TSharedPtr<SCheckBox> CreateIndividualSpritesCheckBox;
    TSharedPtr<SCheckBox> PowerOfTwoCheckBox;
    TSharedPtr<SCheckBox> SquareAtlasCheckBox;
    TSharedPtr<SCheckBox> PreserveQualityCheckBox;
    TSharedPtr<SCheckBox> ForceSmoothingCheckBox;
    
    // UI widgets - sprite list and analysis
    TSharedPtr<SListView<TSharedPtr<FAtlasSpriteInfo>>> SpritesListView;
    TSharedPtr<STextBlock> SpritesSummaryText;
    TSharedPtr<STextBlock> AnalysisResultText;
    TSharedPtr<SButton> AnalyzeButton;
    TSharedPtr<SButton> CreateAtlasButton;
    TSharedPtr<SButton> PreviewButton;
    
    // UI creation methods
    TSharedRef<SWidget> CreateCompactHeaderSection();
    TSharedRef<SWidget> CreateCompactAtlasSettingsSection();
    TSharedRef<SWidget> CreateCompactSpritesSection();
    TSharedRef<SWidget> CreateCompactAnalysisSection();
    TSharedRef<SWidget> CreateCompactActionSection();
    
    // Sprite list methods
    TSharedRef<ITableRow> GenerateCompactSpriteRow(TSharedPtr<FAtlasSpriteInfo> Item, const TSharedRef<STableViewBase>& OwnerTable);
    void OnSpriteSelectionChanged(TSharedPtr<FAtlasSpriteInfo> Item, ESelectInfo::Type SelectInfo);
    
    // Atlas settings event handlers
    void OnAtlasNameChanged(const FText& NewText, ETextCommit::Type CommitType);
    void OnMaxWidthChanged(int32 NewValue);
    void OnMaxHeightChanged(int32 NewValue);
    void OnSpritePaddingChanged(int32 NewValue);
    void OnPackingAlgorithmChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo);
    void OnAtlasSuffixChanged(const FText& NewText, ETextCommit::Type CommitType);
    
    // Options event handlers
    void OnOptimizeSpritesFirstChanged(ECheckBoxState NewState);
    void OnCreateIndividualSpritesChanged(ECheckBoxState NewState);
    void OnPowerOfTwoChanged(ECheckBoxState NewState);
    void OnSquareAtlasChanged(ECheckBoxState NewState);
    void OnPreserveQualityChanged(ECheckBoxState NewState);
    void OnForceSmoothingChanged(ECheckBoxState NewState);
    
    // Action event handlers
    FReply OnAnalyzeAtlas();
    FReply OnCreateAtlas();
    FReply OnPreviewAtlas();
    FReply OnSelectAllSprites();
    FReply OnSelectNoneSprites();
    FReply OnSelectOptimalSprites();
    FReply OnResetToDefaults();
    
    // Helper methods
    void InitializePackingAlgorithms();
    void LoadDefaultSettings();
    void UpdateAnalysisDisplay();
    void UpdateButtonStates();
    TArray<UPaperSprite*> GetSelectedSprites() const;
    bool ValidateSettings() const;
    FText GetValidationErrorText() const;
    void ShowNotification(const FText& Message, bool bSuccess = true);
    
    // Text getters
    FText GetCompactAnalysisText() const;
    FText GetSpritesSummaryText() const;
};