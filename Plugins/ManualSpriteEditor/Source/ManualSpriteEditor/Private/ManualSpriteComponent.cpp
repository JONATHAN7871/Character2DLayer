#include "ManualSpriteComponent.h"
#include "Engine/Engine.h"
#include "Materials/Material.h"
#include "MaterialDomain.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "RenderingThread.h"
#include "PrimitiveViewRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "MaterialShared.h"
#include "Materials/MaterialRenderProxy.h"
#include "Engine/CollisionProfile.h"
#include "SceneManagement.h"
#include "DynamicMeshBuilder.h"
#include "PhysicsEngine/BodySetup.h"
#include "RHI.h"
#include "RHICommandList.h"

/**
 * Scene Proxy для рендеринга кастомной геометрии спрайта
 */
class FManualSpriteSceneProxy final : public FPrimitiveSceneProxy
{
public:
	virtual SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	explicit FManualSpriteSceneProxy(const UManualSpriteComponent* InComponent)
		: FPrimitiveSceneProxy(InComponent)
		, MaterialRelevance(InComponent->GetMaterialRelevance(GetScene().GetFeatureLevel()))
		, Material(nullptr)
		, bUseManualGeometry(false)
		, VertexFactory(GetScene().GetFeatureLevel(), "FManualSpriteSceneProxy")
	{
		// ИСПРАВЛЕНИЕ: Используем const_cast для получения спрайта из const компонента
		UManualSpriteComponent* NonConstComponent = const_cast<UManualSpriteComponent*>(InComponent);
		if (const UManualSprite* ManualSprite = NonConstComponent->GetManualSprite())
		{
			bUseManualGeometry = ManualSprite->bUseManualGeometry && ManualSprite->IsManualGeometryValid();
			
			if (bUseManualGeometry)
			{
				// Копируем геометрию
				ManualGeometry = ManualSprite->ManualGeometry;
				
				// Получаем материал
				Material = NonConstComponent->GetMaterial(0);
				if (!Material)
				{
					Material = UMaterial::GetDefaultMaterial(MD_Surface);
				}
				
				// Строим меш
				BuildMesh();
			}
		}
		
		// Если не используем ручную геометрию, создаём стандартный меш
		if (!bUseManualGeometry)
		{
			BuildDefaultMesh(InComponent);
		}
	}

	virtual ~FManualSpriteSceneProxy() override = default;

	// Основная функция рендеринга
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, const uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_ManualSpriteSceneProxy_GetDynamicMeshElements);

		// Проверяем, есть ли что рендерить
		if (IndexBuffer.Indices.Num() == 0 || VertexBuffer.Vertices.Num() == 0)
		{
			return;
		}

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				// Создаём mesh element
				FMeshBatch& MeshBatch = Collector.AllocateMesh();
				FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
				
				// Настраиваем mesh batch
				MeshBatch.bWireframe = false;
				MeshBatch.VertexFactory = &VertexFactory;
				MeshBatch.MaterialRenderProxy = Material->GetRenderProxy();
				MeshBatch.ReverseCulling = IsLocalToWorldDeterminantNegative();
				MeshBatch.Type = PT_TriangleList;
				MeshBatch.DepthPriorityGroup = SDPG_World;
				MeshBatch.bCanApplyViewModeOverrides = false;
				
				// Настраиваем batch element
				BatchElement.IndexBuffer = &IndexBuffer;
				BatchElement.FirstIndex = 0;
				BatchElement.NumPrimitives = IndexBuffer.Indices.Num() / 3;
				BatchElement.MinVertexIndex = 0;
				BatchElement.MaxVertexIndex = VertexBuffer.Vertices.Num() - 1;
				
				Collector.AddMesh(ViewIndex, MeshBatch);
			}
		}
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		Result.bRenderInMainPass = ShouldRenderInMainPass();
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}

	virtual bool CanBeOccluded() const override
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual uint32 GetMemoryFootprint() const override 
	{ 
		return sizeof(*this) + GetAllocatedSizeInternal(); 
	}

private:
	// Внутренняя функция для подсчёта памяти
	uint32 GetAllocatedSizeInternal() const 
	{ 
		return FPrimitiveSceneProxy::GetAllocatedSize(); 
	}

	// Структуры для хранения геометрии
	struct FManualSpriteVertex
	{
		FVector3f Position;
		FVector2f UV;
		FPackedNormal TangentX;
		FPackedNormal TangentZ;
		FColor Color;
		
		FManualSpriteVertex() 
			: Position(FVector3f::ZeroVector)
			, UV(FVector2f::ZeroVector)
			, TangentX(FVector3f(1, 0, 0))
			, TangentZ(FVector3f(0, 0, 1))
			, Color(FColor::White)
		{
		}
		
		FManualSpriteVertex(const FVector3f& InPosition, const FVector2f& InUV)
			: Position(InPosition)
			, UV(InUV)
			, TangentX(FVector3f(1, 0, 0))
			, TangentZ(FVector3f(0, 0, 1))
			, Color(FColor::White)
		{
		}
	};

	// Vertex Buffer
	class FManualSpriteVertexBuffer final : public FVertexBuffer
	{
	public:
		TArray<FManualSpriteVertex> Vertices;

		virtual void InitRHI(FRHICommandListBase& RHICmdList) override
		{
			FRHIResourceCreateInfo CreateInfo(TEXT("FManualSpriteVertexBuffer"));
			VertexBufferRHI = RHICmdList.CreateVertexBuffer(Vertices.Num() * sizeof(FManualSpriteVertex), BUF_Static, CreateInfo);
			void* Buffer = RHICmdList.LockBuffer(VertexBufferRHI, 0, Vertices.Num() * sizeof(FManualSpriteVertex), RLM_WriteOnly);
			FMemory::Memcpy(Buffer, Vertices.GetData(), Vertices.Num() * sizeof(FManualSpriteVertex));
			RHICmdList.UnlockBuffer(VertexBufferRHI);
		}
	};

	// Index Buffer
	class FManualSpriteIndexBuffer final : public FIndexBuffer
	{
	public:
		TArray<uint32> Indices;

		virtual void InitRHI(FRHICommandListBase& RHICmdList) override
		{
			FRHIResourceCreateInfo CreateInfo(TEXT("FManualSpriteIndexBuffer"));
			IndexBufferRHI = RHICmdList.CreateIndexBuffer(sizeof(uint32), Indices.Num() * sizeof(uint32), BUF_Static, CreateInfo);
			void* Buffer = RHICmdList.LockBuffer(IndexBufferRHI, 0, Indices.Num() * sizeof(uint32), RLM_WriteOnly);
			FMemory::Memcpy(Buffer, Indices.GetData(), Indices.Num() * sizeof(uint32));
			RHICmdList.UnlockBuffer(IndexBufferRHI);
		}
	};

	// Vertex Factory для связи vertex buffer с шейдерами
	class FManualSpriteVertexFactory final : public FLocalVertexFactory
	{
	public:
		FManualSpriteVertexFactory(const ERHIFeatureLevel::Type InFeatureLevel, const char* InDebugName)
			: FLocalVertexFactory(InFeatureLevel, InDebugName)
		{
		}

		void Init(FRHICommandListBase& RHICmdList, const FManualSpriteVertexBuffer* VertexBuffer)
		{
			FLocalVertexFactory::FDataType NewData;
			NewData.PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FManualSpriteVertex, Position, VET_Float3);
			NewData.TextureCoordinates.Add(STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FManualSpriteVertex, UV, VET_Float2));
			NewData.TangentBasisComponents[0] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FManualSpriteVertex, TangentX, VET_PackedNormal);
			NewData.TangentBasisComponents[1] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FManualSpriteVertex, TangentZ, VET_PackedNormal);
			NewData.ColorComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FManualSpriteVertex, Color, VET_Color);
			SetData(RHICmdList, NewData);
		}
	};

	// Построение меша из ручной геометрии
	void BuildMesh()
	{
		if (!ManualGeometry.IsValid())
		{
			return;
		}

		// Очищаем буферы
		VertexBuffer.Vertices.Empty();
		IndexBuffer.Indices.Empty();

		// Заполняем vertex buffer
		for (const auto& Vertex : ManualGeometry.Vertices)
		{
			FVector3f Position3D(Vertex.Position.X, Vertex.Position.Y, 0.0f);
			FVector2f UV(Vertex.UV.X, Vertex.UV.Y);
			VertexBuffer.Vertices.Add(FManualSpriteVertex(Position3D, UV));
		}

		// Заполняем index buffer
		for (const auto& Triangle : ManualGeometry.Triangles)
		{
			IndexBuffer.Indices.Add(Triangle.VertexIndex0);
			IndexBuffer.Indices.Add(Triangle.VertexIndex1);
			IndexBuffer.Indices.Add(Triangle.VertexIndex2);
		}

		// Инициализируем буферы
		ENQUEUE_RENDER_COMMAND(InitManualSpriteBuffers)(
			[this](FRHICommandListImmediate& RHICmdList)
			{
				BeginInitResource(&VertexBuffer);
				BeginInitResource(&IndexBuffer);
				
				// Инициализируем vertex factory
				VertexFactory.Init(RHICmdList, &VertexBuffer);
				BeginInitResource(&VertexFactory);
			});
	}

	// Построение стандартного меша для fallback
	void BuildDefaultMesh(const UManualSpriteComponent* InComponent)
	{
		// Создаём простой квад как fallback
		VertexBuffer.Vertices.Empty();
		IndexBuffer.Indices.Empty();

		// 4 вершины квада
		constexpr float HalfWidth = 50.0f;  // Базовый размер
		constexpr float HalfHeight = 50.0f;
		
		VertexBuffer.Vertices.Add(FManualSpriteVertex(FVector3f(-HalfWidth, -HalfHeight, 0), FVector2f(0, 1)));
		VertexBuffer.Vertices.Add(FManualSpriteVertex(FVector3f(HalfWidth, -HalfHeight, 0), FVector2f(1, 1)));
		VertexBuffer.Vertices.Add(FManualSpriteVertex(FVector3f(HalfWidth, HalfHeight, 0), FVector2f(1, 0)));
		VertexBuffer.Vertices.Add(FManualSpriteVertex(FVector3f(-HalfWidth, HalfHeight, 0), FVector2f(0, 0)));

		// 2 треугольника
		IndexBuffer.Indices.Add(0);
		IndexBuffer.Indices.Add(1);
		IndexBuffer.Indices.Add(2);
		
		IndexBuffer.Indices.Add(0);
		IndexBuffer.Indices.Add(2);
		IndexBuffer.Indices.Add(3);

		// Получаем материал
		UManualSpriteComponent* NonConstComponent = const_cast<UManualSpriteComponent*>(InComponent);
		Material = NonConstComponent->GetMaterial(0);
		if (!Material)
		{
			Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}

		// Инициализируем буферы
		ENQUEUE_RENDER_COMMAND(InitManualSpriteDefaultBuffers)(
			[this](FRHICommandListImmediate& RHICmdList)
			{
				BeginInitResource(&VertexBuffer);
				BeginInitResource(&IndexBuffer);
				
				// Инициализируем vertex factory
				VertexFactory.Init(RHICmdList, &VertexBuffer);
				BeginInitResource(&VertexFactory);
			});
	}

private:
	// Данные для рендеринга
	FMaterialRelevance MaterialRelevance;
	UMaterialInterface* Material;
	bool bUseManualGeometry;
	FManualSpriteGeometry ManualGeometry;
	
	// Буферы для рендеринга
	FManualSpriteVertexBuffer VertexBuffer;
	FManualSpriteIndexBuffer IndexBuffer;
	FManualSpriteVertexFactory VertexFactory;
};

// Реализация UManualSpriteComponent
UManualSpriteComponent::UManualSpriteComponent()
{
	// Настройка компонента по умолчанию
	PrimaryComponentTick.bCanEverTick = false;
	
	// ИСПРАВЛЕНИЕ: Устанавливаем профиль коллизии напрямую без вызова виртуальной функции
	BodyInstance.SetCollisionProfileName("NoCollision");
}

FPrimitiveSceneProxy* UManualSpriteComponent::CreateSceneProxy()
{
	// ИСПРАВЛЕНИЕ: Временно используем родительский SceneProxy чтобы избежать краша
	// Наш кастомный SceneProxy пока что вызывает ошибки с шейдерами
    
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