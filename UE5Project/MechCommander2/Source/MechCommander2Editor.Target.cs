using UnrealBuildTool;
using System.Collections.Generic;

public class MechCommander2EditorTarget : TargetRules
{
    public MechCommander2EditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("MechCommander2");
    }
}
