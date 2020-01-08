// Fill out your copyright notice in the Description page of Project Settings.

#include "FlockingGameMode.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "FishAgent.h"
#include "GrpcServerAdapter.h"
#include "GrpcClientAdapter.h"

void AFlockingGameMode::InitGameState()
{
    Super::InitGameState();
    FishAgent  = Cast<AFishAgent>(GetWorld()->SpawnActor(AFishAgent::StaticClass()));

    ChannelCredentials = UChannelCredentials::MakeInsecureChannelCredentials();
    GRPCClient = NewRpcClient<UActorOperationRpcClient>(TEXT("0.0.0.0:50051"), // or "localhost:50051"
                                                        ChannelCredentials,
                                                        this);

    GRPCClient->EventGetActorTransform.AddDynamic(FishAgent, &AFishAgent::OnFishTransformReturned);
}
