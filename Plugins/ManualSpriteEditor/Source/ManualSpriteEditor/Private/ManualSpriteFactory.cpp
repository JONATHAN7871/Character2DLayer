// --- START OF FILE ManualSpriteFactory.cpp ---

#include "ManualSpriteFactory.h"
#include "ManualSprite.h"

UManualSpriteFactory::UManualSpriteFactory()
{
	// Мы создаем новый ассет, поэтому true
	bCreateNew = true;
	// Мы хотим, чтобы редактор открывался после создания ассета
	bEditAfterNew = true;
	// Указываем класс, который эта фабрика будет создавать
	SupportedClass = UManualSprite::StaticClass();
}

UObject* UManualSpriteFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	// Создаем новый объект типа UManualSprite
	UManualSprite* NewSprite = NewObject<UManualSprite>(InParent, InClass, InName, Flags | RF_Transactional);

	// Устанавливаем базовые значения по умолчанию
	NewSprite->bUseManualGeometry = true;
    
	// Вызываем генерацию геометрии по умолчанию (квад).
	// Это важно, чтобы ассет не был пустым при создании.
	FPropertyChangedEvent DummyEvent(nullptr);
	NewSprite->PostEditChangeProperty(DummyEvent);

	return NewSprite;
}

bool UManualSpriteFactory::ShouldShowInNewMenu() const
{
	return true;
}