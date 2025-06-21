#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Character2DAsset.h"
#include "IDetailsView.h"
#include "WorkspaceMenuStructure.h"
#include "Character2DEnums.h"
#include "Widgets/Docking/SDockTab.h"

// Прямые объявления для уменьшения зависимостей в заголовке
class SCharacter2DAssetViewport;
class SCharacter2DActionPanel;
class SCharacter2DPresetPanel;
class ACharacter2DActor;

class FCharacter2DAssetEditorToolkit : public FAssetEditorToolkit, public FGCObject
{
public:
    // Инициализация редактора
    void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UCharacter2DAsset* InAsset);

    // Переопределения из FAssetEditorToolkit
    virtual FName GetToolkitFName() const override;
    virtual FText GetBaseToolkitName() const override;
    virtual FString GetWorldCentricTabPrefix() const override;
    virtual FLinearColor GetWorldCentricTabColorScale() const override;
    virtual FString GetReferencerName() const override { return TEXT("FCharacter2DAssetEditorToolkit"); }

    virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

    // Переопределение из FGCObject для сборки мусора
    virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

    // ID вкладок
    static const FName ViewportTabID;
    static const FName SkeletalDetailsTabID;
    static const FName SpriteDetailsTabID;
    static const FName ActionsTabID;
    static const FName PresetsTabID;

private:
    // Обработчик события изменения свойств ассета
    void OnAssetPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent);
    
    // Функции для создания вкладок (Tab Spawners)
    TSharedRef<SDockTab> SpawnViewportTab(const FSpawnTabArgs& Args);
    TSharedRef<SDockTab> SpawnSkeletalDetailsTab(const FSpawnTabArgs& Args);
    TSharedRef<SDockTab> SpawnSpriteDetailsTab(const FSpawnTabArgs& Args);
    TSharedRef<SDockTab> SpawnActionsTab(const FSpawnTabArgs& Args);
    TSharedRef<SDockTab> SpawnPresetsTab(const FSpawnTabArgs& Args);

    // Функции для создания панелей Details с фильтрацией свойств
    TSharedRef<IDetailsView> CreateSkeletalDetailsView();
    TSharedRef<IDetailsView> CreateSpriteDetailsView();

    // Ссылки на виджеты и объекты
    TSharedPtr<SCharacter2DAssetViewport> ViewportWidget;
    TSharedPtr<IDetailsView> SkeletalDetailsView;
    TSharedPtr<IDetailsView> SpriteDetailsView;
    TSharedPtr<SCharacter2DActionPanel> ActionPanel;
    TSharedPtr<SCharacter2DPresetPanel> PresetPanel;

    // Редактируемый ассет
    TObjectPtr<UCharacter2DAsset> AssetBeingEdited = nullptr;
    
    // Категория меню для вкладок
    TSharedPtr<FWorkspaceItem> WorkspaceMenuCategory;

    // Текущий режим редактирования (для панели пресетов)
    ECharacter2DEditMode CurrentMode = ECharacter2DEditMode::Body;
};