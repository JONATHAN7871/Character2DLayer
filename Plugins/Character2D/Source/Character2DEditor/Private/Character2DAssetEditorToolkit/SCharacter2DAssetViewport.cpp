#include "Character2DAssetEditorToolkit/SCharacter2DAssetViewport.h"
#include "Character2DActor.h"
#include "AdvancedPreviewScene.h"
#include "EditorViewportClient.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"

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
