using UnrealBuildTool;

public class DirectorsCutTarget : TargetRules
{
	public DirectorsCutTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("DirectorsCut");
		// Keep the project/module identity for cooked content and saved data.
		// UBT records this real executable path in the receipt used by staging
		// and the Windows bootstrap; do not rename the leaf after packaging.
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			OutputFile = "Binaries/Win64/Atlantean Odyssey.exe";
		}
	}
}
