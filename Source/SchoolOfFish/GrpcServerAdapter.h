#pragma once

#include <grpc++/grpc++.h>
#include "CoreMinimal.h"
#include "GrpcActorOperationService.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMeshActor.h"
#include "GrpcServerAdapter.generated.h"

typedef grpc::Server GrpcServer;

class AFishAgent;

UCLASS()
class SCHOOLOFFISH_API AGrpcServerAdapter : public AActor
{
	GENERATED_BODY()

    AGrpcServerAdapter();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	public:
        void StartRPC(AFishAgent* Actor);
		
        //The actor that we are controlling
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ActorOperation)
        AFishAgent* MeshActor = nullptr;
		
		//The TCP port that the gRPC server will bind to
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ActorOperation)
        int port = 50051;
	
        void SetActorTransform(int32 Id, const FTransform& Transform);
		//The gRPC service implementation and server object
        TUniquePtr<GrpcActorOperationService> GRPCService;
        TUniquePtr<GrpcServer> GRPCServer;
};
