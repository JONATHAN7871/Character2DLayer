#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"
#include "Character2DAsset.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "Widgets/Layout/SOverlay.h"

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

	// Позволяет извне «перерисовать» актёра после изменения ассета
	void RefreshPreview();

	// Получить превью актера для Actions панели
	TWeakObjectPtr<ACharacter2DActor> GetPreviewActor() const { return PreviewActor; }

protected:
	// Нужно, чтобы world.tick() вызвался каждый кадр
	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

	// Viewport Widget
	virtual TSharedRef<SEditorViewport> GetViewportWidget() override { return SharedThis(this); }
	virtual TSharedPtr<FExtender> GetExtenders() const override { return nullptr; }
       virtual void BindCommands() override {}

        // Создатель viewport client
        virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

        // Override to add toolbar and overlays
        virtual TSharedRef<SWidget> MakeViewportToolbar() override;
        virtual void PopulateViewportOverlays(TSharedRef<SOverlay> Overlay) override;

       virtual void OnFloatingButtonClicked() override;

       /** Camera control handlers */
       FReply OnViewPerspective();
       FReply OnViewTop();
       FReply OnViewSide();
       FReply OnViewFront();
       FReply OnResetCamera();

       TSharedRef<SWidget> BuildCameraToolbar();

private:
	TSharedPtr<FPreviewScene> PreviewScene;
	ACharacter2DActor* PreviewActor = nullptr;
	UCharacter2DAsset* Asset = nullptr;
	TSharedPtr<FEditorViewportClient> EditorViewportClient;
};

