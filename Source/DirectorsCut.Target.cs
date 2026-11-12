using UnrealBuildTool;

public class DirectorsCutTarget : TargetRules
{
	public DirectorsCutTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("DirectorsCut");
	}
}
