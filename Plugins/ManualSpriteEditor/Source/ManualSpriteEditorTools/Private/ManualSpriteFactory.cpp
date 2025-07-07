// --- START OF FILE ManualSpriteFactory.cpp ---

#include "ManualSpriteFactory.h"
#include "ManualSprite.h"

UManualSpriteFactory::UManualSpriteFactory()
{
	// We create new asset, so true
	bCreateNew = true;
	// We want editor to open after asset creation
	bEditAfterNew = true;
	// Specify class that this factory will create
	SupportedClass = UManualSprite::StaticClass();
}

UObject* UManualSpriteFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	// Create new UManualSprite object
	UManualSprite* NewSprite = NewObject<UManualSprite>(InParent, InClass, InName, Flags | RF_Transactional);

	// Set basic default values
	NewSprite->bUseManualGeometry = true;
    
	// Call default geometry generation (quad).
	// This is important so asset is not empty when created.
	FPropertyChangedEvent DummyEvent(nullptr);
	NewSprite->PostEditChangeProperty(DummyEvent);

	return NewSprite;
}

bool UManualSpriteFactory::ShouldShowInNewMenu() const
{
	return true;
}