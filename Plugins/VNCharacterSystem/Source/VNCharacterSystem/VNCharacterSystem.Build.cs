using UnrealBuildTool;

public class VnCharacterSystem : ModuleRules
{
	public VnCharacterSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		// Для новичков: включаем дополнительные предупреждения
		bEnableExceptions = false;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// Публичные папки для заголовочных файлов
			}
		);
				
		PrivateIncludePaths.AddRange(
			new string[] {
				// Приватные папки для заголовочных файлов
			}
		);
			
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Paper2D"           // Для PaperSpriteComponent
			}
		);
			
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"RenderCore",        // Для работы с рендерингом
				"RHI"                // Для низкоуровневого рендеринга
			}
		);
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// Модули для динамической загрузки
			}
		);
		
		// Настройки для оптимизации
		if (Target.Configuration == UnrealTargetConfiguration.Shipping)
		{
			PublicDefinitions.Add("VN_CHARACTER_SYSTEM_SHIPPING=1");
		}
		else
		{
			PublicDefinitions.Add("VN_CHARACTER_SYSTEM_SHIPPING=0");
		}
		
		// Поддержка мобильных платформ
		if (Target.Platform == UnrealTargetPlatform.Android || 
		    Target.Platform == UnrealTargetPlatform.IOS)
		{
			PublicDefinitions.Add("VN_CHARACTER_SYSTEM_MOBILE=1");
		}
		else
		{
			PublicDefinitions.Add("VN_CHARACTER_SYSTEM_MOBILE=0");
		}
	}
}