// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "ShaderFishPlugin.h"
#include "RenderCommandFence.h"

struct State {
    int32 instanceIndex = 0;
	float position[3] = { 0, 0, 0 };
	float velocity[3] = { 0, 0, 0 };
	float acceleration[3] = { 0, 0, 0 };

    // SteerInfo
    float steerCohesion[3] = { 0, 0, 0 };
    float steerSeparation[3] = { 0, 0, 0 };
    float steerAlignment[3] = { 0, 0, 0 };

    int32 steerCohesionCnt   = 0;
    int32 steerSeparationCnt = 0;
    int32 steerAlignmentCnt  = 0;

    FVector location() const {
        return FVector(position[0], position[1], position[2]);
    }

    FVector vel() const {
        return FVector(velocity[0], velocity[1], velocity[2]);
    }
};

class SHADERFISHPLUGIN_API FishProcessing
{
public:
    FishProcessing(int32 fishCount, float radiusCohesion, float radiusSeparation, float radiusAlignment,
		float mapRangeX, float mapRangeY, float mapRangeZ, float kCohesion, float kSeparation, float kAlignment, 
		float maxAcceleration, float maxVelocity, ERHIFeatureLevel::Type ShaderFeatureLevel);
    ~FishProcessing();

    void calculate(const TArray<State> &currentStates, float deltaTime);
    void getStates(TArray<State>& OutStates) { OutStates = m_states; }

protected:
    void ExecuteComputeShader(const TArray<State> &currentStates, float DeltaTime);
    void ExecuteInRenderThread(int32 threadNumGroupCount,
                               const FFishShader::FParameters& parameters,
                               ERHIFeatureLevel::Type featureLevel,
                               const TArray<State> &currentStates, TArray<State> &result);

private:
    FFishShader::FParameters m_parameters;
    FShaderResourceParameter UploadCurrentStateData;
	ERHIFeatureLevel::Type m_featureLevel;
    TArray<State> m_states;
    int32 m_threadNumGroupCount = 0;
	FRenderCommandFence ReleaseResourcesFence;
};
