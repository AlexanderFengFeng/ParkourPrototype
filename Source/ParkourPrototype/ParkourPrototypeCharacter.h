// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ParkourPrototypeCharacter.generated.h"

UCLASS(config=Game)
class AParkourPrototypeCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
public:
	AParkourPrototypeCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Input)
	float TurnRateGamepad;

protected:

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface
	virtual void Tick(float DeltaSeconds) override;

	virtual void BeginPlay() override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:
	UPROPERTY(EditAnywhere)
	class UAnimMontage* HangingAnimMontage;

	UPROPERTY(EditAnywhere)
	float ChestVerticalOffset = 50.f;
	UPROPERTY(EditAnywhere)
	float ClimbingFrontOffset = 50.f;
	UPROPERTY(EditAnywhere)
	float VerticleHeightStart = 150.f;
	UPROPERTY(EditAnywhere)
	float VerticalAcceptanceHeight = 100.f;

	void ForwardTrace();
	void HeightTrace();

	bool IsHanging = false;
	bool IsClimbingUp = false;
	bool IsWallAvailableToHang = false;

	FVector WallNormal;
	FVector WallLocation;
	FVector HeightLocation;

	void Hang();
	void ClimbUp();
	void DropDown();

	UFUNCTION()
	void ChangeSettingsAfterFinishingClimbUp(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload);
};

