#include "GrpcServerAdapter.h"
#include "FishAgent.h"
#include "Proto_Generated/ActorOperation.grpc.pb.h"

AGrpcServerAdapter::AGrpcServerAdapter()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AGrpcServerAdapter::BeginPlay()
{
    Super::BeginPlay();
}

void AGrpcServerAdapter::StartRPC(AFishAgent* Actor)
{
    MeshActor = Actor;

    //If we have an actor to control, start the gRPC server
    if (this->MeshActor)
    {
        //Instantiate our gRPC service implementation
        this->GRPCService.Reset(new GrpcActorOperationService(this->MeshActor));

        //Create and start the gRPC server
        grpc::ServerBuilder builder;
        builder.AddListeningPort("0.0.0.0:" + std::to_string(this->port), grpc::InsecureServerCredentials());
        builder.RegisterService(this->GRPCService.Get());
        this->GRPCServer.Reset(builder.BuildAndStart().release());
        std::cout << "Server created!" << std::endl;
    }
}

void AGrpcServerAdapter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	//If the gRPC server was running, stop it
    if (this->GRPCServer.IsValid())
    {
        this->GRPCServer->Shutdown();
        this->GRPCServer.Reset(nullptr);
    }
}

void AGrpcServerAdapter::SetActorTransform(int32 Id, const FTransform& Transf)
{
    grpc::ServerContext context;
    rpc::ActorInstanceTransform transf;
    rpc::Empty empty;
    transf.set_index(Id);

    FRotator rotation = Transf.Rotator();
    FVector position  = Transf.GetLocation();
    FVector scale     = Transf.GetScale3D();

    *(transf.mutable_transform()->mutable_rotation()) = rpc::Vector3D(rotation.Pitch,
                                                                      rotation.Yaw,
                                                                      rotation.Roll);

    *(transf.mutable_transform()->mutable_position()) = rpc::Vector3D(position.X,
                                                                      position.Y,
                                                                      position.Z);

    *(transf.mutable_transform()->mutable_scale()) = rpc::Vector3D(scale.X,
                                                                   scale.Y,
                                                                   scale.Z);

    GRPCService->SetActorTransform(&context, &transf, &empty);
}
