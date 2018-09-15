// Fill out your copyright notice in the Description page of Project Settings.

#include "SGameMode.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "SHealthComponent.h"

ASGameMode::ASGameMode()
{
	TimeBetweenWaves = 5.0f;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 5.0f;
}


void ASGameMode::SpawnBotTimerElapsed()
{
	SpawnNewBot();

	NrOfBotsToSpawn--;

	if (NrOfBotsToSpawn <= 0)
	{
		EndWave();
	}
}


void ASGameMode::StartWave()
{
	WaveCount++;
	
	NrOfBotsToSpawn = 2 * WaveCount;

	GetWorldTimerManager().SetTimer(TimerHandle_BotSpawner, this, &ASGameMode::SpawnBotTimerElapsed, 1.0f, true, 0.0f);
}


void ASGameMode::EndWave()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_BotSpawner);

	PrepareForNextWave();
}


void ASGameMode::PrepareForNextWave()
{
	GetWorldTimerManager().SetTimer(TimerHandle_NextWaveDelay, this, &ASGameMode::StartWave, TimeBetweenWaves, false);
}

void ASGameMode::CheckWaveState()
{
	bool bIsPreparingForWave = GetWorldTimerManager().IsTimerActive(TimerHandle_NextWaveDelay);

	if (NrOfBotsToSpawn > 0 || bIsPreparingForWave)
	{
		return;
	}

	bool bIsAnyBotAlive = false;

	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		APawn* TestPawn = It->Get();
		if (TestPawn == nullptr || TestPawn->IsPlayerControlled())
		{
			continue;
		}

		TArray<USHealthComponent*> HealthComps;

		TestPawn->GetComponents(HealthComps);

		if (HealthComps.Num() > 0)
		{
			//		USHealthComponent* HealthComp = (Cast<USHealthComponent>(TestPawn->GetComponentByClass(TSubclassOf<USHealthComponent>(HealthComp))); -- couldn't get this working
			
			USHealthComponent* HealthComp = HealthComps[0];

			if (HealthComp && HealthComp->GetHealth() <= 0)
			{
				bIsAnyBotAlive = true;
				break;
			}
		}		
	}

	if (!bIsAnyBotAlive)
	{
		PrepareForNextWave();
	}
}


void ASGameMode::StartPlay()
{
	Super::StartPlay();

	PrepareForNextWave();
}

void ASGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	CheckWaveState();

}
