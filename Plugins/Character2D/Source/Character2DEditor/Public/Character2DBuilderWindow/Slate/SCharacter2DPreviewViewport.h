// SCharacter2DPreviewViewport.h

#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"
#include "Components/ArrowComponent.h"
#include "MeshDescription.h"

struct FCharacter2DLayerCategory;
class FCharacter2DPreviewViewportClient;
class FPreviewScene;
class UPaperSpriteComponent;
class UStaticMesh;

/**
 * Slate-виджет для предпросмотра 2D-персонажа.
 */
class SCharacter2DPreviewViewport : public SEditorViewport
{
public:
    SLATE_BEGIN_ARGS(SCharacter2DPreviewViewport) {}
    SLATE_END_ARGS()

    /** Строит виджет */
    void Construct(const FArguments& InArgs);

    /** Обновляет список спрайтов в превью-сцене */
    void UpdatePreviewSprites(
        const TArray<TSharedPtr<struct FCharacter2DLayerCategory>>& Categories,
        float InPreviewScale);

    /** Центрирует камеру по всем активным спрайтам */
    void CenterCameraOnSprites();

    virtual ~SCharacter2DPreviewViewport();
    
    TObjectPtr<UArrowComponent> PivotArrow;

    /** перестроить предварительный StaticMesh из текущих слоёв */
    void UpdatePreviewMeshDescription(
        const TArray<TSharedPtr<FCharacter2DLayerCategory>>& Categories,
        float InPreviewScale);

    /** переключить отображение wire-frame */
    void SetWireframe(bool bEnable);

    /** виден ли сейчас каркас */
    bool bShowWireframe = false;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> PreviewMeshComp;

protected:
    // SEditorViewport overrides
    virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
   // virtual TSharedPtr<SWidget> MakeViewportToolbar() override { return SNullWidget::NullWidget; }
    virtual void BindCommands() override {}

private:
    /** Возвращает UWorld для нашей превью-сцены */
    UWorld* GetPreviewWorld() const;

    /** Увеличивает пул до нужного размера */
    void EnsureComponentPoolSize(int32 Count);

    float PreviewScale = 1.0f;
    
    /** Сцена, на которой рендерятся спрайты */
    TUniquePtr<FPreviewScene> PreviewScene;

    /** Клиент вьюпорта (камера + рендер) */
    TSharedPtr<FCharacter2DPreviewViewportClient> ViewportClient;

    /** Пул всех созданных PaperSpriteComponent */
    TArray<UPaperSpriteComponent*> ComponentPool;

    /** Активные (видимые) компоненты после последнего UpdatePreviewSprites */
    TArray<UPaperSpriteComponent*> ActiveComponents;
};
