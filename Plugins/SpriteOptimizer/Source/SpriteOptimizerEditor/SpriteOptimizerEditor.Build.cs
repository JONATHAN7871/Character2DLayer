// Copyright 2025, CRAFTCODE, All Rights Reserved.

using UnrealBuildTool;
public class SpriteOptimizerEditor : ModuleRules
{
	public SpriteOptimizerEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		// Use explicit or shared PCH for faster compilation
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// Core engine dependencies required for basic functionality
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"AppFramework",
				"Core",
				"CoreUObject",
				"Engine",
				"Paper2D",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"UnrealEd",
				"WorkspaceMenuStructure"
			}
		);

		// Editor-specific and UI dependencies for advanced functionality
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"ApplicationCore",
				"AssetRegistry",
				"AssetTools",
				"ContentBrowser",
				"ContentBrowserData",
				"DeveloperSettings",
				"EditorScriptingUtilities",
				"EditorStyle",
				"EditorSubsystem",
				"EditorWidgets",
				"ImageCore",
				"ImageWrapper",
				"InputCore",
				"Paper2DEditor",
				"PropertyEditor",
				"RenderCore",
				"RHI",
				"ToolWidgets",
				"UMG"
			}
		);
	}
}