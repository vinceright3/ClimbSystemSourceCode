// Vince Petrelli All Rights Reserved


#include "Components/CustomMovementComponent.h"

bool UCustomMovementComponent::IsClimbing() const
{	
	if(MovementMode == MOVE_Custom && CustomMovementMode == ECustomMovementMode::ECMM_Climbing)
	{
		return true;
	}

	return false;
}
