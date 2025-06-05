#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "Character2DAsset.h"

class FAssetTypeActions_Character2DAsset : public FAssetTypeActions_Base
{
public:
	explicit FAssetTypeActions_Character2DAsset(uint32 InCategory)
		: MyCategory(InCategory) {}

	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "Character2DAsset", "Character2D Asset"); }
	virtual FColor GetTypeColor() const override { return FColor(0, 200, 255); }
	virtual UClass* GetSupportedClass() const override { return UCharacter2DAsset::StaticClass(); }
	virtual uint32 GetCategories() override { return MyCategory; }

	virtual FText GetAssetDescription(const FAssetData& AssetData) const override
	{
		return NSLOCTEXT("AssetTypeActions", "Character2DAssetDesc", "2D Character definition (sprites or skeletal)");
	}

	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor) override;

private:
	uint32 MyCategory;
};
