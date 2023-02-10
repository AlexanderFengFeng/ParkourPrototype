// Copyright Epic Games, Inc. All Rights Reserved.

#include "ParkourPrototypeCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetSystemLibrary.h"

//////////////////////////////////////////////////////////////////////////
// AParkourPrototypeCharacter

AParkourPrototypeCharacter::AParkourPrototypeCharacter()
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
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AParkourPrototypeCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &AParkourPrototypeCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &AParkourPrototypeCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &AParkourPrototypeCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &AParkourPrototypeCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AParkourPrototypeCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AParkourPrototypeCharacter::TouchStopped);
}

void AParkourPrototypeCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void AParkourPrototypeCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void AParkourPrototypeCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AParkourPrototypeCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AParkourPrototypeCharacter::MoveForward(float Value)
{
	if (IsHanging) return;

	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AParkourPrototypeCharacter::MoveRight(float Value)
{
	if (IsHanging) return;

	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AParkourPrototypeCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ForwardTrace();
	HeightTrace();
}


void AParkourPrototypeCharacter::ForwardTrace()
{
	FVector Start = GetActorLocation() + GetActorForwardVector();
	FVector End = GetActorLocation() + GetActorForwardVector() * ClimbingFrontOffset;
	FHitResult HitResult;
	DrawDebugCylinder(GetWorld(), Start, End, 20.f, 8, FColor::Green);
	if (IsHanging) return;

	bool SweepResult = GetWorld()->SweepSingleByChannel(
		HitResult,
		Start,
		End,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(20.f));

	if (SweepResult)
	{
		IsWallAvailableToHang = true;
		WallLocation = HitResult.Location;
		WallNormal = HitResult.Normal;
		DrawDebugSphere(GetWorld(), WallLocation, 32.f, 16.f, FColor::Red, false);
	}
	else
	{
		IsWallAvailableToHang = false;
	}
}

void AParkourPrototypeCharacter::HeightTrace()
{
	FVector Start = GetActorLocation() + GetActorForwardVector() * ClimbingFrontOffset + GetActorUpVector() * 150.f;
	FVector End = GetActorLocation() + GetActorForwardVector() * ClimbingFrontOffset;

	DrawDebugCylinder(GetWorld(), Start, End, 20.f, 8, FColor::Red);

	FVector PelvisLocation = GetMesh()->GetSocketLocation(TEXT("pelvisSocket"));
	FVector ValidStart = PelvisLocation + GetActorForwardVector() * ClimbingFrontOffset + GetActorUpVector() * 100.f;
	FVector ValidEnd = PelvisLocation + GetActorForwardVector() * ClimbingFrontOffset;
	DrawDebugCylinder(GetWorld(), ValidStart, ValidEnd, 20.f, 8, FColor::Blue);

	if (IsHanging) return;

	FHitResult HitResult;
	bool SweepResult = GetWorld()->SweepSingleByChannel(
		HitResult,
		Start,
		End,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(20.f));

	// TODO: Address issue where we can walk into a hang.
	if (SweepResult)
	{
    	DrawDebugSphere(GetWorld(), HeightLocation, 32.f, 16.f, FColor::Magenta, false);
		HeightLocation = HitResult.Location;
		if (abs(PelvisLocation.Z - HeightLocation.Z) <= 100.f)
		{
			Hang();
		}
	}
}

void AParkourPrototypeCharacter::Hang()
{
	IsHanging = true;
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
	GetCharacterMovement()->StopMovementImmediately();
	if (HangingAnimMontage != nullptr)
	{
		PlayAnimMontage(HangingAnimMontage, 0.f);
		GetMesh()->GetAnimInstance()->Montage_Pause();
	}

	if (IsWallAvailableToHang)
	{

		FVector WallOffset = WallLocation + WallNormal * 10.f; // Position to place character against wall.
		float DestinationZ = HeightLocation.Z - 94.f; // Capsule half height.

		FVector TargetLocation = FVector(WallOffset.X, WallOffset.Y, DestinationZ);
		FRotator TargetRotation = (WallNormal * -1.f).Rotation();
		FLatentActionInfo LatentAction;
		LatentAction.CallbackTarget = this;
		UKismetSystemLibrary::MoveComponentTo(
			GetCapsuleComponent(),
			TargetLocation,
			TargetRotation,
			true,
			true,
			0.2f,
			false,
			EMoveComponentAction::Move,
			LatentAction);
	}
}
