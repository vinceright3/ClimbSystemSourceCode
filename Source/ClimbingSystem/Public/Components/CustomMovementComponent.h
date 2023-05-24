// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomMovementComponent.generated.h"

DECLARE_DELEGATE(FOnEnterClimbState)
DECLARE_DELEGATE(FOnExitClimbState)

class UAnimMontage;
class UAnimInstance;
class AClimbingSystemCharacter;

UENUM(BlueprintType)
namespace ECustomMovementMode
{
	enum Type
	{
		MOVE_Climb UMETA(DisplayName = "Climb Mode")
	};
}
/**
 * 
 */
UCLASS()
class CLIMBINGSYSTEM_API UCustomMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	FOnEnterClimbState OnEnterClimbStateDelegate;
	FOnExitClimbState OnExitClimbStateDelegate;

protected:

#pragma region OverridenFunctions
	
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;
	virtual FVector ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const override;

#pragma endregion

private:

#pragma region ClimbTraces
	
	TArray<FHitResult> DoCapsuleTraceMultiByObject(const FVector& Start,const FVector& End,bool bShowDebugShape = false,bool bDrawPersistantShapes = false);
	FHitResult DoLineTraceSingleByObject(const FVector& Start,const FVector& End,bool bShowDebugShape = false,bool bDrawPersistantShapes = false);

#pragma endregion

#pragma region ClimbCore
	
	bool TraceClimbableSurfaces();

	FHitResult TraceFromEyeHeight(float TraceDistance,float TraceStartOffset = 0.f,bool bShowDebugShape = false,bool bDrawPersistantShapes = false);

	bool CanStartClimbing();

	bool CanClimbDownLedge();

	void StartClimbing();

	void StopClimbing();

	void PhysClimb(float deltaTime, int32 Iterations);

	void ProcessClimableSurfaceInfo();

	bool CheckShouldStopClimbing();

	bool CheckHasReachedFloor();

	FQuat GetClimbRotation(float DeltaTime);

	void SnapMovementToClimableSurfaces(float DeltaTime);

	bool CheckHasReachedLedge();

	void TryStartVaulting();

	bool CanStartVaulting(FVector& OutVaultStartPosition,FVector& OutVaultLandPosition);

	void PlayClimbMontage(UAnimMontage* MontageToPlay);

	UFUNCTION()
	void OnClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void SetMotionWarpTarget(const FName& InWarpTargetName,const FVector& InTargetPosition);

	void HandleHopUp();

	bool CheckCanHopUp(FVector& OutHopUpTargetPosition);

	void HandleHopDown();

	bool CheckCanHopDown(FVector& OutHopDownTargetPosition);

#pragma endregion

#pragma region ClimbCoreVariables
	
	TArray<FHitResult> ClimbableSurfacesTracedResults;

	FVector CurrentClimbableSurfaceLocation;

	FVector CurrentClimbableSurfaceNormal;

	UPROPERTY()
	UAnimInstance* OwningPlayerAnimInstance;

	UPROPERTY()
	AClimbingSystemCharacter* OwningPlayerCharacter;

#pragma endregion

#pragma region ClimbBPVariables
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Character Movement: Climbing",meta = (AllowPrivateAccess = "true"))
	TArray<TEnumAsByte<EObjectTypeQuery> > ClimbableSurfaceTraceTypes;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Character Movement: Climbing",meta = (AllowPrivateAccess = "true"))
	float ClimbCapsuleTraceRadius = 50.f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Character Movement: Climbing",meta = (AllowPrivateAccess = "true"))
	float ClimbCapsuleTraceHalfHeight = 72.f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Character Movement: Climbing",meta = (AllowPrivateAccess = "true"))
	float MaxBreakClimbDeceleration = 400.f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Character Movement: Climbing",meta = (AllowPrivateAccess = "true"))
	float MaxClimbSpeed = 100.f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Character Movement: Climbing",meta = (AllowPrivateAccess = "true"))
	float MaxClimbAcceleration = 300.f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Character Movement: Climbing",meta = (AllowPrivateAccess = "true"))
	float ClimbDownWalkableSurfaceTraceOffset = 100.f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Character Movement: Climbing",meta = (AllowPrivateAccess = "true"))
	float ClimbDownLedgeTraceOffset = 50.f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Character Movement: Climbing",meta = (AllowPrivateAccess = "true"))
	UAnimMontage* IdleToClimbMontage;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Character Movement: Climbing",meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ClimbToTopMontage;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Character Movement: Climbing",meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ClimbDownLedgeMontage;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Character Movement: Climbing",meta = (AllowPrivateAccess = "true"))
	UAnimMontage* VaultMontage;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Character Movement: Climbing",meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HopUpMontage;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Character Movement: Climbing",meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HopDownMontage;

#pragma endregion

public:
	void ToggleClimbing(bool bEnableClimb);
	void RequestHopping();
	bool IsClimbing() const;
	FORCEINLINE FVector GetClimbableSurfaceNormal() const {return CurrentClimbableSurfaceNormal;}
	FVector GetUnrotatedClimbVelocity() const;
};
