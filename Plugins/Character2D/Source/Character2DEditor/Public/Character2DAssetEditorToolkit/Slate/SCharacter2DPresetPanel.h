#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Character2DEnums.h"

DECLARE_DELEGATE_OneParam(FOnPresetChosen, UObject* /*Preset*/)

class UCharacter2DAsset;

class SCharacter2DPresetPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCharacter2DPresetPanel) {}
	SLATE_ARGUMENT(UCharacter2DAsset*, CharacterAsset)
	SLATE_ARGUMENT(ECharacter2DEditMode, Mode)
	SLATE_EVENT   (FOnPresetChosen, OnChosen)
SLATE_END_ARGS()

void Construct(const FArguments& InArgs);
	void Refresh();                      // вызовите при смене режима

private:
	void CollectPresets(TArray<UObject*>& Out);
	TSharedRef<SWidget> MakeTile(UObject* Preset);

	UCharacter2DAsset* Asset      = nullptr;
	ECharacter2DEditMode Mode     = ECharacter2DEditMode::Body;
	FOnPresetChosen     OnChosen;
	TSharedPtr<SVerticalBox>    Container; 
};
