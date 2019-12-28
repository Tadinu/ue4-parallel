// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

#include "GlobalShader.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"

#define CSHADER_FISH_PLUGIN_NAME (TEXT("ShaderFishPluginModule" ))

// (snote) This has to match the one being declared inside the .usf file.
#define CSHADER_PARAMETER_NAME (TEXT("RWData"))

class IShaderFishPluginModule : public IModuleInterface
{

public:

    /**
     * Singleton-like access to this module's interface.  This is just for convenience!
     * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
     *
     * @return Returns singleton instance, loading the module on demand if needed
     */
    static inline IShaderFishPluginModule& Get()
    {
        return FModuleManager::LoadModuleChecked<IShaderFishPluginModule>(CSHADER_FISH_PLUGIN_NAME);
    }

    /**
     * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
     *
     * @return True if the module is loaded and ready to use
     */
    static inline bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded(CSHADER_FISH_PLUGIN_NAME);
    }
};

class FFishShader : public FGlobalShader
{
    DECLARE_SHADER_TYPE(FFishShader, Global);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, ) // Cannot have any name else but FParameters" (UE Rule)
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
        SHADER_PARAMETER(float, DeltaTime)
    END_SHADER_PARAMETER_STRUCT()

    typedef TUniformBufferRef<FParameters> FParametersRef;
public:
    FFishShader() {}
    explicit FFishShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCache(EShaderPlatform Platform) { return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5); }
    static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return true;
        //return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }

	virtual bool Serialize(FArchive& Ar) override { bool bShaderHasOutdatedParams = FGlobalShader::Serialize(Ar); Ar << m_shaderResource; return bShaderHasOutdatedParams; }

    void SetShaderBuffer(FRHICommandList& commandList, const FRWBufferStructured& buffer);
    void SetShaderUAVParameter(FRHICommandList& commandList, FUnorderedAccessViewRHIRef uav);
    void SetShaderSRVParameter(FRHICommandList& commandList, FShaderResourceViewRHIRef srv);
    void SetUniformBufferParameters(FRHICommandList& commandList, const FFishShader::FParameters& parameters);
	void cleanupShaderData(FRHICommandList& commandList);

private:
	FShaderResourceParameter m_shaderResource;
    FRWShaderParameter m_shaderRWParameter; // To be used in case either using only SRV or UAV as the resource
};
