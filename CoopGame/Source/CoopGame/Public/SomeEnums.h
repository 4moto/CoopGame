#pragma once
#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EWaveState : uint8
{
	// Delay between waves
	WaitingtoStart,

	// Spawning new bots
	WaveInProgress,

	// No longer spawning new bots, waiting for players to kill remaining bots
	WaitingToComplete,

	// All bots killed
	WaveComplete,

	// All players are dead
	GameOver,
};