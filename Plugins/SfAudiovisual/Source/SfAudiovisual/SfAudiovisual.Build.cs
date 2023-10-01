// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SfAudiovisual : ModuleRules
{
	public SfAudiovisual(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "NetCore", "GameplayTags", "SfCore", "Niagara" });
	}
}
