// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "FlockingGameMode.generated.h"


class AFishAgent;
class UActorOperationRpcClient;
class UChannelCredentials;
UCLASS()
class SCHOOLOFFISH_API AFlockingGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void InitGameState() override;

    UPROPERTY()
    AFishAgent* FishAgent = nullptr;

    UPROPERTY()
    UActorOperationRpcClient* GRPCClient = nullptr;

    UPROPERTY()
    UChannelCredentials* ChannelCredentials = nullptr;
};
