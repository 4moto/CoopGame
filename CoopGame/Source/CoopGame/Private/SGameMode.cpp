// Fill out your copyright notice in the Description page of Project Settings.

#include "SGameMode.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "SGameState.h"
#include "SPlayerState.h"
#include "SHealthComponent.h"

ASGameMode::ASGameMode()
{
	TimeBetweenWaves = 5.0f;

	GameStateClass = ASGameState::StaticClass();
	PlayerStateClass = ASPlayerState::StaticClass();

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

	SetWaveState(EWaveState::WaveInProgress);
}


void ASGameMode::EndWave()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_BotSpawner);

	SetWaveState(EWaveState::WaitingToComplete);

// if using a timer to spawn instead of just dead robots
	PrepareForNextWave();
}


void ASGameMode::PrepareForNextWave()
{
	SetWaveState(EWaveState::WaitingtoStart);

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

		USHealthComponent* HealthComp = (Cast<USHealthComponent>(TestPawn->GetComponentByClass(USHealthComponent::StaticClass())));
		if (HealthComp && HealthComp->GetHealth() > 0.0f)
		{
			bIsAnyBotAlive = true;

			SetWaveState(EWaveState::WaitingToComplete);

			break;
		}


		/* Alternative
		TArray<USHealthComponent*> HealthComps;
		TestPawn->GetComponents(HealthComps);
		if (HealthComps.Num() > 0)
		{
			//		USHealthComponent* HealthComp = (Cast<USHealthComponent>(TestPawn->GetComponentByClass(TSubclassOf<USHealthComponent>(HealthCompe))); -- got this working above
			USHealthComponent* HealthComp = HealthComps[0];
			if (HealthComp && HealthComp->GetHealth() <= 0)
			{
				bIsAnyBotAlive = true;
				break;
			}
		}
		*/
	}

	if (!bIsAnyBotAlive)
	{
		SetWaveState(EWaveState::WaveComplete);

		PrepareForNextWave();
	}
}

void ASGameMode::CheckAnyPlayerAlive()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->GetPawn())
		{
			APawn* MyPawn = PC->GetPawn();
			USHealthComponent* HealthComp = Cast<USHealthComponent>(MyPawn->GetComponentByClass(USHealthComponent::StaticClass()));
			if (ensure(HealthComp) && HealthComp->GetHealth() > 0.0f)
			{
				// A player is still alive
				return;
			}
		}
	}
	
	// No player alive
	GameOver();


	/* Alternative Method
	bool bIsAnyPlayerAlive = false;


	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->GetPawn())
		{
			APawn* MyPawn = PC->GetPawn();
		}

		TArray<USHealthComponent*> HealthComps;

		PC->GetComponents(HealthComps);

		if (HealthComps.Num() > 0)
		{
			//		USHealthComponent* HealthComp = (Cast<USHealthComponent>(TestPawn->GetComponentByClass(TSubclassOf<USHealthComponent>(HealthComp))); -- couldn't get this working

			USHealthComponent* HealthComp = HealthComps[0];

			if (HealthComp && HealthComp->GetHealth() <= 0)
			{
				bIsAnyPlayerAlive = true;
				break;
			}
		}
		*/
}

void ASGameMode::GameOver()
{
	EndWave();

	// @TODO: Finish up the match, present 'game over' to players

	SetWaveState(EWaveState::GameOver);

	UE_LOG(LogTemp, Log, TEXT("Game Over! Players are dead"));

}

void ASGameMode::SetWaveState(EWaveState NewState)
{
	ASGameState* GS = GetGameState<ASGameState>();
	if (ensureAlways(GS))
	{
		GS->SetWaveState(NewState);
	}
}


void ASGameMode::StartPlay()
{
	Super::StartPlay();

	PrepareForNextWave(); // this is happening too fast and is causing errors since the world isn't finished loading yet!
}

void ASGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	CheckWaveState();

	CheckAnyPlayerAlive();

}
