#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"
#include "Character2DAsset.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "Widgets/SOverlay.h"

class FPreviewScene;
class ACharacter2DActor;
class FEditorViewportClient;

class SCharacter2DAssetViewport : public SEditorViewport, public ICommonEditorViewportToolbarInfoProvider
{
public:
    SLATE_BEGIN_ARGS(SCharacter2DAssetViewport) {}
        SLATE_ARGUMENT(UCharacter2DAsset*, CharacterAsset)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~SCharacter2DAssetViewport() override;

    // Refresh preview after asset changes
    void RefreshPreview();

    // Get preview actor for Actions panel
    TWeakObjectPtr<ACharacter2DActor> GetPreviewActor() const { return PreviewActor; }

    // Selection callback
    void OnActorSelected(AActor* Actor);

protected:
    // Tick for world updates
    virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

    // ICommonEditorViewportToolbarInfoProvider interface
    virtual TSharedRef<SEditorViewport> GetViewportWidget() override { return SharedThis(this); }
    virtual TSharedPtr<FExtender> GetExtenders() const override { return nullptr; }
    virtual void OnFloatingButtonClicked() override;

    // SEditorViewport interface
    virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
    virtual TSharedPtr<SWidget> MakeViewportToolbar() override;
    virtual void PopulateViewportOverlays(TSharedRef<SOverlay> Overlay) override;
    virtual void BindCommands() override {}

    // Camera control handlers
    FReply OnViewPerspective();
    FReply OnViewTop();
    FReply OnViewSide();
    FReply OnViewFront();
    FReply OnResetCamera();

    // Build UI elements
    TSharedRef<SWidget> BuildCameraToolbar();
    TSharedRef<SWidget> BuildTransformToolBar();

private:
    TSharedPtr<FPreviewScene> PreviewScene;
    ACharacter2DActor* PreviewActor = nullptr;
    UCharacter2DAsset* Asset = nullptr;
    TSharedPtr<FEditorViewportClient> EditorViewportClient;
};