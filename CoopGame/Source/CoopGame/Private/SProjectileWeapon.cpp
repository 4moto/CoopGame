	// Fill out your copyright notice in the Description page of Project Settings.

#include "SProjectileWeapon.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "CoopGame.h"

void ASProjectileWeapon::Fire()
{
	AActor* MyOwner = GetOwner();
	if (MyOwner && ProjectileClass)
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		// FRotator MuzzleRotation = MeshComp->GetSocketRotation(MuzzleSocketName);

		FVector ShotDirection = EyeRotation.Vector();
		FVector TraceEnd = EyeLocation + (ShotDirection * 2000);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		
		FHitResult Hit;
		GetWorld()->LineTraceSingleByChannel(Hit, MuzzleLocation, TraceEnd, COLLISION_WEAPON, QueryParams);

	//	Debug LineTrace
	//		DrawDebugLine(GetWorld(), MuzzleLocation, TraceEnd, FColor::White, false, 1.0f, 0, 1.0f);

		FVector TargetLocation = Hit.TraceEnd;

		ShotDirection = (TargetLocation - MuzzleLocation);
		ShotDirection.Normalize();	

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, ShotDirection.Rotation(), SpawnParams);

	}
}
