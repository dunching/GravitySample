// Copyright Epic Games, Inc. All Rights Reserved.

#include "GravitySampleCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include <Kismet/GameplayStatics.h>
#include <Blueprint/AIBlueprintHelperLibrary.h>
#include <NavMesh/NavMeshBoundsVolume.h>

#include <GravityMovementcomponent.h>
#include "FlyingNavFunctionLibrary.h"

//////////////////////////////////////////////////////////////////////////
// AGravitySampleCharacter

AGravitySampleCharacter::AGravitySampleCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UGravityMovementcomponent>(ACharacter::CharacterMovementComponentName))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rate for input
	TurnRateGamepad = 50.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AGravitySampleCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("FClick", IE_Released, this, &AGravitySampleCharacter::FClick);
	PlayerInputComponent->BindAction("GClick", IE_Released, this, &AGravitySampleCharacter::GClick);
	PlayerInputComponent->BindAction("CClick", IE_Released, this, &AGravitySampleCharacter::CClick);
	PlayerInputComponent->BindAction("VClick", IE_Released, this, &AGravitySampleCharacter::VClick);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGravitySampleCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGravitySampleCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &AGravitySampleCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &AGravitySampleCharacter::LookUpAtRate);
}

void AGravitySampleCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void AGravitySampleCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void AGravitySampleCharacter::FClick()
{
	FMinimalViewInfo DesiredView;
	FollowCamera->GetCameraView(0, DesiredView);

	auto StartPt = DesiredView.Location;
	auto StopPt = DesiredView.Location + (DesiredView.Rotation.Vector() * 1000);

	FHitResult Result;

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldStatic);

	FCollisionQueryParams Params;
	Params.bTraceComplex = false;

	if (GetWorld()->LineTraceSingleByObjectType(
		Result,
		StartPt,
		StopPt,
		ObjectQueryParams,
		Params)
		)
	{
		TArray<AActor*>ActorAry;
		UGameplayStatics::GetAllActorsWithTag(this, TEXT("t"), ActorAry);

		for (auto Iter : ActorAry)
		{
			auto CharacterPtr = Cast<AGravitySampleCharacter>(Iter);
			if (CharacterPtr)
			{
				auto NewPt = Result.ImpactPoint -
					(
						Cast<UGravityMovementcomponent>(GetCharacterMovement())->GetGravityDirection() *
						GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
						);

				const FLatentActionInfo LatentInfo(0, FMath::Rand(), TEXT("OnFoundPath1"), this);
				FirstNavigationPathPtr = UFlyingNavFunctionLibrary::FindPathToLocationAsynchronously(
					this,
					LatentInfo,
					CharacterPtr->GetActorLocation(),
					NewPt,
					this
				);

				DrawDebugSphere(GetWorld(), NewPt, 50, 10, FColor::Red, true);
				DrawDebugSphere(GetWorld(), CharacterPtr->GetActorLocation(), 50, 10, FColor::Green, true);
			}
		}
	}
}

void AGravitySampleCharacter::GClick()
{
}

void AGravitySampleCharacter::CClick()
{

}

void AGravitySampleCharacter::VClick()
{

}

void AGravitySampleCharacter::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*>ActorAry;
	UGameplayStatics::GetAllActorsOfClassWithTag(this, ANavMeshBoundsVolume::StaticClass(), TEXT("NavMesh"), ActorAry);

	for (auto Iter : ActorAry)
	{
		auto NavMeshPtr = Cast<ANavMeshBoundsVolume>(Iter);
		if (NavMeshPtr)
		{
//			NavMeshPtr->SetActorTransform(GetActorTransform());

			auto FlyingNavigationDataPtr = UFlyingNavFunctionLibrary::GetFlyingNavigationData(this);
			if (FlyingNavigationDataPtr)
			{
				FlyingNavigationDataPtr->OnFlyingNavGenerationFinished.AddDynamic(this, &AGravitySampleCharacter::OnGenerationFinished1);
				FlyingNavigationDataPtr->RebuildNavigationData();
			}
		}
	}

	GetWorld()->GetTimerManager().SetTimer(NavTimer, this, &AGravitySampleCharacter::Navigation, 1, true);
}

void AGravitySampleCharacter::Tick(float Delta)
{
	Super::Tick(Delta);
}

void AGravitySampleCharacter::Navigation()
{
	if (GetController()->IsA(APlayerController::StaticClass()))
	{
		TArray<AActor*>ActorAry;
		UGameplayStatics::GetAllActorsWithTag(this, TEXT("t"), ActorAry);

		for (auto Iter : ActorAry)
		{
			auto CharacterPtr = Cast<AGravitySampleCharacter>(Iter);
			if (CharacterPtr)
			{
				auto NewPt = GetActorLocation() -
					(
						Cast<UGravityMovementcomponent>(GetCharacterMovement())->GetGravityDirection() *
						GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
						);

				if (FVector::Distance(CharacterPtr->GetActorLocation(), NewPt) > 100)
				{
					const FLatentActionInfo LatentInfo(0, FMath::Rand(), TEXT("OnFoundPath1"), this);
					FirstNavigationPathPtr = UFlyingNavFunctionLibrary::FindPathToLocationAsynchronously(
						this,
						LatentInfo,
						CharacterPtr->GetActorLocation(),
						NewPt,
						this
					);

					DrawDebugSphere(GetWorld(), NewPt, 50, 10, FColor::Red, false, 1);
					DrawDebugSphere(GetWorld(), GetActorLocation(), 50, 10, FColor::Green, false, 1);
				}
			}
		}
	}
}

void AGravitySampleCharacter::OnGenerationFinished1()
{
}

void AGravitySampleCharacter::OnFoundPath1()
{
	auto Result = UFlyingNavFunctionLibrary::GetPathfindingResult(FirstNavigationPathPtr);
	switch (Result)
	{
	case EPathfindingResult::Success:
	{
		TArray<AActor*>ActorAry;
		UGameplayStatics::GetAllActorsWithTag(this, TEXT("t"), ActorAry);

		for (auto Iter : ActorAry)
		{
			auto CharacterPtr = Cast<AGravitySampleCharacter>(Iter);
			if (CharacterPtr)
			{
				UFlyingNavFunctionLibrary::RequestMove(FirstNavigationPathPtr, UAIBlueprintHelperLibrary::GetAIController(CharacterPtr));
				UFlyingNavFunctionLibrary::DrawNavPath(this, FirstNavigationPathPtr);
			}
		}
	}
	break;
	}
}

void AGravitySampleCharacter::OnGenerationFinished2()
{
}

void AGravitySampleCharacter::OnFoundPath2()
{

}

void AGravitySampleCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddActorLocalRotation(FRotator(0, Rate * 5, 0));
}

void AGravitySampleCharacter::LookUpAtRate(float Rate)
{
	auto Rot = CameraBoom->GetRelativeRotation();

	// calculate delta for this frame from the rate information
	CameraBoom->SetRelativeRotation(FRotator(FMath::Clamp(Rot.Pitch + Rate * 5, -70, 70), 0, 0));
}

void AGravitySampleCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		auto Dir = GetCapsuleComponent()->GetForwardVector();

		// get forward vector
		AddMovementInput(Dir, Value);
	}
}

void AGravitySampleCharacter::MoveRight(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		auto Dir = GetCapsuleComponent()->GetRightVector();

		// get forward vector
		AddMovementInput(Dir, Value);
	}
}
