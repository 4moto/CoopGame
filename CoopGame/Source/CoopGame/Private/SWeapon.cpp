// Fill out your copyright notice in the Description page of Project Settings.

#include "SWeapon.h"
#include "DrawDebugHelpers.h"
#include "../Public/CollisionQueryParams.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Components/MeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "CoopGame.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Public/TimerManager.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "SCharacter.h"


static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeapondDrawing(
	TEXT("COOP.DebugWeapons"),
	DebugWeaponDrawing,
	TEXT("Draw Debug Lines for Weapons 1 for lines, 2 for hits"),
	ECVF_Cheat);


// Sets default values
ASWeapon::ASWeapon()
{
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "Target";
	BaseDamage = 20.0f;
	RateOfFire = 600; //Bullets Per Minute
	BulletSpread = 0.0f;
	NumberOfBulletsPerShot = 1;
	ShotsFired = 0;

	RecoilMod = 1.0f;
	AimingSpreadMod = 0.33f;
	
	RecoilPitchUp = 0.5f;	
	RecoilPitchDown = 0.1f;	
	RecoilYawLeft = 0.05f;	
	RecoilYawRight = 0.25f;

	AI_BulletSpread = 5.0f;
	AI_BaseDamage = 10.0f;

	SetReplicates(true);

	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
}

// Called when the game starts or when spawned
void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	// @TODO Either here or in SCharacter - when moving add to or multiply BulletSpread by a number and feed it in when speed > value 
	// Get speed with FVector Speed = MyOwner->GetVelocity();

	// Bullet Spread 
	HalfRad = FMath::DegreesToRadians(BulletSpread);
	AI_HalfRad = FMath::DegreesToRadians(AI_BulletSpread);

	TimeBetweenShots = 60 / RateOfFire;
}


void ASWeapon::Fire()
{
	// Trace the world, from pawn eyes to crosshair location

	if (Role < ROLE_Authority)
	{
		ServerFire();
	}

	AActor* MyOwner = GetOwner();
	if (MyOwner)
	{
		ASCharacter* RecoilingCharacter = Cast<ASCharacter>(MyOwner);

		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector ShotDirection = EyeRotation.Vector();

		// Checks if controlled by a player
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetInstigatorController());

		// Add bullet spread if AI
		if (PC == nullptr)
		{
			ShotDirection = FMath::VRandCone(ShotDirection, AI_HalfRad, AI_HalfRad);
			BaseDamage = AI_BaseDamage;
		}
		else
		{
			if (RecoilingCharacter->bWantsToZoom)
			{
				HalfRad *= AimingSpreadMod;
			}
			else
			{
				ShotDirection = FMath::VRandCone(ShotDirection, HalfRad, HalfRad);
			}
		}

		FVector TraceEnd = EyeLocation + (ShotDirection * 10000);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;

		//Particle FX "Target" parameter -- pretty sure there's no reason for this and TraceEnd can just be used instead
		FVector TracerEndPoint = TraceEnd;

		EPhysicalSurface SurfaceType = SurfaceType_Default;

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, QueryParams))
		{
			//Blocking Hit! Process damage
			AActor* HitActor = Hit.GetActor();

			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			float ActualDamage = BaseDamage;
			if (SurfaceType == SURFACE_FLESHVULNERABLE)
			{
				ActualDamage *= CritMultiplier;
			}

			/* Using MyOwner instead of this SWeapon as damage causer to get it to work with friendly fire in SHealthComponent -- may need to be revisited */
			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), MyOwner, DamageType);

			PlayImpactEffect(SurfaceType, Hit.ImpactPoint);

			//Updates the particle "target" with the hit result
			TracerEndPoint = Hit.ImpactPoint;
//Not sure where this came from			HitScanTrace.SurfaceType = SurfaceType;
		}

		if (DebugWeaponDrawing == 1)
		{
			DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::White, false, 1.0f, 0, 1.0f);
			DrawDebugSphere(GetWorld(), TracerEndPoint, 4.0f, 12, FColor::Yellow, false, 3.0f, 0, 1.0f);
		}

		if (DebugWeaponDrawing > 0)
		{
			DrawDebugSphere(GetWorld(), TracerEndPoint, 4.0f, 12, FColor::Red, false, 3.0f, 0, 1.0f);
		}

		PlayFireEffect(TracerEndPoint);

		if (Role == ROLE_Authority)
		{
			HitScanTrace.TraceTo = TracerEndPoint;
			HitScanTrace.SurfaceType = SurfaceType;
		}
		
		// Apply Weapon recoil
		if (RecoilingCharacter)
		{
			RecoilingCharacter->Recoil(RecoilPitchUp, RecoilPitchDown, RecoilYawRight, RecoilYawRight);
		}
			
		LastFireTime = GetWorld()->TimeSeconds;
	}
	
	ShotsFired++;

//	UE_LOG(LogTemp, Log, TEXT("BulletsFired: %s of %s"), *FString::SanitizeFloat(ShotsFired), *FString::SanitizeFloat(NumberOfBulletsPerShot));

	if (NumberOfBulletsPerShot > ShotsFired)
	{
		Fire();
	}
	else
	{
		ShotsFired = 0;
	}

}


void ASWeapon::StartFire()
{
	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);

	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay);

	/* Include this if you want to allow semi-auto to fire as fast as you can click! */
	//	Fire();

}


void ASWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}


void ASWeapon::ServerFire_Implementation()
{
	Fire();
}

bool ASWeapon::ServerFire_Validate()
{
	return true;
}


void ASWeapon::OnRep_HitScanTrace()
{
	// Play cosmetic FX
	PlayFireEffect(HitScanTrace.TraceTo);
	PlayImpactEffect(HitScanTrace.SurfaceType, HitScanTrace.TraceTo);
}


void ASWeapon::PlayFireEffect(FVector TraceEnd)
{
	if (MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
	}

	FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

	if (TracerEffect)
	{
		UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);
		if (TracerComp)
		{
			TracerComp->SetVectorParameter(TracerTargetName, TraceEnd);
		}
	}

	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner)
	{
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());
		if (PC)
		{
			PC->ClientPlayCameraShake(FireCamShake);
		}
	}

}

void ASWeapon::PlayImpactEffect(EPhysicalSurface SurfaceType, FVector ImpactPoint)
{
	UParticleSystem* SelectedEffect = nullptr;
	switch (SurfaceType)
	{
	case SURFACE_FLESHDEFAULT:
	case SURFACE_FLESHVULNERABLE:
		SelectedEffect = FleshImpactEffect;
		break;
	default:
		SelectedEffect = DefaultImpactEffect;
		break;
	}

	if (SelectedEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		FVector ShotDirection = ImpactPoint - MuzzleLocation;
		ShotDirection.Normalize();

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());
	}
}


void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner);
}