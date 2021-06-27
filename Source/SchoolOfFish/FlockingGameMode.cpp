// Fill out your copyright notice in the Description page of Project Settings.

#include "FlockingGameMode.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "FishAgent.h"
#include "SchoolOfFish.h"

void AFlockingGameMode::InitGameState()
{
	Super::InitGameState();
	GetWorld()->SpawnActor(agent_BP);
#if FISH_ISPC
    UE_LOG(LogTemp, Warning, TEXT("FISH_ISPC ENABLED!"));
#else
    UE_LOG(LogTemp, Warning, TEXT("FISH_ISPC DISABLED!"));
#endif
}
