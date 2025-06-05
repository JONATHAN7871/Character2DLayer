// SCharacter2DBuilderWindow.cpp

#include "Character2DBuilderWindow/SCharacter2DBuilderWindow.h"
#include "Character2DBuilderWindow/Character2DMeshGenerator.h"
#include "Character2DBuilderWindow/AssetData/Character2DAssetData.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SVectorInputBox.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "PropertyCustomizationHelpers.h"
#include "Character2DBuilderWindow/SCharacter2DPreviewViewport.h"
#include "Engine/AssetManager.h"
#include "Misc/MessageDialog.h"
#include "UObject/SavePackage.h"
#define LOCTEXT_NAMESPACE "SCharacter2DBuilderWindow"


void SCharacter2DBuilderWindow::Construct(const FArguments& InArgs)
{
    WeakThisPtr = SharedThis(this);

    // Инициализируем категории
    for (auto Name : { TEXT("Body"), TEXT("Arms"), TEXT("Head") })
    {
        Categories.Add(MakeShared<FCharacter2DLayerCategory>(FName(Name)));
    }

    // Подготовка источников опций
    OutputTypeOptions = {
        MakeShared<ECharacter2DMeshOutputType>(ECharacter2DMeshOutputType::SkeletalMesh),
        MakeShared<ECharacter2DMeshOutputType>(ECharacter2DMeshOutputType::StaticMesh)
    };
    PivotOptions = {
        MakeShared<ECharacter2DRootBonePlacement>(ECharacter2DRootBonePlacement::Origin),
        MakeShared<ECharacter2DRootBonePlacement>(ECharacter2DRootBonePlacement::Center),
        MakeShared<ECharacter2DRootBonePlacement>(ECharacter2DRootBonePlacement::BottomCenter)
    };

    RebuildLayout();

    RefreshCategoryList();
}

void SCharacter2DBuilderWindow::RebuildLayout()
{
    SAssignNew(RootSplitter, SSplitter)

    + SSplitter::Slot().Value(0.3f)
    [
        BuildLayersPanel()
    ]

    + SSplitter::Slot().Value(0.5f)
    [
        BuildViewportPanel()
    ]

    + SSplitter::Slot().Value(0.2f)
    [
        BuildSettingsPanel()
    ];

    ChildSlot
    [
        RootSplitter.ToSharedRef()
    ];
}


TSharedRef<SWidget> SCharacter2DBuilderWindow::BuildLayersPanel()
{
    return SNew(SScrollBox)

    // Заголовок
    + SScrollBox::Slot().Padding(4)
    [
        SNew(STextBlock)
        .Text(LOCTEXT("LayersTitle", "Layers"))
        .Font(FSlateFontInfo(
    FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"),
    16
))
    ]

    // Список категорий
    + SScrollBox::Slot().Padding(4)
    [
        SAssignNew(CategoryContainer, SVerticalBox)
    ]

    // Кнопки Save/Load
    + SScrollBox::Slot().Padding(4).HAlign(HAlign_Center)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot().AutoWidth().Padding(4)
        [
            SNew(SButton)
            .Text(LOCTEXT("SaveAs","Save As Asset..."))
            .OnClicked(this, &SCharacter2DBuilderWindow::HandleSaveAsAsset)
        ]

        + SHorizontalBox::Slot().AutoWidth().Padding(4)
        [
            SNew(SButton)
            .Text(LOCTEXT("Load","Load Asset..."))
            .OnClicked(this, &SCharacter2DBuilderWindow::HandleLoadAsset)
        ]
    ];
}

TSharedRef<SWidget> SCharacter2DBuilderWindow::BuildViewportPanel()
{
    return SNew(SVerticalBox)

    // Сам вьюпорт
    + SVerticalBox::Slot().FillHeight(1.f)
    [
        SAssignNew(PreviewViewport, SCharacter2DPreviewViewport)
    ]

    // Кнопка сброса камеры
    + SVerticalBox::Slot().AutoHeight().Padding(5).HAlign(HAlign_Right)
    [
        SNew(SButton)
        .Text(LOCTEXT("ResetCamera", "Reset Camera"))
        .OnClicked_Lambda([this]() {
            if (PreviewViewport.IsValid())
            {
                PreviewViewport->CenterCameraOnSprites();
            }
            return FReply::Handled();
        })
    ];
}

TSharedRef<SWidget> SCharacter2DBuilderWindow::BuildSettingsPanel()
{
    return SNew(SScrollBox)

    // Output Type
    + SScrollBox::Slot().Padding(4)
    [
        SNew(STextBlock).Text(LOCTEXT("OutType", "Output Type"))
    ]
    + SScrollBox::Slot().Padding(4)
    [
        SAssignNew(OutputTypeCombo, SComboBox<TSharedPtr<ECharacter2DMeshOutputType>>)
        .OptionsSource(&OutputTypeOptions)
        .OnGenerateWidget_Lambda([](TSharedPtr<ECharacter2DMeshOutputType> V) {
            return SNew(STextBlock)
                .Text((*V == ECharacter2DMeshOutputType::StaticMesh)
                    ? LOCTEXT("StaticMesh","Static Mesh")
                    : LOCTEXT("SkeletalMesh","Skeletal Mesh"));
        })
        .OnSelectionChanged_Lambda([](TSharedPtr<ECharacter2DMeshOutputType> Sel, ESelectInfo::Type){
            if (Sel) GetMutableDefault<UCharacter2DMeshGeneratorOptions>()->OutputType = *Sel;
        })
        [
            SNew(STextBlock)
            .Text_Lambda([]() {
                const auto* C = GetDefault<UCharacter2DMeshGeneratorOptions>();
                return (C->OutputType == ECharacter2DMeshOutputType::StaticMesh)
                    ? LOCTEXT("StaticMesh","Static Mesh")
                    : LOCTEXT("SkeletalMesh","Skeletal Mesh");
            })
        ]
    ]

    // Asset Name
    + SScrollBox::Slot().Padding(4)
    [
        SNew(STextBlock).Text(LOCTEXT("AssetName","Asset Name"))
    ]
    + SScrollBox::Slot().Padding(4)
    [
        SNew(SEditableTextBox)
        .Text_Lambda([](){
            return FText::FromString(GetDefault<UCharacter2DMeshGeneratorOptions>()->AssetName);
        })
        .OnTextCommitted_Lambda([](const FText& T, ETextCommit::Type){
            GetMutableDefault<UCharacter2DMeshGeneratorOptions>()->AssetName = T.ToString();
        })
    ]

    // Save Path
    + SScrollBox::Slot().Padding(4)
    [
        SNew(STextBlock).Text(LOCTEXT("SavePath","Save Path"))
    ]
    + SScrollBox::Slot().Padding(4)
    [
        SNew(SEditableTextBox)
        .Text_Lambda([](){
            return FText::FromString(GetDefault<UCharacter2DMeshGeneratorOptions>()->SavePath.Path);
        })
        .OnTextCommitted_Lambda([](const FText& T, ETextCommit::Type){
            GetMutableDefault<UCharacter2DMeshGeneratorOptions>()->SavePath.Path = T.ToString();
        })
    ]

    // Pivot Placement
    + SScrollBox::Slot().Padding(4)
    [
        SNew(STextBlock).Text(LOCTEXT("Pivot","Pivot Placement"))
    ]
    + SScrollBox::Slot().Padding(4)
    [
        SAssignNew(PivotCombo, SComboBox<TSharedPtr<ECharacter2DRootBonePlacement>>)
        .OptionsSource(&PivotOptions)
        .OnGenerateWidget_Lambda([](TSharedPtr<ECharacter2DRootBonePlacement> V){
            switch (*V)
            {
                case ECharacter2DRootBonePlacement::Origin:       return SNew(STextBlock).Text(LOCTEXT("Origin","World Origin"));
                case ECharacter2DRootBonePlacement::Center:       return SNew(STextBlock).Text(LOCTEXT("Center","Center"));
                case ECharacter2DRootBonePlacement::BottomCenter: return SNew(STextBlock).Text(LOCTEXT("Bottom","Bottom Center"));
            }
            return SNew(STextBlock).Text(LOCTEXT("Unknown","Unknown"));
        })
        .OnSelectionChanged_Lambda([](TSharedPtr<ECharacter2DRootBonePlacement> Sel, ESelectInfo::Type){
            if (Sel) GetMutableDefault<UCharacter2DMeshGeneratorOptions>()->PivotPlacement = *Sel;
        })
        [
            SNew(STextBlock)
            .Text_Lambda([](){
                switch (GetDefault<UCharacter2DMeshGeneratorOptions>()->PivotPlacement)
                {
                    case ECharacter2DRootBonePlacement::Origin:       return LOCTEXT("Origin","World Origin");
                    case ECharacter2DRootBonePlacement::Center:       return LOCTEXT("Center","Center");
                    case ECharacter2DRootBonePlacement::BottomCenter: return LOCTEXT("Bottom","Bottom Center");
                }
                return LOCTEXT("Unknown","Unknown");
            })
        ]
        ]
        // Mesh Scale
        + SScrollBox::Slot().Padding(4)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("MeshScale","Mesh Scale"))
        ]
        + SScrollBox::Slot().Padding(4)
        [
            SNew(SNumericEntryBox<float>)
                // Текущее значение
                .Value_Lambda([this]() {
                    return TOptional<float>( GetDefault<UCharacter2DMeshGeneratorOptions>()->MeshScale );
                })
                // При подтверждении (Enter или клик спиннера)
                .OnValueCommitted_Lambda([this](float NewValue, ETextCommit::Type){
                    // Ограничиваем в диапазоне [0.001, 10]
                    const float Clamped = FMath::Clamp(NewValue, 0.001f, 10.0f);
                    GetMutableDefault<UCharacter2DMeshGeneratorOptions>()->MeshScale = Clamped;
                    // И перерисовываем превью
                    RefreshPreview();
                })
                // Минимальное и максимальное значение
                .MinValue(0.001f)
                .MaxValue(10.0f)
                // Разрешить спиннер
                .AllowSpin(true)
        ]

        // Show Wireframe
        + SScrollBox::Slot().Padding(4)
        [
            SNew(SHorizontalBox)

            // Чекбокс
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [
                SNew(SCheckBox)
                .IsChecked_Lambda([this]() {
                    return PreviewViewport->bShowWireframe ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                })
                .OnCheckStateChanged_Lambda([this](ECheckBoxState State) {
                    bool bEnable = (State == ECheckBoxState::Checked);
                    PreviewViewport->SetWireframe(bEnable);
                })
            ]

            // Текст справа
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0, 0, 0)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("Wireframe", "Show Wireframe"))
            ]
        ]

        // Кнопка Generate
        + SScrollBox::Slot().Padding(8)
        [
            SNew(SButton)
            .Text(LOCTEXT("GenMeshBtn", "Generate Mesh"))
            .OnClicked(this, &SCharacter2DBuilderWindow::HandleGenerateMesh)
        ];
}

void SCharacter2DBuilderWindow::RefreshCategoryList()
{
    CategoryContainer->ClearChildren();

    for (auto& Cat : Categories)
    {
        CategoryContainer->AddSlot().AutoHeight().Padding(5)
        [
            BuildCategoryWidget(Cat)
        ];
    }
}

TSharedRef<SWidget> SCharacter2DBuilderWindow::BuildCategoryWidget(TSharedPtr<FCharacter2DLayerCategory> Category)
{
    TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

    // Заголовок категории
    Box->AddSlot().AutoHeight().Padding(2)
    [
        SNew(STextBlock)
        .Text(FText::FromName(Category->CategoryName))
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
    ];

    // Use Grid Mesh
    Box->AddSlot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot().AutoWidth().Padding(4).VAlign(VAlign_Center)
        [
            SNew(SCheckBox)
            .IsChecked_Lambda([Category]() {
                return Category->bUseGridMesh ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
            })
            .OnCheckStateChanged_Lambda([this, Category](ECheckBoxState State) {
                Category->bUseGridMesh = (State == ECheckBoxState::Checked);
                RefreshPreview();
            })
        ]

        + SHorizontalBox::Slot().AutoWidth().Padding(4).VAlign(VAlign_Center)
        [
            SNew(STextBlock).Text(LOCTEXT("UseGridMesh", "Use Grid Mesh"))
        ]
    ];

    // Grid Cell Size
    Box->AddSlot().AutoHeight().Padding(2)
    [
        SNew(SNumericEntryBox<int32>)
        .Value_Lambda([Category]() -> TOptional<int32> { return Category->GridCellSize; })
        .OnValueCommitted_Lambda([this, Category](int32 NewVal, ETextCommit::Type) {
            Category->GridCellSize = FMath::Max(1, NewVal);
            RefreshPreview();
        })
        .MinValue(1)
        .AllowSpin(true)
        .Label()
        [
            SNew(STextBlock).Text(LOCTEXT("CellSize", "Cell Size"))
        ]
        .IsEnabled_Lambda([Category]() { return Category->bUseGridMesh; })
    ];


    // Alpha Threshold
    Box->AddSlot().AutoHeight().Padding(2)
    [
        SNew(SNumericEntryBox<uint8>)
        .Value_Lambda([Category]() -> TOptional<uint8> { return Category->AlphaThreshold; })
        .OnValueCommitted_Lambda([this, Category](uint8 NewVal, ETextCommit::Type) {
            Category->AlphaThreshold = FMath::Clamp<uint8>(NewVal, 0, 255);
            RefreshPreview();
        })
        .MinValue((uint8)0).MaxValue((uint8)255)
        .AllowSpin(true)
        .Label()
        [
            SNew(STextBlock).Text(LOCTEXT("AlphaThreshold", "Alpha Threshold"))
        ]
        .IsEnabled_Lambda([Category]() { return Category->bUseGridMesh; })
    ];


    // Слоты спрайтов
    for (int32 i = 0; i < Category->Slots.Num(); ++i)
    {
        TSharedPtr<FCharacter2DLayerSlot> SlotData = Category->Slots[i];
        Box->AddSlot().AutoHeight().Padding(2)
        [
            SNew(SHorizontalBox)

            // Спрайт
            + SHorizontalBox::Slot().MaxWidth(400).Padding(5)
            [
                SNew(SObjectPropertyEntryBox)
                .AllowedClass(UPaperSprite::StaticClass())
                .ObjectPath_Lambda([SlotData]() {
                    return SlotData->Sprite.IsValid() ? SlotData->Sprite->GetPathName() : FString();
                })
                .OnObjectChanged_Lambda([this, SlotData](const FAssetData& AD) {
                    SlotData->Sprite = Cast<UPaperSprite>(AD.GetAsset());
                    RefreshPreview();
                })
            ]

            // Позиция
            + SHorizontalBox::Slot().MaxWidth(240).Padding(5)
            [
                SNew(SVectorInputBox)
                .X_Lambda([SlotData]() { return SlotData->Location.X; })
                .Y_Lambda([SlotData]() { return SlotData->Location.Y; })
                .Z_Lambda([SlotData]() { return SlotData->Location.Z; })
                .OnXCommitted_Lambda([this, SlotData](float X, ETextCommit::Type){ SlotData->Location.X = X; RefreshPreview(); })
                .OnYCommitted_Lambda([this, SlotData](float Y, ETextCommit::Type){ SlotData->Location.Y = Y; RefreshPreview(); })
                .OnZCommitted_Lambda([this, SlotData](float Z, ETextCommit::Type){ SlotData->Location.Z = Z; RefreshPreview(); })
            ]

            // ↑
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(3)
            [
                SNew(SButton)
                .Text(LOCTEXT("MoveUp", "↑"))
                .OnClicked(this, &SCharacter2DBuilderWindow::HandleMoveLayer, Category, i, true)
            ]

            // ↓
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(3)
            [
                SNew(SButton)
                .Text(LOCTEXT("MoveDown", "↓"))
                .OnClicked(this, &SCharacter2DBuilderWindow::HandleMoveLayer, Category, i, false)
            ]

            // ✕
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5)
            [
                SNew(SButton)
                .Text(LOCTEXT("RemoveSlot", "✕"))
                .ToolTipText(LOCTEXT("RemoveSlotTip", "Remove Layer"))
                .OnClicked(this, &SCharacter2DBuilderWindow::HandleRemoveLayer, Category, i)
            ]

            // 👁️/🚫
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(3)
            [
                SNew(SButton)
                .Text_Lambda([SlotData]() {
                    return FText::FromString(SlotData->bVisible ? TEXT("👁️") : TEXT("🚫"));
                })
                .ToolTipText(LOCTEXT("ToggleVisTip", "Show/Hide Layer"))
                .OnClicked(this, &SCharacter2DBuilderWindow::HandleToggleVisibility, SlotData)
            ]
        ];
    }

    // Кнопка добавить слой
    Box->AddSlot().AutoHeight().Padding(2)
    [
        SNew(SButton)
        .Text(FText::Format(LOCTEXT("AddLayerFmt", "+ Add Layer to {0}"), FText::FromName(Category->CategoryName)))
        .OnClicked(this, &SCharacter2DBuilderWindow::HandleAddLayer, Category)
    ];

    return Box;
}

void SCharacter2DBuilderWindow::RefreshPreview()
{
    if (PreviewViewport.IsValid())
    {
        // grab your user‐configured scale
        const float Scale = GetDefault<UCharacter2DMeshGeneratorOptions>()->MeshScale;
        PreviewViewport->UpdatePreviewSprites(Categories, Scale);
        PreviewViewport->UpdatePreviewMeshDescription(Categories, Scale);
    }
}

FReply SCharacter2DBuilderWindow::HandleAddLayer(TSharedPtr<FCharacter2DLayerCategory> Category)
{
    Category->Slots.Add(MakeShared<FCharacter2DLayerSlot>());
    RefreshCategoryList();
    RefreshPreview();
    return FReply::Handled();
}

FReply SCharacter2DBuilderWindow::HandleMoveLayer(TSharedPtr<FCharacter2DLayerCategory> Category, int32 Index, bool bUp)
{
    int32 NewIndex = bUp ? Index - 1 : Index + 1;
    if (Category->Slots.IsValidIndex(Index) && Category->Slots.IsValidIndex(NewIndex))
    {
        Category->Slots.Swap(Index, NewIndex);
        RefreshCategoryList();
        RefreshPreview();
    }
    return FReply::Handled();
}

FReply SCharacter2DBuilderWindow::HandleRemoveLayer(TSharedPtr<FCharacter2DLayerCategory> Category, int32 Index)
{
    if (Category->Slots.IsValidIndex(Index))
    {
        Category->Slots.RemoveAt(Index);
        RefreshCategoryList();
        RefreshPreview();
    }
    return FReply::Handled();
}

FReply SCharacter2DBuilderWindow::HandleToggleVisibility(TSharedPtr<FCharacter2DLayerSlot> Slot)
{
    Slot->bVisible = !Slot->bVisible;
    RefreshPreview();
    return FReply::Handled();
}

FReply SCharacter2DBuilderWindow::HandleGenerateMesh()
{
    FCharacter2DMeshGenerationOptions Opt;
    auto* Cfg = GetMutableDefault<UCharacter2DMeshGeneratorOptions>();
    Opt.OutputType     = Cfg->OutputType;
    Opt.PivotPlacement = Cfg->PivotPlacement;
    Opt.AssetName      = Cfg->AssetName;
    Opt.SavePath       = Cfg->SavePath.Path;
    Opt.MeshScale      = Cfg->MeshScale;

    Character2DMeshGenerator::GenerateMeshFromOptions(Categories, Opt);
    return FReply::Handled();
}



FReply SCharacter2DBuilderWindow::HandleSaveAsAsset()
{
    /* ――― диалог выбора имени / пакета (без изменений) ――― */
    FSaveAssetDialogConfig SaveConfig;
    SaveConfig.DialogTitleOverride   = LOCTEXT("SaveDialogTitle", "Save Character2D Builder Asset");
    SaveConfig.DefaultPath           = TEXT("/Game");
    SaveConfig.DefaultAssetName      = TEXT("NewCharacter2DBuilder");
    SaveConfig.AssetClassNames.Add(UCharacter2DAssetData::StaticClass()->GetClassPathName());
    SaveConfig.ExistingAssetPolicy   = ESaveAssetDialogExistingAssetPolicy::AllowButWarn;

    const FString SaveObjectPath =
        FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser")
        .Get()
        .CreateModalSaveAssetDialog(SaveConfig);

    if (SaveObjectPath.IsEmpty())
        return FReply::Handled();             // отмена

    const FString PackageName = FPackageName::ObjectPathToPackageName(SaveObjectPath);
    const FString AssetName   = FPackageName::GetLongPackageAssetName(PackageName);

    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("PackageCreationFailed", "Failed to create package."));
        return FReply::Handled();
    }

    /* ――― создаём объект ассета ――― */
    UCharacter2DAssetData* NewAsset = NewObject<UCharacter2DAssetData>(
        Package,
        UCharacter2DAssetData::StaticClass(),
        *AssetName,
        RF_Public | RF_Standalone);

    if (!NewAsset)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("AssetCreationFailed", "Failed to create asset instance."));
        return FReply::Handled();
    }

    /* ---------- сохраняем ГЛОБАЛЬНЫЕ настройки ---------- */
    const UCharacter2DMeshGeneratorOptions* Cfg = GetDefault<UCharacter2DMeshGeneratorOptions>();

    FCharacter2DBuilderGlobals Globals;
    Globals.OutputType      = Cfg->OutputType;
    Globals.PivotPlacement  = Cfg->PivotPlacement;
    Globals.MeshScale       = Cfg->MeshScale;
    Globals.AssetName       = Cfg->AssetName;
    Globals.SavePath        = Cfg->SavePath.Path;
    Globals.bShowWireframe  = PreviewViewport->bShowWireframe;

    NewAsset->Globals = Globals;

    /* ---------- сохраняем категории и слоты ---------- */
    for (const auto& Category : Categories)
    {
        FCharacter2DLayerCategoryData CatData;
        CatData.CategoryName  = Category->CategoryName;
        CatData.bUseGridMesh  = Category->bUseGridMesh;
        CatData.GridCellSize  = Category->GridCellSize;
        CatData.AlphaThreshold= Category->AlphaThreshold;

        for (const auto& Slot : Category->Slots)
        {
            FCharacter2DLayerSlotData SlotData;
            SlotData.Sprite   = Slot->Sprite.Get();
            SlotData.Location = Slot->Location;
            SlotData.bVisible = Slot->bVisible;
            CatData.Slots.Add(SlotData);
        }
        NewAsset->Categories.Add(CatData);
    }

    /* ---------- сохраняем пакет ---------- */
    (void)NewAsset->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(NewAsset);

    const FString PackageFile = FPackageName::LongPackageNameToFilename(
        PackageName, FPackageName::GetAssetPackageExtension());

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.Error         = GWarn;

    if (!UPackage::SavePackage(Package, NewAsset, *PackageFile, SaveArgs))
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("AssetSaveFailed", "Failed to save package."));
    }

    return FReply::Handled();
}

FReply SCharacter2DBuilderWindow::HandleLoadAsset()
{
    // Временный контейнер для выбранного ассета
    TSharedPtr<FAssetData> SelectedAssetData = MakeShared<FAssetData>();

    // Настраиваем фильтр и поведение Asset Picker
    FAssetPickerConfig PickerConfig;
    PickerConfig.Filter.ClassPaths.Add(UCharacter2DAssetData::StaticClass()->GetClassPathName());
    PickerConfig.Filter.bRecursiveClasses = true;
    PickerConfig.InitialAssetViewType      = EAssetViewType::Column;
    PickerConfig.bAllowNullSelection       = false;
    PickerConfig.bShowPathInColumnView     = true;
    PickerConfig.bFocusSearchBoxWhenOpened = true;
    PickerConfig.bShowBottomToolbar        = true;

    PickerConfig.OnAssetSelected = FOnAssetSelected::CreateLambda(
        [SelectedAssetData](const FAssetData& AssetData)
        {
            *SelectedAssetData = AssetData;
        }
    );

    // Собираем модальное окно
    TSharedRef<SWindow> PickerWindow = SNew(SWindow)
        .Title(LOCTEXT("LoadDialogTitle", "Load Character2D Builder Asset"))
        .ClientSize(FVector2D(600, 400))
        .SupportsMinimize(false)
        .SupportsMaximize(false);

    PickerWindow->SetContent(
        SNew(SVerticalBox)

        + SVerticalBox::Slot().FillHeight(1)
        [
            FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser")
            .Get()
            .CreateAssetPicker(PickerConfig)
        ]

        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(10, 5)
        [
            SNew(SButton)
            .Text(LOCTEXT("LoadButton", "Load"))
            .IsEnabled_Lambda([SelectedAssetData]()
            {
                return SelectedAssetData.IsValid() && SelectedAssetData->IsValid();
            })
            .OnClicked_Lambda([WeakThis = WeakThisPtr, SelectedAssetData, WeakWindow = TWeakPtr<SWindow>(PickerWindow)]() -> FReply
            {
                if (SelectedAssetData.IsValid() && SelectedAssetData->IsValid())
                {
                    if (auto Pinned = WeakThis.Pin())
                    {
                        Pinned->LoadFromAsset(*SelectedAssetData);
                    }
                }
                if (auto W = WeakWindow.Pin())
                {
                    W->RequestDestroyWindow();
                }
                return FReply::Handled();
            })
        ]
    );

    GEditor->EditorAddModalWindow(PickerWindow);
    return FReply::Handled();
}

void SCharacter2DBuilderWindow::LoadFromAsset(const FAssetData& InAssetData)
{
    if (!InAssetData.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("LoadFromAsset: Invalid AssetData"));
        return;
    }

    const UCharacter2DAssetData* TempAsset = Cast<UCharacter2DAssetData>(InAssetData.GetAsset());
    if (!TempAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("LoadFromAsset: Asset is null or incorrect type"));
        return;
    }

    // Собираем уникальные пути спрайтов
    TArray<FSoftObjectPath> SpritesToLoad;
    for (const auto& Cat : TempAsset->Categories)
    {
        for (const auto& Slot : Cat.Slots)
        {
            if (Slot.Sprite.IsValid())
            {
                SpritesToLoad.AddUnique(Slot.Sprite.ToSoftObjectPath());
            }
        }
    }

    // Делаем копию FAssetData для колбэка
    FAssetData AssetCopy = InAssetData;

    // Асинхронная загрузка
    UAssetManager::GetStreamableManager().RequestAsyncLoad(
        SpritesToLoad,
        FStreamableDelegate::CreateSP(this, &SCharacter2DBuilderWindow::OnSpritesLoaded, AssetCopy),
        FStreamableManager::AsyncLoadHighPriority
    );
}

void SCharacter2DBuilderWindow::OnSpritesLoaded(FAssetData LoadedAssetData)
{
    const UCharacter2DAssetData* Asset =
        Cast<UCharacter2DAssetData>(LoadedAssetData.GetAsset());
    if (!Asset)
    {
        UE_LOG(LogTemp, Warning, TEXT("OnSpritesLoaded: asset cast failed"));
        return;
    }

    /* ---------- восстановить ГЛОБАЛЬНЫЕ настройки ---------- */
    const FCharacter2DBuilderGlobals& G = Asset->Globals;
    UCharacter2DMeshGeneratorOptions* Cfg = GetMutableDefault<UCharacter2DMeshGeneratorOptions>();

    Cfg->OutputType      = G.OutputType;
    Cfg->PivotPlacement  = G.PivotPlacement;
    Cfg->MeshScale       = G.MeshScale;
    Cfg->AssetName       = G.AssetName;
    Cfg->SavePath.Path   = G.SavePath;

    if (PreviewViewport.IsValid())
    {
        PreviewViewport->SetWireframe(G.bShowWireframe);
    }

    // синхронизируем комбобоксы
    if (OutputTypeCombo.IsValid())
        for (auto& Opt : OutputTypeOptions)
            if (*Opt == G.OutputType) { OutputTypeCombo->SetSelectedItem(Opt); break; }

    if (PivotCombo.IsValid())
        for (auto& Opt : PivotOptions)
            if (*Opt == G.PivotPlacement){ PivotCombo->SetSelectedItem(Opt); break; }

    /* ---------- восстановить категории и слоты ---------- */
    Categories.Empty();

    for (const auto& CatData : Asset->Categories)
    {
        auto NewCat = MakeShared<FCharacter2DLayerCategory>(CatData.CategoryName);
        NewCat->bUseGridMesh   = CatData.bUseGridMesh;
        NewCat->GridCellSize   = CatData.GridCellSize;
        NewCat->AlphaThreshold = CatData.AlphaThreshold;

        for (const auto& SlotData : CatData.Slots)
        {
            auto NewSlot = MakeShared<FCharacter2DLayerSlot>();
            NewSlot->Sprite   = SlotData.Sprite.LoadSynchronous();
            NewSlot->Location = SlotData.Location;
            NewSlot->bVisible = SlotData.bVisible;
            NewCat->Slots.Add(NewSlot);
        }
        Categories.Add(NewCat);
    }

    RefreshCategoryList();
    RefreshPreview();
    ForceRefreshGlobalsUI();
}

void SCharacter2DBuilderWindow::ForceRefreshGlobalsUI()
{
    RebuildLayout();
    RefreshCategoryList();
    RefreshPreview();
}


#undef LOCTEXT_NAMESPACE