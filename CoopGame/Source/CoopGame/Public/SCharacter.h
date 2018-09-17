// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class ASWeapon;
class USHealthComponent;


UCLASS()
class COOPGAME_API ASCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USHealthComponent* HealthComp;

	void MoveForward(float Value);

	void MoveRight(float Value);

	void BeginCrouch();

	void EndCrouch();

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	float ZoomedFOV;

	UPROPERTY(EditDefaultsOnly, Category = "Player", meta = (ClampMin = 0.1, ClampMax = 100))
	float ZoomedInterpSpeed;

	/* Default FOV set during begin play */
	float DefaultFOV;

	void BeginZoom();
	void EndZoom();

	UPROPERTY(Replicated)
	ASWeapon* CurrentWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	TSubclassOf<ASWeapon> StarterWeaponClass;

	UPROPERTY(VisibleDefaultsOnly, Category = "Player")
	FName WeaponAttachSocketName;

	UFUNCTION()
	void OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);


	// Recoil Variables and Functions

	FTimerHandle TimerHandle_Recoil;
	FTimerHandle TimerHandle_StopRecoil;

	bool bIsRecoiling;
	float RecoilMod;
	float FinalRecoilPitch;
	float FinalRecoilYaw;
	float DampedPitch;
	float DampedYaw;

	// The higher the number the slower the recoil effect is
	UPROPERTY(EditDefaultsOnly, Category = "Recoil")
	float RecoilTime;

	// The interval between recoil 'kicks' -- very low is smoothest -- higher is most performant
	UPROPERTY(meta = (ClampMin = 0.0001f, ClampMax = 0.5f))
	float RecoilApplyRate;

	// How quickly the recoil is reduced
	UPROPERTY(EditDefaultsOnly, Category = "Recoil")
	float RecoilDamping;

	UFUNCTION() // not sure I need this one with the way I'm implementing recoil
	void StopRecoiling();

	UFUNCTION()
	void StartRecoiling();


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual FVector GetPawnViewLocation() const override;

	bool bWantsToZoom;

	UFUNCTION(BlueprintCallable, Category = "Player")
		void StartFire();
	UFUNCTION(BlueprintCallable, Category = "Player")
		void StopFire();
	
	/* Pawn Died Previously */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
		bool bDied;
	
	/* Camera Recoil */
	UFUNCTION(BlueprintCallable, Category = "Player")
	void Recoil(float RecoilPitchUp, float RecoilPitchDown, float RecoilYawRight, float RecoilYawLeft);

	UFUNCTION(BlueprintCallable, Category = "Player")
	void SetRecoilMod(float RecoilModifier);
	
};
