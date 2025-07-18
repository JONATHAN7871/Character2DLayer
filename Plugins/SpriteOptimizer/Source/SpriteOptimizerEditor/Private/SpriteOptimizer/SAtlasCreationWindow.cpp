#include "SpriteOptimizer/SAtlasCreationWindow.h"
#include "SpriteOptimizer/SpriteOptimizer.h"
#include "SpriteOptimizer/SAtlasPreviewWindow.h"
#include "Settings/SpriteOptimizerSettings.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/Texture2D.h"
#include "Misc/DateTime.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "SAtlasCreationWindow"

void SAtlasCreationWindow::Construct(const FArguments& InArgs)
{
    // Инициализируем данные
    CurrentAtlasName = FString::Printf(TEXT("Atlas_%d_Sprites"), InArgs._SourceSprites.Num());
    
    // Создаем информацию о спрайтах
    for (UPaperSprite* Sprite : InArgs._SourceSprites)
    {
        if (Sprite)
        {
            TSharedPtr<FAtlasSpriteInfo> SpriteInfo = MakeShared<FAtlasSpriteInfo>(Sprite);
            SpriteInfos.Add(SpriteInfo);
        }
    }
    
    // Инициализируем настройки
    LoadDefaultSettings();
    InitializePackingAlgorithms();
    
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
            
            // Настройки атласа
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                CreateAtlasSettingsSection()
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 2)
            [
                SNew(SSeparator)
            ]
            
            // Опции спрайтов
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                CreateSpriteOptionsSection()
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 2)
            [
                SNew(SSeparator)
            ]
            
            // Список спрайтов
            + SVerticalBox::Slot()
            .FillHeight(0.4f)
            .Padding(0, 5)
            [
                CreateSpritesListSection()
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 2)
            [
                SNew(SSeparator)
            ]
            
            // Анализ
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                CreateAnalysisSection()
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
                CreateActionsSection()
            ]
        ]
    ];
    
    // Начальное обновление UI
    UpdateButtonStates();
}

SAtlasCreationWindow::~SAtlasCreationWindow()
{
    SpriteInfos.Empty();
    LastAnalysisResult = FSpriteAtlasResult();
    UE_LOG(LogTemp, Log, TEXT("AtlasCreationWindow destroyed"));
}

void SAtlasCreationWindow::ShowAtlasCreationWindow(const TArray<UPaperSprite*>& Sprites)
{
    if (Sprites.Num() < 2)
    {
        FNotificationInfo Info(LOCTEXT("AtlasNeedMoreSprites", "Atlas creation requires at least 2 sprites"));
        Info.bFireAndForget = true;
        Info.FadeOutDuration = 3.0f;
        Info.ExpireDuration = 5.0f;
        Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.FailImage"));
        FSlateNotificationManager::Get().AddNotification(Info);
        return;
    }
    
    TSharedRef<SWindow> AtlasWindow = SNew(SWindow)
        .Title(FText::Format(LOCTEXT("AtlasWindowTitle", "Create Sprite Atlas - {0} sprites"), Sprites.Num()))
        .SizingRule(ESizingRule::UserSized)
        .ClientSize(FVector2D(1000, 800))
        .SupportsMaximize(true)
        .SupportsMinimize(false)
        .Content()
        [
            SNew(SAtlasCreationWindow)
            .SourceSprites(Sprites)
        ];
    
    FSlateApplication::Get().AddWindow(AtlasWindow);
}

TSharedRef<SWidget> SAtlasCreationWindow::CreateHeaderSection()
{
    return SNew(SVerticalBox)
        
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(STextBlock)
            .Text(LOCTEXT("AtlasCreationTitle", "🎨 Create Sprite Atlas"))
            .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
            .Justification(ETextJustify::Center)
        ]
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 5)
        [
            SNew(STextBlock)
            .Text(FText::Format(LOCTEXT("AtlasCreationSubtitle", 
                "Combine {0} sprites into a single optimized texture atlas for better performance and memory usage"), 
                SpriteInfos.Num()))
            .Justification(ETextJustify::Center)
            .AutoWrapText(true)
        ];
}

TSharedRef<SWidget> SAtlasCreationWindow::CreateAtlasSettingsSection()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
        .Padding(10)
        [
            SNew(SVerticalBox)
            
            // Заголовок секции
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SHorizontalBox)
                
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("AtlasSettingsTitle", "⚙️ Atlas Settings"))
                    .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SButton)
                    .Text(LOCTEXT("ResetDefaults", "Reset to Defaults"))
                    .OnClicked(this, &SAtlasCreationWindow::OnResetToDefaults)
                    .ToolTipText(LOCTEXT("ResetDefaultsTooltip", "Reset all settings to default values"))
                ]
            ]
            
            // Основные настройки
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10)
            [
                SNew(SUniformGridPanel)
                .SlotPadding(FMargin(5, 2))
                
                // Имя атласа
                + SUniformGridPanel::Slot(0, 0)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("AtlasNameLabel", "Atlas Name:"))
                ]
                
                + SUniformGridPanel::Slot(1, 0)
                [
                    SAssignNew(AtlasNameTextBox, SEditableTextBox)
                    .Text(FText::FromString(CurrentAtlasName))
                    .OnTextCommitted(this, &SAtlasCreationWindow::OnAtlasNameChanged)
                    .ToolTipText(LOCTEXT("AtlasNameTooltip", "Name for the atlas texture asset"))
                ]
                
                // Максимальная ширина
                + SUniformGridPanel::Slot(0, 1)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("MaxWidthLabel", "Max Width:"))
                ]
                
                + SUniformGridPanel::Slot(1, 1)
                [
                    SAssignNew(MaxWidthSpinBox, SSpinBox<int32>)
                    .Value(AtlasSettings.MaxAtlasSize.X)
                    .MinValue(256)
                    .MaxValue(8192)
                    .OnValueChanged(this, &SAtlasCreationWindow::OnMaxWidthChanged)
                    .ToolTipText(LOCTEXT("MaxWidthTooltip", "Maximum atlas width in pixels"))
                ]
                
                // Максимальная высота
                + SUniformGridPanel::Slot(0, 2)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("MaxHeightLabel", "Max Height:"))
                ]
                
                + SUniformGridPanel::Slot(1, 2)
                [
                    SAssignNew(MaxHeightSpinBox, SSpinBox<int32>)
                    .Value(AtlasSettings.MaxAtlasSize.Y)
                    .MinValue(256)
                    .MaxValue(8192)
                    .OnValueChanged(this, &SAtlasCreationWindow::OnMaxHeightChanged)
                    .ToolTipText(LOCTEXT("MaxHeightTooltip", "Maximum atlas height in pixels"))
                ]
                
                // Отступ между спрайтами
                + SUniformGridPanel::Slot(0, 3)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("SpritePaddingLabel", "Sprite Padding:"))
                ]
                
                + SUniformGridPanel::Slot(1, 3)
                [
                    SAssignNew(SpritePaddingSpinBox, SSpinBox<int32>)
                    .Value(AtlasSettings.SpritePadding)
                    .MinValue(0)
                    .MaxValue(20)
                    .OnValueChanged(this, &SAtlasCreationWindow::OnSpritePaddingChanged)
                    .ToolTipText(LOCTEXT("SpritePaddingTooltip", "Pixels between sprites in atlas"))
                ]
                
                // Алгоритм упаковки
                + SUniformGridPanel::Slot(0, 4)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("PackingAlgorithmLabel", "Packing Algorithm:"))
                ]
                
                + SUniformGridPanel::Slot(1, 4)
                [
                    SAssignNew(PackingAlgorithmComboBox, SComboBox<TSharedPtr<FString>>)
                    .OptionsSource(&PackingAlgorithmOptions)
                    .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                    {
                        return SNew(STextBlock).Text(FText::FromString(*Item));
                    })
                    .OnSelectionChanged(this, &SAtlasCreationWindow::OnPackingAlgorithmChanged)
                    .Content()
                    [
                        SNew(STextBlock)
                        .Text_Lambda([this]()
                        {
                            if (PackingAlgorithmComboBox.IsValid() && PackingAlgorithmComboBox->GetSelectedItem().IsValid())
                            {
                                return FText::FromString(*PackingAlgorithmComboBox->GetSelectedItem());
                            }
                            return LOCTEXT("SimpleGrid", "Simple Grid");
                        })
                    ]
                    .ToolTipText(LOCTEXT("PackingAlgorithmTooltip", "Algorithm for arranging sprites in atlas"))
                ]
                
                // Суффикс атласа
                + SUniformGridPanel::Slot(0, 5)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("AtlasSuffixLabel", "Atlas Suffix:"))
                ]
                
                + SUniformGridPanel::Slot(1, 5)
                [
                    SAssignNew(AtlasSuffixTextBox, SEditableTextBox)
                    .Text(FText::FromString(AtlasSettings.AtlasSuffix))
                    .OnTextCommitted(this, &SAtlasCreationWindow::OnAtlasSuffixChanged)
                    .ToolTipText(LOCTEXT("AtlasSuffixTooltip", "Suffix for atlas-related asset names"))
                ]
            ]
        ];
}

TSharedRef<SWidget> SAtlasCreationWindow::CreateSpriteOptionsSection()
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
                .Text(LOCTEXT("SpriteOptionsTitle", "🎯 Sprite Processing Options"))
                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10)
            [
                SNew(SUniformGridPanel)
                .SlotPadding(FMargin(10, 5))
                
                // Первая колонка
                + SUniformGridPanel::Slot(0, 0)
                [
                    SAssignNew(OptimizeSpritesFirstCheckBox, SCheckBox)
                    .IsChecked(AtlasSettings.bOptimizeSpritesFirst ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                    .OnCheckStateChanged(this, &SAtlasCreationWindow::OnOptimizeSpritesFirstChanged)
                    .Content()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("OptimizeFirstLabel", "Optimize Sprites First"))
                        .ToolTipText(LOCTEXT("OptimizeFirstTooltip", "Remove transparent areas from sprites before creating atlas"))
                    ]
                ]
                
                + SUniformGridPanel::Slot(0, 1)
                [
                    SAssignNew(CreateIndividualSpritesCheckBox, SCheckBox)
                    .IsChecked(AtlasSettings.bCreateIndividualSprites ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                    .OnCheckStateChanged(this, &SAtlasCreationWindow::OnCreateIndividualSpritesChanged)
                    .Content()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("CreateIndividualLabel", "Create Individual Sprites"))
                        .ToolTipText(LOCTEXT("CreateIndividualTooltip", "Create separate sprite assets from atlas regions"))
                    ]
                ]
                
                + SUniformGridPanel::Slot(0, 2)
                [
                    SAssignNew(PowerOfTwoCheckBox, SCheckBox)
                    .IsChecked(ECheckBoxState::Unchecked)
                    .OnCheckStateChanged(this, &SAtlasCreationWindow::OnPowerOfTwoChanged)
                    .Content()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("PowerOfTwoLabel", "Power of Two Size"))
                        .ToolTipText(LOCTEXT("PowerOfTwoTooltip", "Force atlas size to be power of two (512, 1024, 2048, etc.)"))
                    ]
                ]
                
                + SUniformGridPanel::Slot(0, 3)
                [
                    SAssignNew(SquareAtlasCheckBox, SCheckBox)
                    .IsChecked(ECheckBoxState::Unchecked)
                    .OnCheckStateChanged(this, &SAtlasCreationWindow::OnSquareAtlasChanged)
                    .Content()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("SquareAtlasLabel", "Square Atlas"))
                        .ToolTipText(LOCTEXT("SquareAtlasTooltip", "Make atlas width and height equal"))
                    ]
                ]
                
                // Вторая колонка - НОВЫЕ ОПЦИИ КАЧЕСТВА
                + SUniformGridPanel::Slot(1, 0)
                [
                    SAssignNew(PreserveQualityCheckBox, SCheckBox)
                    .IsChecked(AtlasSettings.bPreserveOriginalQuality ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                    .OnCheckStateChanged(this, &SAtlasCreationWindow::OnPreserveQualityChanged)
                    .Content()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("PreserveQualityLabel", "Preserve Original Quality"))
                        .ToolTipText(LOCTEXT("PreserveQualityTooltip", "Copy texture quality settings from original sprites (recommended)"))
                    ]
                ]
                
                + SUniformGridPanel::Slot(1, 1)
                [
                    SAssignNew(ForceSmoothingCheckBox, SCheckBox)
                    .IsChecked(AtlasSettings.bForceSmoothing ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                    .OnCheckStateChanged(this, &SAtlasCreationWindow::OnForceSmoothingChanged)
                    .Content()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("ForceSmoothingLabel", "Force Smoothing"))
                        .ToolTipText(LOCTEXT("ForceSmoothingTooltip", "Apply bilinear filtering for smooth appearance (good for character art like eyes)"))
                    ]
                ]
                
                // Информационная подсказка
                + SUniformGridPanel::Slot(1, 2)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("QualityHint", "💡 For character sprites (eyes, faces):\nUse 'Force Smoothing'"))
                    .AutoWrapText(true)
                    .Font(FAppStyle::GetFontStyle("SmallFont"))
                    .ColorAndOpacity(FSlateColor(FLinearColor::Yellow))
                ]
                
                + SUniformGridPanel::Slot(1, 3)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("QualityHint2", "🎯 For pixel art:\nDisable both quality options"))
                    .AutoWrapText(true)
                    .Font(FAppStyle::GetFontStyle("SmallFont"))
                    .ColorAndOpacity(FSlateColor(FColor::Cyan))
                ]
            ]
        ];
}

TSharedRef<SWidget> SAtlasCreationWindow::CreateSpritesListSection()
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
                    .Text(LOCTEXT("SpritesListTitle", "📋 Sprites"))
                    .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(5, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("SelectAll", "All"))
                    .OnClicked(this, &SAtlasCreationWindow::OnSelectAllSprites)
                    .ToolTipText(LOCTEXT("SelectAllTooltip", "Include all sprites in atlas"))
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(5, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("SelectNone", "None"))
                    .OnClicked(this, &SAtlasCreationWindow::OnSelectNoneSprites)
                    .ToolTipText(LOCTEXT("SelectNoneTooltip", "Exclude all sprites from atlas"))
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(5, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("SelectOptimal", "Optimal"))
                    .OnClicked(this, &SAtlasCreationWindow::OnSelectOptimalSprites)
                    .ToolTipText(LOCTEXT("SelectOptimalTooltip", "Include only sprites that benefit from optimization"))
                ]
            ]
            
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(0, 5)
            [
                SAssignNew(SpritesListView, SListView<TSharedPtr<FAtlasSpriteInfo>>)
                .ListItemsSource(&SpriteInfos)
                .OnGenerateRow(this, &SAtlasCreationWindow::GenerateSpriteRow)
                .OnSelectionChanged(this, &SAtlasCreationWindow::OnSpriteSelectionChanged)
                .SelectionMode(ESelectionMode::Multi)
                .HeaderRow
                (
                    SNew(SHeaderRow)
                    
                    + SHeaderRow::Column("Include")
                    .DefaultLabel(LOCTEXT("IncludeHeader", "✓"))
                    .FixedWidth(30)
                    
                    + SHeaderRow::Column("SpriteName")
                    .DefaultLabel(LOCTEXT("SpriteNameHeader", "Sprite Name"))
                    .FillWidth(0.3f)
                    
                    + SHeaderRow::Column("OriginalSize")
                    .DefaultLabel(LOCTEXT("OriginalSizeHeader", "Original Size"))
                    .FillWidth(0.2f)
                    
                    + SHeaderRow::Column("OptimizedSize")
                    .DefaultLabel(LOCTEXT("OptimizedSizeHeader", "Optimized Size"))
                    .FillWidth(0.2f)
                    
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

TSharedRef<SWidget> SAtlasCreationWindow::CreateAnalysisSection()
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
                .Text(LOCTEXT("AnalysisTitle", "📊 Atlas Analysis"))
                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10)
            [
                SAssignNew(AnalysisResultText, STextBlock)
                .Text(GetAnalysisText())
                .AutoWrapText(true)
                .Visibility_Lambda([this]()
                {
                    return LastAnalysisResult.TotalSprites > 0 ? EVisibility::Visible : EVisibility::Collapsed;
                })
            ]
        ];
}

TSharedRef<SWidget> SAtlasCreationWindow::CreateActionsSection()
{
    return SNew(SHorizontalBox)
        
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(5)
        [
            SAssignNew(AnalyzeButton, SButton)
            .Text(LOCTEXT("AnalyzeAtlas", "🔍 Analyze Atlas"))
            .OnClicked(this, &SAtlasCreationWindow::OnAnalyzeAtlas)
            .HAlign(HAlign_Center)
            .ToolTipText(LOCTEXT("AnalyzeAtlasTooltip", "Analyze atlas creation feasibility with current settings"))
        ]
        
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(5)
        [
            SAssignNew(PreviewButton, SButton)
            .Text(LOCTEXT("PreviewAtlas", "👁️ Preview"))
            .OnClicked(this, &SAtlasCreationWindow::OnPreviewAtlas)
            .HAlign(HAlign_Center)
            .ToolTipText(LOCTEXT("PreviewAtlasTooltip", "Preview atlas layout before creation"))
            .IsEnabled(false)
        ]
        
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(5)
        [
            SAssignNew(CreateAtlasButton, SButton)
            .Text(LOCTEXT("CreateAtlas", "🎨 Create Atlas"))
            .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
            .OnClicked(this, &SAtlasCreationWindow::OnCreateAtlas)
            .HAlign(HAlign_Center)
            .ToolTipText(LOCTEXT("CreateAtlasTooltip", "Create the sprite atlas with current settings"))
            .IsEnabled(false)
        ];
}

void SAtlasCreationWindow::InitializePackingAlgorithms()
{
    PackingAlgorithmOptions.Empty();
    PackingAlgorithmOptions.Add(MakeShared<FString>(TEXT("Simple Grid")));
    PackingAlgorithmOptions.Add(MakeShared<FString>(TEXT("Best Fit")));
    PackingAlgorithmOptions.Add(MakeShared<FString>(TEXT("MaxRects Algorithm")));
    
    // Устанавливаем значение по умолчанию
    if (PackingAlgorithmComboBox.IsValid())
    {
        PackingAlgorithmComboBox->SetSelectedItem(PackingAlgorithmOptions[0]);
    }
}

void SAtlasCreationWindow::LoadDefaultSettings()
{
    const USpriteOptimizerSettings* ProjectSettings = GetDefault<USpriteOptimizerSettings>();
    if (ProjectSettings)
    {
        AtlasSettings.MaxAtlasSize = ProjectSettings->DefaultMaxAtlasSize;
        AtlasSettings.SpritePadding = ProjectSettings->DefaultAtlasSpritePadding;
        AtlasSettings.bOptimizeSpritesFirst = ProjectSettings->bDefaultOptimizeSpritesForAtlas;
        AtlasSettings.bCreateIndividualSprites = ProjectSettings->bDefaultCreateIndividualSprites;
        AtlasSettings.AtlasSuffix = ProjectSettings->DefaultAtlasSuffix;
    }
}

// Обработчики событий - настройки атласа
void SAtlasCreationWindow::OnAtlasNameChanged(const FText& NewText, ETextCommit::Type CommitType)
{
    CurrentAtlasName = NewText.ToString();
}

void SAtlasCreationWindow::OnMaxWidthChanged(int32 NewValue)
{
    AtlasSettings.MaxAtlasSize.X = NewValue;
    LastAnalysisResult = FSpriteAtlasResult(); // Сбрасываем анализ
    UpdateButtonStates();
}

void SAtlasCreationWindow::OnMaxHeightChanged(int32 NewValue)
{
    AtlasSettings.MaxAtlasSize.Y = NewValue;
    LastAnalysisResult = FSpriteAtlasResult(); // Сбрасываем анализ
    UpdateButtonStates();
}

void SAtlasCreationWindow::OnSpritePaddingChanged(int32 NewValue)
{
    AtlasSettings.SpritePadding = NewValue;
    LastAnalysisResult = FSpriteAtlasResult(); // Сбрасываем анализ
    UpdateButtonStates();
}

void SAtlasCreationWindow::OnPackingAlgorithmChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo)
{
    if (SelectedItem.IsValid())
    {
        FString Selected = *SelectedItem;
        if (Selected == TEXT("Simple Grid"))
        {
            AtlasSettings.PackingAlgorithm = EAtlasPackingAlgorithm::Simple;
        }
        else if (Selected == TEXT("Best Fit"))
        {
            AtlasSettings.PackingAlgorithm = EAtlasPackingAlgorithm::BestFit;
        }
        else if (Selected == TEXT("MaxRects Algorithm"))
        {
            AtlasSettings.PackingAlgorithm = EAtlasPackingAlgorithm::MaxRects;
        }
        
        LastAnalysisResult = FSpriteAtlasResult(); // Сбрасываем анализ
        UpdateButtonStates();
    }
}

void SAtlasCreationWindow::OnAtlasSuffixChanged(const FText& NewText, ETextCommit::Type CommitType)
{
    AtlasSettings.AtlasSuffix = NewText.ToString();
}

// Обработчики событий - опции
void SAtlasCreationWindow::OnOptimizeSpritesFirstChanged(ECheckBoxState NewState)
{
    AtlasSettings.bOptimizeSpritesFirst = (NewState == ECheckBoxState::Checked);
    
    // Пересчитываем размеры спрайтов
    for (auto& SpriteInfo : SpriteInfos)
    {
        if (SpriteInfo->Sprite)
        {
            if (AtlasSettings.bOptimizeSpritesFirst)
            {
                SpriteInfo->OptimizedSize = FIntPoint(SpriteInfo->AnalysisResult.OptimizedSize.X, SpriteInfo->AnalysisResult.OptimizedSize.Y);
            }
            else
            {
                UTexture2D* SourceTexture = SpriteInfo->Sprite->GetSourceTexture();
                if (SourceTexture)
                {
                    SpriteInfo->OptimizedSize = FIntPoint(SourceTexture->GetSizeX(), SourceTexture->GetSizeY());
                }
            }
        }
    }
    
    LastAnalysisResult = FSpriteAtlasResult(); // Сбрасываем анализ
    UpdateButtonStates();
    
    if (SpritesListView.IsValid())
    {
        SpritesListView->RequestListRefresh();
    }
}

void SAtlasCreationWindow::OnCreateIndividualSpritesChanged(ECheckBoxState NewState)
{
    AtlasSettings.bCreateIndividualSprites = (NewState == ECheckBoxState::Checked);
}

void SAtlasCreationWindow::OnPowerOfTwoChanged(ECheckBoxState NewState)
{
    // Реализация принудительного размера power of two
    bool bPowerOfTwo = (NewState == ECheckBoxState::Checked);
    
    if (bPowerOfTwo)
    {
        // Округляем текущие размеры до ближайшего power of two
        int32 PowerOfTwoWidth = FMath::RoundUpToPowerOfTwo(AtlasSettings.MaxAtlasSize.X);
        int32 PowerOfTwoHeight = FMath::RoundUpToPowerOfTwo(AtlasSettings.MaxAtlasSize.Y);
        
        AtlasSettings.MaxAtlasSize.X = PowerOfTwoWidth;
        AtlasSettings.MaxAtlasSize.Y = PowerOfTwoHeight;
        
        if (MaxWidthSpinBox.IsValid())
        {
            MaxWidthSpinBox->SetValue(PowerOfTwoWidth);
        }
        if (MaxHeightSpinBox.IsValid())
        {
            MaxHeightSpinBox->SetValue(PowerOfTwoHeight);
        }
    }
}

void SAtlasCreationWindow::OnSquareAtlasChanged(ECheckBoxState NewState)
{
    bool bSquare = (NewState == ECheckBoxState::Checked);
    
    if (bSquare)
    {
        // Делаем атлас квадратным, используя максимальное значение
        int32 MaxSize = FMath::Max(AtlasSettings.MaxAtlasSize.X, AtlasSettings.MaxAtlasSize.Y);
        AtlasSettings.MaxAtlasSize.X = MaxSize;
        AtlasSettings.MaxAtlasSize.Y = MaxSize;
        
        if (MaxWidthSpinBox.IsValid())
        {
            MaxWidthSpinBox->SetValue(MaxSize);
        }
        if (MaxHeightSpinBox.IsValid())
        {
            MaxHeightSpinBox->SetValue(MaxSize);
        }
    }
}

// Обработчики действий
FReply SAtlasCreationWindow::OnAnalyzeAtlas()
{
    TArray<UPaperSprite*> SelectedSprites = GetSelectedSprites();
    
    if (SelectedSprites.Num() < 2)
    {
        ShowNotification(LOCTEXT("NeedMoreSprites", "Select at least 2 sprites for atlas creation"), false);
        return FReply::Handled();
    }
    
    ShowNotification(LOCTEXT("AnalyzingAtlas", "Analyzing atlas..."), true);
    
    LastAnalysisResult = USpriteOptimizer::AnalyzeSpriteAtlas(SelectedSprites, AtlasSettings);
    
    UpdateAnalysisDisplay();
    UpdateButtonStates();
    
    if (LastAnalysisResult.bSuccess)
    {
        ShowNotification(LOCTEXT("AnalysisComplete", "Atlas analysis complete!"), true);
    }
    else
    {
        ShowNotification(FText::FromString(LastAnalysisResult.ErrorMessage), false);
    }
    
    return FReply::Handled();
}

FReply SAtlasCreationWindow::OnCreateAtlas()
{
    if (!ValidateSettings())
    {
        ShowNotification(GetValidationErrorText(), false);
        return FReply::Handled();
    }
    
    TArray<UPaperSprite*> SelectedSprites = GetSelectedSprites();
    
    if (SelectedSprites.Num() < 2)
    {
        ShowNotification(LOCTEXT("NeedMoreSpritesCreate", "Select at least 2 sprites to create atlas"), false);
        return FReply::Handled();
    }
    
    FText ProcessingText = FText::Format(LOCTEXT("CreatingAtlas", "Creating atlas '{0}' from {1} sprites..."), 
                                        FText::FromString(CurrentAtlasName), SelectedSprites.Num());
    ShowNotification(ProcessingText, true);
    
    // Создаем атлас
    FSpriteAtlasResult Result = USpriteOptimizer::CreateSpriteAtlas(SelectedSprites, AtlasSettings, CurrentAtlasName);
    
    if (Result.bSuccess)
    {
        FText SuccessText = FText::Format(LOCTEXT("AtlasCreatedSuccess", 
            "✅ Atlas '{0}' created successfully!\n"
            "📐 Size: {1}x{2}\n"
            "📊 Efficiency: {3}%\n"
            "💰 Memory savings: {4}%\n"
            "🎨 Individual sprites: {5}"), 
            FText::FromString(CurrentAtlasName),
            Result.AtlasSize.X, Result.AtlasSize.Y,
            FText::AsNumber(Result.PackingEfficiency),
            FText::AsNumber(Result.MemorySavings),
            Result.CreatedSprites.Num());
            
        ShowNotification(SuccessText, true);
        
        // Обновляем Content Browser
        USpriteOptimizer::RefreshContentBrowser();
        
        // Закрываем окно через 3 секунды
        FTimerHandle TimerHandle;
        FTimerDelegate TimerDelegate;
        TimerDelegate.BindLambda([this]()
        {
            SpriteInfos.Empty();
            LastAnalysisResult = FSpriteAtlasResult();
            
            TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
            if (ParentWindow.IsValid())
            {
                ParentWindow->RequestDestroyWindow();
            }
        });
        
        GEditor->GetTimerManager()->SetTimer(TimerHandle, TimerDelegate, 3.0f, false);
    }
    else
    {
        ShowNotification(FText::FromString(Result.ErrorMessage), false);
    }
    
    return FReply::Handled();
}

FReply SAtlasCreationWindow::OnPreviewAtlas()
{
    // Проверяем, что анализ выполнен
    if (!LastAnalysisResult.bSuccess || LastAnalysisResult.TotalSprites == 0)
    {
        ShowNotification(LOCTEXT("PreviewNeedsAnalysis", "Please run 'Analyze Atlas' first to preview the layout"), false);
        return FReply::Handled();
    }
    
    // Получаем выбранные спрайты
    TArray<UPaperSprite*> SelectedSprites = GetSelectedSprites();
    
    if (SelectedSprites.Num() < 2)
    {
        ShowNotification(LOCTEXT("PreviewNeedsSprites", "Select at least 2 sprites to preview atlas"), false);
        return FReply::Handled();
    }
    
    // Показываем окно предпросмотра
    SAtlasPreviewWindow::ShowAtlasPreview(SelectedSprites, AtlasSettings, LastAnalysisResult);
    
    return FReply::Handled();
}

FReply SAtlasCreationWindow::OnSelectAllSprites()
{
    for (auto& SpriteInfo : SpriteInfos)
    {
        SpriteInfo->bIncludeInAtlas = true;
    }
    
    if (SpritesListView.IsValid())
    {
        SpritesListView->RequestListRefresh();
    }
    
    UpdateButtonStates();
    return FReply::Handled();
}

FReply SAtlasCreationWindow::OnSelectNoneSprites()
{
    for (auto& SpriteInfo : SpriteInfos)
    {
        SpriteInfo->bIncludeInAtlas = false;
    }
    
    if (SpritesListView.IsValid())
    {
        SpritesListView->RequestListRefresh();
    }
    
    UpdateButtonStates();
    return FReply::Handled();
}

FReply SAtlasCreationWindow::OnSelectOptimalSprites()
{
    const USpriteOptimizerSettings* ProjectSettings = GetDefault<USpriteOptimizerSettings>();
    float MinSavings = ProjectSettings ? ProjectSettings->MinimumSavingsForAutoSelect : 30.0f;
    
    for (auto& SpriteInfo : SpriteInfos)
    {
        SpriteInfo->bIncludeInAtlas = SpriteInfo->AnalysisResult.bSuccess && 
                                     SpriteInfo->AnalysisResult.SavingsPercent > MinSavings;
    }
    
    if (SpritesListView.IsValid())
    {
        SpritesListView->RequestListRefresh();
    }
    
    UpdateButtonStates();
    return FReply::Handled();
}

FReply SAtlasCreationWindow::OnResetToDefaults()
{
    LoadDefaultSettings();
    
    // Обновляем UI элементы
    if (MaxWidthSpinBox.IsValid())
    {
        MaxWidthSpinBox->SetValue(AtlasSettings.MaxAtlasSize.X);
    }
    if (MaxHeightSpinBox.IsValid())
    {
        MaxHeightSpinBox->SetValue(AtlasSettings.MaxAtlasSize.Y);
    }
    if (SpritePaddingSpinBox.IsValid())
    {
        SpritePaddingSpinBox->SetValue(AtlasSettings.SpritePadding);
    }
    if (OptimizeSpritesFirstCheckBox.IsValid())
    {
        OptimizeSpritesFirstCheckBox->SetIsChecked(AtlasSettings.bOptimizeSpritesFirst ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
    }
    if (CreateIndividualSpritesCheckBox.IsValid())
    {
        CreateIndividualSpritesCheckBox->SetIsChecked(AtlasSettings.bCreateIndividualSprites ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
    }
    if (AtlasSuffixTextBox.IsValid())
    {
        AtlasSuffixTextBox->SetText(FText::FromString(AtlasSettings.AtlasSuffix));
    }
    if (PackingAlgorithmComboBox.IsValid())
    {
        PackingAlgorithmComboBox->SetSelectedItem(PackingAlgorithmOptions[0]);
    }
    if (PowerOfTwoCheckBox.IsValid())
    {
        PowerOfTwoCheckBox->SetIsChecked(ECheckBoxState::Unchecked);
    }
    if (SquareAtlasCheckBox.IsValid())
    {
        SquareAtlasCheckBox->SetIsChecked(ECheckBoxState::Unchecked);
    }
    
    // Сбрасываем анализ
    LastAnalysisResult = FSpriteAtlasResult();
    UpdateButtonStates();
    
    ShowNotification(LOCTEXT("SettingsReset", "Settings reset to defaults"), true);
    
    return FReply::Handled();
}

// Работа со списком спрайтов
TSharedRef<ITableRow> SAtlasCreationWindow::GenerateSpriteRow(TSharedPtr<FAtlasSpriteInfo> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(STableRow<TSharedPtr<FAtlasSpriteInfo>>, OwnerTable)
        [
            SNew(SHorizontalBox)
            
            // Checkbox включения
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .HAlign(HAlign_Center)
            .Padding(5, 0)
            [
                SNew(SCheckBox)
                .IsChecked(Item->bIncludeInAtlas ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                .OnCheckStateChanged_Lambda([Item, this](ECheckBoxState NewState)
                {
                    Item->bIncludeInAtlas = (NewState == ECheckBoxState::Checked);
                    UpdateButtonStates();
                })
            ]
            
            // Имя спрайта
            + SHorizontalBox::Slot()
            .FillWidth(0.3f)
            .VAlign(VAlign_Center)
            .Padding(5, 2)
            [
                SNew(STextBlock)
                .Text(FText::FromString(Item->Sprite ? Item->Sprite->GetName() : TEXT("Unknown")))
                .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
            ]
            
            // Оригинальный размер
            + SHorizontalBox::Slot()
            .FillWidth(0.2f)
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
            
            // Оптимизированный размер
            + SHorizontalBox::Slot()
            .FillWidth(0.2f)
            .VAlign(VAlign_Center)
            .Padding(5, 2)
            [
                SNew(STextBlock)
                .Text_Lambda([Item, this]()
                {
                    if (AtlasSettings.bOptimizeSpritesFirst && Item->AnalysisResult.bSuccess)
                    {
                        return FText::FromString(FString::Printf(TEXT("%.0fx%.0f"), 
                            Item->AnalysisResult.OptimizedSize.X, Item->AnalysisResult.OptimizedSize.Y));
                    }
                    else if (Item->Sprite && Item->Sprite->GetSourceTexture())
                    {
                        UTexture2D* Texture = Item->Sprite->GetSourceTexture();
                        return FText::FromString(FString::Printf(TEXT("%.0fx%.0f"), 
                            (float)Texture->GetSizeX(), (float)Texture->GetSizeY()));
                    }
                    return FText::FromString(TEXT("N/A"));
                })
                .ColorAndOpacity_Lambda([this]()
                {
                    return AtlasSettings.bOptimizeSpritesFirst ? FSlateColor(FLinearColor::Green) : FSlateColor(FLinearColor::White);
                })
            ]
            
            // Экономия
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
                    if (Item->AnalysisResult.SavingsPercent > 50.0f) return FSlateColor(FLinearColor::Green);
                    if (Item->AnalysisResult.SavingsPercent > 25.0f) return FSlateColor(FLinearColor::Yellow);
                    return FSlateColor(FLinearColor::White);
                })
            ]
            
            // Статус
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
                        return FText::FromString(TEXT("🟢 Excellent"));
                    }
                    else if (Item->AnalysisResult.SavingsPercent > 25.0f)
                    {
                        return FText::FromString(TEXT("🟡 Good"));
                    }
                    else
                    {
                        return FText::FromString(TEXT("🔴 Poor"));
                    }
                })
            ]
        ];
}

void SAtlasCreationWindow::OnSpriteSelectionChanged(TSharedPtr<FAtlasSpriteInfo> Item, ESelectInfo::Type SelectInfo)
{
    UpdateButtonStates();
}

// Вспомогательные методы
void SAtlasCreationWindow::UpdateAnalysisDisplay()
{
    if (AnalysisResultText.IsValid())
    {
        AnalysisResultText->SetText(GetAnalysisText());
    }
}

void SAtlasCreationWindow::UpdateButtonStates()
{
    TArray<UPaperSprite*> SelectedSprites = GetSelectedSprites();
    bool bHasEnoughSprites = SelectedSprites.Num() >= 2;
    bool bHasValidAnalysis = LastAnalysisResult.bSuccess && LastAnalysisResult.TotalSprites > 0;
    
    if (AnalyzeButton.IsValid())
    {
        AnalyzeButton->SetEnabled(bHasEnoughSprites);
    }
    
    if (CreateAtlasButton.IsValid())
    {
        CreateAtlasButton->SetEnabled(bHasValidAnalysis && ValidateSettings());
    }
    
    if (PreviewButton.IsValid())
    {
        PreviewButton->SetEnabled(bHasValidAnalysis);
    }
}

TArray<UPaperSprite*> SAtlasCreationWindow::GetSelectedSprites() const
{
    TArray<UPaperSprite*> SelectedSprites;
    
    for (const auto& SpriteInfo : SpriteInfos)
    {
        if (SpriteInfo->bIncludeInAtlas && SpriteInfo->Sprite)
        {
            SelectedSprites.Add(SpriteInfo->Sprite.Get());
        }
    }
    
    return SelectedSprites;
}

FText SAtlasCreationWindow::GetAnalysisText() const
{
    if (LastAnalysisResult.TotalSprites == 0)
    {
        return LOCTEXT("NoAnalysisYet", "Click 'Analyze Atlas' to see detailed statistics");
    }
    
    if (!LastAnalysisResult.bSuccess)
    {
        return FText::Format(LOCTEXT("AnalysisError", "❌ Analysis failed: {0}"), 
                           FText::FromString(LastAnalysisResult.ErrorMessage));
    }
    
    return FText::Format(LOCTEXT("AnalysisSuccess",
        "✅ Atlas Analysis Results:\n"
        "📐 Estimated atlas size: {0}x{1} pixels\n"
        "📦 Packing efficiency: {2}%\n"
        "💾 Memory usage: {3} MB\n"
        "💰 Memory savings: {4}%\n"
        "🎨 Sprites included: {5}"),
        LastAnalysisResult.AtlasSize.X, LastAnalysisResult.AtlasSize.Y,
        FText::AsNumber(LastAnalysisResult.PackingEfficiency),
        FText::AsNumber((LastAnalysisResult.AtlasSize.X * LastAnalysisResult.AtlasSize.Y * 4) / (1024.0f * 1024.0f)),
        FText::AsNumber(LastAnalysisResult.MemorySavings),
        LastAnalysisResult.TotalSprites
    );
}

FText SAtlasCreationWindow::GetAtlasInfoText() const
{
    TArray<UPaperSprite*> SelectedSprites = GetSelectedSprites();
    return FText::Format(LOCTEXT("AtlasInfoFormat", 
        "Selected {0} sprites for atlas creation"), SelectedSprites.Num());
}

bool SAtlasCreationWindow::ValidateSettings() const
{
    if (CurrentAtlasName.IsEmpty())
    {
        return false;
    }
    
    if (AtlasSettings.MaxAtlasSize.X < 256 || AtlasSettings.MaxAtlasSize.Y < 256)
    {
        return false;
    }
    
    if (AtlasSettings.MaxAtlasSize.X > 8192 || AtlasSettings.MaxAtlasSize.Y > 8192)
    {
        return false;
    }
    
    return true;
}

FText SAtlasCreationWindow::GetValidationErrorText() const
{
    if (CurrentAtlasName.IsEmpty())
    {
        return LOCTEXT("EmptyAtlasName", "Atlas name cannot be empty");
    }
    
    if (AtlasSettings.MaxAtlasSize.X < 256 || AtlasSettings.MaxAtlasSize.Y < 256)
    {
        return LOCTEXT("AtlasTooSmall", "Atlas size must be at least 256x256 pixels");
    }
    
    if (AtlasSettings.MaxAtlasSize.X > 8192 || AtlasSettings.MaxAtlasSize.Y > 8192)
    {
        return LOCTEXT("AtlasTooBig", "Atlas size cannot exceed 8192x8192 pixels");
    }
    
    return LOCTEXT("UnknownValidationError", "Settings validation failed");
}

void SAtlasCreationWindow::ShowNotification(const FText& Message, bool bSuccess)
{
    FNotificationInfo Info(Message);
    Info.bFireAndForget = true;
    Info.FadeOutDuration = 3.0f;
    Info.ExpireDuration = 5.0f;
    
    if (bSuccess)
    {
        Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.SuccessImage"));
    }
    else
    {
        Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.FailImage"));
    }
    
    FSlateNotificationManager::Get().AddNotification(Info);
}

void SAtlasCreationWindow::OnPreserveQualityChanged(ECheckBoxState NewState)
{
    AtlasSettings.bPreserveOriginalQuality = (NewState == ECheckBoxState::Checked);
    
    // Если включено сохранение качества, отключаем принудительное сглаживание
    if (AtlasSettings.bPreserveOriginalQuality && ForceSmoothingCheckBox.IsValid())
    {
        AtlasSettings.bForceSmoothing = false;
        ForceSmoothingCheckBox->SetIsChecked(ECheckBoxState::Unchecked);
    }
    
    LastAnalysisResult = FSpriteAtlasResult(); // Сбрасываем анализ
    UpdateButtonStates();
    
    UE_LOG(LogTemp, Log, TEXT("Atlas: Preserve Original Quality = %s"), 
           AtlasSettings.bPreserveOriginalQuality ? TEXT("True") : TEXT("False"));
}

void SAtlasCreationWindow::OnForceSmoothingChanged(ECheckBoxState NewState)
{
    AtlasSettings.bForceSmoothing = (NewState == ECheckBoxState::Checked);
    
    // Если включено принудительное сглаживание, отключаем сохранение оригинального качества
    if (AtlasSettings.bForceSmoothing && PreserveQualityCheckBox.IsValid())
    {
        AtlasSettings.bPreserveOriginalQuality = false;
        PreserveQualityCheckBox->SetIsChecked(ECheckBoxState::Unchecked);
    }
    
    LastAnalysisResult = FSpriteAtlasResult(); // Сбрасываем анализ
    UpdateButtonStates();
    
    UE_LOG(LogTemp, Log, TEXT("Atlas: Force Smoothing = %s"), 
           AtlasSettings.bForceSmoothing ? TEXT("True") : TEXT("False"));
}

#undef LOCTEXT_NAMESPACE