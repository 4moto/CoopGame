// Fill out your copyright notice in the Description page of Project Settings.

#include "SCharacter.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "../Public/Components/SHealthComponent.h"
#include "Public/TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "Public/SWeapon.h"
#include "SUsableActor.h"
#include "Engine/World.h"
#include "CoopGame.h"


// Sets default values
ASCharacter::ASCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->bEnableCameraLag = false;
	SpringArmComp->CameraLagSpeed = 15;

	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));

	AimingSpeedMod = 0.5f;
	AimingFOV = 50;
	AimingInterpSpeed = 16;

	RecoilMod = 1;
	RecoilTime = 0.1f;
	RecoilApplyRate = 0.02f;
	RecoilDamping = 0.01f;

	WeaponAttachSocketName = "WeaponSocket";

	MaxUseDistance = 800;
	bHasNewFocus = true;
	SprintingSpeedMod = 2.5f;
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

// Called every frame
void ASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float TargetFOV = bIsAiming ? AimingFOV : DefaultFOV;

	float NewFOV = FMath::FInterpTo(CameraComp->FieldOfView, TargetFOV, DeltaTime, AimingInterpSpeed);

	CameraComp->SetFieldOfView(NewFOV);

	if (bWantsToRun && !IsSprinting())
	{
		SetSprinting(true);
	}

	if (Controller && Controller->IsLocalController())
	{
		ASUsableActor* Usable = GetUsableInView();

		// End Focus
		if (FocusedUsableActor != Usable)
		{
			if (FocusedUsableActor)
			{
				FocusedUsableActor->OnEndFocus();
			}

			bHasNewFocus = true;
		}

		// Assign new Focus
		FocusedUsableActor = Usable;

		// Start Focus.
		if (Usable)
		{
			if (bHasNewFocus)
			{
				Usable->OnBeginFocus();
				bHasNewFocus = false;
			}
		}
	}	
}


void ASCharacter::SetSprinting(bool NewSprinting)
{
	bWantsToRun = NewSprinting;

	if (bIsCrouched)
		UnCrouch();

	// TODO: Stop weapon fire

	if (Role < ROLE_Authority)
	{
		ServerSetSprinting(NewSprinting);
	}
}


void ASCharacter::OnStartSprinting()
{
	SetSprinting(true);
}


void ASCharacter::OnStopSprinting()
{
	SetSprinting(false);
}


void ASCharacter::ServerSetSprinting_Implementation(bool NewSprinting)
{
	SetSprinting(NewSprinting);
}


bool ASCharacter::ServerSetSprinting_Validate(bool NewSprinting)
{
	return true;
}


bool ASCharacter::IsSprinting() const
{
	if (!GetCharacterMovement())
		return false;

	return (bWantsToRun && !IsAiming() && !GetVelocity().IsZero()
		// Don't allow sprint while strafing sideways or standing still (1.0 is straight forward, -1.0 is backward while near 0 is sideways or standing still)
		&& ((GetVelocity().GetSafeNormal2D() | GetActorRotation().Vector()) > 0.8)); // Changing this value to 0.1 allows for diagonal sprinting. (holding W+A or W+D keys)
}


float ASCharacter::GetSprintingSpeedModifier() const
{
	return SprintingSpeedMod;
}


void ASCharacter::Recoil(float RecoilPitchUp, float RecoilPitchDown, float RecoilYawRight, float RecoilYawLeft)
{
	bIsRecoiling = true;

	FinalRecoilPitch = FMath::FRandRange(-RecoilPitchUp, RecoilPitchDown) * RecoilMod;
	FinalRecoilYaw = FMath::FRandRange(-RecoilYawLeft, RecoilYawRight) * RecoilMod;

	FTimerDelegate TimerDel;

	TimerDel.BindUFunction(this, FName("StartRecoiling"), FinalRecoilPitch, FinalRecoilYaw);
	GetWorldTimerManager().SetTimer(TimerHandle_Recoil, TimerDel, RecoilApplyRate, true, 0.0);
	GetWorldTimerManager().SetTimer(TimerHandle_StopRecoil, this, &ASCharacter::StopRecoiling, RecoilTime, false, RecoilTime);


	/* Logging Recoil Events
	UE_LOG(LogTemp, Log, TEXT("Final Recoil Pitch and Yaw: %s of %s"), *FString::SanitizeFloat(FinalRecoilPitch), *FString::SanitizeFloat(FinalRecoilYaw));
	UE_LOG(LogTemp, Log, TEXT("Recoil Pitch and Up/Down: %s of %s"), *FString::SanitizeFloat(RecoilPitchUp), *FString::SanitizeFloat(RecoilPitchDown));
	UE_LOG(LogTemp, Log, TEXT("Recoil Yaw and Left/Right: %s of %s"), *FString::SanitizeFloat(RecoilYawLeft), *FString::SanitizeFloat(RecoilYawRight));
	*/

// Attempts at doing the above logic
//	FTimerDelegate = FTimerDelegate::CreateUObject( this, StartRecoiling, FinalRecoilPitch, FinalRecoilYaw);
//	GetWorldTimerManager().SetTimer(TimerHandle_Recoil, this, &ASCharacter::StartRecoiling(FinalRecoilPitch, FinalRecoilYaw), RecoilApplyRate, true, 0.0f);
}


void ASCharacter::StartRecoiling()
{
	float PartialRecoilPitch;
	float PartialRecoilYaw;

	PartialRecoilPitch = (FinalRecoilPitch + DampedPitch) * (RecoilApplyRate / RecoilTime);
	PartialRecoilYaw = (FinalRecoilYaw + DampedYaw) * (RecoilApplyRate / RecoilTime);

	AddControllerPitchInput(PartialRecoilPitch);
	AddControllerYawInput(PartialRecoilYaw);

//	UE_LOG(LogTemp, Log, TEXT("PartialRecoil Pitch and Yaw: %s of %s"), *FString::SanitizeFloat(PartialRecoilPitch), *FString::SanitizeFloat(PartialRecoilYaw));

	//Damp out recoil
	if (PartialRecoilPitch > 0)
	{
		DampedPitch -= RecoilDamping;
	}
	else
	{
		DampedPitch += RecoilDamping;
	}
	
	if (PartialRecoilYaw > 0)
	{
		DampedYaw -= RecoilDamping;
	}
	else
	{
		DampedYaw += RecoilDamping;
	}
//	UE_LOG(LogTemp, Log, TEXT("Final Recoil Pitch and Yaw: %s of %s"), *FString::SanitizeFloat(FinalRecoilPitch), *FString::SanitizeFloat(FinalRecoilYaw));
//	UE_LOG(LogTemp, Log, TEXT("Damped Pitch and Yaw: %s of %s"), *FString::SanitizeFloat(DampedPitch), *FString::SanitizeFloat(DampedYaw));
}


void ASCharacter::Use()
{
	// Only allow on server. If called on client push this request to the server
	if (Role == ROLE_Authority)
	{
		ASUsableActor* Usable = GetUsableInView();
		if (Usable)
		{
			Usable->OnUsed(this);
		}
	}
	else
	{
		ServerUse();
	}
}


/* bPerforms ray-trace to find closest looked-at UsableActor.*/
ASUsableActor * ASCharacter::GetUsableInView()
{
	FVector CamLoc;
	FRotator CamRot;

	if (Controller == nullptr)
	{
		return nullptr;
	}

	/* This retrieves the camera point of view to find the start and direciton to trace */
	Controller->GetPlayerViewPoint(CamLoc, CamRot);
	const FVector TraceStart = CamLoc;
	const FVector Direction = CamRot.Vector();
	const FVector TraceEnd = TraceStart + (Direction * MaxUseDistance);

	FCollisionQueryParams TraceParams(FName(TEXT("TraceUsableActor")), true, this);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = false;
	TraceParams.bTraceComplex = true;

	/* FHitResults is passed in with the trace function and holds the results of the trace */
	FHitResult Hit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, TraceParams);

//	DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 1.0f);

	return Cast<ASUsableActor>(Hit.GetActor());
}


void ASCharacter::ServerUse_Implementation()
{
	Use();
}


bool ASCharacter::ServerUse_Validate()
{
	return true;
}


void ASCharacter::SetAiming(bool NewAiming)
{
	bIsAiming = NewAiming;

	if (Role < ROLE_Authority)
	{
		ServerSetAiming(NewAiming);
	}

}

void ASCharacter::ServerSetAiming_Implementation(bool NewAiming)
{
	SetAiming(NewAiming);
}

bool ASCharacter::ServerSetAiming_Validate(bool NewAiming)
{
	return true;
}

void ASCharacter::BeginAiming()
{
	SetAiming(true);
	SpringArmComp->CameraLagSpeed = 20;

	GetCharacterMovement()->MaxWalkSpeed *= AimingSpeedMod;

	// Reduce recoil when zooming
	RecoilMod += -0.5f;
}


void ASCharacter::EndAiming()
{
	SetAiming(false);
	SpringArmComp->CameraLagSpeed = 15;

	GetCharacterMovement()->MaxWalkSpeed /= AimingSpeedMod;

	// Reduce recoil when zooming
	RecoilMod += 0.5f;
}

bool ASCharacter::IsAiming() const
{
	return bIsAiming;
}

float ASCharacter::GetAimingSpeedModifier() const
{
	return AimingSpeedMod;
}


FRotator ASCharacter::GetAimOffsets() const
{
	const FVector AimDirWS = GetBaseAimRotation().Vector();
	const FVector AimDirLS = ActorToWorld().InverseTransformVectorNoScale(AimDirWS);
	const FRotator AimRotLS = AimDirLS.Rotation();

	return AimRotLS;
}


void ASCharacter::StopRecoiling()
{
	bIsRecoiling = false;
	GetWorldTimerManager().ClearTimer(TimerHandle_Recoil);
	
	FinalRecoilPitch = 0.0f;
	FinalRecoilYaw = 0.0f;
	DampedPitch = 0.0f;
	DampedYaw = 0.0f;
}


void ASCharacter::OnStartJump()
{
	bPressedJump = true;

	SetIsJumping(true);
}


void ASCharacter::OnStopJump()
{
	bPressedJump = false;
}


bool ASCharacter::IsInitiatedJump() const
{
	return bIsJumping;
}


void ASCharacter::SetIsJumping(bool NewJumping)
{
	// Go to standing pose if trying to jump while crouched
	if (bIsCrouched && NewJumping)
	{
		UnCrouch();
	}
	else
	{
		bIsJumping = NewJumping;
	}

	if (Role < ROLE_Authority)
	{
		ServerSetIsJumping(NewJumping);
	}
}


void ASCharacter::ServerSetIsJumping_Implementation(bool NewJumping)
{
	SetIsJumping(NewJumping);
}

bool ASCharacter::ServerSetIsJumping_Validate(bool NewJumping)
{
	return true;
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

	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &ASCharacter::BeginAiming);
	PlayerInputComponent->BindAction("Zoom", IE_Released, this, &ASCharacter::EndAiming);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASCharacter::StopFire);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ASCharacter::OnStartSprinting);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ASCharacter::OnStopSprinting);

	// Interaction
	PlayerInputComponent->BindAction("Use", IE_Pressed, this, &ASCharacter::Use);

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

	DOREPLIFETIME_CONDITION(ASCharacter, bWantsToRun, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ASCharacter, bIsAiming, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ASCharacter, bIsJumping, COND_SkipOwner);

	DOREPLIFETIME(ASCharacter, CurrentWeapon);
	DOREPLIFETIME(ASCharacter, bDied);
}