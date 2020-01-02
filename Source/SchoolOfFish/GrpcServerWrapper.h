#pragma once

#include <grpc++/grpc++.h>
#include "CoreMinimal.h"
#include "GrpcServiceImp.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMeshActor.h"
#include "GrpcServerWrapper.generated.h"

typedef grpc::Server GrpcServer;

class AFishAgent;

UCLASS()
class SCHOOLOFFISH_API AGrpcServerWrapper : public AActor
{
	GENERATED_BODY()

	AGrpcServerWrapper();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	public:
        void StartActorRPC(AFishAgent* Actor);
		
        //The actor that we are controlling
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ActorOperation)
        AFishAgent* MeshActor = nullptr;
		
		//The TCP port that the gRPC server will bind to
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ActorOperation)
        int port = 50051;
	
	private:
		
		//The gRPC service implementation and server object
        TUniquePtr<GrpcServiceImp> GRPCService;
        TUniquePtr<GrpcServer> GRPCServer;
};
