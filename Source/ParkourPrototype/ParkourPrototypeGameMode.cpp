// Copyright Epic Games, Inc. All Rights Reserved.

#include "ParkourPrototypeGameMode.h"
#include "ParkourPrototypeCharacter.h"
#include "UObject/ConstructorHelpers.h"

AParkourPrototypeGameMode::AParkourPrototypeGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
