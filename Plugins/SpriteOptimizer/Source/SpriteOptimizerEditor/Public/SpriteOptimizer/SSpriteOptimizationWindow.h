#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "SpriteOptimizer/SpriteOptimizer.h"
#include "PaperSprite.h"
#include "Materials/MaterialInterface.h"
#include "PropertyCustomizationHelpers.h"
#include "SpriteOptimizer.h"

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
    
    // Показ окна оптимизации
    static void ShowOptimizationWindow(const TArray<UPaperSprite*>& Sprites);

private:
    // === ОСНОВНЫЕ ДАННЫЕ ===
    TArray<TSharedPtr<FSpriteOptimizationRow>> SpriteRows;
    TArray<FSpriteOptimizationResult> OptimizationResults;
    FSpriteOptimizationSettings CurrentSettings;
    
    // === ОСНОВНЫЕ ВИДЖЕТЫ ===
    TSharedPtr<SListView<TSharedPtr<FSpriteOptimizationRow>>> SpriteListView;
    TSharedPtr<STextBlock> SummaryText;
    TSharedPtr<SButton> OptimizeButton;
    TSharedPtr<SButton> PreviewButton;
    
    // === НАСТРОЙКИ ОПТИМИЗАЦИИ ===
    TSharedPtr<SComboBox<TSharedPtr<FString>>> MaterialComboBox;
    TArray<TSharedPtr<FString>> MaterialOptions;
    TArray<UMaterialInterface*> MaterialAssets;
    TSharedPtr<SSpinBox<float>> PixelsPerUnitSpinBox;
    TSharedPtr<SSpinBox<int32>> PaddingSpinBox;
    TSharedPtr<SCheckBox> CreateBackupCheckBox;
    TSharedPtr<SCheckBox> ReplaceOriginalsCheckBox;
    TSharedPtr<SCheckBox> UseProjectSettingsCheckBox;
    
    // === ОБРАБОТЧИКИ СОБЫТИЙ ===
    FReply OnAnalyzeSprites();
    FReply OnOptimizeSprites();
    FReply OnPreviewOptimization();
    FReply OnSelectAll();
    FReply OnSelectNone();
    FReply OnSelectOptimal();
    FReply OnOpenProjectSettings();
    FReply OnResetToDefaults();
    
    // === СОЗДАНИЕ UI СЕКЦИЙ ===
    TSharedRef<SWidget> CreateHeaderSection();
    TSharedRef<SWidget> CreateSettingsSection();
    TSharedRef<SWidget> CreateSpriteListSection();
    TSharedRef<SWidget> CreateSummarySection();
    TSharedRef<SWidget> CreateActionSection();
    
    // === РАБОТА С ТАБЛИЦЕЙ ===
    TSharedRef<ITableRow> GenerateSpriteRow(TSharedPtr<FSpriteOptimizationRow> Item, const TSharedRef<STableViewBase>& OwnerTable);
    void OnSpriteSelectionChanged(TSharedPtr<FSpriteOptimizationRow> Item, ESelectInfo::Type SelectInfo);
    
    // === ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ===
    void UpdateSummary();
    void UpdateButtonStates();
    TArray<TSharedPtr<FSpriteOptimizationRow>> GetSelectedSprites() const;
    void RefreshAnalysis();
    void LoadSettingsFromProject();
    void InitializeMaterialOptions();
    
    // === ОБРАБОТЧИКИ НАСТРОЕК ===
    void OnMaterialChanged(const FAssetData& AssetData);
    void OnMaterialComboChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo);
    void OnPixelsPerUnitChanged(float NewValue);
    void OnPaddingChanged(int32 NewValue);
    void OnCreateBackupChanged(ECheckBoxState NewState);
    void OnReplaceOriginalsChanged(ECheckBoxState NewState);
    void OnUseProjectSettingsChanged(ECheckBoxState NewState);
    
    // === ПОЛУЧЕНИЕ ТЕКСТА СТАТИСТИКИ ===
    FText GetSummaryText() const;
    FText GetSelectedSpritesText() const;
    FText GetPreviewText() const;
    
    // === УВЕДОМЛЕНИЯ И ПРЕДПРОСМОТР ===
    void ShowNotification(const FText& Message, int32 State = 0);
    void ShowOptimizationPreview();
    static FReply ClosePreviewWindow(TSharedPtr<SWindow> WindowToClose);

    TSharedRef<SWidget> CreateCompactSettingsSection();
    TSharedRef<SWidget> CreateCompactSpriteListSection();
    TSharedRef<SWidget> CreateCompactActionSection();
    TSharedRef<ITableRow> GenerateCompactSpriteRow(TSharedPtr<FSpriteOptimizationRow> Item, const TSharedRef<STableViewBase>& OwnerTable);
    FText GetCompactSummaryText() const;
};