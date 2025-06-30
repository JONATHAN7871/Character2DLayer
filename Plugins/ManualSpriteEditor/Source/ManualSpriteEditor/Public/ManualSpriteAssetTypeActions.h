#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "ManualSprite.h"

class MANUALSPRITEEDITOR_API FManualSpriteAssetTypeActions : public FAssetTypeActions_Base
{
public:
	// Имя типа ассета
	virtual FText GetName() const override;
	
	// Цвет в Content Browser
	virtual FColor GetTypeColor() const override;
	
	// Класс, который поддерживает этот action
	virtual UClass* GetSupportedClass() const override;
	
	// Категория в Content Browser
	virtual uint32 GetCategories() override;
	
	// Открытие редактора для ассета
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	
	// Может ли импортировать этот тип
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override;
	
	// Контекстное меню в Content Browser
	virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;
	
	// Создание нового ассета
	UObject* CreateAssetFromFile(const FString& InPathName, UObject* InParent, UClass* InClass, const FString& InFilename);

private:
	// Функция для конвертации обычного спрайта в ручной
	void ConvertToManualSprite(TArray<TWeakObjectPtr<UManualSprite>> Objects);
	
	// Функция для сброса геометрии к автоматической  
	void ResetToAutoGeometry(TArray<TWeakObjectPtr<UManualSprite>> Objects);

	// ИСПРАВЛЕНИЕ: Правильное объявление функции
	void CreateManualSpriteFromTexture(UTexture2D* Texture);
};