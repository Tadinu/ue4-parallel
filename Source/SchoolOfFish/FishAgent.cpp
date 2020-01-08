// Fill out your copyright notice in the Description page of Project Settings.

#include "FishAgent.h"
#include <iostream>

#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "ConstructorHelpers.h"
#include "ParallelFor.h"
#include "Event.h"
#include "ScopeLock.h"
#include "Engine/StaticMesh.h"

#include "FlockingGameMode.h"

FVector checkMapRange(const FVector &map, const FVector &currentPosition, const FVector &currentVelocity)
{
	FVector newVelocity = currentVelocity;
	if (currentPosition.X > map.X || currentPosition.X < -map.X) {
		newVelocity.X *= -1.f;
	}

	if (currentPosition.Y > map.Y || currentPosition.Y < -map.Y) {
		newVelocity.Y *= -1.f;
	}

	if (currentPosition.Z > map.Z || currentPosition.Z < -3000.f) {
		newVelocity.Z *= -1.f;
	}
	return newVelocity;
}

int32 getCommandLineArgIntValue(FString key, int32 defaultValue)
{
	int32 value;
	if (FParse::Value(FCommandLine::Get(), *key, value)) {
		return value;
	}
	return defaultValue;
}

float getCommandLineArgFloatValue(FString key, float defaultValue)
{
	float value;
	if (FParse::Value(FCommandLine::Get(), *key, value)) {
		return value;
	}
	return defaultValue;
}

FString getCommandLineArgStringValue(FString key, FString defaultValue)
{
	FString value;
	if (FParse::Value(FCommandLine::Get(), *key, value)) {
		return value.Replace(TEXT("="), TEXT("")).Replace(TEXT("\""), TEXT(""));
	}
	return defaultValue;
}

AFishAgent::AFishAgent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Available calcModes:
	// CPU_SINGLE - CPU single-threaded calculation of flocking behaviour
	// CPU_MULTI - CPU multi-threaded calculation of flocking behaviour
	// GPU_MULTI - GPU multi-threaded calculation of flocking behaviour

    FString mode = getCommandLineArgStringValue("calcMode", "GPU_MULTI");
	if ("CPU_SINGLE" == mode) {
		m_isCpuSingleThread = true;
		m_isGpu = false;
	}
	else if ("CPU_MULTI" == mode) {
		m_isCpuSingleThread = false;
		m_isGpu = false;
	}
	else if ("GPU_MULTI" == mode) {
		m_isGpu = true;
	}
	else {
		m_isCpuSingleThread = true;
		m_isGpu = false;
	}

    m_fishNum = getCommandLineArgIntValue("agentCount", 1000);
	m_maxVelocity = getCommandLineArgFloatValue("maxVelocity", 2500.f);
	m_maxAcceleration = getCommandLineArgFloatValue("maxAcceleration", 1500.f);
	m_radiusCohesion = getCommandLineArgFloatValue("radiusCohesion", 1000.f);
	m_radiusSeparation = getCommandLineArgFloatValue("radiusSeparation", 120.f);
	m_radiusAlignment = getCommandLineArgFloatValue("radiusAlignment", 240.f);
	m_kCohesion = getCommandLineArgFloatValue("kCohesion", 100.f);
	m_kSeparation = getCommandLineArgFloatValue("kSeparation", 1.f);
	m_kAlignment = getCommandLineArgFloatValue("kAlignment", 20.f);
	m_mapSize.X = getCommandLineArgFloatValue("mapSizeX", 20000.f);
	m_mapSize.Y = getCommandLineArgFloatValue("mapSizeY", 20000.f);
	m_mapSize.Z = getCommandLineArgFloatValue("mapSizeZ", 0.f);
	m_oceanFloorZ = getCommandLineArgFloatValue("oceanFloorZ", -3000.f);
	m_spawningRange = getCommandLineArgFloatValue("spawningRange", 0.9f);

	m_currentStatesIndex = 0;
	m_previousStatesIndex = 1;

	PrimaryActorTick.bCanEverTick = true;

    // Create Instanced Fish Mesh Component
    m_staticMesh = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Game/Fish/SM_fish'")).Object;
}

void AFishAgent::CreateFishMeshComponent(UStaticMesh* FishMesh)
{
    m_instancedStaticMeshComponent = NewObject<UInstancedStaticMeshComponent>(this, TEXT("FishMeshComp"));
    SetRootComponent(m_instancedStaticMeshComponent);
    m_instancedStaticMeshComponent->RegisterComponent();
    m_instancedStaticMeshComponent->SetFlags(RF_Transactional);
    m_instancedStaticMeshComponent->SetStaticMesh(FishMesh);
    this->AddOwnedComponent(m_instancedStaticMeshComponent);
    this->AddInstanceComponent(m_instancedStaticMeshComponent);
}

void AFishAgent::OnConstruction(const FTransform& Transform)
{
    CreateFishMeshComponent(m_staticMesh);

	if (!m_isGpu) {
        m_fishStates.SetNumZeroed(m_fishNum);
	}

    std::cout << m_isGpu << m_fishNum << m_fishStates.Num() << std::endl;

    for (int32 i = 0; i < m_fishNum; i++) {
        randomGen.GenerateNewSeed();
        FVector randomPos(randomGen.FRandRange(-m_spawningRange * m_mapSize.X, m_mapSize.X * m_spawningRange),
                          randomGen.FRandRange(-m_spawningRange * m_mapSize.Y, m_mapSize.Y * m_spawningRange),
                          randomGen.FRandRange(m_spawningRange * m_oceanFloorZ, m_mapSize.Z * m_spawningRange));
        FRotator randRotator(0, randomGen.FRandRange(0, 360), 0);

		FTransform t;
		t.SetLocation(randomPos);
		t.SetRotation(randRotator.Quaternion());
        t.SetScale3D(10.f * FVector::OneVector);

        int32 instanceIndex = m_instancedStaticMeshComponent->AddInstance(t);
        //std::cout << " ADD INSTANCE " << instanceIndex << std::endl;
		if (m_isGpu) {
            State gpustate;
            gpustate.instanceIndex = instanceIndex;
            gpustate.position[0] = randomPos.X; gpustate.position[1] = randomPos.Y; gpustate.position[2] = randomPos.Z;
            gpustate.velocity[0] = randRotator.Vector().X * 10000; gpustate.velocity[1] = randRotator.Vector().Y * 10000; gpustate.velocity[2] = randRotator.Vector().Z * 10000;
            gpustate.acceleration[0] = 0.f; gpustate.acceleration[1] = 0.f; gpustate.acceleration[2] = 0.f;
            m_gpuFishStates.Add(std::move(gpustate));
		} else {
			FFishState state;
			FFishState stateCopy;
            m_fishStates[i].SetNumZeroed(2);
            stateCopy.instanceIndex = state.instanceIndex = instanceIndex;
			stateCopy.position = state.position = randomPos;
			stateCopy.velocity = state.velocity = randRotator.Vector() * 10000;
			stateCopy.acceleration = state.acceleration = FVector::ZeroVector;
            m_fishStates[i][m_currentStatesIndex]  = std::move(state);
            m_fishStates[i][m_previousStatesIndex] = std::move(stateCopy);
		}
	}
}

bool AFishAgent::SetFishInstanceTransform(int32 InstanceIndex, const FTransform& FishTransform)
{
    if(InstanceIndex < 0 || InstanceIndex >= m_instancedStaticMeshComponent->GetInstanceCount()) {
        return false;
    }

    // Update self-managed fish instance state
    if (m_isGpu) {
        float DeltaTime = GetWorld()->GetDeltaSeconds();
        const State& previousFishState = m_gpuFishStates[InstanceIndex];
        State& newFishState            = m_gpuFishStates[InstanceIndex];

        newFishState.setLocation(FishTransform.GetLocation());

        FHitResult hit(ForceInit);
        if (collisionDetected(previousFishState.location(),
                              FishTransform.GetLocation(), hit)) {
            newFishState.position[0] -= newFishState.velocity[0] * DeltaTime;
            newFishState.position[1] -= newFishState.velocity[1] * DeltaTime;
            newFishState.position[2] -= newFishState.velocity[2] * DeltaTime;
            newFishState.velocity[0] *= -1.f;
            newFishState.velocity[1] *= -1.f;
            newFishState.velocity[2] *= -1.f;
            newFishState.position[0] += newFishState.velocity[0] * DeltaTime;
            newFishState.position[1] += newFishState.velocity[1] * DeltaTime;
            newFishState.position[2] += newFishState.velocity[2] * DeltaTime;
        }    

        // Update the Instance Static Mesh Component
        FTransform newTransform;
        if(!m_instancedStaticMeshComponent->GetInstanceTransform(InstanceIndex, newTransform)) {
            return false;
        }

        newTransform.SetLocation(newFishState.location());
        //transform.SetRotation(FRotationMatrix::MakeFromX(newFishState.vel()).Rotator().Add(0.f, -90.f, 0.f).Quaternion());

        m_instancedStaticMeshComponent->UpdateInstanceTransform(InstanceIndex, newTransform, false, true);
    }
    else {
        //FFishState& fishState = m_fishStates[InstanceIndex][0];
        //fishState.position = FishTransform.GetLocation();

        // TBD...
    }
    return true;
}

FTransform AFishAgent::GetFishInstanceTransform(int32 InstanceIndex)
{
    if(InstanceIndex < 0 || InstanceIndex >= m_instancedStaticMeshComponent->GetInstanceCount()) {
        return FTransform::Identity;
    }

    FTransform transform;
    m_instancedStaticMeshComponent->GetInstanceTransform(InstanceIndex, transform);
    return transform;
}

bool AFishAgent::AddForceToFishInstance(int32 InstanceIndex, const FVector& Force)
{
    if(InstanceIndex < 0 || InstanceIndex >= m_instancedStaticMeshComponent->GetInstanceCount()) {
        return false;
    }
    m_instancedStaticMeshComponent->AddImpulse(Force);
    return false;
}

void AFishAgent::BeginPlay()
{
    Super::BeginPlay();

    GameMode = Cast<AFlockingGameMode>(GetWorld()->GetAuthGameMode());

    if (m_isGpu) {
#if USING_GPU_COMPUTER_SHADER
        m_gpuProcessing = std::make_unique<FishShaderProcessing>(m_fishNum,
            m_radiusCohesion, m_radiusSeparation, m_radiusAlignment,
            m_mapSize.X, m_mapSize.Y, m_mapSize.Z,
            m_kCohesion, m_kSeparation, m_kAlignment,
            m_maxAcceleration, m_maxVelocity,
            GetWorld()->Scene->GetFeatureLevel()
        );
#endif
    }
}

void AFishAgent::SendToServerRequestFishTransform(int32 FishIndex)
{
    if(FishIndex < 0 || FishIndex >= m_fishNum) return;
    if(GameMode) {
        FActorInstanceInfo fishInfo;
        fishInfo.instanceIndex = FishIndex;
        const State& gpuFishState = m_gpuFishStates[FishIndex];

        fishInfo.position        = gpuFishState.location();
        fishInfo.velocity        = gpuFishState.vel();
        fishInfo.acceleration    = gpuFishState.accel();
        fishInfo.steerCohesion   = gpuFishState.steerCoh();
        fishInfo.steerSeparation = gpuFishState.steerSepa();
        fishInfo.steerAlignmen   = gpuFishState.steerAlign();

        fishInfo.steerCohesionCnt   = gpuFishState.steerCohesionCnt;
        fishInfo.steerSeparationCnt = gpuFishState.steerSeparationCnt;
        fishInfo.steerAlignmentCnt  = gpuFishState.steerAlignmentCnt;

        GameMode->GRPCClient->RequestGetActorTransform(fishInfo, context);
    }
}

void AFishAgent::OnFishTransformReturned(UActorOperationRpcClient* RpcClient,
                                         const FActorInstanceTransform& Response,
                                         FGrpcStatus Status)
{
    if(EGrpcStatusCode::Ok != Status.ErrorCode) return;

    int32 fishIndex = Response.id;
    if(fishIndex < 0  || fishIndex >= m_fishNum) return;
    m_gpuFishStates[fishIndex].setLocation(Response.transf.GetLocation());
}

void AFishAgent::SendToServerEnvironmentInfoToServer(float DeltaTime)
{
    if(GameMode) {
        // SEND [INFORM ENVIRONMENT INFO => COMPUTE SERVER]
        FActorEnvironmentInfo envInfo;
        envInfo.instanceCount = m_fishNum;

        envInfo.radiusCohesion   = m_radiusCohesion;
        envInfo.radiusSeparation = m_radiusSeparation;
        envInfo.radiusAlignment  = m_radiusAlignment;

        envInfo.mapRangeX = m_mapSize.X;
        envInfo.mapRangeY = m_mapSize.Y;
        envInfo.mapRangeZ = m_mapSize.Z;

        envInfo.kCohesion   = m_kCohesion;
        envInfo.kSeparation = m_kSeparation;
        envInfo.kAlignment  = m_kAlignment;
        envInfo.maxAcceleration = m_maxAcceleration;
        envInfo.maxVelocity = m_maxVelocity;
        envInfo.calculationsPerThread = 1;
        envInfo.DeltaTime = DeltaTime;

        GameMode->GRPCClient->InformActorEnvironmentInfo(envInfo, context);
    }
}

void AFishAgent::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (m_isGpu) {
        if (m_elapsedTime > 0.016f) {
            //m_instancedStaticMeshComponent->InitPerInstanceRenderData(false);
#if USING_GPU_COMPUTER_SHADER
            TArray<State> previousGpuFishStates = m_gpuFishStates;
            m_gpuProcessing->calculate(m_gpuFishStates, DeltaTime);
            m_gpuProcessing->getStates(m_gpuFishStates);

            verify(previousGpuFishStates.Num() == m_gpuFishStates.Num())

            for (int32 i = 0; i < previousGpuFishStates.Num(); i++) {
                FHitResult hit(ForceInit);
                m_gpuFishStates[i].instanceIndex = previousGpuFishStates[i].instanceIndex;
                if (collisionDetected(previousGpuFishStates[i].location(),
                                      m_gpuFishStates[i].location(), hit)) {
                    m_gpuFishStates[i].position[0] -= m_gpuFishStates[i].velocity[0] * DeltaTime;
                    m_gpuFishStates[i].position[1] -= m_gpuFishStates[i].velocity[1] * DeltaTime;
                    m_gpuFishStates[i].position[2] -= m_gpuFishStates[i].velocity[2] * DeltaTime;
                    m_gpuFishStates[i].velocity[0] *= -1.f;
                    m_gpuFishStates[i].velocity[1] *= -1.f;
                    m_gpuFishStates[i].velocity[2] *= -1.f;
                    m_gpuFishStates[i].position[0] += m_gpuFishStates[i].velocity[0] * DeltaTime;
                    m_gpuFishStates[i].position[1] += m_gpuFishStates[i].velocity[1] * DeltaTime;
                    m_gpuFishStates[i].position[2] += m_gpuFishStates[i].velocity[2] * DeltaTime;
                }
            }
#endif
            // [ENVIRONMENT INFO] --
            static bool notYetSent = true;
            if(notYetSent) {
                SendToServerEnvironmentInfoToServer(DeltaTime);
                notYetSent = false;
            }

            // [REQUEST FISH TRANSFORM] --
            //
            FTransform transform;
            bool bWorldSpace = false;
            for (int32 i = 0; i < m_fishNum; i++) {
#if (!USING_GPU_COMPUTER_SHADER)
                SendToServerRequestFishTransform(i);
#endif
                // Fetch the latest transform result returned from Server
                const State& gpuFishState = m_gpuFishStates[i];
                transform = FTransform::Identity;

                if(!m_instancedStaticMeshComponent->GetInstanceTransform(gpuFishState.instanceIndex, transform, bWorldSpace)) {
                    continue;
                }

                transform.SetLocation(gpuFishState.location());
                //transform.SetRotation(FRotationMatrix::MakeFromX(gpuFishState.vel()).Rotator().Add(0.f, -90.f, 0.f).Quaternion());
                if(transform.ContainsNaN()) {
                    std::cout << gpuFishState.instanceIndex << " NAN" << std::endl;
                    continue;
                }

                m_instancedStaticMeshComponent->UpdateInstanceTransform(i, transform, bWorldSpace);
            }
#if 0 // THIS CAUSE CRASH ON PerInstanceRenderData going INVALID
            m_instancedStaticMeshComponent->ReleasePerInstanceRenderData();
#endif
            m_instancedStaticMeshComponent->MarkRenderStateDirty();
            m_elapsedTime = 0.f;
        } else {
            m_elapsedTime += DeltaTime;
            SetFishUpdateRequest(false);
        }
    } else {
        if (m_elapsedTime > 0.016f) {
            cpuCalculate(m_fishStates, DeltaTime, m_isCpuSingleThread);
            m_elapsedTime = 0.f;
        } else {
            m_elapsedTime += DeltaTime;
        }
    }
}

void AFishAgent::cpuCalculate(TArray<TArray<FFishState>>& agents, float DeltaTime, bool isSingleThread)
{
	float kCoh = m_kCohesion, kSep = m_kSeparation, kAlign = m_kAlignment;
	float rCohesion = m_radiusCohesion, rSeparation = m_radiusSeparation, rAlignment = m_radiusAlignment;
	float maxAccel = m_maxAcceleration;
	float maxVel = m_maxVelocity;
	FVector mapSz = m_mapSize;
	static const int32 cnt = m_fishNum;

	int currentStatesIndex = m_currentStatesIndex;
	int previousStatesIndex = m_previousStatesIndex;
	
	ParallelFor(cnt, [&agents, currentStatesIndex, previousStatesIndex, kCoh, kSep, kAlign, rCohesion, rSeparation, rAlignment, maxAccel, maxVel, mapSz, DeltaTime, isSingleThread](int32 fishNum) {
		FVector cohesion(FVector::ZeroVector), separation(FVector::ZeroVector), alignment(FVector::ZeroVector);
		int32 cohesionCnt = 0, separationCnt = 0, alignmentCnt = 0;
		for (int i = 0; i < cnt; i++) {
			if (i != fishNum) {
				float distance = FVector::Distance(agents[i][previousStatesIndex].position, agents[fishNum][previousStatesIndex].position);
				if (distance > 0) {
					if (distance < rCohesion) {
						cohesion += agents[i][previousStatesIndex].position;
						cohesionCnt++;
					}
					if (distance < rSeparation) {
						separation += agents[i][previousStatesIndex].position - agents[fishNum][previousStatesIndex].position;
						separationCnt++;
					}
					if (distance < rAlignment) {
						alignment += agents[i][previousStatesIndex].velocity;
						alignmentCnt++;
					}
				}
			}
		}

		if (cohesionCnt != 0) {
			cohesion /= cohesionCnt;
			cohesion -= agents[fishNum][previousStatesIndex].position;
			cohesion.Normalize();
		}

		if (separationCnt != 0) {
			separation /= separationCnt;
			separation *= -1.f;
			separation.Normalize();
		}

		if (alignmentCnt != 0) {
			alignment /= alignmentCnt;
			alignment.Normalize();
		}

		agents[fishNum][currentStatesIndex].acceleration = (cohesion * kCoh + separation * kSep + alignment * kAlign).GetClampedToMaxSize(maxAccel);
		agents[fishNum][currentStatesIndex].acceleration.Z = 0;

		agents[fishNum][currentStatesIndex].velocity += agents[fishNum][currentStatesIndex].acceleration * DeltaTime;
		agents[fishNum][currentStatesIndex].velocity  = agents[fishNum][currentStatesIndex].velocity.GetClampedToMaxSize(maxVel);
		agents[fishNum][currentStatesIndex].position += agents[fishNum][currentStatesIndex].velocity * DeltaTime;
		agents[fishNum][currentStatesIndex].velocity = checkMapRange(mapSz, agents[fishNum][currentStatesIndex].position, agents[fishNum][currentStatesIndex].velocity);
	}, isSingleThread);

	for (int i = 0; i < cnt; i++) {
		FHitResult hit(ForceInit);
		if (collisionDetected(agents[i][previousStatesIndex].position, agents[i][currentStatesIndex].position, hit)) {
			agents[i][currentStatesIndex].position -= agents[i][currentStatesIndex].velocity * DeltaTime;
			agents[i][currentStatesIndex].velocity *= -1.0;
			agents[i][currentStatesIndex].position += agents[i][currentStatesIndex].velocity * DeltaTime;
		}
	}

	for (int i = 0; i < cnt; i++) {
		FTransform transform;
        m_instancedStaticMeshComponent->GetInstanceTransform(agents[i][0].instanceIndex, transform);
		transform.SetLocation(agents[i][0].position);
		FVector direction = agents[i][0].velocity;
		direction.Normalize();
		transform.SetRotation(FRotationMatrix::MakeFromX(direction).Rotator().Add(0.f, -90.f, 0.f).Quaternion());
        m_instancedStaticMeshComponent->UpdateInstanceTransform(agents[i][0].instanceIndex, transform, false, false);
	}

    std::cout << " INSTANCED RANDOM SEED " << m_instancedStaticMeshComponent->InstancingRandomSeed << std::endl;
#if 0 // THIS CAUSE CRASH ON PerInstanceRenderData going INVALID
    m_instancedStaticMeshComponent->ReleasePerInstanceRenderData();
#endif

	m_instancedStaticMeshComponent->MarkRenderStateDirty();

	swapFishStatesIndexes();
}

bool AFishAgent::collisionDetected(const FVector &start, const FVector &end, FHitResult &hitResult)
{
	FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, this);
	RV_TraceParams.bTraceComplex = true;
	RV_TraceParams.bReturnPhysicalMaterial = false;

	return GetWorld()->SweepSingleByChannel(hitResult, start, end, FQuat(), ECC_WorldStatic, FCollisionShape::MakeSphere(200), RV_TraceParams);
}

void AFishAgent::swapFishStatesIndexes()
{
	int32 tmp = m_currentStatesIndex;
	m_currentStatesIndex = m_previousStatesIndex;
	m_previousStatesIndex = tmp;
}
