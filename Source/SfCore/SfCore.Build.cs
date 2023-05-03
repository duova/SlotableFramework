// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SfCore : ModuleRules
{
	public SfCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "NetCore" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });
	}
}
