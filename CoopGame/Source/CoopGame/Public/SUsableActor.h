// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SUsableActor.generated.h"

UCLASS()
class COOPGAME_API ASUsableActor : public AActor
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, Category = "Mesh")
	UStaticMeshComponent* MeshComp;

public:	
	// Sets default values for this actor's properties
	ASUsableActor();

	/* Player is looking at */
	virtual void OnBeginFocus();

	/* Player is no longer looking at */
	virtual void OnEndFocus();

	/* Called when player interacts with object */
	virtual void OnUsed(APawn* InstigatorPawn);
	
};
