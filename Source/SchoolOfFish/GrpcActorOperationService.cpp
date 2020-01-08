#include "GrpcActorOperationService.h"
#include "ThreadingHelpers.h"
#include "FishAgent.h"

GrpcActorOperationService::GrpcActorOperationService(AFishAgent* InMeshActor):
                MeshActor(InMeshActor)
{}

grpc::Status GrpcActorOperationService::SetActorTransform(grpc::ServerContext* context,
                                                          const rpc::ActorInstanceTransform* request,
                                                          rpc::Empty* response)
{
    int32 instanceIndex = request->index();
    rpc::Vector3D rotation   = request->transform().rotation();
    rpc::Vector3D position   = request->transform().position();
    rpc::Vector3D scale      = request->transform().scale();
    FTransform actorTransform(FRotator(rotation.x(), rotation.y(), rotation.z()),
                              FVector(position.x(), position.y(), position.z()),
                              FVector(scale.x(), scale.y(), scale.z()));

    //Reset the actor's transform on the game thread
    TFuture<void> result = AsyncNamed<void>(ENamedThreads::GameThread, [this, instanceIndex, actorTransform]()
	{
        this->MeshActor->SetFishInstanceTransform(instanceIndex, actorTransform);
	});
	
	//Block the gRPC thread until the lamda completes
	result.Get();
	return grpc::Status::OK;
}

grpc::Status GrpcActorOperationService::GetActorTransform(grpc::ServerContext* context,
                                                          const rpc::ActorInstanceInfo* request,
                                                          rpc::ActorInstanceTransform* response)
{
    int32 instanceIndex = request->instanceindex();
    //Retrieve the current transform for the actor on the game thread
    TFuture<FTransform> result = AsyncNamed<FTransform>(ENamedThreads::GameThread, [this, instanceIndex]() {
        return this->MeshActor->GetFishInstanceTransform(instanceIndex);
	});
	
	//Block the gRPC thread until the lamda completes
	FTransform transform = result.Get();
	
	//Copy the location vector into the response message
	FVector location = transform.GetLocation();
    rpc::Transform* transf = response->mutable_transform();
    transf->mutable_position()->set_x(location.X);
    transf->mutable_position()->set_y(location.Y);
    transf->mutable_position()->set_z(location.Z);
	
	//Copy the rotation euler angles into the response message
	FVector eulerAngles = transform.GetRotation().Euler();
    transf->mutable_rotation()->set_x(eulerAngles.X);
    transf->mutable_rotation()->set_y(eulerAngles.Y);
    transf->mutable_rotation()->set_z(eulerAngles.Z);
	
	//Copy the scale vector into the response message
	FVector scale = transform.GetScale3D();
    transf->mutable_scale()->set_x(scale.X);
    transf->mutable_scale()->set_y(scale.Y);
    transf->mutable_scale()->set_z(scale.Z);
	
	return grpc::Status::OK;
}

grpc::Status GrpcActorOperationService::ApplyForceToActor(grpc::ServerContext* context, const rpc::ActorInstanceForce* request, rpc::Empty* response)
{
    //Apply the specified physics force to the actor on the game thread
    int32 instanceIndex = request->index();
    FVector force(request->force().x(), request->force().y(), request->force().z());
    TFuture<void> result = AsyncNamed<void>(ENamedThreads::GameThread, [this, instanceIndex, force]() {
        this->MeshActor->AddForceToFishInstance(instanceIndex, force);
	});
	
	//Block the gRPC thread until the lamda completes
	result.Get();
	return grpc::Status::OK;
}
