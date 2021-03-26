// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class BUIProjectTemplate : ModuleRules
{
    private string PluginsPath
    {
        get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../../Plugins/")); }
    }

    protected void AddSPUD()
    {
        // Linker
        PrivateDependencyModuleNames.AddRange(new string[] { "SPUD" });
        // Headers
        PublicIncludePaths.Add(Path.Combine(PluginsPath, "SPUD", "Source", "SPUD", "Public"));
    }

    public BUIProjectTemplate(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

        PrivateDependencyModuleNames.AddRange(new string[] { "ImGui" });

        AddSPUD();

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
