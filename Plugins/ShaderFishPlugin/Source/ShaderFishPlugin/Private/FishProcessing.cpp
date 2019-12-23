// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "FishProcessing.h"
#include "CoreUObject.h"
#include "Engine.h"

#include <iostream>
#define NUM_THREADS_PER_GROUP_DIMENSION 128 

FishProcessing::FishProcessing(int32 fishCount, float radiusCohesion, float radiusSeparation, float radiusAlignment,
	float mapRangeX, float mapRangeY, float mapRangeZ, float kCohesion, float kSeparation, float kAlignment,
	float maxAcceleration, float maxVelocity, ERHIFeatureLevel::Type ShaderFeatureLevel)
{
	m_featureLevel = ShaderFeatureLevel;
	m_constantParameters.fishCount = fishCount;
	m_constantParameters.radiusCohesion = radiusCohesion;
	m_constantParameters.radiusSeparation = radiusSeparation;
	m_constantParameters.radiusAlignment = radiusAlignment;
	m_constantParameters.mapRangeX = mapRangeX;
	m_constantParameters.mapRangeY = mapRangeY;
	m_constantParameters.mapRangeZ = mapRangeZ;
	m_constantParameters.kCohesion = kCohesion;
	m_constantParameters.kSeparation = kSeparation;
	m_constantParameters.kAlignment = kAlignment;
	m_constantParameters.maxAcceleration = maxAcceleration;
	m_constantParameters.maxVelocity = maxVelocity;
	m_constantParameters.calculationsPerThread = 1;

	m_variableParameters = FVariableParameters();

    m_states.SetNumZeroed(fishCount);

    m_threadNumGroupCount = (fishCount % (NUM_THREADS_PER_GROUP_DIMENSION * m_constantParameters.calculationsPerThread) == 0 ?
        fishCount / (NUM_THREADS_PER_GROUP_DIMENSION * m_constantParameters.calculationsPerThread) :
        fishCount / (NUM_THREADS_PER_GROUP_DIMENSION * m_constantParameters.calculationsPerThread) + 1);
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
	m_variableParameters.DeltaTime = DeltaTime;

    ENQUEUE_RENDER_COMMAND(FComputeShaderRunner) (
        [&, this, currentStates=currentStates](FRHICommandListImmediate& RHICmdList)
		{ 
            this->ExecuteInRenderThread(currentStates, m_states);
        }
	);
}

void FishProcessing::ExecuteInRenderThread(const TArray<State> &currentStates, TArray<State> &result)
{
	check(IsInRenderingThread());
    int32 fishNum = m_constantParameters.fishCount;
    verify(currentStates.Num() == fishNum &&
           result.Num() == fishNum);

    // Prepare RHI Resource Data
    //
    TResourceArray<State> rhiData;
    for (int32 i = 0; i < 2 * fishNum; i++) {
        rhiData.Add(currentStates[i % fishNum]);
	}

    FRHIResourceCreateInfo rhiResource;
    rhiResource.ResourceArray = &rhiData;

    static int32 stateSize = sizeof(State);
    int32 bufferSize = stateSize * 2 * fishNum;

    FStructuredBufferRHIRef buffer = RHICreateStructuredBuffer(stateSize, bufferSize,
                                                               BUF_UnorderedAccess | BUF_ShaderResource | 0 , rhiResource);
	FUnorderedAccessViewRHIRef uav = RHICreateUnorderedAccessView(buffer, false, false);

    FRHICommandListImmediate& commandList = GRHICommandList.GetImmediateCommandList();

    // Compute shader operation on the buffer
    //
    TShaderMapRef<FFishShader> shader(GetGlobalShaderMap(m_featureLevel));
	commandList.SetComputeShader(shader->GetComputeShader());
	shader->setShaderData(commandList, uav);
	shader->setUniformBuffers(commandList, m_constantParameters, m_variableParameters);
	DispatchComputeShader(commandList, *shader, 1, m_threadNumGroupCount, 1);
	shader->cleanupShaderData(commandList);	

    // Read result data from the buffer
    //
    char* shaderData = (char*)commandList.LockStructuredBuffer(buffer, 0, bufferSize,
                                                               EResourceLockMode::RLM_ReadOnly);
    State* newStateData = reinterpret_cast<State*>(shaderData);

    int32 bufferCount = buffer->GetSize()/stateSize;
    verify(bufferCount == rhiData.Num())
    //std::cout << "BUFFER ELEMENT NUM: " << bufferCount << std::endl;

    for (int32 i = 0; i < fishNum; i++) {
        if(newStateData) {
            result[i] = *newStateData;
            newStateData++;
        }
    }

	commandList.UnlockStructuredBuffer(buffer);
}
