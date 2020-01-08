#pragma once

#include "CoreMinimal.h"
#include "Conduit.h"
#include "GenUtils.h"
#include "RpcClient.h"
#include "GrpcClientAdapter.generated.h"

// Enums:
// Structures:
USTRUCT()
struct FActorInstanceIndex
{
    GENERATED_BODY()

public:
    // Conduits and GRPC stub
    UPROPERTY(Transient)
    int32 id = 0;
};

USTRUCT()
struct FActorInstanceTransform
{
    GENERATED_BODY()

public:
    // Conduits and GRPC stub
    UPROPERTY(Transient)
    int32 id = 0;

    UPROPERTY(Transient)
    FTransform transf = FTransform::Identity;
};

USTRUCT()
struct FGRPCEmpty
{
    GENERATED_BODY()
};

USTRUCT()
struct FActorInstanceInfo
{
    GENERATED_BODY()

public:
    // Conduits and GRPC stub
    UPROPERTY()
    int32 instanceIndex = 0;
    UPROPERTY()
    FVector position = FVector::ZeroVector;
    UPROPERTY()
    FVector velocity = FVector::ZeroVector;
    UPROPERTY()
    FVector acceleration = FVector::ZeroVector;

    UPROPERTY()
    FVector steerCohesion = FVector::ZeroVector;
    UPROPERTY()
    FVector steerSeparation = FVector::ZeroVector;
    UPROPERTY()
    FVector steerAlignmen = FVector::ZeroVector;

    UPROPERTY()
    int32 steerCohesionCnt = 0;
    UPROPERTY()
    int32 steerSeparationCnt = 0;
    UPROPERTY()
    int32 steerAlignmentCnt = 0;
};


USTRUCT()
struct FActorEnvironmentInfo
{
    GENERATED_BODY()

public:
    UPROPERTY()
    int32 instanceCount = 0;
    UPROPERTY()
    float radiusCohesion = 0.f;
    UPROPERTY()
    float radiusSeparation = 0.f;
    UPROPERTY()
    float radiusAlignment = 0.f;
    UPROPERTY()
    float mapRangeX = 0.f;
    UPROPERTY()
    float mapRangeY = 0.f;
    UPROPERTY()
    float mapRangeZ = 0.f;
    UPROPERTY()
    float kCohesion = 0.f;
    UPROPERTY()
    float kSeparation = 0.f;
    UPROPERTY()
    float kAlignment = 0.f;
    UPROPERTY()
    float maxAcceleration = 0.f;
    UPROPERTY()
    float maxVelocity = 0.f;
    UPROPERTY()
    int32 calculationsPerThread = 0;
    UPROPERTY()
    float DeltaTime = 0.f;
};

// Forward class definitions (for delegates)
class UActorOperationRpcClient;

// Dispatcher delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FEventGetActorTransform, UActorOperationRpcClient*, RpcClient, const FActorInstanceTransform&, Response, FGrpcStatus, Status);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FEventInformActorEnvironmentInfo, UActorOperationRpcClient*, RpcClient, const FGRPCEmpty&, Response, FGrpcStatus, Status);


UCLASS()
class UActorOperationRpcClient : public URpcClient
{
    GENERATED_BODY()

public:
    // Conduits and GRPC stub
    TConduit<TRequestWithContext<FActorInstanceInfo>, TResponseWithStatus<FActorInstanceTransform>> ConduitGetActorTransform;

    TConduit<TRequestWithContext<FActorEnvironmentInfo>, TResponseWithStatus<FGRPCEmpty>> ConduitInformActorEnvironmentInfo;

    UPROPERTY()
    FEventGetActorTransform EventGetActorTransform;

    UPROPERTY()
    FEventInformActorEnvironmentInfo EventInformActorEnvironmentInfo;


    // Methods
    void HierarchicalInit() override;

    void HierarchicalUpdate() override;

    UFUNCTION()
    bool RequestGetActorTransform(const FActorInstanceInfo& Request, const FGrpcClientContext& Context);

    UFUNCTION()
    bool InformActorEnvironmentInfo(const FActorEnvironmentInfo& Request, const FGrpcClientContext& Context);
};


