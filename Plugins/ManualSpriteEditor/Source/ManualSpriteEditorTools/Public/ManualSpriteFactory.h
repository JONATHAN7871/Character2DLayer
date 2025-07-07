// --- START OF FILE ManualSpriteFactory.h ---

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "ManualSpriteFactory.generated.h"

UCLASS()
class MANUALSPRITEEDITORTOOLS_API UManualSpriteFactory : public UFactory
{
	GENERATED_BODY()

public:
	UManualSpriteFactory();

	// UFactory interface
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override;
};