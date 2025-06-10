#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Character2DAsset.h"
#include "IDetailsView.h"
#include "WorkspaceMenuStructure.h"
#include "Character2DEnums.h"
#include "Slate/SCharacter2DPresetPanel.h"
#include "Widgets/Docking/SDockTab.h"
#include "IDetailCustomization.h"

class SCharacter2DAssetViewport;
class SCharacter2DActionPanel;

class FCharacter2DSpriteCustomization : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};

class FCharacter2DAssetEditorToolkit : public FAssetEditorToolkit, public FGCObject
{
public:
        void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UCharacter2DAsset* InAsset);

	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FString GetReferencerName() const override { return TEXT("FCharacter2DAssetEditorToolkit"); }

	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

	// Tab IDs
	static const FName ViewportTabID;
	static const FName SkeletalDetailsTabID;
	static const FName SpriteDetailsTabID;
	static const FName GeneralDetailsTabID;
	static const FName ActionsTabID;
	static const FName PresetsTabID;

	void AddReferencedObjects(FReferenceCollector& Collector) override;

	ECharacter2DEditMode CurrentMode = ECharacter2DEditMode::Body;
	TSharedPtr<SCharacter2DPresetPanel> PresetPanel;

private:
	/** Вызывается когда в DetailsView что-то отредактировали */
	void OnAssetPropertyChanged(const FPropertyChangedEvent&);
	
	// Tab spawners
	TSharedRef<SDockTab> SpawnViewportTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnSkeletalDetailsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnSpriteDetailsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnActionsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnPresetsTab(const FSpawnTabArgs& Args);

	// Create details views with specific property filters
	TSharedRef<IDetailsView> CreateSkeletalDetailsView();
	TSharedRef<IDetailsView> CreateSpriteDetailsView();

	// Widget references
	TSharedPtr<SCharacter2DAssetViewport> ViewportWidget;
	TSharedPtr<IDetailsView> SkeletalDetailsView;
	TSharedPtr<IDetailsView> SpriteDetailsView;
	TSharedPtr<SCharacter2DActionPanel> ActionPanel;

	TObjectPtr<UCharacter2DAsset> AssetBeingEdited = nullptr;
	TSharedPtr<FWorkspaceItem> WorkspaceMenuCategory;
};
