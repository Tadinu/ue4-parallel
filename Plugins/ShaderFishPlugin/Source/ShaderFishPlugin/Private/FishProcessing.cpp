// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "FishProcessing.h"
#include "CoreUObject.h"
#include "Engine.h"
#include "RenderGraphUtils.h"

#include <iostream>
#define NUM_THREADS_PER_GROUP_DIMENSION 128 

FishProcessing::FishProcessing(int32 fishCount, float radiusCohesion, float radiusSeparation, float radiusAlignment,
	float mapRangeX, float mapRangeY, float mapRangeZ, float kCohesion, float kSeparation, float kAlignment,
	float maxAcceleration, float maxVelocity, ERHIFeatureLevel::Type ShaderFeatureLevel)
{
	m_featureLevel = ShaderFeatureLevel;
    m_parameters.fishCount = fishCount;
    m_parameters.radiusCohesion = radiusCohesion;
    m_parameters.radiusSeparation = radiusSeparation;
    m_parameters.radiusAlignment = radiusAlignment;
    m_parameters.mapRangeX = mapRangeX;
    m_parameters.mapRangeY = mapRangeY;
    m_parameters.mapRangeZ = mapRangeZ;
    m_parameters.kCohesion = kCohesion;
    m_parameters.kSeparation = kSeparation;
    m_parameters.kAlignment = kAlignment;
    m_parameters.maxAcceleration = maxAcceleration;
    m_parameters.maxVelocity = maxVelocity;
    m_parameters.calculationsPerThread = 1;

    m_states.SetNumZeroed(fishCount);

    m_threadNumGroupCount = (fishCount % (NUM_THREADS_PER_GROUP_DIMENSION * m_parameters.calculationsPerThread) == 0 ?
        fishCount / (NUM_THREADS_PER_GROUP_DIMENSION * m_parameters.calculationsPerThread) :
        fishCount / (NUM_THREADS_PER_GROUP_DIMENSION * m_parameters.calculationsPerThread) + 1);
	m_threadNumGroupCount = m_threadNumGroupCount == 0 ? 1 : m_threadNumGroupCount;
}

FishProcessing::~FishProcessing()
{
}

void FishProcessing::calculate(const TArray<State> &currentStates, float deltaTime)
{
	ExecuteComputeShader(currentStates, deltaTime);
	ReleaseResourcesFence.BeginFence();
	ReleaseResourcesFence.Wait();
}

void FishProcessing::ExecuteComputeShader(const TArray<State> &currentStates, float DeltaTime)
{
    m_parameters.DeltaTime = DeltaTime;

    ENQUEUE_RENDER_COMMAND(FComputeShaderRunner) (
        [&, this, currentStates=currentStates](FRHICommandListImmediate& RHICmdList)
		{ 
            this->ExecuteInRenderThread(m_threadNumGroupCount,
                                        m_parameters,
                                        m_featureLevel,
                                        currentStates, m_states);
        }
	);
}

void FishProcessing::ExecuteInRenderThread(int32 threadNumGroupCount,
                                           const FFishShader::FParameters& parameters,
                                           ERHIFeatureLevel::Type featureLevel,
                                           const TArray<State> &currentStates, TArray<State> &result)
{
	check(IsInRenderingThread());
    if(parameters.fishCount <= 0) {
         return;
    }
    verify(currentStates.Num() == parameters.fishCount &&
           result.Num() == parameters.fishCount);

    // Prepare RHI Resource Data
    //
    TResourceArray<State> rhiData;
    for (int32 i = 0; i < 2 * parameters.fishCount; i++) {
        rhiData.Add(currentStates[i % parameters.fishCount]);
	}
    FRHIResourceCreateInfo resource(&rhiData);

    // Create RW Structured Buffer
    static int32 stateSize = sizeof(State);
    int32 bufferSize = stateSize * 2 * parameters.fishCount;

    FRWBufferStructured buffer;
    buffer.Buffer = RHICreateStructuredBuffer(stateSize, bufferSize,
                                              BUF_UnorderedAccess | BUF_ShaderResource | 0,
                                              resource);
    buffer.UAV = RHICreateUnorderedAccessView(buffer.Buffer, false, false);
    buffer.SRV = RHICreateShaderResourceView(buffer.Buffer);

    // Transition resource to GPU
    FRHICommandListImmediate& rhiCommandList = GRHICommandList.GetImmediateCommandList();
    rhiCommandList.TransitionResource(EResourceTransitionAccess::ERWBarrier,
                                      EResourceTransitionPipeline::EGfxToCompute,
                                      buffer.UAV);

    // Dispatch & Compute shader in GPU
    TShaderMapRef<FFishShader> shader(GetGlobalShaderMap(featureLevel));
#if 0
    // Call to SetComputeShader & SetShaderParameters here-in
    FComputeShaderUtils::Dispatch(rhiCommandList, *shader, parameters, FIntVector(1, threadNumGroupCount, 1));
#else
    rhiCommandList.SetComputeShader(shader->GetComputeShader());
    shader->SetShaderUAVParameter(rhiCommandList, buffer.UAV);
    shader->SetShaderSRVParameter(rhiCommandList, buffer.SRV);
    shader->SetUniformBufferParameters(rhiCommandList, parameters);
    DispatchComputeShader(rhiCommandList, *shader, threadNumGroupCount, 1, 1);
    shader->cleanupShaderData(rhiCommandList);
#endif

    // Read result data from the buffer
    //
    char* shaderData = (char*)rhiCommandList.LockStructuredBuffer(buffer.Buffer, 0, bufferSize,
                                                               EResourceLockMode::RLM_ReadOnly);
    State* newStateData = reinterpret_cast<State*>(shaderData);

    int32 bufferCount = buffer.Buffer->GetSize()/stateSize;
    verify(bufferCount == rhiData.Num())
    //std::cout << "BUFFER ELEMENT NUM: " << bufferCount << std::endl;

    for (int32 i = 0; i < parameters.fishCount; i++) {
        if(newStateData) {
            result[i] = *newStateData;
            newStateData++;
        }
    }

    rhiCommandList.UnlockStructuredBuffer(buffer.Buffer);
}
