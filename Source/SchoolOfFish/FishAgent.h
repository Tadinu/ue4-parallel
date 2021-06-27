// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Chaos/KinematicGeometryParticles.h"
#include "FishProcessing.h"
#include "SchoolOfFish.h"
#include "FishAgent.generated.h"

#if !FISH_ISPC
struct FishState {
	int32 instanceId;
	FVector position;
	FVector velocity;
	FVector acceleration;
};
#endif

UCLASS()
class SCHOOLOFFISH_API AFishAgent : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFishAgent();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
#if FISH_ISPC
    void cpuCalculate(float DeltaTime);
#else
    void cpuCalculate(float DeltaTime, bool isSingleThread);
#endif
	bool collisionDetected(const FVector &start, const FVector &end, FHitResult &hitResult);

private:
	// ~
	// General settings to compute flocking behaviour
    UPROPERTY()
    int32 m_fishNum = 0;            // total instances of fish
    UPROPERTY()
    FVector m_mapSize = FVector::ZeroVector;          // size of area where fish can flock
    UPROPERTY()
    float m_oceanFloorZ= 0.f;		// Z coordinate of ocean floor
    UPROPERTY()
    float m_spawningRange= 0.f;		// spawning range in persents of map size
    UPROPERTY()
    float m_maxVelocity= 0.f;        // maximum velocity of fish
    UPROPERTY()
    float m_maxAcceleration= 0.f;    // maximum acceleration of fish
    UPROPERTY()
    float m_radiusCohesion= 0.f;     // Cohesion radius. The radius inside which the fish will tend to inside the circle (approach)
    UPROPERTY()
    float m_radiusSeparation= 0.f;   // Separation radius. The radius within which the fish will tend to avoid collisions
    UPROPERTY()
    float m_radiusAlignment= 0.f;    // Alignment radius. The radius inside which the fish will tend to follow in one direction
	// Gain factors for the three types of fish behavior. By default  all three gain factors are equals 1.0f
    UPROPERTY()
    float m_kCohesion= 0.f;
    UPROPERTY()
    float m_kSeparation= 0.f;
    UPROPERTY()
    float m_kAlignment = 0.f;
	// ~

#if FISH_ISPC
    //TKinematicGeometryParticles<float, 3, EGeometryParticlesSimType::RigidBody> m_fishStates;
    TArray<TArray<FVector>> m_fishesPositions;
    TArray<TArray<FVector>> m_fishesVels;
    TArray<TArray<FVector>> m_fishesAccels;
#else
	// Array of fish states if flocking behaviour calculates on CPU
    TArray<TArray<TSharedPtr<FishState>>> m_fishStates;
#endif

	// index of fish states array where stored current states of fish
    UPROPERTY()
    int32 m_currentStatesIndex = 0;

	// index of fish states array where stored previous states of fish
    UPROPERTY()
    int32 m_previousStatesIndex = 1;

	// Array of fish states if flocking behaviour calculates on GPU
	TArray<State> m_gpuFishStates;

	// Single or multithreaded algorithm. CPU only.
    UPROPERTY()
	bool m_isCpuSingleThread;
	
	// This flag indicates where fish state should be calculated. On GPU or on CPU. CPU - by default
    UPROPERTY()
    bool m_isGpu = false;

	// time elapsed from last calculation
    UPROPERTY()
	float m_elapsedTime = 0.f;

	// Fish static mesh object
    UPROPERTY()
    UStaticMesh *m_staticMesh = nullptr;

	// Fish instanced static mesh component. This component contains all of the fish instances on the scene
    UPROPERTY()
    UInstancedStaticMeshComponent* m_instancedStaticMeshComponent = nullptr;

	// Pointer to the class FishProcessing which uses compute shader plugin to calculate flocking behaviour on GPU
    TUniquePtr<FishProcessing> m_gpuProcessing = nullptr;
};
