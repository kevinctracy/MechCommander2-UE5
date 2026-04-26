using UnrealBuildTool;
using System.Collections.Generic;

public class MechCommander2Target : TargetRules
{
    public MechCommander2Target(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("MechCommander2");
    }
}
