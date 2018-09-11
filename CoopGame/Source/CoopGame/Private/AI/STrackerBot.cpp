// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/AI/STrackerBot.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Public/NavigationSystem.h"
#include "Public/NavigationPath.h"

// Sets default values
ASTrackerBot::ASTrackerBot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCanEverAffectNavigation(false);
	RootComponent = MeshComp;
}

// Called when the game starts or when spawned
void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();
	
}

FVector ASTrackerBot::GetNextPathPoint()
{
	//Hacky cheat that won't hold up in multiplayer
//	ACharacter* PlayerPawn = UGameplayStatics::GetPlayerCharacter(this, 0);
	ACharacter* PlayerPawn = nullptr;
	
	UNavigationPath* NavPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), PlayerPawn);

	/*
	if (NavPath->PathPoints.Num() > 1)
	{
		// Return the next point in the path
		return NavPath->PathPoints[1];
	}
	*/
	
	//Failed to find path
	return GetActorLocation();
}

// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

