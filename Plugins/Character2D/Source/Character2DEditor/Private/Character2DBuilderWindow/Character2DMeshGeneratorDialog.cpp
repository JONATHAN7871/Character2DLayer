// Character2DMeshGeneratorDialog.cpp

#include "Character2DBuilderWindow/Character2DMeshGeneratorDialog.h"
#include "Character2DBuilderWindow/Character2DMeshGenerator.h"

#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "SCharacter2DMeshGeneratorDialog"

void SCharacter2DMeshGeneratorDialog::Construct(const FArguments& InArgs)
{
	Categories = InArgs._Categories;

	// Заполняем списки опций
	OutputTypeOptions = {
		MakeShared<ECharacter2DMeshOutputType>(ECharacter2DMeshOutputType::SkeletalMesh),
		MakeShared<ECharacter2DMeshOutputType>(ECharacter2DMeshOutputType::StaticMesh)
	};
	PivotPlacementOptions = {
		MakeShared<ECharacter2DRootBonePlacement>(ECharacter2DRootBonePlacement::Origin),
		MakeShared<ECharacter2DRootBonePlacement>(ECharacter2DRootBonePlacement::Center),
		MakeShared<ECharacter2DRootBonePlacement>(ECharacter2DRootBonePlacement::BottomCenter)
	};

	ChildSlot
	[
		SNew(SScrollBox)

		// Output Type
		+ SScrollBox::Slot().Padding(4)
		[
			SNew(STextBlock).Text(LOCTEXT("OutputType", "Output Type"))
		]
		+ SScrollBox::Slot().Padding(4)
		[
			SAssignNew(OutputTypeCombo, SComboBox<TSharedPtr<ECharacter2DMeshOutputType>>)
			.OptionsSource(&OutputTypeOptions)
			.InitiallySelectedItem(OutputTypeOptions[0])
			.OnSelectionChanged(this, &SCharacter2DMeshGeneratorDialog::OnOutputTypeChanged)
			.OnGenerateWidget_Lambda([](TSharedPtr<ECharacter2DMeshOutputType> Item) {
				return SNew(STextBlock)
					.Text(*Item == ECharacter2DMeshOutputType::StaticMesh
						? LOCTEXT("StaticMesh", "Static Mesh")
						: LOCTEXT("SkeletalMesh", "Skeletal Mesh"));
			})
			[
				SNew(STextBlock)
				.Text(this, &SCharacter2DMeshGeneratorDialog::GetOutputTypeLabel)
			]
		]

		// Asset Name
		+ SScrollBox::Slot().Padding(4)
		[
			SNew(STextBlock).Text(LOCTEXT("AssetName", "Asset Name"))
		]
		+ SScrollBox::Slot().Padding(4)
		[
			SNew(SEditableTextBox)
			.Text(this, &SCharacter2DMeshGeneratorDialog::GetAssetNameText)
			.OnTextChanged_Lambda([&](const FText& NewText) { AssetName = NewText.ToString(); })
		]

		// Save Path
		+ SScrollBox::Slot().Padding(4)
		[
			SNew(STextBlock).Text(LOCTEXT("SavePath", "Save Path"))
		]
		+ SScrollBox::Slot().Padding(4)
		[
			SNew(SEditableTextBox)
			.Text(this, &SCharacter2DMeshGeneratorDialog::GetSavePathText)
			.OnTextChanged_Lambda([&](const FText& NewText) { SavePath = NewText.ToString(); })
		]

		// Pivot Placement
		+ SScrollBox::Slot().Padding(4)
		[
			SNew(STextBlock).Text(LOCTEXT("PivotPlacement", "Pivot Placement"))
		]
		+ SScrollBox::Slot().Padding(4)
		[
			SAssignNew(PivotCombo, SComboBox<TSharedPtr<ECharacter2DRootBonePlacement>>)
			.OptionsSource(&PivotPlacementOptions)
			.InitiallySelectedItem(PivotPlacementOptions[0])
			.OnSelectionChanged(this, &SCharacter2DMeshGeneratorDialog::OnPivotPlacementChanged)
			.OnGenerateWidget_Lambda([](TSharedPtr<ECharacter2DRootBonePlacement> Item) {
				switch (*Item)
				{
				case ECharacter2DRootBonePlacement::Origin:       return SNew(STextBlock).Text(LOCTEXT("PivotOrigin", "World Origin"));
				case ECharacter2DRootBonePlacement::Center:       return SNew(STextBlock).Text(LOCTEXT("PivotCenter", "Center"));
				case ECharacter2DRootBonePlacement::BottomCenter: return SNew(STextBlock).Text(LOCTEXT("PivotBottom", "Bottom Center"));
				}
				return SNew(STextBlock).Text(LOCTEXT("PivotUnknown", "Unknown"));
			})
			[
				SNew(STextBlock)
				.Text(this, &SCharacter2DMeshGeneratorDialog::GetPivotPlacementLabel)
			]
		]

		// Buttons
		+ SScrollBox::Slot().Padding(8).HAlign(HAlign_Right)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().AutoWidth().Padding(4)
			[
				SNew(SButton)
				.Text(LOCTEXT("Generate", "Generate"))
				.OnClicked(this, &SCharacter2DMeshGeneratorDialog::OnGenerateClicked)
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(4)
			[
				SNew(SButton)
				.Text(LOCTEXT("Cancel", "Cancel"))
				.OnClicked(this, &SCharacter2DMeshGeneratorDialog::OnCancelClicked)
			]
		]
	];

	OutputTypeCombo->RefreshOptions();
	PivotCombo->RefreshOptions();
}

void SCharacter2DMeshGeneratorDialog::OnOutputTypeChanged(TSharedPtr<ECharacter2DMeshOutputType> NewValue, ESelectInfo::Type)
{
	SelectedOutputType = NewValue ? *NewValue : ECharacter2DMeshOutputType::SkeletalMesh;
}

void SCharacter2DMeshGeneratorDialog::OnPivotPlacementChanged(TSharedPtr<ECharacter2DRootBonePlacement> NewValue, ESelectInfo::Type)
{
	SelectedPivotPlacement = NewValue ? *NewValue : ECharacter2DRootBonePlacement::Origin;
}

FText SCharacter2DMeshGeneratorDialog::GetOutputTypeLabel() const
{
	return (SelectedOutputType == ECharacter2DMeshOutputType::StaticMesh)
		? LOCTEXT("StaticMeshLabel", "Static Mesh")
		: LOCTEXT("SkeletalMeshLabel", "Skeletal Mesh");
}

FText SCharacter2DMeshGeneratorDialog::GetPivotPlacementLabel() const
{
	switch (SelectedPivotPlacement)
	{
	case ECharacter2DRootBonePlacement::Origin:       return LOCTEXT("PivotOriginLabel", "World Origin");
	case ECharacter2DRootBonePlacement::Center:       return LOCTEXT("PivotCenterLabel", "Center");
	case ECharacter2DRootBonePlacement::BottomCenter: return LOCTEXT("PivotBottomLabel", "Bottom Center");
	}
	return LOCTEXT("PivotUnknownLabel", "Unknown");
}

FReply SCharacter2DMeshGeneratorDialog::OnGenerateClicked()
{
	FCharacter2DMeshGenerationOptions Options;
	Options.OutputType     = SelectedOutputType;
	Options.PivotPlacement = SelectedPivotPlacement;
	Options.AssetName      = AssetName;
	Options.SavePath       = SavePath;

	Character2DMeshGenerator::GenerateMeshFromOptions(Categories, Options);

	if (TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(SharedThis(this)))
	{
		Window->RequestDestroyWindow();
	}
	return FReply::Handled();
}

FReply SCharacter2DMeshGeneratorDialog::OnCancelClicked()
{
	if (TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(SharedThis(this)))
	{
		Window->RequestDestroyWindow();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
