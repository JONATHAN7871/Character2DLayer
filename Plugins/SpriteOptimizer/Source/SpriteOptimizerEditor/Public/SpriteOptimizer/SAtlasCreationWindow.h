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
#include "Widgets/Layout/SBorder.h"
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
    
    // Статический метод для показа окна
    static void ShowAtlasCreationWindow(const TArray<UPaperSprite*>& Sprites);

private:
    // Данные
    TArray<TSharedPtr<FAtlasSpriteInfo>> SpriteInfos;
    FSpriteAtlasSettings AtlasSettings;
    FSpriteAtlasResult LastAnalysisResult;
    TArray<TSharedPtr<FString>> PackingAlgorithmOptions;
    
    // UI элементы - основные настройки
    TSharedPtr<SEditableTextBox> AtlasNameTextBox;
    TSharedPtr<SSpinBox<int32>> MaxWidthSpinBox;
    TSharedPtr<SSpinBox<int32>> MaxHeightSpinBox;
    TSharedPtr<SSpinBox<int32>> SpritePaddingSpinBox;
    TSharedPtr<SComboBox<TSharedPtr<FString>>> PackingAlgorithmComboBox;
    TSharedPtr<SEditableTextBox> AtlasSuffixTextBox;
    
    // UI элементы - опции
    TSharedPtr<SCheckBox> OptimizeSpritesFirstCheckBox;
    TSharedPtr<SCheckBox> CreateIndividualSpritesCheckBox;
    TSharedPtr<SCheckBox> PowerOfTwoCheckBox;
    TSharedPtr<SCheckBox> SquareAtlasCheckBox;
    TSharedPtr<SCheckBox> PreserveQualityCheckBox;      // НОВОЕ
    TSharedPtr<SCheckBox> ForceSmoothingCheckBox;       // НОВОЕ
    
    // UI элементы - список спрайтов
    TSharedPtr<SListView<TSharedPtr<FAtlasSpriteInfo>>> SpritesListView;
    
    // UI элементы - анализ и действия
    TSharedPtr<STextBlock> AnalysisResultText;
    TSharedPtr<SButton> AnalyzeButton;
    TSharedPtr<SButton> CreateAtlasButton;
    TSharedPtr<SButton> PreviewButton;

    // НОВЫЕ UI элементы для компактного отображения
    TSharedPtr<STextBlock> SpritesSummaryText;
    
    // НОВЫЕ методы
    TSharedRef<SWidget> CreateCompactSpritesListSection();
    TSharedRef<ITableRow> GenerateCompactSpriteRow(TSharedPtr<FAtlasSpriteInfo> Item, const TSharedRef<STableViewBase>& OwnerTable);
    FText GetSpritesSummaryText() const;
    
    // Методы создания UI
    TSharedRef<SWidget> CreateHeaderSection();
    TSharedRef<SWidget> CreateAtlasSettingsSection();
    TSharedRef<SWidget> CreateSpriteOptionsSection();
    TSharedRef<SWidget> CreateSpritesListSection();
    TSharedRef<SWidget> CreateAnalysisSection();
    TSharedRef<SWidget> CreateActionsSection();
    
    // Обработчики событий - настройки атласа
    void OnAtlasNameChanged(const FText& NewText, ETextCommit::Type CommitType);
    void OnMaxWidthChanged(int32 NewValue);
    void OnMaxHeightChanged(int32 NewValue);
    void OnSpritePaddingChanged(int32 NewValue);
    void OnPackingAlgorithmChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo);
    void OnAtlasSuffixChanged(const FText& NewText, ETextCommit::Type CommitType);
    
    // Обработчики событий - опции
    void OnOptimizeSpritesFirstChanged(ECheckBoxState NewState);
    void OnCreateIndividualSpritesChanged(ECheckBoxState NewState);
    void OnPowerOfTwoChanged(ECheckBoxState NewState);
    void OnSquareAtlasChanged(ECheckBoxState NewState);
    void OnPreserveQualityChanged(ECheckBoxState NewState);     // НОВОЕ
    void OnForceSmoothingChanged(ECheckBoxState NewState);      // НОВОЕ
    
    // Обработчики событий - действия
    FReply OnAnalyzeAtlas();
    FReply OnCreateAtlas();
    FReply OnPreviewAtlas();
    FReply OnSelectAllSprites();
    FReply OnSelectNoneSprites();
    FReply OnSelectOptimalSprites();
    FReply OnResetToDefaults();
    
    // Работа со списком спрайтов
    TSharedRef<ITableRow> GenerateSpriteRow(TSharedPtr<FAtlasSpriteInfo> Item, const TSharedRef<STableViewBase>& OwnerTable);
    void OnSpriteSelectionChanged(TSharedPtr<FAtlasSpriteInfo> Item, ESelectInfo::Type SelectInfo);
    
    // Вспомогательные методы
    void InitializePackingAlgorithms();
    void LoadDefaultSettings();
    void UpdateAnalysisDisplay();
    void UpdateButtonStates();
    TArray<UPaperSprite*> GetSelectedSprites() const;
    FText GetAnalysisText() const;
    FText GetAtlasInfoText() const;
    
    // Валидация
    bool ValidateSettings() const;
    FText GetValidationErrorText() const;
    
    // Уведомления
    void ShowNotification(const FText& Message, bool bSuccess = true);
    
    // Данные для UI
    FString CurrentAtlasName;

    TSharedRef<SWidget> CreateCompactHeaderSection();
    TSharedRef<SWidget> CreateCompactAtlasSettingsSection();
    TSharedRef<SWidget> CreateCompactSpritesSection();
    TSharedRef<SWidget> CreateCompactAnalysisSection();
    TSharedRef<SWidget> CreateCompactActionSection();
    FText GetCompactAnalysisText() const;
};