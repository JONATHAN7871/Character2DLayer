#include "ManualSpriteMeshGeneratorDialog.h"
#include "ManualSpriteMeshGenerator.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SVectorInputBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "SManualSpriteMeshGeneratorDialog"

void SManualSpriteMeshGeneratorDialog::Construct(const FArguments& InArgs)
{
    ManualSprite = InArgs._ManualSprite;

    // Инициализация опций
    MeshTypeOptions = {
        MakeShared<EManualSpriteMeshType>(EManualSpriteMeshType::StaticMesh),
        MakeShared<EManualSpriteMeshType>(EManualSpriteMeshType::SkeletalMesh)
    };

    PivotOptions = {
        MakeShared<EManualSpritePivotPlacement>(EManualSpritePivotPlacement::Origin),
        MakeShared<EManualSpritePivotPlacement>(EManualSpritePivotPlacement::Center),
        MakeShared<EManualSpritePivotPlacement>(EManualSpritePivotPlacement::BottomCenter),
        MakeShared<EManualSpritePivotPlacement>(EManualSpritePivotPlacement::Custom)
    };

    LoadSettings();

    ChildSlot
    [
        SNew(SScrollBox)

        // Заголовок
        + SScrollBox::Slot().Padding(8, 8, 8, 4)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("Title", "Generate Mesh from Manual Sprite"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
        ]

        // Тип меша
        + SScrollBox::Slot().Padding(8, 4)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("MeshType", "Mesh Type"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
        ]
        + SScrollBox::Slot().Padding(8, 2, 8, 8)
        [
            SAssignNew(MeshTypeCombo, SComboBox<TSharedPtr<EManualSpriteMeshType>>)
            .OptionsSource(&MeshTypeOptions)
            .OnSelectionChanged(this, &SManualSpriteMeshGeneratorDialog::OnMeshTypeChanged)
            .OnGenerateWidget_Lambda([](TSharedPtr<EManualSpriteMeshType> Item) {
                return SNew(STextBlock)
                    .Text(*Item == EManualSpriteMeshType::StaticMesh
                        ? LOCTEXT("StaticMeshOption", "Static Mesh")
                        : LOCTEXT("SkeletalMeshOption", "Skeletal Mesh"));
            })
            [
                SNew(STextBlock)
                .Text(this, &SManualSpriteMeshGeneratorDialog::GetMeshTypeText)
            ]
        ]

        // Размещение пивота
        + SScrollBox::Slot().Padding(8, 4)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("PivotPlacement", "Pivot Placement"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
        ]
        + SScrollBox::Slot().Padding(8, 2, 8, 8)
        [
            SAssignNew(PivotCombo, SComboBox<TSharedPtr<EManualSpritePivotPlacement>>)
            .OptionsSource(&PivotOptions)
            .OnSelectionChanged(this, &SManualSpriteMeshGeneratorDialog::OnPivotPlacementChanged)
            .OnGenerateWidget_Lambda([](TSharedPtr<EManualSpritePivotPlacement> Item) {
                switch (*Item)
                {
                case EManualSpritePivotPlacement::Origin:       return SNew(STextBlock).Text(LOCTEXT("PivotOrigin", "World Origin"));
                case EManualSpritePivotPlacement::Center:       return SNew(STextBlock).Text(LOCTEXT("PivotCenter", "Center"));
                case EManualSpritePivotPlacement::BottomCenter: return SNew(STextBlock).Text(LOCTEXT("PivotBottom", "Bottom Center"));
                case EManualSpritePivotPlacement::Custom:       return SNew(STextBlock).Text(LOCTEXT("PivotCustom", "Custom Offset"));
                }
                return SNew(STextBlock).Text(LOCTEXT("PivotUnknown", "Unknown"));
            })
            [
                SNew(STextBlock)
                .Text(this, &SManualSpriteMeshGeneratorDialog::GetPivotPlacementText)
            ]
        ]

        // Кастомное смещение пивота
        + SScrollBox::Slot().Padding(8, 2, 8, 8)
        [
            SNew(SHorizontalBox)
            .Visibility_Lambda([this]() {
                return SelectedPivotPlacement == EManualSpritePivotPlacement::Custom 
                    ? EVisibility::Visible : EVisibility::Collapsed;
            })

            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("CustomOffset", "Custom Offset:"))
            ]

            + SHorizontalBox::Slot().FillWidth(1.0f)
            [
                SNew(SVectorInputBox)
                .X_Lambda([this]() { return CustomPivotOffset.X; })
                .Y_Lambda([this]() { return CustomPivotOffset.Y; })
                .Z_Lambda([this]() { return CustomPivotOffset.Z; })
                .OnXCommitted_Lambda([this](float NewValue, ETextCommit::Type) { CustomPivotOffset.X = NewValue; })
                .OnYCommitted_Lambda([this](float NewValue, ETextCommit::Type) { CustomPivotOffset.Y = NewValue; })
                .OnZCommitted_Lambda([this](float NewValue, ETextCommit::Type) { CustomPivotOffset.Z = NewValue; })
            ]
        ]

        // Масштаб меша
        + SScrollBox::Slot().Padding(8, 4)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("MeshScale", "Mesh Scale"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
        ]
        + SScrollBox::Slot().Padding(8, 2, 8, 8)
        [
            SNew(SNumericEntryBox<float>)
            .Value_Lambda([this]() { return TOptional<float>(MeshScale); })
            .OnValueCommitted_Lambda([this](float NewValue, ETextCommit::Type) {
                MeshScale = FMath::Clamp(NewValue, 0.001f, 100.0f);
            })
            .MinValue(0.001f)
            .MaxValue(100.0f)
            .AllowSpin(true)
            .SpinBoxStyle(&FCoreStyle::Get().GetWidgetStyle<FSpinBoxStyle>("SpinBox"))
        ]

        // Имя ассета
        + SScrollBox::Slot().Padding(8, 4)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("AssetName", "Asset Name"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
        ]
        + SScrollBox::Slot().Padding(8, 2, 8, 8)
        [
            SNew(SEditableTextBox)
            .Text(this, &SManualSpriteMeshGeneratorDialog::GetAssetNameText)
            .OnTextChanged_Lambda([this](const FText& NewText) { AssetName = NewText.ToString(); })
        ]

        // Путь сохранения
        + SScrollBox::Slot().Padding(8, 4)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("SavePath", "Save Path"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
        ]
        + SScrollBox::Slot().Padding(8, 2, 8, 8)
        [
            SNew(SEditableTextBox)
            .Text(this, &SManualSpriteMeshGeneratorDialog::GetSavePathText)
            .OnTextChanged_Lambda([this](const FText& NewText) { SavePath = NewText.ToString(); })
        ]

        // Настройки материала
        + SScrollBox::Slot().Padding(8, 8, 8, 4)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("MaterialSettings", "Material Settings"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
        ]

        // Создавать материал
        + SScrollBox::Slot().Padding(8, 2)
        [
            SNew(SHorizontalBox)

            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [
                SNew(SCheckBox)
                .IsChecked_Lambda([this]() {
                    return bCreateMaterial ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                })
                .OnCheckStateChanged_Lambda([this](ECheckBoxState State) {
                    bCreateMaterial = (State == ECheckBoxState::Checked);
                })
            ]

            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0, 0, 0)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("CreateMaterial", "Create Material"))
            ]
        ]

        // Unlit материал
        + SScrollBox::Slot().Padding(24, 2)
        [
            SNew(SHorizontalBox)
            .Visibility_Lambda([this]() {
                return bCreateMaterial ? EVisibility::Visible : EVisibility::Collapsed;
            })

            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [
                SNew(SCheckBox)
                .IsChecked_Lambda([this]() {
                    return bCreateUnlitMaterial ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                })
                .OnCheckStateChanged_Lambda([this](ECheckBoxState State) {
                    bCreateUnlitMaterial = (State == ECheckBoxState::Checked);
                })
            ]

            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0, 0, 0)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("UnlitMaterial", "Unlit Material"))
            ]
        ]

        // Двусторонний материал
        + SScrollBox::Slot().Padding(24, 2, 8, 8)
        [
            SNew(SHorizontalBox)
            .Visibility_Lambda([this]() {
                return bCreateMaterial ? EVisibility::Visible : EVisibility::Collapsed;
            })

            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [
                SNew(SCheckBox)
                .IsChecked_Lambda([this]() {
                    return bTwoSidedMaterial ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                })
                .OnCheckStateChanged_Lambda([this](ECheckBoxState State) {
                    bTwoSidedMaterial = (State == ECheckBoxState::Checked);
                })
            ]

            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0, 0, 0)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("TwoSidedMaterial", "Two Sided"))
            ]
        ]

        // Кнопки
        + SScrollBox::Slot().Padding(8, 16, 8, 8).HAlign(HAlign_Right)
        [
            SNew(SHorizontalBox)

            + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("Generate", "Generate"))
                .IsEnabled(this, &SManualSpriteMeshGeneratorDialog::CanGenerate)
                .ToolTipText(this, &SManualSpriteMeshGeneratorDialog::GetGenerateButtonTooltip)
                .OnClicked(this, &SManualSpriteMeshGeneratorDialog::OnGenerateClicked)
            ]

            + SHorizontalBox::Slot().AutoWidth()
            [
                SNew(SButton)
                .Text(LOCTEXT("Cancel", "Cancel"))
                .OnClicked(this, &SManualSpriteMeshGeneratorDialog::OnCancelClicked)
            ]
        ]
    ];

    InitializeFromSettings();
}

void SManualSpriteMeshGeneratorDialog::InitializeFromSettings()
{
    // Установка выбранных значений в комбобоксы
    for (auto& Option : MeshTypeOptions)
    {
        if (*Option == SelectedMeshType)
        {
            MeshTypeCombo->SetSelectedItem(Option);
            break;
        }
    }

    for (auto& Option : PivotOptions)
    {
        if (*Option == SelectedPivotPlacement)
        {
            PivotCombo->SetSelectedItem(Option);
            break;
        }
    }
}

void SManualSpriteMeshGeneratorDialog::LoadSettings()
{
    // Загружаем из ManualSprite если доступно, иначе из глобальных настроек
    if (ManualSprite)
    {
        SelectedMeshType = EManualSpriteMeshType::StaticMesh; // Можно добавить в ManualSprite
        SelectedPivotPlacement = ManualSprite->PivotPlacement;
        CustomPivotOffset = ManualSprite->CustomPivotOffset;
        MeshScale = ManualSprite->MeshScale;
        AssetName = ManualSprite->GetName() + TEXT("_Mesh");
        bCreateMaterial = true;
        bCreateUnlitMaterial = true;
        bTwoSidedMaterial = true;
    }
    else
    {
        // Fallback к глобальным настройкам
        const UManualSpriteMeshGeneratorOptions* Settings = GetDefault<UManualSpriteMeshGeneratorOptions>();
        SelectedMeshType = Settings->MeshType;
        SelectedPivotPlacement = Settings->PivotPlacement;
        CustomPivotOffset = Settings->CustomPivotOffset;
        MeshScale = Settings->MeshScale;
        AssetName = Settings->AssetBaseName;
    }
	
    SavePath = GetDefault<UManualSpriteMeshGeneratorOptions>()->SavePath.Path;
}

void SManualSpriteMeshGeneratorDialog::SaveSettings()
{
    // Сохраняем в ManualSprite
    if (ManualSprite)
    {
        ManualSprite->PivotPlacement = SelectedPivotPlacement;
        ManualSprite->CustomPivotOffset = CustomPivotOffset;
        ManualSprite->MeshScale = MeshScale;
        (void)ManualSprite->MarkPackageDirty();
    }
	
    // Также сохраняем в глобальные настройки
    UManualSpriteMeshGeneratorOptions* Settings = GetMutableDefault<UManualSpriteMeshGeneratorOptions>();
    Settings->MeshType = SelectedMeshType;
    Settings->PivotPlacement = SelectedPivotPlacement;
    Settings->CustomPivotOffset = CustomPivotOffset;
    Settings->MeshScale = MeshScale;
    Settings->AssetBaseName = AssetName;
    Settings->SavePath.Path = SavePath;
    Settings->bCreateMaterial = bCreateMaterial;
    Settings->bCreateUnlitMaterial = bCreateUnlitMaterial;
    Settings->bTwoSidedMaterial = bTwoSidedMaterial;
    Settings->SaveConfig();
}

void SManualSpriteMeshGeneratorDialog::OnMeshTypeChanged(TSharedPtr<EManualSpriteMeshType> NewValue, ESelectInfo::Type)
{
    if (NewValue.IsValid())
    {
        SelectedMeshType = *NewValue;
    }
}

void SManualSpriteMeshGeneratorDialog::OnPivotPlacementChanged(TSharedPtr<EManualSpritePivotPlacement> NewValue, ESelectInfo::Type)
{
    if (NewValue.IsValid())
    {
        SelectedPivotPlacement = *NewValue;
    }
}

FText SManualSpriteMeshGeneratorDialog::GetMeshTypeText() const
{
    return SelectedMeshType == EManualSpriteMeshType::StaticMesh
        ? LOCTEXT("StaticMeshSelected", "Static Mesh")
        : LOCTEXT("SkeletalMeshSelected", "Skeletal Mesh");
}

FText SManualSpriteMeshGeneratorDialog::GetPivotPlacementText() const
{
    switch (SelectedPivotPlacement)
    {
    case EManualSpritePivotPlacement::Origin:       return LOCTEXT("PivotOriginSelected", "World Origin");
    case EManualSpritePivotPlacement::Center:       return LOCTEXT("PivotCenterSelected", "Center");
    case EManualSpritePivotPlacement::BottomCenter: return LOCTEXT("PivotBottomSelected", "Bottom Center");
    case EManualSpritePivotPlacement::Custom:       return LOCTEXT("PivotCustomSelected", "Custom Offset");
    }
    return LOCTEXT("PivotUnknownSelected", "Unknown");
}

FReply SManualSpriteMeshGeneratorDialog::OnGenerateClicked()
{
    if (!CanGenerate())
    {
        return FReply::Handled();
    }

    // Сохраняем настройки
    SaveSettings();

    // Создаем параметры генерации
    FManualSpriteMeshGenerationParams Params;
    Params.MeshType = SelectedMeshType;
    Params.PivotPlacement = SelectedPivotPlacement;
    Params.CustomPivotOffset = CustomPivotOffset;
    Params.MeshScale = MeshScale;
    Params.AssetName = AssetName;
    Params.SavePath = SavePath;
    Params.bCreateMaterial = bCreateMaterial;
    Params.bCreateUnlitMaterial = bCreateUnlitMaterial;
    Params.bTwoSidedMaterial = bTwoSidedMaterial;

    // Генерируем меш
    bool bSuccess = ManualSpriteMeshGenerator::GenerateMeshFromSprite(ManualSprite, Params);

    if (bSuccess)
    {
        // Закрываем диалог
        if (TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(SharedThis(this)))
        {
            Window->RequestDestroyWindow();
        }
    }

    return FReply::Handled();
}

FReply SManualSpriteMeshGeneratorDialog::OnCancelClicked()
{
    if (TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(SharedThis(this)))
    {
        Window->RequestDestroyWindow();
    }
    return FReply::Handled();
}

bool SManualSpriteMeshGeneratorDialog::CanGenerate() const
{
    return ManualSprite != nullptr 
        && ManualSprite->bUseManualGeometry 
        && ManualSprite->IsManualGeometryValid()
        && !AssetName.IsEmpty()
        && !SavePath.IsEmpty();
}

FText SManualSpriteMeshGeneratorDialog::GetGenerateButtonTooltip() const
{
    if (!ManualSprite)
    {
        return LOCTEXT("NoSpriteTooltip", "No Manual Sprite provided");
    }
    
    if (!ManualSprite->bUseManualGeometry)
    {
        return LOCTEXT("NoManualGeometryTooltip", "Manual geometry is not enabled");
    }
    
    if (!ManualSprite->IsManualGeometryValid())
    {
        return LOCTEXT("InvalidGeometryTooltip", "Manual geometry is invalid");
    }
    
    if (AssetName.IsEmpty())
    {
        return LOCTEXT("EmptyAssetNameTooltip", "Asset name cannot be empty");
    }
    
    if (SavePath.IsEmpty())
    {
        return LOCTEXT("EmptySavePathTooltip", "Save path cannot be empty");
    }
    
    return LOCTEXT("GenerateTooltip", "Generate mesh from Manual Sprite geometry");
}

#undef LOCTEXT_NAMESPACE