// Copyright Epic Games, Inc. All Rights Reserved.

#include "GravitySampleGameMode.h"
#include "GravitySampleCharacter.h"
#include "UObject/ConstructorHelpers.h"

AGravitySampleGameMode::AGravitySampleGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
