#include "VNCharacterSystemModule.h"
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// Определяем категорию логирования
DEFINE_LOG_CATEGORY(LogVNCharacter);

#define LOCTEXT_NAMESPACE "FVNCharacterSystemModule"

void FVNCharacterSystemModule::StartupModule()
{
	// Логируем старт модуля
	VN_LOG(Log, TEXT("VN Character System module starting up..."));
	
	// Настраиваем логирование
	SetupLogging();
	
	// Регистрируем компоненты
	RegisterComponents();
	
	VN_LOG(Log, TEXT("VN Character System module started successfully!"));
}

void FVNCharacterSystemModule::ShutdownModule()
{
	VN_LOG(Log, TEXT("VN Character System module shutting down..."));
	
	// Отменяем регистрацию компонентов
	UnregisterComponents();
	
	VN_LOG(Log, TEXT("VN Character System module shut down successfully."));
}

void FVNCharacterSystemModule::RegisterComponents()
{
	// В будущем здесь будет регистрация кастомных компонентов для редактора
	// Например, кастомные property editors, visualizers и т.д.
	
	VN_LOG_DEBUG(TEXT("Components registered"));
}

void FVNCharacterSystemModule::UnregisterComponents()
{
	// Отменяем регистрацию компонентов
	
	VN_LOG_DEBUG(TEXT("Components unregistered"));
}

void FVNCharacterSystemModule::SetupLogging()
{
	// Настройка уровней логирования в зависимости от сборки
#if VN_CHARACTER_SYSTEM_SHIPPING
	// В shipping сборке показываем только критические ошибки
	LogVNCharacter.SetVerbosity(ELogVerbosity::Error);
#elif UE_BUILD_DEBUG
	// В debug сборке показываем все
	LogVNCharacter.SetVerbosity(ELogVerbosity::VeryVerbose);
#else
	// В остальных сборках показываем до Warning включительно
	LogVNCharacter.SetVerbosity(ELogVerbosity::Warning);
#endif

	VN_LOG_DEBUG(TEXT("Logging setup complete"));
}

#undef LOCTEXT_NAMESPACE
	
// Регистрируем модуль
IMPLEMENT_MODULE(FVNCharacterSystemModule, VnCharacterSystem)