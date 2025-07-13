#include "ManualSpriteEditorToolsModule.h"
#include "ManualSpriteAssetTypeActions.h"
#include "ManualSpriteEditorCommands.h"
#include "ManualSpriteMeshGeneratorOptions.h" // НОВОЕ
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "ISettingsModule.h" // НОВОЕ

#define LOCTEXT_NAMESPACE "FManualSpriteEditorToolsModule"

void FManualSpriteEditorToolsModule::StartupModule()
{
    // Регистрируем команды для Undo/Redo и горячих клавиш
    FManualSpriteEditorCommands::Register();

    // НОВОЕ: Регистрируем настройки в Project Settings
    RegisterProjectSettings();

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
    // НОВОЕ: Отменяем регистрацию настроек
    UnregisterProjectSettings();

    // Отменяем регистрацию команд
    FManualSpriteEditorCommands::Unregister();

    // Отменяем регистрацию типов ассетов
    UnregisterAssetTypeActions();

    UE_LOG(LogTemp, Log, TEXT("Manual Sprite Editor Tools module shutdown"));
}

// НОВЫЙ МЕТОД
void FManualSpriteEditorToolsModule::RegisterProjectSettings()
{
    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
    {
        SettingsModule->RegisterSettings("Project", "Plugins", "ManualSpriteMeshGenerator",
            LOCTEXT("RuntimeSettingsName", "Manual Sprite Mesh Generator"),
            LOCTEXT("RuntimeSettingsDescription", "Settings for Manual Sprite Mesh Generation"),
            GetMutableDefault<UManualSpriteMeshGeneratorOptions>()
        );
    }
}

// НОВЫЙ МЕТОД
void FManualSpriteEditorToolsModule::UnregisterProjectSettings()
{
    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
    {
        SettingsModule->UnregisterSettings("Project", "Plugins", "ManualSpriteMeshGenerator");
    }
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
        "📖 Создавайте ассеты Manual Sprite из Content Browser\n"
        "🔧 Генерируйте мешы через кнопку Generate Mesh"));
    NotificationInfo.ExpireDuration = 5.0f;
    NotificationInfo.bFireAndForget = true;
    NotificationInfo.bUseLargeFont = true;
    FSlateNotificationManager::Get().AddNotification(NotificationInfo);
    
    ShowUsageInstructions();
}

void FManualSpriteEditorToolsModule::ShowUsageInstructions()
{
    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("🎨 ===== MANUAL SPRITE EDITOR v3.0 READY! ====="));
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
    UE_LOG(LogTemp, Warning, TEXT("🔧 ШАГ 5: ГЕНЕРИРУЙТЕ МЕШ (НОВОЕ!)"));
    UE_LOG(LogTemp, Warning, TEXT("   • Generate Mesh (Ctrl+M) - Создание StaticMesh/SkeletalMesh"));
    UE_LOG(LogTemp, Warning, TEXT("   • Настройка пивота, масштаба, смещения"));
    UE_LOG(LogTemp, Warning, TEXT("   • Автоматическое создание материалов"));
    UE_LOG(LogTemp, Warning, TEXT("   • Project Settings → Manual Sprite Mesh Generator для глобальных настроек"));
    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("   УПРАВЛЕНИЕ РЕДАКТИРОВАНИЕМ:"));
    UE_LOG(LogTemp, Warning, TEXT("   • Ctrl+C/V - Копирование/Вставка вершин"));
    UE_LOG(LogTemp, Warning, TEXT("   • Ctrl+Z/Y - Undo/Redo"));
    UE_LOG(LogTemp, Warning, TEXT("   • Ctrl+A - Выделить все"));
    UE_LOG(LogTemp, Warning, TEXT("   • Delete - Удалить выделенное"));
    UE_LOG(LogTemp, Warning, TEXT("   • G - Переключить сетку"));
    UE_LOG(LogTemp, Warning, TEXT("   • Ctrl+G - Переключить привязку"));
    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("🔥 НОВЫЕ ВОЗМОЖНОСТИ v3.0:"));
    UE_LOG(LogTemp, Warning, TEXT("   ✅ Генерация StaticMesh и SkeletalMesh"));
    UE_LOG(LogTemp, Warning, TEXT("   ✅ Настройка пивота (Origin, Center, Bottom, Custom)"));
    UE_LOG(LogTemp, Warning, TEXT("   ✅ Произвольный масштаб и смещение мешей"));
    UE_LOG(LogTemp, Warning, TEXT("   ✅ Автоматическое создание материалов"));
    UE_LOG(LogTemp, Warning, TEXT("   ✅ Unlit и Lit материалы, двусторонние материалы"));
    UE_LOG(LogTemp, Warning, TEXT("   ✅ Интеграция с Project Settings"));
    UE_LOG(LogTemp, Warning, TEXT("   ✅ Горячие клавиши для быстрого доступа"));
    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("🎉 Ваш Manual Sprite Editor готов к использованию!"));
    UE_LOG(LogTemp, Warning, TEXT(""));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FManualSpriteEditorToolsModule, ManualSpriteEditorTools)