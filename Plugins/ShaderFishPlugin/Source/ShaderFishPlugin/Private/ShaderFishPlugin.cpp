// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ShaderFishPlugin.h"
#include "CoreUObject.h"
#include "Engine.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"

#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

#include <iostream>

class FShaderFishPluginModule : public IShaderFishPluginModule
{
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

void FShaderFishPluginModule::StartupModule()
{
    FString PluginShaderDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("ShaderFishPlugin"))->GetBaseDir(), TEXT("Shaders")));

    std::cout << "PLUGIN DIR " << TCHAR_TO_UTF8(*PluginShaderDir) <<std::endl;
    AddShaderSourceDirectoryMapping(TEXT("/Plugin/ShaderFishPlugin"), PluginShaderDir);
}

void FShaderFishPluginModule::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
    // we call this function before unloading the module.
}
IMPLEMENT_MODULE(FShaderFishPluginModule, ShaderFishPlugin)
//DEFINE_LOG_CATEGORY(LogShaderFish);

// ###################################################################################################################3
// [FFishShader] ==
//
// Ref: https://www.unrealengine.com/en-US/tech-blog/how-to-add-global-shaders-to-ue4
FFishShader::FFishShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
	: FGlobalShader(Initializer)
{
    m_shaderResource.Bind(Initializer.ParameterMap, CSHADER_PARAMETER_NAME, SPF_Mandatory);
}

void FFishShader::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
    FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
}

void FFishShader::SetShaderBuffer(FRHICommandList& commandList, const FRWBufferStructured& buffer)
{
    m_shaderRWParameter.SetBuffer(commandList, GetComputeShader(), buffer);
}

void FFishShader::SetShaderUAVParameter(FRHICommandList& commandList, FUnorderedAccessViewRHIRef uav)
{
    SetUAVParameter(commandList, GetComputeShader(), m_shaderResource, uav);
}

void FFishShader::SetShaderSRVParameter(FRHICommandList& commandList, FShaderResourceViewRHIRef srv)
{
    SetSRVParameter(commandList, GetComputeShader(), m_shaderResource, srv);
}

void FFishShader::SetUniformBufferParameters(FRHICommandList& commandList, const FFishShader::FParameters& parameters)
{
    SetLocalUniformBufferParameter(commandList, GetComputeShader(), GetUniformBufferParameter(FFishShader::FParameters::FTypeInfo::GetStructMetadata()),
        FFishShader::FParametersRef::CreateLocalUniformBuffer(commandList, parameters, UniformBuffer_SingleDraw));
}

void FFishShader::cleanupShaderData(FRHICommandList& commandList)
{
    SetUAVParameter(commandList, GetComputeShader(), m_shaderResource, FUnorderedAccessViewRHIRef());
    m_shaderRWParameter.UnsetUAV(commandList, GetComputeShader());
}

// Ref: https://forums.unrealengine.com/development-discussion/c-gameplay-programming/29352-tutorial-pixel-and-compute-shaders-in-ue4?58489-Tutorial-Pixel-and-Compute-Shaders-in-UE4=
// The shader compilation: When you have placed your shader code in the correct folder, you need to compile it.
// This is done using a macro in your Cpp source called IMPLEMENT_SHADER_TYPE. One problem with this though is
// that if you simply place this and your other code in your main project, you are going to have problems since
// this macro must be available and run during an early phase of engine start up, and in addition to this cannot
// be rerun if you decide to recompile your code in-editor. Ignoring this will actually crash the engine.
// The solution is to put the shader declaration and initialization into a plugin and override the load phase of
// the plugin in question to "PostConfigInit".
IMPLEMENT_SHADER_TYPE(, FFishShader, TEXT("/Plugin/ShaderFishPlugin/ComputeFishShader.usf"),
                                     TEXT("MainComputeShader"), SF_Compute)
