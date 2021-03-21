// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class BUIProjectTemplateEditorTarget : TargetRules
{
	public BUIProjectTemplateEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        bUsePDBFiles = true;
		ExtraModuleNames.AddRange( new string[] { "BUIProjectTemplate", "BUIProjectTemplateEditor" } );
	}
}
