// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SfTargeting : ModuleRules
{
	public SfTargeting(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "NetCore", "GameplayTags", "SfCore"});
	}
}