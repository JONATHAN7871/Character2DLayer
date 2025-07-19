#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Модуль системы VN персонажей
 * 
 * Основной модуль плагина, отвечающий за:
 * - Инициализацию системы
 * - Регистрацию компонентов и акторов
 * - Настройку логирования
 * - Интеграцию с редактором (в будущем)
 */
class FVNCharacterSystemModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/**
	 * Проверка, загружен ли модуль
	 * @return true если модуль загружен и готов к использованию
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("VNCharacterSystem");
	}

	/**
	 * Получить экземпляр модуля
	 * @return Ссылка на экземпляр модуля
	 */
	static inline FVNCharacterSystemModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FVNCharacterSystemModule>("VNCharacterSystem");
	}

private:
	
	/** Регистрация компонентов в системе рефлексии */
	void RegisterComponents();
	
	/** Отмена регистрации компонентов */
	void UnregisterComponents();
	
	/** Настройка логирования */
	void SetupLogging();
};

// Макрос для логирования специально для нашего модуля
DECLARE_LOG_CATEGORY_EXTERN(LogVNCharacter, Log, All);

// Удобные макросы для новичков
#define VN_LOG(Verbosity, Format, ...) \
UE_LOG(LogVNCharacter, Verbosity, Format, ##__VA_ARGS__)

#define VN_LOG_WARNING(Format, ...) \
VN_LOG(Warning, Format, ##__VA_ARGS__)

#define VN_LOG_ERROR(Format, ...) \
VN_LOG(Error, Format, ##__VA_ARGS__)

#if VN_CHARACTER_SYSTEM_SHIPPING
#define VN_LOG_DEBUG(Format, ...)
#else
#define VN_LOG_DEBUG(Format, ...) \
VN_LOG(Log, Format, ##__VA_ARGS__)
#endif