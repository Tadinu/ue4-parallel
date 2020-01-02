#include "GrpcServiceImp.h"
#include "ThreadingHelpers.h"
#include "FishAgent.h"

GrpcServiceImp::GrpcServiceImp(AFishAgent* InMeshActor):
                MeshActor(InMeshActor)
{}

grpc::Status GrpcServiceImp::SetActorTransform(grpc::ServerContext* context, const InstanceTransform* request, Empty* response)
{
    int32 instanceIndex = request->index();
    Vector3D rotation   = request->transform().rotation();
    Vector3D position   = request->transform().position();
    Vector3D scale      = request->transform().scale();
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

grpc::Status GrpcServiceImp::GetActorTransform(grpc::ServerContext* context, const InstanceIndex* request, Transform* response)
{
    int32 instanceIndex = request->index();
    //Retrieve the current transform for the actor on the game thread
    TFuture<FTransform> result = AsyncNamed<FTransform>(ENamedThreads::GameThread, [this, instanceIndex]() {
        return this->MeshActor->GetFishInstanceTransform(instanceIndex);
	});
	
	//Block the gRPC thread until the lamda completes
	FTransform transform = result.Get();
	
	//Copy the location vector into the response message
	FVector location = transform.GetLocation();
	response->mutable_position()->set_x(location.X);
	response->mutable_position()->set_y(location.Y);
	response->mutable_position()->set_z(location.Z);
	
	//Copy the rotation euler angles into the response message
	FVector eulerAngles = transform.GetRotation().Euler();
	response->mutable_rotation()->set_x(eulerAngles.X);
	response->mutable_rotation()->set_y(eulerAngles.Y);
	response->mutable_rotation()->set_z(eulerAngles.Z);
	
	//Copy the scale vector into the response message
	FVector scale = transform.GetScale3D();
	response->mutable_scale()->set_x(scale.X);
	response->mutable_scale()->set_y(scale.Y);
	response->mutable_scale()->set_z(scale.Z);
	
	return grpc::Status::OK;
}

grpc::Status GrpcServiceImp::ApplyForceToActor(grpc::ServerContext* context, const InstanceForce* request, Empty* response)
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
