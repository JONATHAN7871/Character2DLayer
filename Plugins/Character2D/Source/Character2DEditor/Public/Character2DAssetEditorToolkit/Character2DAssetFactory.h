#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "Character2DAssetFactory.generated.h"

UCLASS()
class CHARACTER2DEDITOR_API UCharacter2DAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UCharacter2DAssetFactory();

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags,
									  UObject* Context, FFeedbackContext* Warn) override;

	virtual bool ShouldShowInNewMenu() const override { return true; }
};
