using UnrealBuildTool;
using System.IO;
public class DeltaPhoneModes : ModuleRules
{
    public DeltaPhoneModes(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] {"Core", "CoreUObject", "Engine", "InputCore", "Slate", "SlateCore", "Renderer", "RenderCore", "RHI", "ApplicationCore", "Launch"});
        PrivateIncludePaths.Add(Path.Combine(EngineDirectory,"Source/Runtime/Renderer/Private"));
        AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "DeltaPhoneModes_APL.xml"));
    }
}
