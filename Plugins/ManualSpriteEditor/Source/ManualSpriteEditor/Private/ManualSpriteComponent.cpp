#include "ManualSpriteComponent.h"
#include "Engine/Engine.h"
#include "Materials/Material.h"
#include "Engine/CollisionProfile.h"

UManualSpriteComponent::UManualSpriteComponent()
{
	// Настройка компонента по умолчанию
	PrimaryComponentTick.bCanEverTick = false;
	
	// Устанавливаем профиль коллизии напрямую без вызова виртуальной функции
	BodyInstance.SetCollisionProfileName("NoCollision");
}

FPrimitiveSceneProxy* UManualSpriteComponent::CreateSceneProxy()
{
	UManualSprite* ManualSprite = GetManualSprite();
	if (ManualSprite && ManualSprite->bUseManualGeometry && ManualSprite->IsManualGeometryValid())
	{
		// TODO: Здесь будет наш кастомный SceneProxy когда исправим проблемы с рендерингом
		// Пока что логируем, что мы используем ручную геометрию
		UE_LOG(LogTemp, Warning, TEXT("ManualSprite using manual geometry with %d vertices and %d triangles"), 
			   ManualSprite->ManualGeometry.Vertices.Num(), 
			   ManualSprite->ManualGeometry.Triangles.Num());
	}
    
	// Используем стандартный SceneProxy от родительского класса
	return Super::CreateSceneProxy();
}

void UManualSpriteComponent::SetManualSprite(UManualSprite* NewSprite)
{
	SetSprite(NewSprite);
	MarkRenderStateDirty();
}

UManualSprite* UManualSpriteComponent::GetManualSprite()
{
	return Cast<UManualSprite>(GetSprite());
}

bool UManualSpriteComponent::IsUsingManualGeometry()
{
	UManualSprite* ManualSprite = GetManualSprite();
	return ManualSprite && ManualSprite->bUseManualGeometry && ManualSprite->IsManualGeometryValid();
}

void UManualSpriteComponent::OnUpdateTransform(const EUpdateTransformFlags UpdateTransformFlags, const ETeleportType Teleport)
{
	Super::OnUpdateTransform(UpdateTransformFlags, Teleport);
    
	// Обновляем рендеринг при изменении трансформа
	MarkRenderStateDirty();
}