#include "Character2DAssetEditorToolkit/SCharacter2DAssetViewport.h"
#include "Character2DActor.h"
#include "AdvancedPreviewScene.h"
#include "EditorViewportClient.h"
#include "UnrealWidget.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SOverlay.h"

void SCharacter2DAssetViewport::OnFloatingButtonClicked()
{
}

SCharacter2DAssetViewport::~SCharacter2DAssetViewport()
{
	if (PreviewActor && PreviewActor->IsValidLowLevel())
	{
		PreviewActor->Destroy();
	}
}

void SCharacter2DAssetViewport::Construct(const FArguments& InArgs)
{
	Asset = InArgs._CharacterAsset;
	PreviewScene = MakeShareable(
		new FAdvancedPreviewScene(FPreviewScene::ConstructionValues())
	);

    SEditorViewport::Construct(SEditorViewport::FArguments());


	if (Asset && PreviewScene->GetWorld())
	{
		PreviewActor = PreviewScene->GetWorld()->SpawnActor<ACharacter2DActor>();
		PreviewActor->CharacterAsset = Asset;
		PreviewActor->RefreshFromAsset();

		// фронтальный вид
		EditorViewportClient->SetViewLocation(FVector(0.f, 500.f, 75.f));
		EditorViewportClient->SetViewRotation(FRotator(0.f, -90.f, 0.f));
                EditorViewportClient->EngineShowFlags.SetPaper2DSprites(true);
                // Use legacy widget mode enum to avoid compile issues
                EditorViewportClient->SetWidgetMode(UE::Widget::WM_Translate);
        }
}

void SCharacter2DAssetViewport::Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime)
{
	SEditorViewport::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (PreviewScene.IsValid() && PreviewScene->GetWorld())
	{
		PreviewScene->GetWorld()->Tick(LEVELTICK_All, InDeltaTime);
	}
}

void SCharacter2DAssetViewport::RefreshPreview()
{
	if (PreviewActor)
	{
		PreviewActor->RefreshFromAsset();
	}
}

TSharedRef<FEditorViewportClient> SCharacter2DAssetViewport::MakeEditorViewportClient()
{
        EditorViewportClient = MakeShareable(new FEditorViewportClient(nullptr, PreviewScene.Get()));
        EditorViewportClient->SetViewModes(VMI_Lit, VMI_Lit);
        EditorViewportClient->SetRealtime(true);
        return EditorViewportClient.ToSharedRef();
}

TSharedRef<SWidget> SCharacter2DAssetViewport::MakeViewportToolbar()
{
    return BuildCameraToolbar();
}

void SCharacter2DAssetViewport::PopulateViewportOverlays(TSharedRef<SOverlay> Overlay)
{
    Overlay->AddSlot()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Top)
        .Padding(FMargin(2))
    [
        BuildCameraToolbar()
    ];
}

TSharedRef<SWidget> SCharacter2DAssetViewport::BuildCameraToolbar()
{
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().Padding(2)
        [
            SNew(SButton).Text(FText::FromString("Perspective")).OnClicked(this, &SCharacter2DAssetViewport::OnViewPerspective)
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(2)
        [
            SNew(SButton).Text(FText::FromString("Top")).OnClicked(this, &SCharacter2DAssetViewport::OnViewTop)
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(2)
        [
            SNew(SButton).Text(FText::FromString("Side")).OnClicked(this, &SCharacter2DAssetViewport::OnViewSide)
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(2)
        [
            SNew(SButton).Text(FText::FromString("Front")).OnClicked(this, &SCharacter2DAssetViewport::OnViewFront)
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(2)
        [
            SNew(SButton).Text(FText::FromString("Reset Camera")).OnClicked(this, &SCharacter2DAssetViewport::OnResetCamera)
        ];
}

FReply SCharacter2DAssetViewport::OnViewPerspective()
{
    if (EditorViewportClient.IsValid())
    {
        EditorViewportClient->SetViewLocation(FVector(300.f, 300.f, 300.f));
        EditorViewportClient->SetViewRotation(FRotator(-45.f, -45.f, 0.f));
    }
    return FReply::Handled();
}

FReply SCharacter2DAssetViewport::OnViewTop()
{
    if (EditorViewportClient.IsValid())
    {
        EditorViewportClient->SetViewLocation(FVector(0.f, 0.f, 600.f));
        EditorViewportClient->SetViewRotation(FRotator(-90.f, 0.f, 0.f));
    }
    return FReply::Handled();
}

FReply SCharacter2DAssetViewport::OnViewSide()
{
    if (EditorViewportClient.IsValid())
    {
        EditorViewportClient->SetViewLocation(FVector(0.f, 600.f, 75.f));
        EditorViewportClient->SetViewRotation(FRotator(0.f, -90.f, 0.f));
    }
    return FReply::Handled();
}

FReply SCharacter2DAssetViewport::OnViewFront()
{
    if (EditorViewportClient.IsValid())
    {
        EditorViewportClient->SetViewLocation(FVector(-600.f, 0.f, 75.f));
        EditorViewportClient->SetViewRotation(FRotator(0.f, 0.f, 0.f));
    }
    return FReply::Handled();
}

FReply SCharacter2DAssetViewport::OnResetCamera()
{
    if (EditorViewportClient.IsValid())
    {
        EditorViewportClient->SetViewLocation(FVector(0.f, 500.f, 75.f));
        EditorViewportClient->SetViewRotation(FRotator(0.f, -90.f, 0.f));
    }
    return FReply::Handled();
}

