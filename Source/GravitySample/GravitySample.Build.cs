// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GravitySample : ModuleRules
{
	public GravitySample(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDefinitions.Add("DRAWDEBUGSHAPE=1");

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "NavigationSystem",
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "GravityMovementcomponent",
            "FlyingNavSystem",
            "AIModule",
        });

    }
}
