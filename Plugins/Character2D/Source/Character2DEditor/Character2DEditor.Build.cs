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
                "RHI"
            });
        
        
        PrivateDependencyModuleNames.AddRange(new string[] {"MeshUtilitiesCommon"
        });
    }
}