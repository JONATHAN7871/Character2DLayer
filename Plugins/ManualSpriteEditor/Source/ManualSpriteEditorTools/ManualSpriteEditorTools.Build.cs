using UnrealBuildTool;

public class ManualSpriteEditorTools : ModuleRules
{
	public ManualSpriteEditorTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// EDITOR модуль - зависимость от Runtime модуля
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Paper2D",
			"ManualSpriteEditor" // Зависимость от нашего Runtime модуля
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Projects",
			"InputCore",
			"EditorWidgets",
			"UnrealEd",
			"Slate",
			"SlateCore",
			"AssetTools",
			"PropertyEditor",
			"RenderCore",
			"RHI",
			"AssetRegistry",
			"EditorSubsystem"
		});

		// Добавляем UI модули если доступны
		if (Target.bBuildEditor)
		{
			string[] OptionalModules = {
				"ToolMenus",
				"ToolWidgets",
				"EditorStyle"
			};
			
			foreach (string OptionalModule in OptionalModules)
			{
				try
				{
					PrivateDependencyModuleNames.Add(OptionalModule);
					System.Console.WriteLine("✅ Added module: " + OptionalModule);
				}
				catch
				{
					System.Console.WriteLine("⚠️ Module not available: " + OptionalModule);
				}
			}
		}
		
		// Версионирование
		PublicDefinitions.Add("MANUAL_SPRITE_EDITOR_VERSION_MAJOR=1");
		PublicDefinitions.Add("MANUAL_SPRITE_EDITOR_VERSION_MINOR=4");
		
		// Флаги для отладки
		if (Target.Configuration == UnrealTargetConfiguration.Debug || 
		    Target.Configuration == UnrealTargetConfiguration.DebugGame)
		{
			PublicDefinitions.Add("MANUAL_SPRITE_EDITOR_DEBUG=1");
		}
		else
		{
			PublicDefinitions.Add("MANUAL_SPRITE_EDITOR_DEBUG=0");
		}
		
		System.Console.WriteLine("✅ Manual Sprite Editor Tools v1.4 - Build completed successfully");
	}
}