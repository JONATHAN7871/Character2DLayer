using UnrealBuildTool;

public class SpriteOptimizerEditor : ModuleRules
{
	public SpriteOptimizerEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject", 
				"Engine",
				"UnrealEd",
				"Slate",
				"SlateCore",
				"AppFramework", // Добавьте
				"ToolMenus",
				"WorkspaceMenuStructure",
				"Paper2D"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"ApplicationCore",
				"AssetTools",
				"AssetRegistry", 
				"ContentBrowser",
				"ContentBrowserData",
				"DeveloperSettings",
				"EditorSubsystem",
				"EditorStyle", // Для совместимости
				"EditorWidgets",
				"ImageCore",
				"ImageWrapper", 
				"InputCore",
				"Paper2DEditor",
				"PropertyEditor",
				"RenderCore",
				"RHI",
				"ToolWidgets",
				"UMG",
				"EditorScriptingUtilities"
			}
		);
	}
}