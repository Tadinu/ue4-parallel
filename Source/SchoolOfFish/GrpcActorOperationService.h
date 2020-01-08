#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "Proto_Generated/ActorOperation.grpc.pb.h"

class AFishAgent;

class GrpcActorOperationService : public rpc::ActorOperation::Service
{
	public:
        GrpcActorOperationService(AFishAgent* InMeshActor);
        virtual grpc::Status SetActorTransform(grpc::ServerContext* context, const rpc::ActorInstanceTransform* request, rpc::Empty* response);
        virtual grpc::Status GetActorTransform(grpc::ServerContext* context, const rpc::ActorInstanceInfo* request, rpc::ActorInstanceTransform* response);
        virtual grpc::Status ApplyForceToActor(grpc::ServerContext* context, const rpc::ActorInstanceForce* request, rpc::Empty* response);
		
	private:
        AFishAgent* MeshActor = nullptr;
};
