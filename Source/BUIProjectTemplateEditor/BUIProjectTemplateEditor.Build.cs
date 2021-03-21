using UnrealBuildTool;

public class BUIProjectTemplateEditor : ModuleRules
{
	public BUIProjectTemplateEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"AssetRegistry",
				"AssetTools",
				"MainFrame",
				"BlueprintGraph",
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"UnrealEd",
                "BlueprintGraph",
                "UMG",
                "Json",
                "JsonUtilities",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"RenderCore",
				"RHI",
				"SourceControl",
				"PropertyEditor",
                "GameplayAbilities",
                "GameplayAbilitiesEditor",
                "GameplayTags",
				"GameplayTagsEditor",
                "RawMesh",
                "SlateCore",
                "StaticMeshEditor",
                "WorldBrowser",
				"BUIProjectTemplate",
                "EditorStyle",
                "DesktopPlatform",
            }
        );

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"AssetRegistry",
				"AssetTools",
				"MainFrame",
			}
		);
	}
}
