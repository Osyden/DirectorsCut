using UnrealBuildTool;

public class DirectorsCut : ModuleRules
{
	public DirectorsCut(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "HTTP", "Json", "JsonUtilities", "UMG", "Slate", "SlateCore", "AudioCaptureCore", "ProceduralMeshComponent", "RHI", "RenderCore"
		});
		PrivateDependencyModuleNames.AddRange(new string[] { "LLMPlugin", "ACERuntime", "ACECore", "HairStrandsCore", "AnimGraphRuntime" });
	}
}
