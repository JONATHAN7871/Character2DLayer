// Character2DMeshGeneratorDialog.h
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"
#include "Character2DBuilderWindow/AssetData/Character2DLayerData.h"
#include "Character2DMeshGeneratorOptions.h"

/**
 * Модальный диалог настройки лишь общего поведения генерации:
 * вывода, пути, пивота и имени ассета.
 * Параметры по каждой категории (grid-mesh, размеры ячейки, порог)
 * перенесены непосредственно в SCharacter2DBuilderWindow и
 * структуру FCharacter2DLayerCategory.
 */
class SCharacter2DMeshGeneratorDialog : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCharacter2DMeshGeneratorDialog) {}
        SLATE_ARGUMENT(TArray<TSharedPtr<FCharacter2DLayerCategory>>, Categories)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    // Слои
    TArray<TSharedPtr<FCharacter2DLayerCategory>> Categories;

    // Общие опции
    FString                        AssetName        = TEXT("GeneratedMesh");
    FString                        SavePath         = TEXT("/Game/Character2DBuilder/Generated");
    ECharacter2DMeshOutputType     SelectedOutputType   = ECharacter2DMeshOutputType::SkeletalMesh;
    ECharacter2DRootBonePlacement SelectedPivotPlacement = ECharacter2DRootBonePlacement::Origin;

    // Опции для ComboBox
    TArray<TSharedPtr<ECharacter2DMeshOutputType>>       OutputTypeOptions;
    TArray<TSharedPtr<ECharacter2DRootBonePlacement>>   PivotPlacementOptions;

    // Виджеты, чтобы можно было RefreshOptions()
    TSharedPtr<SComboBox<TSharedPtr<ECharacter2DMeshOutputType>>>     OutputTypeCombo;
    TSharedPtr<SComboBox<TSharedPtr<ECharacter2DRootBonePlacement>>> PivotCombo;

    // Отрисовка текста
    FText GetOutputTypeLabel() const;
    FText GetPivotPlacementLabel() const;
    FText GetAssetNameText() const { return FText::FromString(AssetName); }
    FText GetSavePathText() const  { return FText::FromString(SavePath); }

    // Обработчики
    void OnOutputTypeChanged(TSharedPtr<ECharacter2DMeshOutputType> NewVal, ESelectInfo::Type);
    void OnPivotPlacementChanged(TSharedPtr<ECharacter2DRootBonePlacement> NewVal, ESelectInfo::Type);

    // Кнопки
    FReply OnGenerateClicked();
    FReply OnCancelClicked();
};
