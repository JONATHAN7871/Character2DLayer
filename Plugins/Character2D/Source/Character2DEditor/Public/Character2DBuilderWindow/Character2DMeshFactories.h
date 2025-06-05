#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "Engine/StaticMesh.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/Material.h"
#include "Character2DMeshFactories.generated.h"

// ================== Материал ===================
UCLASS()
class UCharacter2D_MaterialFactory : public UFactory
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TObjectPtr<UTexture> InitialTexture;

	UCharacter2D_MaterialFactory();

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

// ================== Скелет ===================
UENUM()
enum class ECharacter2D_RootBoneReference : uint8
{
	Relative,
	Absolute
};

UCLASS()
class UCharacter2D_SkeletonFactory : public UFactory
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TObjectPtr<UStaticMesh> StaticMesh;
	UPROPERTY()
	FVector RootPosition = FVector::ZeroVector;
	UPROPERTY()
	ECharacter2D_RootBoneReference PositionReference = ECharacter2D_RootBoneReference::Relative;

	UCharacter2D_SkeletonFactory();

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InContext, FFeedbackContext* InWarn) override;
};

// ================== SkeletalMesh ===================
UCLASS()
class UCharacter2D_SkeletalMeshFactory : public UFactory
{
	GENERATED_BODY()
public:
	FReferenceSkeleton ReferenceSkeleton;
	UPROPERTY()
	TObjectPtr<USkeleton> Skeleton;
	UPROPERTY()
	TObjectPtr<UStaticMesh> StaticMesh;
	UPROPERTY()
	FName BindBoneName;

	UCharacter2D_SkeletalMeshFactory();

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InContext, FFeedbackContext* InWarn) override;
};
