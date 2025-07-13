#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"
#include "ManualSprite.h"
#include "ManualSpriteMeshGeneratorOptions.h"

class SManualSpriteMeshGeneratorDialog : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SManualSpriteMeshGeneratorDialog) {}
        SLATE_ARGUMENT(UManualSprite*, ManualSprite)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    // Данные
    UManualSprite* ManualSprite = nullptr;
    
    // Локальные настройки диалога
    EManualSpriteMeshType SelectedMeshType = EManualSpriteMeshType::StaticMesh;
    EManualSpritePivotPlacement SelectedPivotPlacement = EManualSpritePivotPlacement::Center;
    FVector CustomPivotOffset = FVector::ZeroVector;
    float MeshScale = 1.0f;
    FVector MeshOffset = FVector::ZeroVector;
    FString AssetName = TEXT("ManualSpriteMesh");
    FString SavePath = TEXT("/Game/GeneratedMeshes");
    bool bCreateMaterial = true;
    bool bCreateUnlitMaterial = true;
    bool bTwoSidedMaterial = true;

    // Опции для ComboBox
    TArray<TSharedPtr<EManualSpriteMeshType>> MeshTypeOptions;
    TArray<TSharedPtr<EManualSpritePivotPlacement>> PivotOptions;

    // Виджеты
    TSharedPtr<SComboBox<TSharedPtr<EManualSpriteMeshType>>> MeshTypeCombo;
    TSharedPtr<SComboBox<TSharedPtr<EManualSpritePivotPlacement>>> PivotCombo;

    // Методы UI
    void InitializeFromSettings();
    void LoadSettings();
    void SaveSettings();

    // Обработчики изменений
    void OnMeshTypeChanged(TSharedPtr<EManualSpriteMeshType> NewValue, ESelectInfo::Type);
    void OnPivotPlacementChanged(TSharedPtr<EManualSpritePivotPlacement> NewValue, ESelectInfo::Type);

    // Получение текста для отображения
    FText GetMeshTypeText() const;
    FText GetPivotPlacementText() const;
    FText GetAssetNameText() const { return FText::FromString(AssetName); }
    FText GetSavePathText() const { return FText::FromString(SavePath); }

    // Обработчики кнопок
    FReply OnGenerateClicked();
    FReply OnCancelClicked();

    // Валидация
    bool CanGenerate() const;
    FText GetGenerateButtonTooltip() const;
};
