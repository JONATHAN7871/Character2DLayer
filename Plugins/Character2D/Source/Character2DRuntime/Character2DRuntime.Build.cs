using UnrealBuildTool;

public class Character2DRuntime : ModuleRules
{
	public Character2DRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// Только рантайм-модули
		PublicDependencyModuleNames.AddRange(
			new[] { "Core", "CoreUObject", "Engine", "Paper2D" });

		PrivateDependencyModuleNames.AddRange(
			new[] { "SlateCore" });
		
	}
}