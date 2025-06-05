#include "Character2DAssetEditorToolkit/Character2DAssetFactory.h"

#include "Character2DAsset.h"

UCharacter2DAssetFactory::UCharacter2DAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UCharacter2DAsset::StaticClass();
}

UObject* UCharacter2DAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags,
													UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UCharacter2DAsset>(InParent, Class, Name, Flags);
}
