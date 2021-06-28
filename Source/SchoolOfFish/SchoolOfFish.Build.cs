// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class SchoolOfFish : ModuleRules
{
	public SchoolOfFish(ReadOnlyTargetRules Target) : base(Target)
	{
        bEnableExceptions = true;
        CppStandard = CppStandardVersion.Cpp17;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Projects", "RHI", "IntelISPC", "ShaderFishPlugin" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Projects", "RHI", "IntelISPC", "ShaderFishPlugin" });
		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}

