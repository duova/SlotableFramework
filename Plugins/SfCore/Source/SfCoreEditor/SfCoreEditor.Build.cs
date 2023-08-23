using UnrealBuildTool;

public class SfCoreEditor : ModuleRules
{
    public SfCoreEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "BlueprintGraph",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "SfCore",
                "InputCore",
                "AssetTools",
                "UnrealEd", // for FAssetEditorManager
                "KismetWidgets",
                "KismetCompiler",
                "BlueprintGraph",
                "GraphEditor",
                "Kismet",  // for FWorkflowCentricApplication
                "PropertyEditor",
                "EditorStyle",
                "Sequencer",
                "DetailCustomizations",
                "Settings",
                "RenderCore"
            }
        );
    }
}