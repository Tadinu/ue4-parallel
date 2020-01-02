#include "GrpcServerWrapper.h"
#include "FishAgent.h"

AGrpcServerWrapper::AGrpcServerWrapper()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AGrpcServerWrapper::BeginPlay()
{
	Super::BeginPlay();
}

void AGrpcServerWrapper::StartActorRPC(AFishAgent* Actor)
{
    MeshActor = Actor;

    //If we have an actor to control, start the gRPC server
    if (this->MeshActor)
    {
        //Instantiate our gRPC service implementation
        this->GRPCService.Reset(new GrpcServiceImp(this->MeshActor));

        //Create and start the gRPC server
        grpc::ServerBuilder builder;
        builder.AddListeningPort("0.0.0.0:" + std::to_string(this->port), grpc::InsecureServerCredentials());
        builder.RegisterService(this->GRPCService.Get());
        this->GRPCServer.Reset(builder.BuildAndStart().release());
    }
}

void AGrpcServerWrapper::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	//If the gRPC server was running, stop it
	if (this->GRPCServer.IsValid() == true)
	{
		this->GRPCServer->Shutdown();
		this->GRPCServer.Reset(nullptr);
	}
}
