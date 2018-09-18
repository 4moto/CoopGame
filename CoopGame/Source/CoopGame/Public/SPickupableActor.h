// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SUsableActor.h"
#include "SPickupableActor.generated.h"

class USoundCue;

/**
 * 
 */
UCLASS()
class COOPGAME_API ASPickupableActor : public ASUsableActor
{
	GENERATED_BODY()

public:
	
	void BeginPlay() override;

	void OnUsed(APawn* InstigatorPawn) override;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
		USoundCue* PickupSound;
	
	
};
