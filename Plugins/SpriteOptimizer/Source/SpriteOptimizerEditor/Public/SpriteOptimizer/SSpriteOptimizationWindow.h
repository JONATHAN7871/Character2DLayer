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

    void Construct(const FArguments& InArgs);
    
    // Показ окна оптимизации
    static void ShowOptimizationWindow(const TArray<UPaperSprite*>& Sprites);

private:
    // Данные
    TArray<TSharedPtr<FSpriteOptimizationRow>> SpriteRows;
    TArray<FSpriteOptimizationResult> OptimizationResults;
    FSpriteOptimizationSettings CurrentSettings;
    
    // Виджеты
    TSharedPtr<SListView<TSharedPtr<FSpriteOptimizationRow>>> SpriteListView;
    TSharedPtr<STextBlock> SummaryText;
    TSharedPtr<SButton> OptimizeButton;
    TSharedPtr<SButton> PreviewButton;
    
    // Настройки
    TSharedPtr<SObjectPropertyEntryBox> MaterialSelector;
    TSharedPtr<SSpinBox<float>> PixelsPerUnitSpinBox;
    TSharedPtr<SSpinBox<int32>> PaddingSpinBox;
    TSharedPtr<SCheckBox> CreateBackupCheckBox;
    TSharedPtr<SCheckBox> ReplaceOriginalsCheckBox;
    TSharedPtr<SCheckBox> UseProjectSettingsCheckBox;
    
    // Обработчики событий
    FReply OnAnalyzeSprites();
    FReply OnOptimizeSprites();
    FReply OnPreviewOptimization();
    FReply OnSelectAll();
    FReply OnSelectNone();
    FReply OnSelectOptimal();
    FReply OnOpenProjectSettings();
    FReply OnResetToDefaults();
    
    // Создание UI секций
    TSharedRef<SWidget> CreateHeaderSection();
    TSharedRef<SWidget> CreateSettingsSection();
    TSharedRef<SWidget> CreateSpriteListSection();
    TSharedRef<SWidget> CreateSummarySection();
    TSharedRef<SWidget> CreateActionSection();
    
    // Работа с таблицей
    TSharedRef<ITableRow> GenerateSpriteRow(TSharedPtr<FSpriteOptimizationRow> Item, const TSharedRef<STableViewBase>& OwnerTable);
    void OnSpriteSelectionChanged(TSharedPtr<FSpriteOptimizationRow> Item, ESelectInfo::Type SelectInfo);
    
    // Вспомогательные функции
    void UpdateSummary();
    void UpdateButtonStates();
    TArray<TSharedPtr<FSpriteOptimizationRow>> GetSelectedSprites() const;
    void RefreshAnalysis();
    void LoadSettingsFromProject();
    
    // Обработчики настроек
    void OnMaterialChanged(const FAssetData& AssetData);
    void OnPixelsPerUnitChanged(float NewValue);
    void OnPaddingChanged(int32 NewValue);
    void OnCreateBackupChanged(ECheckBoxState NewState);
    void OnReplaceOriginalsChanged(ECheckBoxState NewState);
    void OnUseProjectSettingsChanged(ECheckBoxState NewState);
    
    // Получение текста статистики
    FText GetSummaryText() const;
    FText GetSelectedSpritesText() const;
    FText GetPreviewText() const;
    
    // Показ уведомлений
    void ShowNotification(const FText& Message, int32 State = 0);
    
    // Создание окна предпросмотра
    void ShowOptimizationPreview();
};