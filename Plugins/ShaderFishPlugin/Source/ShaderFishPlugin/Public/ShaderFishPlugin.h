// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GlobalShader.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FConstantParameters, )
SHADER_PARAMETER(int, fishCount)
SHADER_PARAMETER(float, radiusCohesion)
SHADER_PARAMETER(float, radiusSeparation)
SHADER_PARAMETER(float, radiusAlignment)
SHADER_PARAMETER(float, mapRangeX)
SHADER_PARAMETER(float, mapRangeY)
SHADER_PARAMETER(float, mapRangeZ)
SHADER_PARAMETER(float, kCohesion)
SHADER_PARAMETER(float, kSeparation)
SHADER_PARAMETER(float, kAlignment)
SHADER_PARAMETER(float, maxAcceleration)
SHADER_PARAMETER(float, maxVelocity)
SHADER_PARAMETER(int, calculationsPerThread)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FVariableParameters, )
SHADER_PARAMETER(float, DeltaTime)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

typedef TUniformBufferRef<FConstantParameters> FConstantParametersRef;
typedef TUniformBufferRef<FVariableParameters> FVariableParametersRef;

class FShaderFishPluginModule : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FShaderFishPluginModule, Global);
public:
	FShaderFishPluginModule() {}
	explicit FShaderFishPluginModule(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCache(EShaderPlatform Platform) { return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5); }
    static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }

	virtual bool Serialize(FArchive& Ar) override { bool bShaderHasOutdatedParams = FGlobalShader::Serialize(Ar); Ar << m_shaderResource; return bShaderHasOutdatedParams; }

	void setShaderData(FRHICommandList& commandList, FUnorderedAccessViewRHIRef uav);
	void setUniformBuffers(FRHICommandList& commandList, FConstantParameters& constants, FVariableParameters& variables);
	void cleanupShaderData(FRHICommandList& commandList);

private:
	FShaderResourceParameter m_shaderResource;
};
