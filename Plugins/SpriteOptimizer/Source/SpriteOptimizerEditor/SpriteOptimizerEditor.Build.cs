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
				"EditorStyle",
				"EditorWidgets",
				"ToolMenus",
				"WorkspaceMenuStructure"
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
				"DesktopPlatform",
				"DeveloperSettings",
				"EditorSubsystem",
				"ImageCore",
				"ImageWrapper",
				"InputCore",
				"Paper2D",
				"Paper2DEditor",
				"PropertyEditor",
				"RenderCore",
				"RHI",
				"SkeletalMeshDescription",
				"SkeletalMeshEditor",
				"SkeletalMeshUtilitiesCommon",
				"StaticMeshDescription",
				"MeshDescription",
				"MeshConversion",
				"MeshUtilitiesCommon",
				"ToolWidgets",
				"UMG",
				"EditorScriptingUtilities"
			}
		);
	}
}