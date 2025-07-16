using UnrealBuildTool;

public class Character2DEditor : ModuleRules
{
	public Character2DEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",    
				"UMG",
				"WorkspaceMenuStructure",
				"EditorStyle",
				"EditorWidgets", 
				"AdvancedPreviewScene", 
				"Character2DRuntime", 
				"InputCore", 
				"RenderCore", 
				"SkeletalMeshDescription", 
				"AssetTools",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"SkeletalMeshEditor",
				"SkeletalMeshUtilitiesCommon",
				"AnimGraphRuntime",
				"AssetRegistry",
				"UnrealEd",
				"PropertyEditor",
				"ToolWidgets",
				"EditorFramework",
				"ContentBrowser",
				"ToolMenus",
				"MeshConversion",
				"StaticMeshDescription",
				"MeshDescription",
				"Paper2D",
				"RHI",
				"ImageWrapper",          // Для работы с изображениями
				"ImageCore",             // Для работы с пикселями
				"DeveloperSettings"      // Для настроек
			});
        
		PrivateDependencyModuleNames.AddRange(new string[] {
			"MeshUtilitiesCommon",
			"DesktopPlatform",          // Для диалогов файлов
			"EditorSubsystem",          // Для подсистем редактора
			"ApplicationCore",          // Для Application функций
			"Paper2DEditor"             // ДОБАВЛЕНО: для FSpriteAssetInitParameters
		});
	}
}