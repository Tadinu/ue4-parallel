#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "Proto_Generated/ActorOperation.grpc.pb.h"

class AFishAgent;

class GrpcServiceImp : public ActorOperation::Service
{
	public:
        GrpcServiceImp(AFishAgent* InMeshActor);
        virtual grpc::Status SetActorTransform(grpc::ServerContext* context, const InstanceTransform* request, Empty* response);
        virtual grpc::Status GetActorTransform(grpc::ServerContext* context, const InstanceIndex* request, Transform* response);
        virtual grpc::Status ApplyForceToActor(grpc::ServerContext* context, const InstanceForce* request, Empty* response);
		
	private:
        AFishAgent* MeshActor = nullptr;
};
