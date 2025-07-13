#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "Engine/StaticMesh.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/Material.h"
#include "ManualSpriteMeshFactories.generated.h"

// Фабрика материалов для Manual Sprite
UCLASS()
class UManualSpriteMaterialFactory : public UFactory
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TObjectPtr<UTexture> SourceTexture;

	UPROPERTY()
	bool bUnlitMaterial = true;

	UPROPERTY()
	bool bTwoSided = true;

	UManualSpriteMaterialFactory();

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

// Фабрика скелетов для Manual Sprite
UCLASS()
class UManualSpriteSkeletonFactory : public UFactory
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FVector RootBonePosition = FVector::ZeroVector;

	UManualSpriteSkeletonFactory();

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InContext, FFeedbackContext* InWarn) override;
};

// Фабрика SkeletalMesh для Manual Sprite
UCLASS()
class UManualSpriteSkeletalMeshFactory : public UFactory
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TObjectPtr<UStaticMesh> SourceStaticMesh;

	UPROPERTY()
	TObjectPtr<USkeleton> TargetSkeleton;

	UPROPERTY()
	FName RootBoneName = TEXT("Root");

	UManualSpriteSkeletalMeshFactory();

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InContext, FFeedbackContext* InWarn) override;
};