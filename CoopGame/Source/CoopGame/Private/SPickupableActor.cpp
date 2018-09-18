// Fill out your copyright notice in the Description page of Project Settings.

#include "SPickupableActor.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"


ASPickupableActor::ASPickupableActor()
{
	MeshComp->SetSimulatePhysics(true);

	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bReplicateMovement = true;
}

void ASPickupableActor::BeginPlay()
{
	Super::BeginPlay();

	// TODO: Remove Hack to workaround constructor call not currently working.
	MeshComp->SetSimulatePhysics(true);
}


void ASPickupableActor::OnUsed(APawn* InstigatorPawn)
{
	ASUsableActor::OnUsed(InstigatorPawn);

	UGameplayStatics::PlaySoundAtLocation( *this, PickupSound, GetActorLocation());
}