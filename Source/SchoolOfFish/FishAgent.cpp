// Fill out your copyright notice in the Description page of Project Settings.

#include "FishAgent.h"
#include "FlockingGameMode.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Async/ParallelFor.h"
#include "HAL/Event.h"
#include "Misc/ScopeLock.h"
#include "Engine/StaticMesh.h"
#include "SchoolOfFish.h"
#if FISH_ISPC
#include "FishAgentUtils.ispc.generated.h"
#endif

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

AFishAgent::AFishAgent()
{
	// Available calcModes:
	// CPU_SINGLE - CPU single-threaded calculation of flocking behaviour
	// CPU_MULTI - CPU multi-threaded calculation of flocking behaviour
	// GPU_MULTI - GPU multi-threaded calculation of flocking behaviour

	FString mode = getCommandLineArgStringValue("calcMode", "CPU_SINGLE");
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
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshOb(TEXT("StaticMesh'/Game/Fish/SM_fish'"));
	m_staticMesh = StaticMeshOb.Object;
    verify(m_staticMesh);
    UE_LOG(LogTemp, Warning, TEXT("%ld"), m_staticMesh);
}

void AFishAgent::OnConstruction(const FTransform& Transform)
{
	m_instancedStaticMeshComponent = NewObject<UInstancedStaticMeshComponent>(this);
	m_instancedStaticMeshComponent->RegisterComponent();
	m_instancedStaticMeshComponent->SetStaticMesh(m_staticMesh);
	m_instancedStaticMeshComponent->SetFlags(RF_Transactional);
	this->AddInstanceComponent(m_instancedStaticMeshComponent);

	if (!m_isGpu) {
#if FISH_ISPC
        m_fishesPositions.SetNum(2);
        m_fishesVels.SetNum(2);
        m_fishesAccels.SetNum(2);

        m_fishesPositions[0].SetNum(m_fishNum);
        m_fishesPositions[1].SetNum(m_fishNum);

        m_fishesVels[0].SetNum(m_fishNum);
        m_fishesVels[1].SetNum(m_fishNum);

        m_fishesAccels[0].SetNum(m_fishNum);
        m_fishesAccels[1].SetNum(m_fishNum);
#else
        m_fishStates.SetNum(m_fishNum);
#endif
	}

	for (int i = 0; i < m_fishNum; i++) {
		FVector randomPos(FMath::RandRange(-m_spawningRange * m_mapSize.X, m_mapSize.X * m_spawningRange),
			FMath::RandRange(-m_spawningRange * m_mapSize.Y, m_mapSize.Y * m_spawningRange),
			FMath::RandRange(m_spawningRange * m_oceanFloorZ, m_mapSize.Z * m_spawningRange));
		FRotator randRotator(0, FMath::RandRange(0, 360), 0);

		FTransform t;
		t.SetLocation(randomPos);
		t.SetRotation(randRotator.Quaternion());
		t.SetScale3D(FVector(10, 10, 10));

		int32 instanceId = m_instancedStaticMeshComponent->AddInstance(t);

		if (m_isGpu) {
			State gpustate;
			gpustate.instanceId = instanceId;
			gpustate.position[0] = randomPos.X; gpustate.position[1] = randomPos.Y; gpustate.position[2] = randomPos.Z;
			gpustate.velocity[0] = randRotator.Vector().X * 10000; gpustate.velocity[1] = randRotator.Vector().Y * 10000; gpustate.velocity[2] = randRotator.Vector().Z * 10000;
			gpustate.acceleration[0] = 0.f; gpustate.acceleration[1] = 0.f; gpustate.acceleration[2] = 0.f;
			m_gpuFishStates.Add(gpustate);
		} else {
#if FISH_ISPC
            m_fishesPositions[m_currentStatesIndex][i] =
            m_fishesPositions[m_previousStatesIndex][i] = randomPos;

            m_fishesVels[m_currentStatesIndex][i] =
            m_fishesVels[m_previousStatesIndex][i] = randRotator.Vector() * 10000;

            m_fishesAccels[m_currentStatesIndex][i] =
            m_fishesAccels[m_previousStatesIndex][i] = FVector::ZeroVector;
#else
			FishState state;
			FishState stateCopy;
            m_fishStates[i].SetNum(2);
			stateCopy.instanceId = state.instanceId = instanceId;;
			stateCopy.position = state.position = randomPos;
			stateCopy.velocity = state.velocity = randRotator.Vector() * 10000;
			stateCopy.acceleration = state.acceleration = FVector::ZeroVector;
            m_fishStates[i][m_currentStatesIndex] = MakeShared<FishState>(state);
            m_fishStates[i][m_previousStatesIndex] = MakeShared<FishState>(stateCopy);
#endif
		}
	}
}

void AFishAgent::BeginPlay()
{
	Super::BeginPlay();

	if (m_isGpu) {
        m_gpuProcessing = MakeUnique<FishProcessing>(m_fishNum,
			m_radiusCohesion, m_radiusSeparation, m_radiusAlignment,
			m_mapSize.X, m_mapSize.Y, m_mapSize.Z,
			m_kCohesion, m_kSeparation, m_kAlignment,
			m_maxAcceleration, m_maxVelocity,
			GetWorld()->Scene->GetFeatureLevel()
		);
	}
}

void AFishAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (m_isGpu) {
		if (m_elapsedTime > 0.016f) {
			m_gpuProcessing->calculate(m_gpuFishStates, DeltaTime);
			TArray<State> previousGpuFishStates = m_gpuFishStates;
			m_gpuFishStates = m_gpuProcessing->getStates();

			if (previousGpuFishStates.Num() != 0 && previousGpuFishStates.Num() == m_gpuFishStates.Num()) {
				for (int i = 0; i < previousGpuFishStates.Num(); i++) {
					FHitResult hit(ForceInit);
					if (collisionDetected(FVector(previousGpuFishStates[i].position[0], previousGpuFishStates[i].position[1], previousGpuFishStates[i].position[2]),
						FVector(m_gpuFishStates[i].position[0], m_gpuFishStates[i].position[1], m_gpuFishStates[i].position[2]), hit)) {
						m_gpuFishStates[i].position[0] -= m_gpuFishStates[i].velocity[0] * DeltaTime;
						m_gpuFishStates[i].position[1] -= m_gpuFishStates[i].velocity[1] * DeltaTime;
						m_gpuFishStates[i].position[2] -= m_gpuFishStates[i].velocity[2] * DeltaTime;
						m_gpuFishStates[i].velocity[0] *= -1.0;
						m_gpuFishStates[i].velocity[1] *= -1.0;
						m_gpuFishStates[i].velocity[2] *= -1.0;
						m_gpuFishStates[i].position[0] += m_gpuFishStates[i].velocity[0] * DeltaTime;
						m_gpuFishStates[i].position[1] += m_gpuFishStates[i].velocity[1] * DeltaTime;
						m_gpuFishStates[i].position[2] += m_gpuFishStates[i].velocity[2] * DeltaTime;
					}
				}
			}

			for (int i = 0; i < m_fishNum; i++) {
				FTransform transform;
				m_instancedStaticMeshComponent->GetInstanceTransform(m_gpuFishStates[i].instanceId, transform);
				transform.SetLocation(FVector(m_gpuFishStates[i].position[0], m_gpuFishStates[i].position[1], m_gpuFishStates[i].position[2]));
				transform.SetRotation(FRotationMatrix::MakeFromX(FVector(m_gpuFishStates[i].velocity[0], m_gpuFishStates[i].velocity[1], m_gpuFishStates[i].velocity[2])).Rotator().Add(0.f, -90.f, 0.f).Quaternion());
				m_instancedStaticMeshComponent->UpdateInstanceTransform(m_gpuFishStates[i].instanceId, transform, true, false);
			}
			m_instancedStaticMeshComponent->MarkRenderStateDirty();
			m_elapsedTime = 0.f;
		} else {
			m_elapsedTime += DeltaTime;
		}
	} else {
		if (m_elapsedTime > 0.016f) {
#if FISH_ISPC
            cpuCalculate(DeltaTime);
#else
            cpuCalculate(DeltaTime, m_isCpuSingleThread);
#endif
			m_elapsedTime = 0.f;
		} else {
			m_elapsedTime += DeltaTime;
		}
	}
}

#if FISH_ISPC
void AFishAgent::cpuCalculate(float DeltaTime)
{
    // Calculate its next [CUR] state
    FVector maxAccel(m_maxAcceleration);
    FVector maxVel(m_maxVelocity);
    // Update ~[m_currentStatesIndex] state info
    ispc::UpdateFishAgentState((ispc::FVector*)m_fishesPositions[m_currentStatesIndex].GetData(),
                               (const ispc::FVector*)m_fishesPositions[m_previousStatesIndex].GetData(),
                               (ispc::FVector*)m_fishesVels[m_currentStatesIndex].GetData(),
                               (const ispc::FVector*)m_fishesVels[m_previousStatesIndex].GetData(),
                               (ispc::FVector*)m_fishesAccels[m_currentStatesIndex].GetData(),
                               (const ispc::FVector*)m_fishesAccels[m_previousStatesIndex].GetData(),
                               m_fishNum,
                               DeltaTime,
                               m_kCohesion,
                               m_kSeparation,
                               m_kAlignment,

                               m_radiusCohesion,
                               m_radiusSeparation,
                               m_radiusAlignment,

                               (ispc::FVector&)maxAccel,
                               (ispc::FVector&)maxVel,
                               (ispc::FVector&)m_mapSize);

    // If [CUR] <-> [PREV] hits something, revert [CUR] direction
    for (auto i = 0; i < m_fishNum; ++i) {
        FHitResult hit(ForceInit);
        if (collisionDetected(m_fishesPositions[m_previousStatesIndex][i], m_fishesPositions[m_currentStatesIndex][i], hit)) {
            m_fishesPositions[m_currentStatesIndex][i] -= m_fishesVels[m_currentStatesIndex][i] * DeltaTime;
            m_fishesVels[m_currentStatesIndex][i] *= -1.0f;
            m_fishesPositions[m_currentStatesIndex][i] += m_fishesVels[m_currentStatesIndex][i] * DeltaTime;
        }
    }

    // Set all fishes to the [CUR] states
    for (auto i = 0; i < m_fishNum; ++i) {
        FTransform transform;
        m_instancedStaticMeshComponent->GetInstanceTransform(i, transform);
        transform.SetLocation(m_fishesPositions[0][i]);
        FVector direction = m_fishesVels[0][i];
        direction.Normalize();
        transform.SetRotation(FRotationMatrix::MakeFromX(direction).Rotator().Add(0.f, -90.f, 0.f).Quaternion());
        m_instancedStaticMeshComponent->UpdateInstanceTransform(i, transform, false, false);
    }
    m_instancedStaticMeshComponent->MarkRenderStateDirty();

    // UPDATE [PREV] <- [CUR]
    Swap(m_currentStatesIndex, m_previousStatesIndex);
}
#else
void AFishAgent::cpuCalculate(float DeltaTime, bool isSingleThread)
{
	float kCoh = m_kCohesion, kSep = m_kSeparation, kAlign = m_kAlignment;
	float rCohesion = m_radiusCohesion, rSeparation = m_radiusSeparation, rAlignment = m_radiusAlignment;
	float maxAccel = m_maxAcceleration;
	float maxVel = m_maxVelocity;
	FVector mapSz = m_mapSize;
	static const int32 cnt = m_fishNum;

	int currentStatesIndex = m_currentStatesIndex;
	int previousStatesIndex = m_previousStatesIndex;
	
    ParallelFor(cnt, [this, currentStatesIndex, previousStatesIndex, kCoh, kSep, kAlign, rCohesion, rSeparation, rAlignment, maxAccel, maxVel, mapSz, DeltaTime, isSingleThread](int32 fishNum) {
		FVector cohesion(FVector::ZeroVector), separation(FVector::ZeroVector), alignment(FVector::ZeroVector);
		int32 cohesionCnt = 0, separationCnt = 0, alignmentCnt = 0;
		for (int i = 0; i < cnt; i++) {
			if (i != fishNum) {
                float distance = FVector::Distance(m_fishStates[i][previousStatesIndex]->position, m_fishStates[fishNum][previousStatesIndex]->position);
				if (distance > 0) {
					if (distance < rCohesion) {
                        cohesion += m_fishStates[i][previousStatesIndex]->position;
						cohesionCnt++;
					}
					if (distance < rSeparation) {
                        separation += m_fishStates[i][previousStatesIndex]->position - m_fishStates[fishNum][previousStatesIndex]->position;
						separationCnt++;
					}
					if (distance < rAlignment) {
                        alignment += m_fishStates[i][previousStatesIndex]->velocity;
						alignmentCnt++;
					}
				}
			}
		}

		if (cohesionCnt != 0) {
			cohesion /= cohesionCnt;
            cohesion -= m_fishStates[fishNum][previousStatesIndex]->position;
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

        m_fishStates[fishNum][currentStatesIndex]->acceleration = (cohesion * kCoh + separation * kSep + alignment * kAlign).GetClampedToMaxSize(maxAccel);
        m_fishStates[fishNum][currentStatesIndex]->acceleration.Z = 0;

        m_fishStates[fishNum][currentStatesIndex]->velocity += m_fishStates[fishNum][currentStatesIndex]->acceleration * DeltaTime;
        m_fishStates[fishNum][currentStatesIndex]->velocity  = m_fishStates[fishNum][currentStatesIndex]->velocity.GetClampedToMaxSize(maxVel);
        m_fishStates[fishNum][currentStatesIndex]->position += m_fishStates[fishNum][currentStatesIndex]->velocity * DeltaTime;
        m_fishStates[fishNum][currentStatesIndex]->velocity = checkMapRange(mapSz, m_fishStates[fishNum][currentStatesIndex]->position, m_fishStates[fishNum][currentStatesIndex]->velocity);
        //UE_LOG(LogTemp, Warning, TEXT("%d %d %s %s"), fishNum, currentStatesIndex, *m_fishStates[fishNum][currentStatesIndex]->position.ToString(), *m_fishStates[fishNum][currentStatesIndex]->velocity.ToString());
	}, isSingleThread);

	for (int i = 0; i < cnt; i++) {
		FHitResult hit(ForceInit);
        if (collisionDetected(m_fishStates[i][previousStatesIndex]->position, m_fishStates[i][currentStatesIndex]->position, hit)) {
            m_fishStates[i][currentStatesIndex]->position -= m_fishStates[i][currentStatesIndex]->velocity * DeltaTime;
            m_fishStates[i][currentStatesIndex]->velocity *= -1.0;
            m_fishStates[i][currentStatesIndex]->position += m_fishStates[i][currentStatesIndex]->velocity * DeltaTime;
		}
	}

	for (int i = 0; i < cnt; i++) {
		FTransform transform;
        m_instancedStaticMeshComponent->GetInstanceTransform(m_fishStates[i][0]->instanceId, transform);
        transform.SetLocation(m_fishStates[i][0]->position);
        FVector direction = m_fishStates[i][0]->velocity;
		direction.Normalize();
		transform.SetRotation(FRotationMatrix::MakeFromX(direction).Rotator().Add(0.f, -90.f, 0.f).Quaternion());
        m_instancedStaticMeshComponent->UpdateInstanceTransform(m_fishStates[i][0]->instanceId, transform, false, false);
	}

	m_instancedStaticMeshComponent->MarkRenderStateDirty();

    Swap(m_currentStatesIndex, m_previousStatesIndex);
}
#endif

bool AFishAgent::collisionDetected(const FVector &start, const FVector &end, FHitResult &hitResult)
{
	FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, this);
	RV_TraceParams.bTraceComplex = true;
	RV_TraceParams.bReturnPhysicalMaterial = false;

	return GetWorld()->SweepSingleByChannel(hitResult, start, end, FQuat(), ECC_WorldStatic, FCollisionShape::MakeSphere(200), RV_TraceParams);
}
