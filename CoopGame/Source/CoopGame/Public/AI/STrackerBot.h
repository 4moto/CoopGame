// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "STrackerBot.generated.h"

class UStaticMeshComponent;

UCLASS()
class COOPGAME_API ASTrackerBot : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ASTrackerBot();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	UStaticMeshComponent* MeshComp;

	FVector GetNextPathPoint();

	//Next point in navigation path
	FVector NextPathPoint;

	UPROPERTY(EditDefaultsOnly, Category = "TracketBot")
	float MovementForce;

	UPROPERTY(EditDefaultsOnly, Category = "TracketBot")
	float RequiredDistanceToTarget;

	UPROPERTY(EditDefaultsOnly, Category = "TracketBot")
	bool bUseVelocityChange;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	
};
