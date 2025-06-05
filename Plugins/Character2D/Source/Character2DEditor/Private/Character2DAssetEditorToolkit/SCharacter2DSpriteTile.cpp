#include "Character2DAssetEditorToolkit/SCharacter2DSpriteTile.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Brushes/SlateImageBrush.h"

void SCharacter2DSpriteTile::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot().AutoHeight()
		[
			SNew(SImage)
			.Image(new FSlateImageBrush(InArgs._PreviewTexture, FVector2D(64,64)))
		]
		+SVerticalBox::Slot().AutoHeight().Padding(4)
		[
			SNew(STextBlock)
			.Text(InArgs._Label)
			.Justification(ETextJustify::Center)
		]
	];
}
