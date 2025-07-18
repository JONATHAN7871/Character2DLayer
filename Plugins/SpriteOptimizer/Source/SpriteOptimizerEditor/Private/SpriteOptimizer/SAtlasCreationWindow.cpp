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
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/Texture2D.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "SAtlasCreationWindow"

void SAtlasCreationWindow::Construct(const FArguments& InArgs)
{
    // Initialize data
    CurrentAtlasName = FString::Printf(TEXT("Atlas_%d_Sprites"), InArgs._SourceSprites.Num());
    
    // Create sprite info objects
    for (UPaperSprite* Sprite : InArgs._SourceSprites)
    {
        if (Sprite)
        {
            TSharedPtr<FAtlasSpriteInfo> SpriteInfo = MakeShared<FAtlasSpriteInfo>(Sprite);
            SpriteInfos.Add(SpriteInfo);
        }
    }
    
    // Initialize settings
    LoadDefaultSettings();
    InitializePackingAlgorithms();
    
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
                    CreateCompactHeaderSection()
                ]
                
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 2)
                [
                    SNew(SSeparator)
                ]
                
                // Atlas settings
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    CreateCompactAtlasSettingsSection()
                ]
                
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 2)
                [
                    SNew(SSeparator)
                ]
                
                // Sprites
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    CreateCompactSpritesSection()
                ]
                
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 2)
                [
                    SNew(SSeparator)
                ]
                
                // Analysis
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    CreateCompactAnalysisSection()
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

TSharedRef<SWidget> SAtlasCreationWindow::CreateCompactHeaderSection()
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
        .Padding(0, 3)
        [
            SNew(STextBlock)
            .Text(FText::Format(LOCTEXT("AtlasCreationSubtitle", 
                "Combine {0} sprites into an optimized texture atlas"), SpriteInfos.Num()))
            .Justification(ETextJustify::Center)
            .Font(FAppStyle::GetFontStyle("SmallFont"))
        ];
}

TSharedRef<SWidget> SAtlasCreationWindow::CreateCompactAtlasSettingsSection()
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
                .Text(LOCTEXT("AtlasSettingsTitle", "⚙️ Atlas Settings"))
                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
            ]
            
            // First row settings
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 8)
            [
                SNew(SHorizontalBox)
                
                // Atlas Name
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(0, 0, 5, 0)
                [
                    SNew(SVerticalBox)
                    
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("AtlasNameLabel", "Name:"))
                        .Font(FAppStyle::GetFontStyle("SmallFont"))
                    ]
                    
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 2)
                    [
                        SAssignNew(AtlasNameTextBox, SEditableTextBox)
                        .Text(FText::FromString(CurrentAtlasName))
                        .OnTextCommitted(this, &SAtlasCreationWindow::OnAtlasNameChanged)
                    ]
                ]
                
                // Max Size
                + SHorizontalBox::Slot()
                .FillWidth(0.8f)
                .Padding(5, 0)
                [
                    SNew(SVerticalBox)
                    
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("MaxSizeLabel", "Max Size:"))
                        .Font(FAppStyle::GetFontStyle("SmallFont"))
                    ]
                    
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 2)
                    [
                        SNew(SHorizontalBox)
                        
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SAssignNew(MaxWidthSpinBox, SSpinBox<int32>)
                            .Value(AtlasSettings.MaxAtlasSize.X)
                            .MinValue(256)
                            .MaxValue(8192)
                            .OnValueChanged(this, &SAtlasCreationWindow::OnMaxWidthChanged)
                        ]
                        
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(3, 0)
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(TEXT("x")))
                            .Font(FAppStyle::GetFontStyle("SmallFont"))
                        ]
                        
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SAssignNew(MaxHeightSpinBox, SSpinBox<int32>)
                            .Value(AtlasSettings.MaxAtlasSize.Y)
                            .MinValue(256)
                            .MaxValue(8192)
                            .OnValueChanged(this, &SAtlasCreationWindow::OnMaxHeightChanged)
                        ]
                    ]
                ]
                
                // Algorithm
                + SHorizontalBox::Slot()
                .FillWidth(0.8f)
                .Padding(5, 0, 0, 0)
                [
                    SNew(SVerticalBox)
                    
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("AlgorithmLabel", "Algorithm:"))
                        .Font(FAppStyle::GetFontStyle("SmallFont"))
                    ]
                    
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 2)
                    [
                        SAssignNew(PackingAlgorithmComboBox, SComboBox<TSharedPtr<FString>>)
                        .OptionsSource(&PackingAlgorithmOptions)
                        .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                        {
                            return SNew(STextBlock).Text(FText::FromString(*Item)).Font(FAppStyle::GetFontStyle("SmallFont"));
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
                            .Font(FAppStyle::GetFontStyle("SmallFont"))
                        ]
                    ]
                ]
            ]
            
            // Compact options in one row
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SNew(SHorizontalBox)
                
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SAssignNew(OptimizeSpritesFirstCheckBox, SCheckBox)
                    .IsChecked(AtlasSettings.bOptimizeSpritesFirst ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                    .OnCheckStateChanged(this, &SAtlasCreationWindow::OnOptimizeSpritesFirstChanged)
                    .Content()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("OptimizeFirstLabel", "Optimize First"))
                        .Font(FAppStyle::GetFontStyle("SmallFont"))
                    ]
                ]
                
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SAssignNew(CreateIndividualSpritesCheckBox, SCheckBox)
                    .IsChecked(AtlasSettings.bCreateIndividualSprites ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                    .OnCheckStateChanged(this, &SAtlasCreationWindow::OnCreateIndividualSpritesChanged)
                    .Content()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("CreateIndividualLabel", "Individual Sprites"))
                        .Font(FAppStyle::GetFontStyle("SmallFont"))
                    ]
                ]
                
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SAssignNew(PreserveQualityCheckBox, SCheckBox)
                    .IsChecked(AtlasSettings.bPreserveOriginalQuality ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                    .OnCheckStateChanged(this, &SAtlasCreationWindow::OnPreserveQualityChanged)
                    .Content()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("PreserveQualityLabel", "Preserve Quality"))
                        .Font(FAppStyle::GetFontStyle("SmallFont"))
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> SAtlasCreationWindow::CreateCompactSpritesSection()
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
                    .Text(LOCTEXT("SpritesListTitle", "📋 Sprites"))
                    .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(3, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("SelectAll", "All"))
                    .OnClicked(this, &SAtlasCreationWindow::OnSelectAllSprites)
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(3, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("SelectNone", "None"))
                    .OnClicked(this, &SAtlasCreationWindow::OnSelectNoneSprites)
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(3, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("SelectOptimal", "Best"))
                    .OnClicked(this, &SAtlasCreationWindow::OnSelectOptimalSprites)
                ]
            ]
            
            // Compact summary
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SAssignNew(SpritesSummaryText, STextBlock)
                .Text(GetSpritesSummaryText())
                .AutoWrapText(true)
                .Font(FAppStyle::GetFontStyle("SmallFont"))
                .ColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)))
            ]
            
            // Compact sprite list
            + SVerticalBox::Slot()
            .MaxHeight(150.0f)
            .Padding(0, 5)
            [
                SAssignNew(SpritesListView, SListView<TSharedPtr<FAtlasSpriteInfo>>)
                .ListItemsSource(&SpriteInfos)
                .OnGenerateRow(this, &SAtlasCreationWindow::GenerateCompactSpriteRow)
                .OnSelectionChanged(this, &SAtlasCreationWindow::OnSpriteSelectionChanged)
                .SelectionMode(ESelectionMode::Multi)
                .HeaderRow
                (
                    SNew(SHeaderRow)
                    
                    + SHeaderRow::Column("Include")
                    .DefaultLabel(LOCTEXT("IncludeHeader", ""))
                    .FixedWidth(25)
                    
                    + SHeaderRow::Column("SpriteName")
                    .DefaultLabel(LOCTEXT("SpriteNameHeader", "Sprite"))
                    .FillWidth(0.5f)
                    
                    + SHeaderRow::Column("OptimizedSize")
                    .DefaultLabel(LOCTEXT("OptimizedSizeHeader", "Size"))
                    .FillWidth(0.25f)
                    
                    + SHeaderRow::Column("Savings")
                    .DefaultLabel(LOCTEXT("SavingsHeader", "Savings"))
                    .FillWidth(0.25f)
                )
            ]
        ];
}

TSharedRef<SWidget> SAtlasCreationWindow::CreateCompactAnalysisSection()
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
                .Text(LOCTEXT("AnalysisTitle", "📊 Analysis"))
                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SAssignNew(AnalysisResultText, STextBlock)
                .Text(GetCompactAnalysisText())
                .AutoWrapText(true)
                .Font(FAppStyle::GetFontStyle("SmallFont"))
                .Visibility_Lambda([this]()
                {
                    return LastAnalysisResult.TotalSprites > 0 ? EVisibility::Visible : EVisibility::Collapsed;
                })
            ]
        ];
}

TSharedRef<SWidget> SAtlasCreationWindow::CreateCompactActionSection()
{
    return SNew(SHorizontalBox)
        
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(3)
        [
            SAssignNew(AnalyzeButton, SButton)
            .Text(LOCTEXT("AnalyzeAtlas", "🔍 Analyze"))
            .OnClicked(this, &SAtlasCreationWindow::OnAnalyzeAtlas)
            .HAlign(HAlign_Center)
        ]
        
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(3)
        [
            SAssignNew(PreviewButton, SButton)
            .Text(LOCTEXT("PreviewAtlas", "👁️ Preview"))
            .OnClicked(this, &SAtlasCreationWindow::OnPreviewAtlas)
            .HAlign(HAlign_Center)
            .IsEnabled(false)
        ]
        
        + SHorizontalBox::Slot()
        .FillWidth(1.5f)
        .Padding(3)
        [
            SAssignNew(CreateAtlasButton, SButton)
            .Text(LOCTEXT("CreateAtlas", "🎨 Create Atlas"))
            .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
            .OnClicked(this, &SAtlasCreationWindow::OnCreateAtlas)
            .HAlign(HAlign_Center)
            .IsEnabled(false)
        ];
}

TSharedRef<ITableRow> SAtlasCreationWindow::GenerateCompactSpriteRow(TSharedPtr<FAtlasSpriteInfo> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(STableRow<TSharedPtr<FAtlasSpriteInfo>>, OwnerTable)
        [
            SNew(SHorizontalBox)
            
            // Include checkbox
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
                    
                    if (SpritesSummaryText.IsValid())
                    {
                        SpritesSummaryText->SetText(GetSpritesSummaryText());
                    }
                })
            ]
            
            // Sprite name
            + SHorizontalBox::Slot()
            .FillWidth(0.5f)
            .VAlign(VAlign_Center)
            .Padding(5, 2)
            [
                SNew(STextBlock)
                .Text_Lambda([Item]()
                {
                    if (Item->Sprite)
                    {
                        FString Name = Item->Sprite->GetName();
                        Name = Name.Replace(TEXT("T_"), TEXT(""));
                        Name = Name.Replace(TEXT("_Sprite"), TEXT(""));
                        
                        if (Name.Len() > 15)
                        {
                            return FText::FromString(Name.Left(12) + TEXT("..."));
                        }
                        return FText::FromString(Name);
                    }
                    return FText::FromString(TEXT("Unknown"));
                })
                .Font(FAppStyle::GetFontStyle("SmallFont"))
                .ToolTipText_Lambda([Item]()
                {
                    return Item->Sprite ? FText::FromString(Item->Sprite->GetName()) : FText::FromString(TEXT("Unknown sprite"));
                })
            ]
            
            // Optimized size
            + SHorizontalBox::Slot()
            .FillWidth(0.25f)
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
                .Font(FAppStyle::GetFontStyle("SmallFont"))
                .ColorAndOpacity_Lambda([this]()
                {
                    return AtlasSettings.bOptimizeSpritesFirst ? FSlateColor(FLinearColor::Green) : FSlateColor(FLinearColor::White);
                })
            ]
            
            // Savings + status
            + SHorizontalBox::Slot()
            .FillWidth(0.25f)
            .VAlign(VAlign_Center)
            .Padding(5, 2)
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
                    .Font(FAppStyle::GetFontStyle("SmallFont"))
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(5, 0, 0, 0)
                [
                    SNew(STextBlock)
                    .Text_Lambda([Item]()
                    {
                        if (!Item->AnalysisResult.bSuccess)
                        {
                            return FText::FromString(TEXT("ERROR"));
                        }
                        else if (Item->AnalysisResult.SavingsPercent > 50.0f)
                        {
                            return FText::FromString(TEXT("GREAT"));
                        }
                        else if (Item->AnalysisResult.SavingsPercent > 25.0f)
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
                        if (!Item->AnalysisResult.bSuccess) 
                        {
                            return FSlateColor(FLinearColor::Red);
                        }
                        else if (Item->AnalysisResult.SavingsPercent > 50.0f) 
                        {
                            return FSlateColor(FLinearColor::Green);
                        }
                        else if (Item->AnalysisResult.SavingsPercent > 25.0f) 
                        {
                            return FSlateColor(FLinearColor::Yellow);
                        }
                        else 
                        {
                            return FSlateColor(FLinearColor::Red);
                        }
                    })
                    .Font(FAppStyle::GetFontStyle("SmallFont"))
                    .ToolTipText_Lambda([Item]()
                    {
                        if (!Item->AnalysisResult.bSuccess)
                        {
                            return FText::FromString(Item->AnalysisResult.ErrorMessage);
                        }
                        else if (Item->AnalysisResult.SavingsPercent > 50.0f)
                        {
                            return FText::FromString(TEXT("Excellent optimization potential"));
                        }
                        else if (Item->AnalysisResult.SavingsPercent > 25.0f)
                        {
                            return FText::FromString(TEXT("Good optimization potential"));
                        }
                        else
                        {
                            return FText::FromString(TEXT("Poor optimization potential"));
                        }
                    })
                ]
            ]
        ];
}

void SAtlasCreationWindow::OnSpriteSelectionChanged(TSharedPtr<FAtlasSpriteInfo> Item, ESelectInfo::Type SelectInfo)
{
    UpdateButtonStates();
}

void SAtlasCreationWindow::InitializePackingAlgorithms()
{
    PackingAlgorithmOptions.Empty();
    PackingAlgorithmOptions.Add(MakeShared<FString>(TEXT("Simple Grid")));
    PackingAlgorithmOptions.Add(MakeShared<FString>(TEXT("Best Fit")));
    PackingAlgorithmOptions.Add(MakeShared<FString>(TEXT("MaxRects Algorithm")));
    
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

// Atlas settings event handlers
void SAtlasCreationWindow::OnAtlasNameChanged(const FText& NewText, ETextCommit::Type CommitType)
{
    CurrentAtlasName = NewText.ToString();
}

void SAtlasCreationWindow::OnMaxWidthChanged(int32 NewValue)
{
    AtlasSettings.MaxAtlasSize.X = NewValue;
    LastAnalysisResult = FSpriteAtlasResult();
    UpdateButtonStates();
}

void SAtlasCreationWindow::OnMaxHeightChanged(int32 NewValue)
{
    AtlasSettings.MaxAtlasSize.Y = NewValue;
    LastAnalysisResult = FSpriteAtlasResult();
    UpdateButtonStates();
}

void SAtlasCreationWindow::OnSpritePaddingChanged(int32 NewValue)
{
    AtlasSettings.SpritePadding = NewValue;
    LastAnalysisResult = FSpriteAtlasResult();
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
        
        LastAnalysisResult = FSpriteAtlasResult();
        UpdateButtonStates();
    }
}

void SAtlasCreationWindow::OnAtlasSuffixChanged(const FText& NewText, ETextCommit::Type CommitType)
{
    AtlasSettings.AtlasSuffix = NewText.ToString();
}

// Options event handlers
void SAtlasCreationWindow::OnOptimizeSpritesFirstChanged(ECheckBoxState NewState)
{
    AtlasSettings.bOptimizeSpritesFirst = (NewState == ECheckBoxState::Checked);
    
    // Recalculate sprite sizes
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
    
    LastAnalysisResult = FSpriteAtlasResult();
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
    bool bPowerOfTwo = (NewState == ECheckBoxState::Checked);
    
    if (bPowerOfTwo)
    {
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

void SAtlasCreationWindow::OnPreserveQualityChanged(ECheckBoxState NewState)
{
    AtlasSettings.bPreserveOriginalQuality = (NewState == ECheckBoxState::Checked);
    
    if (AtlasSettings.bPreserveOriginalQuality && ForceSmoothingCheckBox.IsValid())
    {
        AtlasSettings.bForceSmoothing = false;
        ForceSmoothingCheckBox->SetIsChecked(ECheckBoxState::Unchecked);
    }
    
    LastAnalysisResult = FSpriteAtlasResult();
    UpdateButtonStates();
}

void SAtlasCreationWindow::OnForceSmoothingChanged(ECheckBoxState NewState)
{
    AtlasSettings.bForceSmoothing = (NewState == ECheckBoxState::Checked);
    
    if (AtlasSettings.bForceSmoothing && PreserveQualityCheckBox.IsValid())
    {
        AtlasSettings.bPreserveOriginalQuality = false;
        PreserveQualityCheckBox->SetIsChecked(ECheckBoxState::Unchecked);
    }
    
    LastAnalysisResult = FSpriteAtlasResult();
    UpdateButtonStates();
}

// Action event handlers
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
    
    // Create atlas
    FSpriteAtlasResult Result = USpriteOptimizer::CreateSpriteAtlas(SelectedSprites, AtlasSettings, CurrentAtlasName);
    
    if (Result.bSuccess)
    {
        FText NotificationText = FText::Format(LOCTEXT("AtlasCreatedSuccess", 
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

        ShowNotification(NotificationText, true);
        
        // Refresh Content Browser
        USpriteOptimizer::RefreshContentBrowser();
        
        // Close window after 3 seconds
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
    // Check that analysis has been performed
    if (!LastAnalysisResult.bSuccess || LastAnalysisResult.TotalSprites == 0)
    {
        ShowNotification(LOCTEXT("PreviewNeedsAnalysis", "Please run 'Analyze Atlas' first to preview the layout"), false);
        return FReply::Handled();
    }
    
    // Get selected sprites
    TArray<UPaperSprite*> SelectedSprites = GetSelectedSprites();
    
    if (SelectedSprites.Num() < 2)
    {
        ShowNotification(LOCTEXT("PreviewNeedsSprites", "Select at least 2 sprites to preview atlas"), false);
        return FReply::Handled();
    }
    
    // Show preview window
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
    
    // Update UI elements
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
    
    // Reset analysis
    LastAnalysisResult = FSpriteAtlasResult();
    UpdateButtonStates();
    
    ShowNotification(LOCTEXT("SettingsReset", "Settings reset to defaults"), true);
    
    return FReply::Handled();
}

void SAtlasCreationWindow::UpdateAnalysisDisplay()
{
    if (AnalysisResultText.IsValid())
    {
        AnalysisResultText->SetText(GetCompactAnalysisText());
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

FText SAtlasCreationWindow::GetCompactAnalysisText() const
{
    if (LastAnalysisResult.TotalSprites == 0)
    {
        return LOCTEXT("NoAnalysisYet", "Click 'Analyze' to see atlas statistics");
    }
    
    if (!LastAnalysisResult.bSuccess)
    {
        return FText::Format(LOCTEXT("AnalysisError", "Analysis failed: {0}"), 
                           FText::FromString(LastAnalysisResult.ErrorMessage));
    }
    
    return FText::Format(LOCTEXT("CompactAnalysisSuccess",
        "Atlas: {0}x{1} • Efficiency: {2}% • Memory savings: {3}% • {4} sprites"),
        LastAnalysisResult.AtlasSize.X, LastAnalysisResult.AtlasSize.Y,
        FText::AsNumber(LastAnalysisResult.PackingEfficiency),
        FText::AsNumber(LastAnalysisResult.MemorySavings),
        LastAnalysisResult.TotalSprites
    );
}

FText SAtlasCreationWindow::GetSpritesSummaryText() const
{
    int32 SelectedCount = 0;
    int32 TotalCount = SpriteInfos.Num();
    float TotalSavings = 0.0f;
    int32 ValidSprites = 0;
    
    for (const auto& SpriteInfo : SpriteInfos)
    {
        if (SpriteInfo->bIncludeInAtlas)
        {
            SelectedCount++;
            
            if (SpriteInfo->AnalysisResult.bSuccess)
            {
                TotalSavings += SpriteInfo->AnalysisResult.SavingsPercent;
                ValidSprites++;
            }
        }
    }
    
    float AverageSavings = ValidSprites > 0 ? TotalSavings / ValidSprites : 0.0f;
    
    if (SelectedCount == 0)
    {
        return FText::Format(LOCTEXT("NoSpritesSelected", 
            "❌ No sprites selected for atlas creation. Use buttons above to select sprites."), 
            SelectedCount, TotalCount);
    }
    else if (SelectedCount == TotalCount)
    {
        return FText::Format(LOCTEXT("AllSpritesSelected", 
            "✅ All {0} sprites selected for atlas creation\n"
            "💰 Average savings: {1}% per sprite"), 
            SelectedCount, 
            FText::AsNumber(AverageSavings, &FNumberFormattingOptions::DefaultWithGrouping()));
    }
    else
    {
        return FText::Format(LOCTEXT("PartialSpritesSelected", 
            "✅ {0} of {1} sprites selected for atlas creation\n"
            "💰 Average savings: {2}% per selected sprite"), 
            SelectedCount, TotalCount,
            FText::AsNumber(AverageSavings, &FNumberFormattingOptions::DefaultWithGrouping()));
    }
}

#undef LOCTEXT_NAMESPACE