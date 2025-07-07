#include "ManualSpriteEditorToolsModule.h"
#include "ManualSpriteAssetTypeActions.h"
#include "ManualSpriteEditorCommands.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "FManualSpriteEditorToolsModule"

void FManualSpriteEditorToolsModule::StartupModule()
{
	// Регистрируем команды для Undo/Redo и горячих клавиш
	FManualSpriteEditorCommands::Register();

	// Регистрируем типы ассетов
	RegisterAssetTypeActions();

	// Показываем инструкции по использованию после инициализации движка
	FCoreDelegates::OnPostEngineInit.AddLambda([this]()
	{
		ShowSuccessMessage();
	});

	UE_LOG(LogTemp, Warning, TEXT("✅ Manual Sprite Editor Tools module started"));
}

void FManualSpriteEditorToolsModule::ShutdownModule()
{
	// Отменяем регистрацию команд
	FManualSpriteEditorCommands::Unregister();

	// Отменяем регистрацию типов ассетов
	UnregisterAssetTypeActions();

	UE_LOG(LogTemp, Log, TEXT("Manual Sprite Editor Tools module shutdown"));
}

void FManualSpriteEditorToolsModule::RegisterAssetTypeActions()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	// Создаем и регистрируем Asset Type Actions для Manual Sprite
	ManualSpriteAssetTypeActions = MakeShareable(new FManualSpriteAssetTypeActions);
	AssetTools.RegisterAssetTypeActions(ManualSpriteAssetTypeActions.ToSharedRef());

	UE_LOG(LogTemp, Log, TEXT("✅ Registered ManualSprite asset type actions"));
}

void FManualSpriteEditorToolsModule::UnregisterAssetTypeActions()
{
	// Отменяем регистрацию Asset Type Actions
	if (ManualSpriteAssetTypeActions.IsValid())
	{
		if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
		{
			IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
			AssetTools.UnregisterAssetTypeActions(ManualSpriteAssetTypeActions.ToSharedRef());
		}
		ManualSpriteAssetTypeActions.Reset();
	}

	UE_LOG(LogTemp, Log, TEXT("Unregistered ManualSprite asset type actions"));
}

void FManualSpriteEditorToolsModule::ShowSuccessMessage()
{
	// Показываем уведомление об успехе
	FNotificationInfo NotificationInfo(LOCTEXT("Success", 
		"🎨 Manual Sprite Editor Ready!\n"
		"✅ Редактор полностью функционален\n"
		"📖 Создавайте ассеты Manual Sprite из Content Browser"));
	NotificationInfo.ExpireDuration = 5.0f;
	NotificationInfo.bFireAndForget = true;
	NotificationInfo.bUseLargeFont = true;
	FSlateNotificationManager::Get().AddNotification(NotificationInfo);
	
	ShowUsageInstructions();
}

void FManualSpriteEditorToolsModule::ShowUsageInstructions()
{
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("🎨 ===== MANUAL SPRITE EDITOR v1.4 READY! ====="));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("📖 ИСПОЛЬЗОВАНИЕ РЕДАКТОРА:"));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("🎯 ШАГ 1: СОЗДАЙТЕ MANUAL SPRITE"));
	UE_LOG(LogTemp, Warning, TEXT("   • Content Browser → Right Click → Manual Sprite"));
	UE_LOG(LogTemp, Warning, TEXT("   • Назовите ваш ассет Manual Sprite"));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("🔧 ШАГ 2: ОТКРОЙТЕ РЕДАКТОР"));
	UE_LOG(LogTemp, Warning, TEXT("   • Двойной клик по ассету Manual Sprite"));
	UE_LOG(LogTemp, Warning, TEXT("   • Откроется специализированный редактор Manual Sprite"));
	UE_LOG(LogTemp, Warning, TEXT("   • Полнофункциональная среда редактирования"));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("⚙️ ШАГ 3: НАСТРОЙТЕ СПРАЙТ"));
	UE_LOG(LogTemp, Warning, TEXT("   • Панель Details: Установите Source Texture"));
	UE_LOG(LogTemp, Warning, TEXT("   • Включите 'Use Manual Geometry'"));
	UE_LOG(LogTemp, Warning, TEXT("   • Viewport показывает редактируемую геометрию"));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("🎮 ШАГ 4: РЕДАКТИРУЙТЕ ГЕОМЕТРИЮ"));
	UE_LOG(LogTemp, Warning, TEXT("   РЕЖИМЫ ТУЛБАРА:"));
	UE_LOG(LogTemp, Warning, TEXT("   • Select (Q) - Перемещение вершин"));
	UE_LOG(LogTemp, Warning, TEXT("   • Add Vertex (W) - Клик для добавления"));
	UE_LOG(LogTemp, Warning, TEXT("   • Triangle (E) - Выберите 3 вершины"));
	UE_LOG(LogTemp, Warning, TEXT("   • Delete (R) - Удаление элементов"));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("   АВТОТРИАНГУЛЯЦИЯ:"));
	UE_LOG(LogTemp, Warning, TEXT("   • Auto (T) - Умная триангуляция"));
	UE_LOG(LogTemp, Warning, TEXT("   • Fan (F1) - Из центра"));
	UE_LOG(LogTemp, Warning, TEXT("   • Delaunay (F2) - Оптимальная"));
	UE_LOG(LogTemp, Warning, TEXT("   • Convex Hull (F3) - Граница"));
	UE_LOG(LogTemp, Warning, TEXT("   • Ear Clipping (F4) - Сложные формы"));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("   УПРАВЛЕНИЕ РЕДАКТИРОВАНИЕМ:"));
	UE_LOG(LogTemp, Warning, TEXT("   • Ctrl+C/V - Копирование/Вставка вершин"));
	UE_LOG(LogTemp, Warning, TEXT("   • Ctrl+Z/Y - Undo/Redo"));
	UE_LOG(LogTemp, Warning, TEXT("   • Ctrl+A - Выделить все"));
	UE_LOG(LogTemp, Warning, TEXT("   • Delete - Удалить выделенное"));
	UE_LOG(LogTemp, Warning, TEXT("   • G - Переключить сетку"));
	UE_LOG(LogTemp, Warning, TEXT("   • Ctrl+G - Переключить привязку"));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("   НАВИГАЦИЯ ПО VIEWPORT:"));
	UE_LOG(LogTemp, Warning, TEXT("   • Колесо мыши - Зум"));
	UE_LOG(LogTemp, Warning, TEXT("   • Средняя кнопка мыши - Панорамирование"));
	UE_LOG(LogTemp, Warning, TEXT("   • Drag Box - Множественное выделение"));
	UE_LOG(LogTemp, Warning, TEXT("   • Ctrl+Drag - Добавить к выделению"));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("🔧 ВОЗМОЖНОСТИ РЕДАКТОРА:"));
	UE_LOG(LogTemp, Warning, TEXT("   ✅ Специализированный редактор Manual Sprite"));
	UE_LOG(LogTemp, Warning, TEXT("   ✅ Улучшенный тулбар со всеми инструментами"));
	UE_LOG(LogTemp, Warning, TEXT("   ✅ Профессиональный viewport"));
	UE_LOG(LogTemp, Warning, TEXT("   ✅ Превью спрайта в реальном времени"));
	UE_LOG(LogTemp, Warning, TEXT("   ✅ Сетка с привязкой"));
	UE_LOG(LogTemp, Warning, TEXT("   ✅ Полная поддержка Undo/Redo"));
	UE_LOG(LogTemp, Warning, TEXT("   ✅ Операции Copy/Paste"));
	UE_LOG(LogTemp, Warning, TEXT("   ✅ Выделение множественных вершин"));
	UE_LOG(LogTemp, Warning, TEXT("   ✅ 4 алгоритма триангуляции"));
	UE_LOG(LogTemp, Warning, TEXT("   ✅ Отображение статистики вершин"));
	UE_LOG(LogTemp, Warning, TEXT("   ✅ Автоматический расчет UV"));
	UE_LOG(LogTemp, Warning, TEXT("   ✅ Валидация геометрии"));
	UE_LOG(LogTemp, Warning, TEXT("   ✅ ИСПРАВЛЕНА синхронизация спрайта и точек"));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("💡 СОВЕТЫ:"));
	UE_LOG(LogTemp, Warning, TEXT("   • Начните с геометрии по умолчанию (квад)"));
	UE_LOG(LogTemp, Warning, TEXT("   • Используйте Auto Triangulate (T) для быстрых результатов"));
	UE_LOG(LogTemp, Warning, TEXT("   • Включите привязку для точного позиционирования"));
	UE_LOG(LogTemp, Warning, TEXT("   • Сохраняйтесь часто во время сложного редактирования"));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("🔥 ИСПРАВЛЕНИЯ v1.4:"));
	UE_LOG(LogTemp, Warning, TEXT("   ✅ Спрайт и точки теперь полностью синхронизированы"));
	UE_LOG(LogTemp, Warning, TEXT("   ✅ Правильная система координат viewport"));
	UE_LOG(LogTemp, Warning, TEXT("   ✅ Оптимизированный рендеринг через Canvas"));
	UE_LOG(LogTemp, Warning, TEXT("   ✅ Удален лишний код (~1200 строк)"));
	UE_LOG(LogTemp, Warning, TEXT("   ✅ Улучшена производительность"));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("🎉 Ваш Manual Sprite Editor готов к использованию!"));
	UE_LOG(LogTemp, Warning, TEXT(""));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FManualSpriteEditorToolsModule, ManualSpriteEditorTools)