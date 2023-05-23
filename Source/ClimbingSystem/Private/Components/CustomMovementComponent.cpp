// Vince Petrelli All Rights Reserved


#include "Components/CustomMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ClimbingSystem/ClimbingSystemCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "ClimbingSystem/ClimbingSystemCharacter.h"
#include "MotionWarpingComponent.h"

#include "ClimbingSystem/DebugHelper.h"

void UCustomMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningPlayerAnimInstance = CharacterOwner->GetMesh()->GetAnimInstance();

	if(OwningPlayerAnimInstance)
	{
		OwningPlayerAnimInstance->OnMontageEnded.AddDynamic(this,&UCustomMovementComponent::OnClimbMontageEnded);
		OwningPlayerAnimInstance->OnMontageBlendingOut.AddDynamic(this,&UCustomMovementComponent::OnClimbMontageEnded);
	}

	OwningPlayerCharacter = Cast<AClimbingSystemCharacter>(CharacterOwner);
}

void UCustomMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	Super::TickComponent(DeltaTime,TickType, ThisTickFunction);

}

void UCustomMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	if(IsClimbing())
	{
		bOrientRotationToMovement = false;
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(48.f);

		OnEnterClimbStateDelegate.ExecuteIfBound();
	}

	if(PreviousMovementMode == MOVE_Custom && PreviousCustomMode == ECustomMovementMode::MOVE_Climb)
	{
		bOrientRotationToMovement = true;
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(96.f);
		
		const FRotator DirtyRotation = UpdatedComponent->GetComponentRotation();
		const FRotator CleanStandRotation = FRotator(0.f,DirtyRotation.Yaw,0.f);
		UpdatedComponent->SetRelativeRotation(CleanStandRotation);

		StopMovementImmediately();

		OnExitClimbStateDelegate.ExecuteIfBound();
	}

	Super::OnMovementModeChanged(PreviousMovementMode,PreviousCustomMode);
}

void UCustomMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	if(IsClimbing())
	{
		PhysClimb(deltaTime,Iterations);
	}

	Super::PhysCustom(deltaTime,Iterations);
}

float UCustomMovementComponent::GetMaxSpeed() const
{
	if(IsClimbing())
	{
		return MaxClimbSpeed;
	}
	else
	{
		return Super::GetMaxSpeed();
	}
}

float UCustomMovementComponent::GetMaxAcceleration() const
{
	if(IsClimbing())
	{
		return MaxClimbAcceleration;
	}
	else
	{
		return Super::GetMaxAcceleration();
	}
}

FVector UCustomMovementComponent::ConstrainAnimRootMotionVelocity(const FVector & RootMotionVelocity, const FVector & CurrentVelocity) const
{	
	const bool bIsPlayingRMMontage =
	IsFalling() && OwningPlayerAnimInstance && OwningPlayerAnimInstance->IsAnyMontagePlaying();

	if(bIsPlayingRMMontage)
	{
		return RootMotionVelocity;
	}
	else
	{
		return Super::ConstrainAnimRootMotionVelocity(RootMotionVelocity,CurrentVelocity);
	}
}
 
#pragma region ClimbTraces

TArray<FHitResult> UCustomMovementComponent::DoCapsuleTraceMultiByObject(const FVector & Start, const FVector & End, bool bShowDebugShape,bool bDrawPersistantShapes)
{	
	TArray<FHitResult> OutCapsuleTraceHitResults;

	EDrawDebugTrace::Type DebugTraceType = EDrawDebugTrace::None;

	if(bShowDebugShape)
	{
		DebugTraceType = EDrawDebugTrace::ForOneFrame;

		if(bDrawPersistantShapes)
		{
			DebugTraceType = EDrawDebugTrace::Persistent;
		}
	}

	UKismetSystemLibrary::CapsuleTraceMultiForObjects(
		this,
		Start,
		End,
		ClimbCapsuleTraceRadius,
		ClimbCapsuleTraceHalfHeight,
		ClimbableSurfaceTraceTypes,
		false,
		TArray<AActor*>(),
		DebugTraceType,
		OutCapsuleTraceHitResults,
		false
	);

	return OutCapsuleTraceHitResults;
}

FHitResult UCustomMovementComponent::DoLineTraceSingleByObject(const FVector & Start, const FVector & End, bool bShowDebugShape,bool bDrawPersistantShapes)
{	
	FHitResult OutHit;

	EDrawDebugTrace::Type DebugTraceType = EDrawDebugTrace::None;

	if(bShowDebugShape)
	{
		DebugTraceType = EDrawDebugTrace::ForOneFrame;

		if(bDrawPersistantShapes)
		{
			DebugTraceType = EDrawDebugTrace::Persistent;
		}
	}

	UKismetSystemLibrary::LineTraceSingleForObjects(
		this,
		Start,
		End,
		ClimbableSurfaceTraceTypes,
		false,
		TArray<AActor*>(),
		DebugTraceType,
		OutHit,
		false
	);

	return OutHit;
}

#pragma endregion

#pragma region ClimbCore

void UCustomMovementComponent::ToggleClimbing(bool bEnableClimb)
{
	if(bEnableClimb)
	{
		if(CanStartClimbing())
		{ 
			//Enter the climb state
			PlayClimbMontage(IdleToClimbMontage);
		}
		else if(CanClimbDownLedge())
		{
			PlayClimbMontage(ClimbDownLedgeMontage);
		}
		else
		{
			TryStartVaulting();
		}
	}
	
	if(!bEnableClimb)
	{
		//Stop climbing
		StopClimbing();
	}
}

bool UCustomMovementComponent::CanStartClimbing()
{
	if(IsFalling()) return false;
	if(!TraceClimbableSurfaces()) return false;
	if(!TraceFromEyeHeight(100.f).bBlockingHit) return false;

	return true;
}

bool UCustomMovementComponent::CanClimbDownLedge()
{
	if(IsFalling()) return false;
	 
	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector ComponentForward = UpdatedComponent->GetForwardVector();
	const FVector DownVector = -UpdatedComponent->GetUpVector();

	const FVector WalkableSurfaceTraceStart = ComponentLocation + ComponentForward * ClimbDownWalkableSurfaceTraceOffset;
	const FVector WalkableSurfaceTraceEnd = WalkableSurfaceTraceStart + DownVector * 100.f;

	FHitResult WalkableSurfaceHit = DoLineTraceSingleByObject(WalkableSurfaceTraceStart,WalkableSurfaceTraceEnd);

	const FVector LedgeTraceStart = WalkableSurfaceHit.TraceStart + ComponentForward * ClimbDownLedgeTraceOffset;
	const FVector LedgeTraceEnd = LedgeTraceStart + DownVector * 200.f;

	FHitResult LedgeTraceHit = DoLineTraceSingleByObject(LedgeTraceStart,LedgeTraceEnd);

	if(WalkableSurfaceHit.bBlockingHit && !LedgeTraceHit.bBlockingHit)
	{
		return true;
	}

	return false;
}

void UCustomMovementComponent::StartClimbing()
{
	SetMovementMode(MOVE_Custom,ECustomMovementMode::MOVE_Climb);
}

void UCustomMovementComponent::StopClimbing()
{
	SetMovementMode(MOVE_Falling);
}

void UCustomMovementComponent::PhysClimb(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return; 
	}

	/*Process all the climbable surfaces info*/
	TraceClimbableSurfaces();
	ProcessClimableSurfaceInfo();

	/*Check if we should stop climbing*/
	if(CheckShouldStopClimbing() || CheckHasReachedFloor())
	{
		StopClimbing();
	}

	RestorePreAdditiveRootMotionVelocity();

	if( !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() )
	{	//Define the max climb speed and acceleration
		CalcVelocity(deltaTime, 0.f, true, MaxBreakClimbDeceleration);
	}

	ApplyRootMotionToVelocity(deltaTime);

	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FVector Adjusted = Velocity * deltaTime;
	FHitResult Hit(1.f);

	//Handle climb rotation
	SafeMoveUpdatedComponent(Adjusted, GetClimbRotation(deltaTime), true, Hit);

	if (Hit.Time < 1.f)
	{
		//adjust and try again
		HandleImpact(Hit, deltaTime, Adjusted);
		SlideAlongSurface(Adjusted, (1.f-Hit.Time), Hit.Normal, Hit, true);
	}

	if(!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() )
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	}
	 
	/*Snap movement to climbable surfaces*/
	SnapMovementToClimableSurfaces(deltaTime);

	if(CheckHasReachedLedge())
	{	
		PlayClimbMontage(ClimbToTopMontage);
	}
}

void UCustomMovementComponent::ProcessClimableSurfaceInfo()
{
	CurrentClimbableSurfaceLocation = FVector::ZeroVector;
	CurrentClimbableSurfaceNormal = FVector::ZeroVector;

	if(ClimbableSurfacesTracedResults.IsEmpty()) return;

	for(const FHitResult& TracedHitResult:ClimbableSurfacesTracedResults)
	{
		CurrentClimbableSurfaceLocation += TracedHitResult.ImpactPoint;
		CurrentClimbableSurfaceNormal += TracedHitResult.ImpactNormal;
	}

	CurrentClimbableSurfaceLocation /= ClimbableSurfacesTracedResults.Num();
	CurrentClimbableSurfaceNormal = CurrentClimbableSurfaceNormal.GetSafeNormal();
}

bool UCustomMovementComponent::CheckShouldStopClimbing()
{
	if(ClimbableSurfacesTracedResults.IsEmpty()) return true;

	const float DotResult = FVector::DotProduct(CurrentClimbableSurfaceNormal,FVector::UpVector);
	const float DegreeDiff = FMath::RadiansToDegrees(FMath::Acos(DotResult));
	 
	if(DegreeDiff<=60.f)
	{
		return true;
	}
	
	return false;
}

bool UCustomMovementComponent::CheckHasReachedFloor()
{
	const FVector DownVector = -UpdatedComponent->GetUpVector();
	const FVector StartOffset = DownVector * 50.f;

	const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;
	const FVector End = Start + DownVector;

	TArray<FHitResult> PossibleFloorHits = DoCapsuleTraceMultiByObject(Start,End);

	if(PossibleFloorHits.IsEmpty()) return false;

	for(const FHitResult& PossibleFloorHit:PossibleFloorHits)
	{	
		const bool bFloorReached =
		FVector::Parallel(-PossibleFloorHit.ImpactNormal,FVector::UpVector) &&
		GetUnrotatedClimbVelocity().Z<-10.f;

		if(bFloorReached)
		{
			return true;
		}
	}

	return false;
}

FQuat UCustomMovementComponent::GetClimbRotation(float DeltaTime)
{	
	const FQuat CurrentQuat = UpdatedComponent->GetComponentQuat();

	if(HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity())
	{
		return CurrentQuat;
	}

	const FQuat TargetQuat = FRotationMatrix::MakeFromX(-CurrentClimbableSurfaceNormal).ToQuat();

	return FMath::QInterpTo(CurrentQuat,TargetQuat,DeltaTime,5.f);
}

void UCustomMovementComponent::SnapMovementToClimableSurfaces(float DeltaTime)
{
	const FVector ComponentForward = UpdatedComponent->GetForwardVector();
	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();

	const FVector ProjectedCharacterToSurface = 
	(CurrentClimbableSurfaceLocation - ComponentLocation).ProjectOnTo(ComponentForward);

	const FVector SnapVector = -CurrentClimbableSurfaceNormal * ProjectedCharacterToSurface.Length();

	UpdatedComponent->MoveComponent(
	SnapVector*DeltaTime*MaxClimbSpeed,
	UpdatedComponent->GetComponentQuat(),
	true);
}

bool UCustomMovementComponent::CheckHasReachedLedge()
{
	FHitResult LedgetHitResult = TraceFromEyeHeight(100.f,50.f);

	if(!LedgetHitResult.bBlockingHit)
	{
		const FVector WalkableSurfaceTraceStart = LedgetHitResult.TraceEnd;

		const FVector DownVector = -UpdatedComponent->GetUpVector();
		const FVector WalkableSurfaceTraceEnd = WalkableSurfaceTraceStart + DownVector * 100.f;

		FHitResult WalkabkeSurfaceHitResult =
		DoLineTraceSingleByObject(WalkableSurfaceTraceStart,WalkableSurfaceTraceEnd);

		if(WalkabkeSurfaceHitResult.bBlockingHit && GetUnrotatedClimbVelocity().Z > 10.f)
		{
			return true;
		}
	}

	return false;
}

void UCustomMovementComponent::TryStartVaulting()
{	
	FVector VaultStartPosition;
	FVector VaultLandPosition;

	if(CanStartVaulting(VaultStartPosition,VaultLandPosition))
	{
		SetMotionWarpTarget(FName("VaultStartPoint"),VaultStartPosition);
		SetMotionWarpTarget(FName("VaultEndPoint"),VaultLandPosition);

		StartClimbing();
		PlayClimbMontage(VaultMontage);
	}
	
}

bool UCustomMovementComponent::CanStartVaulting(FVector& OutVaultStartPosition,FVector& OutVaultLandPosition)
{
	if(IsFalling()) return false;

	OutVaultStartPosition = FVector::ZeroVector;
	OutVaultLandPosition = FVector::ZeroVector;

	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector ComponentForward = UpdatedComponent->GetForwardVector();
	const FVector UpVector = UpdatedComponent->GetUpVector();
	const FVector DownVector = -UpdatedComponent->GetUpVector();

	for(int32 i = 0; i<5; i++)
	{
		const FVector Start = ComponentLocation + UpVector * 100.f + 
		ComponentForward * 100.f * (i+1);

		const FVector End = Start + DownVector * 100.f * (i+1);

		FHitResult VaultTraceHit = DoLineTraceSingleByObject(Start,End);

		if(i == 0 && VaultTraceHit.bBlockingHit)
		{
			OutVaultStartPosition = VaultTraceHit.ImpactPoint;
		}

		if(i == 3 && VaultTraceHit.bBlockingHit)
		{
			OutVaultLandPosition = VaultTraceHit.ImpactPoint;
		}
	}

	if(OutVaultStartPosition!=FVector::ZeroVector && OutVaultLandPosition!=FVector::ZeroVector)
	{
		return true;
	}
	else
	{
		return false;
	}
	
}

bool UCustomMovementComponent::IsClimbing() const
{	
	return MovementMode == MOVE_Custom && CustomMovementMode == ECustomMovementMode::MOVE_Climb;
}
 
//Trace for climbable surfaces, return true if there are indeed valid surfaces, false otherwise
bool UCustomMovementComponent::TraceClimbableSurfaces()
{	
	const FVector StartOffset = UpdatedComponent->GetForwardVector() * 30.f;
	const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;
	const FVector End = Start + UpdatedComponent->GetForwardVector();

	ClimbableSurfacesTracedResults = DoCapsuleTraceMultiByObject(Start,End);

	return !ClimbableSurfacesTracedResults.IsEmpty();
}

FHitResult UCustomMovementComponent::TraceFromEyeHeight(float TraceDistance, float TraceStartOffset)
{
	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector EyeHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight + TraceStartOffset);
	
	const FVector Start = ComponentLocation + EyeHeightOffset;
	const FVector End = Start + UpdatedComponent->GetForwardVector() * TraceDistance;

	return DoLineTraceSingleByObject(Start,End);
}

void UCustomMovementComponent::PlayClimbMontage(UAnimMontage * MontageToPlay)
{
	if(!MontageToPlay) return;
	if(!OwningPlayerAnimInstance) return;
	if(OwningPlayerAnimInstance->IsAnyMontagePlaying()) return;

	OwningPlayerAnimInstance->Montage_Play(MontageToPlay);
}
 
void UCustomMovementComponent::OnClimbMontageEnded(UAnimMontage * Montage, bool bInterrupted)
{
	if(Montage == IdleToClimbMontage || Montage == ClimbDownLedgeMontage)
	{
		StartClimbing();
		StopMovementImmediately();
	}
	
	if(Montage == ClimbToTopMontage || Montage == VaultMontage)
	{
		SetMovementMode(MOVE_Walking);
	}
}

void UCustomMovementComponent::RequestHopping()
{	
	const FVector UnrotatedLastInputVector =
	UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(),GetLastInputVector());
	
	const float DotResult =
	FVector::DotProduct(UnrotatedLastInputVector.GetSafeNormal(), FVector::UpVector);

	Debug::Print(TEXT("Dot result: ") + FString::SanitizeFloat(DotResult));

	if(DotResult>=0.9f)
	{
		Debug::Print(TEXT("Hop Up"));
	}
	else if(DotResult<=-0.9f)
	{
		Debug::Print(TEXT("Hop Down"));
	}
	else
	{
		Debug::Print(TEXT("Invalid Input Range"));
	}
}

void UCustomMovementComponent::SetMotionWarpTarget(const FName & InWarpTargetName, const FVector & InTargetPosition)
{
	if(!OwningPlayerCharacter) return;

	OwningPlayerCharacter->GetMotionWarpingComponent()->AddOrUpdateWarpTargetFromLocation(
		InWarpTargetName,
		InTargetPosition
	);
}

FVector UCustomMovementComponent::GetUnrotatedClimbVelocity() const
{
	return UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(),Velocity);
}

#pragma endregion