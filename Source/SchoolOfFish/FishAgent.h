// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "FishShaderProcessing.h"

#include <memory>
#include "FishAgent.generated.h"

#define USING_GPU_COMPUTER_SHADER (0)

USTRUCT()
struct SCHOOLOFFISH_API FFishState
{
    GENERATED_BODY()

    int32 instanceIndex;
	FVector position;
	FVector velocity;
	FVector acceleration;
};

UCLASS()
class SCHOOLOFFISH_API AFishAgent : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
    AFishAgent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

    void CreateFishMeshComponent(UStaticMesh* FishMesh);
    bool SetFishInstanceTransform(int32 InstanceIndex, const FTransform& FishTransform);
    FTransform GetFishInstanceTransform(int32 InstanceIndex);
    bool AddForceToFishInstance(int32 InstanceIndex, const FVector& Force);
    FORCEINLINE void SetFishUpdateRequest(bool bInUpdateNeeded) { bUpdateNeeded = bInUpdateNeeded; }
    FORCEINLINE bool IsFishUpdateNeeded() { return bUpdateNeeded; }
protected:
    void cpuCalculate(TArray<TArray<FFishState>>& agents, float DeltaTime, bool isSingleThread);
	bool collisionDetected(const FVector &start, const FVector &end, FHitResult &hitResult);
	void swapFishStatesIndexes();

private:
    FRandomStream randomGen;
	// ~
	// General settings to compute flocking behaviour
    UPROPERTY()
    int32 m_fishNum;            // total instances of fish

    UPROPERTY()
    bool bUpdateNeeded = false;

    UPROPERTY()
	FVector m_mapSize;          // size of area where fish can flock
    UPROPERTY()
	float m_oceanFloorZ;		// Z coordinate of ocean floor
    UPROPERTY()
	float m_spawningRange;		// spawning range in persents of map size
    UPROPERTY()
	float m_maxVelocity;        // maximum velocity of fish
    UPROPERTY()
	float m_maxAcceleration;    // maximum acceleration of fish
    UPROPERTY()
	float m_radiusCohesion;     // Cohesion radius. The radius inside which the fish will tend to inside the circle (approach) 
    UPROPERTY()
	float m_radiusSeparation;   // Separation radius. The radius within which the fish will tend to avoid collisions 
    UPROPERTY()
	float m_radiusAlignment;    // Alignment radius. The radius inside which the fish will tend to follow in one direction
	// Gain factors for the three types of fish behavior. By default  all three gain factors are equals 1.0f
    UPROPERTY()
	float m_kCohesion;          
    UPROPERTY()
	float m_kSeparation;
    UPROPERTY()
	float m_kAlignment;
	// ~

	// Array of fish states if flocking behaviour calculates on CPU    
    TArray<TArray<FFishState>> m_fishStates;

	// index of fish states array where stored current states of fish
    UPROPERTY()
	int32 m_currentStatesIndex;

	// index of fish states array where stored previous states of fish
    UPROPERTY()
    int32 m_previousStatesIndex;

	// Array of fish states if flocking behaviour calculates on GPU
    TArray<State> m_gpuFishStates;

	// Single or multithreaded algorithm. CPU only.
    UPROPERTY()
	bool m_isCpuSingleThread;
	
	// This flag indicates where fish state should be calculated. On GPU or on CPU. CPU - by default
    UPROPERTY()
	bool m_isGpu;

	// time elapsed from last calculation
    UPROPERTY()
	float m_elapsedTime = 0.f;

	// Fish instanced static mesh component. This component contains all of the fish instances on the scene
    // A component that efficiently renders multiple instances of the same StaticMesh
    UPROPERTY()
    UInstancedStaticMeshComponent *m_instancedStaticMeshComponent = nullptr;

    UPROPERTY()
    UStaticMesh* m_staticMesh = nullptr;

	// Pointer to the class FishProcessing which uses compute shader plugin to calculate flocking behaviour on GPU
    std::unique_ptr<FishShaderProcessing> m_gpuProcessing = nullptr;
};
