#include "ManualSpriteAssetTypeActions.h"
#include "ManualSpriteEditorToolkit.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/MessageDialog.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Engine/Texture2D.h"
#include "Misc/PackageName.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"

#define LOCTEXT_NAMESPACE "ManualSpriteAssetTypeActions"

FText FManualSpriteAssetTypeActions::GetName() const
{
	return LOCTEXT("ManualSpriteAssetTypeActionsName", "Manual Sprite");
}

FColor FManualSpriteAssetTypeActions::GetTypeColor() const
{
	return FColor(129, 196, 115); // Зелёный цвет для отличия от обычных спрайтов
}

UClass* FManualSpriteAssetTypeActions::GetSupportedClass() const
{
	return UManualSprite::StaticClass();
}

uint32 FManualSpriteAssetTypeActions::GetCategories()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	return AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Paper2D")), LOCTEXT("Paper2DAssetCategory", "Paper2D"));
}

void FManualSpriteAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (auto* ManualSprite = Cast<UManualSprite>(*ObjIt))
		{
			const TSharedRef<FManualSpriteEditorToolkit> EditorToolkit = MakeShareable(new FManualSpriteEditorToolkit());
			EditorToolkit->InitManualSpriteEditor(Mode, EditWithinLevelEditor, ManualSprite);
		}
	}
}

bool FManualSpriteAssetTypeActions::HasActions(const TArray<UObject*>& InObjects) const
{
	return true;
}

void FManualSpriteAssetTypeActions::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	auto ManualSprites = GetTypedWeakObjectPtrs<UManualSprite>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ManualSprite_ResetGeometry", "Reset to Auto Geometry"),
		LOCTEXT("ManualSprite_ResetGeometryTooltip", "Disable manual geometry and use automatic triangulation"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([this, ManualSprites]()
			{
				ResetToAutoGeometry(ManualSprites);
			}),
			FCanExecuteAction()
		)
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ManualSprite_EnableManualMode", "Enable Manual Mode"),
		LOCTEXT("ManualSprite_EnableManualModeTooltip", "Enable manual geometry editing mode"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([ManualSprites]()
			{
				for (auto& WeakSprite : ManualSprites)
				{
					if (UManualSprite* Sprite = WeakSprite.Get())
					{
						Sprite->bUseManualGeometry = true;
						if (Sprite->ManualGeometry.Vertices.Num() == 0)
						{
							// Генерируем базовую геометрию если её нет
							FPropertyChangedEvent DummyEvent(nullptr);
							Sprite->PostEditChangeProperty(DummyEvent);
						}
						Sprite->MarkPackageDirty();
					}
				}
			}),
			FCanExecuteAction()
		)
	);
}

UObject* FManualSpriteAssetTypeActions::CreateAssetFromFile(const FString& InPathName, UObject* InParent, UClass* InClass, const FString& InFilename)
{
	// Создание нового Manual Sprite ассета
	UManualSprite* NewSprite = NewObject<UManualSprite>(InParent, InClass, FName(*InPathName), RF_Public | RF_Standalone);
	
	// Здесь можно добавить логику инициализации из файла, если нужно
	
	return NewSprite;
}

void FManualSpriteAssetTypeActions::ConvertToManualSprite(TArray<TWeakObjectPtr<UManualSprite>> Objects)
{
	// Логика конвертации обычного спрайта в ручной
	// Пока что оставляем пустой - можно добавить позже
}

void FManualSpriteAssetTypeActions::ResetToAutoGeometry(TArray<TWeakObjectPtr<UManualSprite>> Objects)
{
	const FText ConfirmText = LOCTEXT("ManualSprite_ResetGeometryConfirm", 
		"This will disable manual geometry and revert to automatic triangulation. Are you sure?");

	if (FMessageDialog::Open(EAppMsgType::YesNo, ConfirmText) == EAppReturnType::Yes)
	{
		for (auto& WeakObject : Objects)
		{
			if (UManualSprite* ManualSprite = WeakObject.Get())
			{
				ManualSprite->bUseManualGeometry = false;
				ManualSprite->MarkPackageDirty();
			}
		}
	}
}

// ИСПРАВЛЕННАЯ функция для создания Manual Sprite из текстуры
void FManualSpriteAssetTypeActions::CreateManualSpriteFromTexture(UTexture2D* Texture)
{
	if (!Texture)
		return;
		
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	
	// Получаем путь к текстуре
	FString TexturePath = Texture->GetPathName();
	FString PackagePath = FPackageName::GetLongPackagePath(TexturePath);
	FString TextureName = Texture->GetName();
	
	// Создаем имя для нового Manual Sprite
	FString SpriteName = FString::Printf(TEXT("%s_ManualSprite"), *TextureName);
	FString PackageName = FString::Printf(TEXT("%s/%s"), *PackagePath, *SpriteName);
	
	// Создаем уникальное имя если такой ассет уже существует
	FString FinalPackageName;
	FString FinalAssetName;
	AssetTools.CreateUniqueAssetName(PackageName, TEXT(""), FinalPackageName, FinalAssetName);
	
	// Создаем новый Manual Sprite
	UPackage* Package = CreatePackage(*FinalPackageName);
	UManualSprite* NewSprite = NewObject<UManualSprite>(Package, *FinalAssetName, RF_Public | RF_Standalone | RF_Transactional);
	
	if (NewSprite)
	{
		// ИСПРАВЛЕНИЕ: Используем правильный способ установки текстуры
		// В UPaperSprite используется SetSourceTexture, но проверим через reflection
		if (UFunction* SetTextureFunc = NewSprite->FindFunction(FName("SetSourceTexture")))
		{
			// Если функция существует, вызываем её
			struct FSetSourceTextureParams
			{
				UTexture2D* InTexture;
			} Params;
			Params.InTexture = Texture;
			
			NewSprite->ProcessEvent(SetTextureFunc, &Params);
		}
		else
		{
			// Альтернативный способ - прямое присвоение через reflection
			if (FProperty* SourceTextureProperty = NewSprite->GetClass()->FindPropertyByName(FName("SourceTexture")))
			{
				SourceTextureProperty->SetValue_InContainer(NewSprite, Texture);
			}
		}
		
		// Включаем ручную геометрию и генерируем базовую
		NewSprite->bUseManualGeometry = true;
		FPropertyChangedEvent DummyEvent(nullptr);
		NewSprite->PostEditChangeProperty(DummyEvent);
		
		// Помечаем пакет как измененный
		Package->MarkPackageDirty();
		
		// Регистрируем ассет в Asset Registry
		FAssetRegistryModule::AssetCreated(NewSprite);
		
		// Логируем успешное создание
		UE_LOG(LogTemp, Log, TEXT("Created Manual Sprite '%s' from texture '%s'"), *FinalAssetName, *TextureName);
	}
}

#undef LOCTEXT_NAMESPACE