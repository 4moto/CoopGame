// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SomeEnums.h"
#include "SGameMode.generated.h"



/**
 * 
 */

UCLASS()
class COOPGAME_API ASGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
protected:

	FTimerHandle TimerHandle_BotSpawner;
	FTimerHandle TimerHandle_NextWaveDelay;

	// Bots to spawn in current wave
	int32 NrOfBotsToSpawn;

	// Current Wave #
	int32 WaveCount;
	
	// Time between waves
	UPROPERTY(EditDefaultsOnly, Category = "GameMode")
	float TimeBetweenWaves;

protected:
	
	// Hook for BP to spawn a single bot
	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
	void SpawnNewBot();

	UFUNCTION()
	void SpawnBotTimerElapsed();

	// Start Spawning Bots
	UFUNCTION()
	void StartWave();

	// Stop Spawning Bots
	UFUNCTION()
	void EndWave();
	
	// Set timer for next StartWave
	UFUNCTION()
	void PrepareForNextWave();

	UFUNCTION()
	void CheckWaveState();

	UFUNCTION()
	void CheckAnyPlayerAlive();

	UFUNCTION()
	void GameOver();

	UFUNCTION()
	void SetWaveState(EWaveState NewState);
	
public:

	ASGameMode();

	virtual void StartPlay() override; 
	virtual void Tick(float DeltaSeconds) override;
};
