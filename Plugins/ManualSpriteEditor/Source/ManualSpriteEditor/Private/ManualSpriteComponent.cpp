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
		const TArray<FManualSpriteVertex>& Vertices = ManualSprite->ManualGeometry.Vertices;
		const TArray<FManualSpriteTriangle>& Triangles = ManualSprite->ManualGeometry.Triangles;
		
		UE_LOG(LogTemp, Warning, TEXT("ManualSprite using manual geometry with %d vertices and %d triangles"), 
			   Vertices.Num(), Triangles.Num());
	}
    
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