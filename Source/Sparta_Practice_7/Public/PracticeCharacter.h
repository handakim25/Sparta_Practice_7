// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PracticeCharacter.generated.h"

UCLASS()
class SPARTA_PRACTICE_7_API APracticeCharacter : public APawn
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APracticeCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
