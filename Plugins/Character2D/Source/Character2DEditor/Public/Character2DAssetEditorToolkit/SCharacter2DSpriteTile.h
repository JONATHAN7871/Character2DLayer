#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SCharacter2DSpriteTile : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCharacter2DSpriteTile) {}
	SLATE_ARGUMENT(FText      , Label)
	SLATE_ARGUMENT(UTexture2D*, PreviewTexture)
SLATE_END_ARGS()

void Construct(const FArguments& InArgs);
};
