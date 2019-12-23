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
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FConstantParameters, "constants");
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FVariableParameters, "variables");

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
FFishShader::FFishShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
	: FGlobalShader(Initializer)
{
	m_shaderResource.Bind(Initializer.ParameterMap, TEXT("data"));
}

void FFishShader::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
    FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
}

void FFishShader::setShaderData(FRHICommandList& commandList, FUnorderedAccessViewRHIRef uav)
{
	if (m_shaderResource.IsBound())
		commandList.SetUAVParameter(GetComputeShader(), m_shaderResource.GetBaseIndex(), uav);
}

void FFishShader::setUniformBuffers(FRHICommandList& commandList, FConstantParameters& constants, FVariableParameters& variables)
{
	SetUniformBufferParameter(commandList, GetComputeShader(), GetUniformBufferParameter<FConstantParameters>(),
		FConstantParametersRef::CreateUniformBufferImmediate(constants, UniformBuffer_SingleDraw));
	SetUniformBufferParameter(commandList, GetComputeShader(), GetUniformBufferParameter<FVariableParameters>(),
		FVariableParametersRef::CreateUniformBufferImmediate(variables, UniformBuffer_SingleDraw));
}

void FFishShader::cleanupShaderData(FRHICommandList& commandList)
{
	if (m_shaderResource.IsBound())
		commandList.SetUAVParameter(GetComputeShader(), m_shaderResource.GetBaseIndex(), FUnorderedAccessViewRHIRef());
}

IMPLEMENT_SHADER_TYPE(, FFishShader, TEXT("/Plugin/ShaderFishPlugin/ComputeFishShader.usf"),       TEXT("VS_test"),       SF_Compute)
