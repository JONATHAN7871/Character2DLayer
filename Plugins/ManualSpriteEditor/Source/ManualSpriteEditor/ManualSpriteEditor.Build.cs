using UnrealBuildTool;

public class ManualSpriteEditor : ModuleRules
{
	public ManualSpriteEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange([]);
				
		PrivateIncludePaths.AddRange([]);
			
		PublicDependencyModuleNames.AddRange([
			"Core",
			"CoreUObject",
			"Engine",
			"Paper2D"
		]);
			
		PrivateDependencyModuleNames.AddRange([
			"Projects",
			"InputCore",
			"EditorWidgets",
			"UnrealEd",
			"ToolMenus",
			"Slate",
			"SlateCore",
			"EditorStyle",
			"AssetTools",
			"Paper2DEditor",
			"PropertyEditor",
			"RenderCore",
			"RHI"
		]);
		
		DynamicallyLoadedModuleNames.AddRange([]);
	}
}