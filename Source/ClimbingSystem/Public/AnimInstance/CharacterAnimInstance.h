// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CharacterAnimInstance.generated.h"

class AClimbingSystemCharacter;
class UCustomMovementComponent;
/**
 * 
 */
UCLASS()
class CLIMBINGSYSTEM_API UCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	UPROPERTY()
	AClimbingSystemCharacter* ClimbingSystemCharacter;

	UPROPERTY()
	UCustomMovementComponent* CustomMovementComponent;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = Reference,meta = (AllowPrivateAccess = "true"))
	float GroundSpeed;
	void GetGroundSpeed();

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = Reference,meta = (AllowPrivateAccess = "true"))
	float AirSpeed;
	void GetAirSpeed();

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = Reference,meta = (AllowPrivateAccess = "true"))
	bool bShouldMove;
	void GetShouldMove();

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = Reference,meta = (AllowPrivateAccess = "true"))
	bool bIsFalling;
	void GetIsFalling();
};
