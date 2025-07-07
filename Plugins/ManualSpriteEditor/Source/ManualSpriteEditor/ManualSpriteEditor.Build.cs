using UnrealBuildTool;

public class ManualSpriteEditor : ModuleRules
{
	public ManualSpriteEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// RUNTIME модуль - только основные зависимости
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Paper2D"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Projects",
			"RenderCore",
			"RHI"
		});

		// Никаких Editor зависимостей в Runtime модуле!
		// Все Editor functionality будет в отдельном модуле ManualSpriteEditorTools
	}
}