// Fill out your copyright notice in the Description page of Project Settings.


#include "PracticeGameMode.h"

#include "PracticeCharacter.h"
#include "PracticeController.h"

APracticeGameMode::APracticeGameMode()
{
	DefaultPawnClass = APracticeCharacter::StaticClass();
	PlayerControllerClass = APracticeController::StaticClass();
}
