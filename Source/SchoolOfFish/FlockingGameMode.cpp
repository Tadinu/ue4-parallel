// Fill out your copyright notice in the Description page of Project Settings.

#include "FlockingGameMode.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "FishAgent.h"
#include "GrpcServerWrapper.h"

void AFlockingGameMode::InitGameState()
{
	Super::InitGameState();
    AFishAgent* fishAgent                 = Cast<AFishAgent>(GetWorld()->SpawnActor(AFishAgent::StaticClass()));
    AGrpcServerWrapper* grpcServerWrapper = Cast<AGrpcServerWrapper>(GetWorld()->SpawnActor(AGrpcServerWrapper::StaticClass()));
    grpcServerWrapper->StartActorRPC(fishAgent);
}
