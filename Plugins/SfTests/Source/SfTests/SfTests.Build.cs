// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SfTests : ModuleRules
{
	public SfTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "SfCore", "CoreUObject", "Engine", "SfCore", "Gauntlet", "NetCore" });
	}
}
