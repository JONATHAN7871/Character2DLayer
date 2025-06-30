#pragma once

#include "CoreMinimal.h"
#include "PaperSpriteComponent.h"
#include "ManualSprite.h"
#include "ManualSpriteComponent.generated.h"

/**
 * Кастомный компонент спрайта с поддержкой ручной геометрии
 */
UCLASS(ClassGroup=(Rendering, Common), hidecategories=(Object,Activation,"Components|Activation"), ShowCategories=(Mobility), ComponentWrapperClass, meta=(BlueprintSpawnableComponent))
class MANUALSPRITEEDITOR_API UManualSpriteComponent : public UPaperSpriteComponent
{
	GENERATED_BODY()

public:
	UManualSpriteComponent();

	// Переопределяем для создания кастомного SceneProxy
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

	// Установка кастомного спрайта
	UFUNCTION(BlueprintCallable, Category = "Sprite", meta = (CallInEditor = "true"))
	void SetManualSprite(UManualSprite* NewSprite);

	// ИСПРАВЛЕНИЕ: Убираем const чтобы избежать проблем с GetSprite()
	// Получение кастомного спрайта
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Sprite")
	UManualSprite* GetManualSprite();

	// ИСПРАВЛЕНИЕ: Убираем const чтобы избежать проблем с GetSprite()
	// Проверка, используется ли ручная геометрия
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Sprite", meta = (CallInEditor = "true"))
	bool IsUsingManualGeometry();

protected:
	// Обновление при изменении спрайта
	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) override;
};