// Fill out your copyright notice in the Description page of Project Settings.

#include "SUsableActor.h"
#include "Components/StaticMeshComponent.h"


// Sets default values
ASUsableActor::ASUsableActor()
{
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;
}

void ASUsableActor::OnUsed(APawn* InstigatorPawn)
{
	// Nothing to do here...
}


void ASUsableActor::OnBeginFocus()
{
	// Used by custom PostProcess to render outlines
	MeshComp->SetRenderCustomDepth(true);
}


void ASUsableActor::OnEndFocus()
{
	// Used by custom PostProcess to render outlines
	MeshComp->SetRenderCustomDepth(false);
}

