// SCharacter2DAssetViewport.h

#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"
#include "Character2DAsset.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "TimerManager.h"

class FPreviewScene;
class ACharacter2DActor;
class FEditorViewportClient;
class FExtender;
class FUICommandList;

/**
 * Окно предпросмотра Character2DAsset с обычным EditorViewportToolbar
 */
class SCharacter2DAssetViewport
    : public SEditorViewport
    , public ICommonEditorViewportToolbarInfoProvider
{
public:
    SLATE_BEGIN_ARGS(SCharacter2DAssetViewport) {}
        /** Указываем Asset для предпросмотра */
        SLATE_ARGUMENT(UCharacter2DAsset*, CharacterAsset)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~SCharacter2DAssetViewport() override;

    void RefreshPreview();
    void ForceRefreshPreview();
    FString GetDebugInfo() const;

    TWeakObjectPtr<ACharacter2DActor> GetPreviewActor() const { return PreviewActor; }

protected:
    // SEditorViewport overrides
    virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
    virtual void BindCommands() override;
    virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

    /** Создаёт тулбар-виджет — SEditorViewport вставит его в оверлей */
    virtual TSharedPtr<SWidget> MakeViewportToolbar() override;

    // ICommonEditorViewportToolbarInfoProvider
    virtual TSharedRef<SEditorViewport> GetViewportWidget()                { return SharedThis(this); }
    virtual TSharedPtr<FEditorViewportClient> GetViewportClient() const    { return EditorViewportClient; }
    virtual TSharedPtr<SWidget> GetViewportToolbarWidget() const           
    {
        // Мы здесь в const-контексте, а MakeViewportToolbar() не const,
        // поэтому берём его через const_cast:
        return const_cast<SCharacter2DAssetViewport*>(this)->MakeViewportToolbar();
    }
    virtual TSharedPtr<FExtender> GetExtenders() const                     { return MakeShared<FExtender>(); }
    virtual TSharedPtr<FUICommandList> GetCommandList() const              { return SEditorViewport::GetCommandList(); }
    virtual void OnFloatingButtonClicked()                                 {}
    virtual bool IsVisible() const override                                        { return true; }

private:
    TSharedPtr<FPreviewScene> PreviewScene;
    TWeakObjectPtr<ACharacter2DActor> PreviewActor;
    UCharacter2DAsset*             Asset = nullptr;
    TSharedPtr<FEditorViewportClient> EditorViewportClient;
    FTimerHandle                   RefreshTimerHandle;
};
