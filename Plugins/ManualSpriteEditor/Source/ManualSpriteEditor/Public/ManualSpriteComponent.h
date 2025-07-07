#pragma once

#include "CoreMinimal.h"
#include "PaperSpriteComponent.h"
#include "ManualSprite.h"
#include "ManualSpriteComponent.generated.h"

/**
 * Custom sprite component with manual geometry support
 */
UCLASS(ClassGroup=(Rendering, Common), hidecategories=(Object,Activation,"Components|Activation"), ShowCategories=(Mobility), ComponentWrapperClass, meta=(BlueprintSpawnableComponent))
class MANUALSPRITEEDITOR_API UManualSpriteComponent : public UPaperSpriteComponent
{
	GENERATED_BODY()

public:
	UManualSpriteComponent();

	// Override to create custom SceneProxy
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

	// Set custom sprite
	UFUNCTION(BlueprintCallable, Category = "Sprite", meta = (CallInEditor = "true"))
	void SetManualSprite(UManualSprite* NewSprite);

	// FIXED: Remove const to avoid issues with GetSprite()
	// Get custom sprite
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Sprite")
	UManualSprite* GetManualSprite();

	// FIXED: Remove const to avoid issues with GetSprite()
	// Check if using manual geometry
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Sprite", meta = (CallInEditor = "true"))
	bool IsUsingManualGeometry();

protected:
	// Update when sprite changes
	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) override;
};