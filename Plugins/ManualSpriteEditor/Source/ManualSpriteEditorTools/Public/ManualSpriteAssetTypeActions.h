#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "ManualSprite.h"

/**
 * Asset Type Actions для Manual Sprite ассетов - ОЧИЩЕННАЯ ВЕРСИЯ
 */
class MANUALSPRITEEDITORTOOLS_API FManualSpriteAssetTypeActions : public FAssetTypeActions_Base
{
public:
	// Asset type name
	virtual FText GetName() const override;
	
	// Color in Content Browser
	virtual FColor GetTypeColor() const override;
	
	// Class that this action supports
	virtual UClass* GetSupportedClass() const override;
	
	// Category in Content Browser
	virtual uint32 GetCategories() override;
	
	// Open editor for asset
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	
	// Can import this type
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override;
	
	// Context menu in Content Browser
	virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;

private:
	// Function to reset geometry to automatic  
	void ResetToAutoGeometry(TArray<TWeakObjectPtr<UManualSprite>> Objects);
};