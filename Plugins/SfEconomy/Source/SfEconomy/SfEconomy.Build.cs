﻿// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SfEconomy : ModuleRules
{
	public SfEconomy(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "NetCore", "GameplayTags", "SfCore" });
	}
}
