// Fill out your copyright notice in the Description page of Project Settings.

#include "SCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "../Public/Components/SHealthComponent.h"
#include "Public/TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "Public/SWeapon.h"
#include "CoopGame.h"


// Sets default values
ASCharacter::ASCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->bEnableCameraLag = true;
	SpringArmComp->CameraLagSpeed = 15;

	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));

	ZoomedFOV = 50;
	ZoomedInterpSpeed = 16;

	RecoilMod = 1;
	RecoilApplyRate = 0.1f;
	RecoilDamping = 1.0f;

	WeaponAttachSocketName = "WeaponSocket";
}

// Called when the game starts or when spawned
void ASCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	DefaultFOV = CameraComp->FieldOfView;
	HealthComp->OnHealthChanged.AddDynamic(this, &ASCharacter::OnHealthChanged);

	if (Role == ROLE_Authority)
	{
		//Spawn a default weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		CurrentWeapon = GetWorld()->SpawnActor<ASWeapon>(StarterWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (CurrentWeapon)
		{
			CurrentWeapon->SetOwner(this);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocketName);
		}
	}		
}


void ASCharacter::Recoil(float RecoilPitchUp, float RecoilPitchDown, float RecoilYawRight, float RecoilYawLeft)
{
	bIsRecoiling = true;

	FinalRecoilPitch += FMath::FRandRange(RecoilPitchDown, -RecoilPitchUp) * RecoilMod;
	FinalRecoilYaw += FMath::FRandRange(-RecoilYawLeft, RecoilYawRight) * RecoilMod;

	FTimerHandle TimerHandle_Recoil;
	FTimerDelegate TimerDel;

	TimerDel.BindUFunction(this, FName("StartRecoiling"), FinalRecoilPitch, FinalRecoilYaw);
	GetWorldTimerManager().SetTimer(TimerHandle_Recoil, TimerDel, RecoilApplyRate, true, 0.0);

// Attempts at doing the above logic
//	FTimerDelegate = FTimerDelegate::CreateUObject( this, StartRecoiling, FinalRecoilPitch, FinalRecoilYaw);
//	GetWorldTimerManager().SetTimer(TimerHandle_Recoil, this, &ASCharacter::StartRecoiling(FinalRecoilPitch, FinalRecoilYaw), RecoilApplyRate, true, 0.0f);
}


void ASCharacter::StartRecoiling(float RecoilPitch, float RecoilYaw)
{
	float PartialRecoilPitch;
	float PartialRecoilYaw;

	PartialRecoilPitch = RecoilPitch * (RecoilApplyRate / RecoilTime);
	PartialRecoilYaw = RecoilYaw * (RecoilApplyRate / RecoilTime);

	AddControllerPitchInput(PartialRecoilPitch);
	AddControllerYawInput(PartialRecoilYaw);

	FinalRecoilPitch -= RecoilDamping;
	FinalRecoilYaw -= RecoilDamping;
}


void ASCharacter::StopRecoiling()
{
	bIsRecoiling = false;
}



// Called every frame
void ASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float TargetFOV = bWantsToZoom ? ZoomedFOV : DefaultFOV;

	float NewFOV = FMath::FInterpTo(CameraComp->FieldOfView, TargetFOV, DeltaTime, ZoomedInterpSpeed);

	CameraComp->SetFieldOfView(NewFOV);

}


void ASCharacter::StartFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFire();
	}
}


void ASCharacter::StopFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}


// Adds the float to the recoil multiplier (0 = no impact)
void ASCharacter::SetRecoilMod(float RecoilModifier)
{
	RecoilMod += RecoilModifier;
}


void ASCharacter::BeginZoom()
{
	bWantsToZoom = true;
	SpringArmComp->CameraLagSpeed = 20;
	
	// Reduce recoil when zooming
	RecoilMod += -0.5f;

}


void ASCharacter::EndZoom()
{
	bWantsToZoom = false;
	SpringArmComp->CameraLagSpeed = 15;
	
	// Reduce recoil when zooming
	RecoilMod += 0.5f;
}


void ASCharacter::MoveForward(float Value)
{
	AddMovementInput(GetActorForwardVector() * Value);
}


void ASCharacter::MoveRight(float Value)
{
	AddMovementInput(GetActorRightVector() * Value);
}


void ASCharacter::BeginCrouch()
{
	Crouch();
}


void ASCharacter::EndCrouch()
{
	UnCrouch();
}

void ASCharacter::OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const UDamageType * DamageType, AController * InstigatedBy, AActor * DamageCauser)
{
	if (Health <= 0.0f && !bDied)
	{
		// Die!
		bDied = true;

		GetMovementComponent()->StopMovementImmediately();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		DetachFromControllerPendingDestroy();

		SetLifeSpan(10.0f);
	}
}

// Called to bind functionality to input
void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacter::MoveRight);

	PlayerInputComponent->BindAxis("LookUp", this, &ASCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookRight", this, &ASCharacter::AddControllerYawInput);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASCharacter::EndCrouch);

	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &ASCharacter::BeginZoom);
	PlayerInputComponent->BindAction("Zoom", IE_Released, this, &ASCharacter::EndZoom);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASCharacter::StopFire);

}


FVector ASCharacter::GetPawnViewLocation() const
{
	if (CameraComp)
	{
		return CameraComp->GetComponentLocation();
	}

	return Super::GetPawnViewLocation();
}


void ASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCharacter, CurrentWeapon);
	DOREPLIFETIME(ASCharacter, bDied);
}