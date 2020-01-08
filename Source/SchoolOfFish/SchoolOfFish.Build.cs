// Fill out your copyright notice in the Description page of Project Settings.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Net.NetworkInformation;
using System.Text;
using System.Text.RegularExpressions;
using UnrealBuildTool;

public class SchoolOfFish : ModuleRules
{
	private UnrealTargetPlatform Platform;
    private UnrealTargetConfiguration Configuration;

    // name of root folders in the project folder
    private static readonly string GRPC_STRIPPED_FOLDER = "GrpcIncludes";
    private static readonly string GRPC_LIBS_FOLDER = "GrpcLibraries";

    private string GRPC_INCLUDE_ROOT;
    private string GRPC_LIB_ROOT;

	public class GRPCModuleDepPaths
    {
        public readonly string[] HeaderPaths;
        public readonly string[] LibraryPaths;

        public GRPCModuleDepPaths(string[] headerPaths, string[] libraryPaths)
        {
            HeaderPaths = headerPaths;
            LibraryPaths = libraryPaths;
        }

        public override string ToString()
        {
            return "Headers:\n" + string.Join("\n", HeaderPaths) + "\nLibs:\n" + string.Join("\n", LibraryPaths);
        }
    }

    [Conditional("DEBUG")]
    [Conditional("TRACE")]
    private void clog(params object[] objects)
    {
        Console.WriteLine(string.Join(", ", objects));
    }

	    private IEnumerable<string> FindFilesInDirectory(string dir, string suffix = "")
    {
        List<string> matches = new List<string>();
        if (Directory.Exists(dir))
        {
            string[] files = Directory.GetFiles(dir);
            Regex regex = new Regex(".+\\." + suffix);

            foreach (string file in files)
            {
                if (regex.Match(file).Success)
                    matches.Add(file);
            }
        }

        return matches;
    }

    private string GetConfigurationString()
    {
        return (Configuration == UnrealTargetConfiguration.Shipping) ? "Release" : "Debug";
    }

    public GRPCModuleDepPaths GatherDeps()
    {
        string RootPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../ThirdParty/grpc/"));

        GRPC_INCLUDE_ROOT = Path.Combine(RootPath, GRPC_STRIPPED_FOLDER);
        GRPC_LIB_ROOT = Path.Combine(RootPath, GRPC_LIBS_FOLDER);


        List<string> headers = new List<string>();
        List<string> libs = new List<string>();

        string PlatformLibRoot = "";

        PlatformLibRoot = Path.Combine(GRPC_LIB_ROOT, Platform.ToString());
        if (Platform == UnrealTargetPlatform.Win64)
        {
            libs.AddRange(FindFilesInDirectory(PlatformLibRoot, "lib"));
        }
        else
        {
            libs.AddRange(FindFilesInDirectory(PlatformLibRoot, "a"));
        }

        clog("PlatformLibRoot: " + PlatformLibRoot);

        headers.Add(Path.Combine(GRPC_INCLUDE_ROOT, "include"));
        headers.Add(Path.Combine(GRPC_INCLUDE_ROOT, "third_party", "protobuf", "src"));

        return new GRPCModuleDepPaths(headers.ToArray(), libs.ToArray());

    }

	public SchoolOfFish(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// GRPC --
		//
	    PublicDefinitions.Add("GOOGLE_PROTOBUF_NO_RTTI");
        PublicDefinitions.Add("GOOGLE_PROTOBUF_USE_UNALIGNED=0");
        PublicDefinitions.Add("GPR_FORBID_UNREACHABLE_CODE");
        PublicDefinitions.Add("GRPC_ALLOW_EXCEPTIONS=0");

        //TODO: We do this because in file generated_message_table_driven.h that located in protobuf sources
        //TODO: line 174: static_assert(std::is_pod<AuxillaryParseTableField>::value, "");
        //TODO: causes С4647 level 3 warning __is_pod behavior change
        //TODO: UE4 threading some warnings as errors, and we have no chance to suppress this stuff
        //TODO: So, we don't want to change any third-party code, this why we add this definition
        PublicDefinitions.Add("__NVCC__");

		Platform = Target.Platform;
        Configuration = Target.Configuration;

        GRPCModuleDepPaths moduleDepPaths = GatherDeps();
        Console.WriteLine(moduleDepPaths.ToString());

        PublicIncludePaths.AddRange(moduleDepPaths.HeaderPaths);
        PublicAdditionalLibraries.AddRange(moduleDepPaths.LibraryPaths);

		//AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL");
        AddEngineThirdPartyPrivateStaticDependencies(Target, "zlib");

		// ----------------------------------------------------------------------------------------------------------------------------
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "RHI", "ShaderFishPlugin", "InfraworldRuntime" });

		PrivateDependencyModuleNames.AddRange(new string[] {  "InfraworldRuntime" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
