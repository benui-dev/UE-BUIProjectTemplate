// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class BUIProjectTemplateTarget : TargetRules
{
	public BUIProjectTemplateTarget( TargetInfo Target ) : base( Target )
	{
		Type = TargetType.Game;
		bUseLoggingInShipping = false;
		bUseIncrementalLinking = true;
		// bUseMallocProfiler = true;
		bUseUnityBuild = false;
		bUsePCHFiles = false;
		bEnforceIWYU = true;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange( new string[] { "BUIProjectTemplate" } );

#if WITH_EDITOR
		ExtraModuleNames.AddRange( new string[] { "BUIProjectTemplateEditor" } );
#endif
	}
}
