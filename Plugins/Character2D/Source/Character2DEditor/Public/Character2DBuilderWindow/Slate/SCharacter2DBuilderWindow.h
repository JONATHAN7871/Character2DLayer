// SCharacter2DBuilderWindow.h

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Templates/SharedPointer.h"
#include "Character2DBuilderWindow/AssetData/Character2DLayerData.h"
#include "Character2DBuilderWindow/Character2DMeshGeneratorOptions.h"

class SCharacter2DPreviewViewport;

/**
 * Окно билдера 2D-персонажа с настройками:
 * - для каждой категории: grid/ячейка/порог
 * - глобальные: имя ассета, путь, output type, pivot, масштаб
 */
class SCharacter2DBuilderWindow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCharacter2DBuilderWindow) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    // UI
    TSharedRef<SWidget> BuildLayersPanel();
    TSharedRef<SWidget> BuildViewportPanel();
    TSharedRef<SWidget> BuildSettingsPanel();
    TSharedRef<SWidget> BuildCategoryWidget(TSharedPtr<FCharacter2DLayerCategory> Category);

    // Обновление
    void RefreshCategoryList();
    void RefreshPreview();

    // Обработчики категорий
    FReply HandleAddLayer(TSharedPtr<FCharacter2DLayerCategory> Category);
    FReply HandleMoveLayer(TSharedPtr<FCharacter2DLayerCategory> Category, int32 Index, bool bUp);
    FReply HandleRemoveLayer(TSharedPtr<FCharacter2DLayerCategory> Category, int32 Index);
    FReply HandleToggleVisibility(TSharedPtr<FCharacter2DLayerSlot> Slot);

    // Генерация и сохранение
    FReply HandleGenerateMesh();
    FReply HandleSaveAsAsset();
    FReply HandleLoadAsset();

    // Асинхр. загрузка
    void LoadFromAsset(const FAssetData& InAssetData);
    void OnSpritesLoaded(FAssetData LoadedAssetData);

    void RebuildLayout();
    void ForceRefreshGlobalsUI();
    TSharedPtr<SSplitter> RootSplitter;

    // Данные
    TArray<TSharedPtr<FCharacter2DLayerCategory>> Categories;
    TWeakPtr<SCharacter2DBuilderWindow>           WeakThisPtr;

    // Виджеты
    TSharedPtr<SVerticalBox>                           CategoryContainer;
    TSharedPtr<SCharacter2DPreviewViewport>            PreviewViewport;

    // ========== Новые глобальные параметры ==========

    // AssetName / SavePath
    TSharedPtr<SEditableTextBox>                       AssetNameTextBox;
    TSharedPtr<SEditableTextBox>                       SavePathTextBox;

    // OutputType
    TArray<TSharedPtr<ECharacter2DMeshOutputType>>     OutputTypeOptions;
    TSharedPtr<SComboBox<TSharedPtr<ECharacter2DMeshOutputType>>> OutputTypeCombo;

    // PivotPlacement
    TArray<TSharedPtr<ECharacter2DRootBonePlacement>>  PivotOptions;
    TSharedPtr<SComboBox<TSharedPtr<ECharacter2DRootBonePlacement>>> PivotCombo;

    // MeshScale
    float                                              MeshScale = 1.0f;
};
