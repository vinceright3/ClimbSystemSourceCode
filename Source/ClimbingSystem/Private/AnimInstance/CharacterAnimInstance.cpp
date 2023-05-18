// Vince Petrelli All Rights Reserved


#include "AnimInstance/CharacterAnimInstance.h"
#include "ClimbingSystem/ClimbingSystemCharacter.h"
#include "Components/CustomMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	ClimbingSystemCharacter = Cast<AClimbingSystemCharacter>(TryGetPawnOwner());

	if(ClimbingSystemCharacter)
	{
		CustomMovementComponent = ClimbingSystemCharacter->GetCustomMovementComponent();
	}
}

void UCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if(!ClimbingSystemCharacter || !CustomMovementComponent) return;

	GetGroundSpeed();
	GetAirSpeed();
	GetShouldMove();
	GetIsFalling();
	GetIsClimbing();
}

void UCharacterAnimInstance::GetGroundSpeed()
{	
	GroundSpeed = UKismetMathLibrary::VSizeXY(ClimbingSystemCharacter->GetVelocity());
}

void UCharacterAnimInstance::GetAirSpeed()
{
	AirSpeed = ClimbingSystemCharacter->GetVelocity().Z;
}

void UCharacterAnimInstance::GetShouldMove()
{	
	bShouldMove =
	CustomMovementComponent->GetCurrentAcceleration().Size()>0&&
	GroundSpeed>5.f &&
	!bIsFalling;
}

void UCharacterAnimInstance::GetIsFalling()
{
	bIsFalling = CustomMovementComponent->IsFalling();
}

void UCharacterAnimInstance::GetIsClimbing()
{
	bIsClimbing = CustomMovementComponent->IsClimbing();
}