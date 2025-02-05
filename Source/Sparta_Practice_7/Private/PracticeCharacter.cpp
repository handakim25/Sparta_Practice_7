// Fill out your copyright notice in the Description page of Project Settings.


#include "PracticeCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "MaterialHLSLTree.h"
#include "PracticeController.h"

// Sets default values
APracticeCharacter::APracticeCharacter()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	CapsuleComponent->SetCapsuleSize(34.0f, 88.0f);
	CapsuleComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	SetRootComponent(CapsuleComponent);

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(CapsuleComponent);

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComponent->TargetArmLength = 300.0f;
	SpringArmComponent->SetupAttachment(CapsuleComponent);

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);

	MoveSpeed = 600.0f;
	Accel = 2048.0f;
	TurnSmoothingDamp = 5.0f;

	JumpVelocity = 500.0f;
	TerminalSpeed = -2500.0f;
	LandingLockTime = 0.1f;
	Gravity = -981.0f;
	GroundCheckDistance = 1.0f;
	AirSpeedMultiplier = 0.2f;
	FallSpeed = 0.0f;
	bIsFall = false;
	bIsJumping = false;
	bIsLanding = false;
	bApplyGravity = true;
	
	MouseXSensitive = 180.0f;
	MouseYSensitive = 180.0f;
	MaxControllerRotation = FRotator(180.0f, 180.0f, 180.0f);
}

// Called when the game starts or when spawned
void APracticeCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 초기 공중 상태 결정
	if (IsOnGround())
	{
		bIsFall = false;
	}
	else
	{
		bIsFall = true;
	}
}

void APracticeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (const APracticeController* PracticeController = Cast<APracticeController>(GetController()))
		{
			if (PracticeController->MoveAction)
			{
				EnhancedInputComponent->BindAction(
					PracticeController->MoveAction,
					ETriggerEvent::Triggered,
					this,
					&APracticeCharacter::MoveInput
				);
			}
			
			if (PracticeController->MoveAction)
			{
				EnhancedInputComponent->BindAction(
					PracticeController->MoveAction,
					ETriggerEvent::Completed,
					this,
					&APracticeCharacter::MoveInput
				);
			}

			if (PracticeController->LookAction)
			{
				EnhancedInputComponent->BindAction(
					PracticeController->LookAction,
					ETriggerEvent::Triggered,
					this,
					&APracticeCharacter::LookInput
				);
			}

			if (PracticeController->JumpAction)
			{
				EnhancedInputComponent->BindAction(
					PracticeController->JumpAction,
					ETriggerEvent::Triggered,
					this,
					&APracticeCharacter::StartJumpInput
				);
			}
		}
	}
}

void APracticeCharacter::MoveInput(const FInputActionValue& Value)
{
	// Trigger, Completed를 처리해서 항상 현재 이동 방향을 가지고 있다.
	// X축 : Forward, Y축 : Right
	InputDirection = Value.Get<FVector2D>().GetSafeNormal();
}

void APracticeCharacter::LookInput(const FInputActionValue& Value)
{
	// 카메라가 꼭대기로 올라가면(아래를 보게 되면) -90
	// 카메라가 바닥으로 내려가면(위를 보게 되면) +90
	// Y를 일단은 반전시키지 않고 바로 적용하기로 한다.
	// Y : Pitch, X : Yaw
	const FVector2D LookDirection = Value.Get<FVector2D>();
	AddControllerRotation(LookDirection.Y, LookDirection.X, 0.0f);
}

void APracticeCharacter::Jump()
{
	bIsJumping = true;
	FallSpeed = JumpVelocity;

	FaceDirection(GetMoveDirectionFromController());
}

void APracticeCharacter::StartJumpInput(const FInputActionValue& Value)
{
	if (!bIsFall && !bIsLanding)
	{
		Jump();
	}
}


void APracticeCharacter::PlaneMove(float DeltaTime)
{
	// 새로운 속력 계산
	const float TargetVelocity = InputDirection.IsZero() ? 0.0f : MoveSpeed;
	Velocity = CalculateVelocity(Velocity, TargetVelocity, DeltaTime);

	// 새로운 속도를 계산
	// 카메라 기준 이동 벡터를 그대로 사용
	MoveDirection = GetMoveDirectionFromController();

	// MoveDirection으로 회전
	FaceDirection(MoveDirection, DeltaTime, TurnSmoothingDamp);
}

float APracticeCharacter::CalculateVelocity(float CurrentVelocity, float TargetVelocity, const float DeltaTime) const
{
	if (FMath::IsNearlyEqual(TargetVelocity, CurrentVelocity))
	{
		return TargetVelocity;
	}

	const float Alpha = FMath::Clamp(DeltaTime * Accel, 0.f, 1.f);
	return FMath::Lerp(CurrentVelocity, TargetVelocity, Alpha);
}

FVector APracticeCharacter::GetMoveDirectionFromController() const
{
	// 그 후에 각각을 입력 벡터를 이용해서 현재 프레임에서의 월드 기준 이동 방향을 구한다.
	const FRotator ControllerRotator = FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector ControllerForwardVector = FRotationMatrix(ControllerRotator).GetUnitAxis(EAxis::X);
	const FVector ControllerRightVector = FRotationMatrix(ControllerRotator).GetUnitAxis(EAxis::Y);
	return (ControllerForwardVector * InputDirection.X + ControllerRightVector * InputDirection.Y).GetSafeNormal();
}

void APracticeCharacter::FaceDirection(FVector NewDirection, float DeltaTime, float Damp)
{
	if (NewDirection.IsZero())
		return;
	
	FRotator TargetRotator = FRotationMatrix::MakeFromX(NewDirection).Rotator();
	TargetRotator.Pitch = 0.0f;
	TargetRotator.Roll = 0.0f;

	FRotator SmoothRotator = FMath::RInterpTo(GetActorRotation(), TargetRotator, DeltaTime, Damp);
	SetActorRotation(SmoothRotator);
}

void APracticeCharacter::FaceDirection(FVector NewDirection)
{
	if (NewDirection.IsZero())
		return;
	
	FRotator TargetRotator = FRotationMatrix::MakeFromX(NewDirection).Rotator();
	TargetRotator.Pitch = 0.0f;
	TargetRotator.Roll = 0.0f;

	SetActorRotation(TargetRotator);
}

void APracticeCharacter::Move(float DeltaTime)
{
	// Problem1 : 이동을 한 번에 처리하니까 바로 지면에 충돌해서 움직일 수 없는 상황이 발생
	// 수평 / 수직 이동을 분리해서 처리한다.
	FVector HorizontalDelta = MoveDirection * Velocity * DeltaTime;
	FHitResult HorizontalHit;
	AddActorWorldOffset(HorizontalDelta, true, &HorizontalHit);

	// Problem2 : 경사로를 올라갈 수 없는 문제
	// 1. 평지에서 경사로로 이동할 경우(평지에서 이동하고 남은 거리를 경사로로 이동)
	// 2. 경사로에서 경사로로 이동할 경우(경사로 방향으로 이동 벡터를 조정)
	float SlopeAngle = 0.f;
	if (HorizontalHit.IsValidBlockingHit())
	{
		// 
		SlopeAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(HorizontalHit.ImpactNormal, FVector::UpVector)));
		UE_LOG(LogTemp, Display, TEXT("Angle : %f"), SlopeAngle);
	}
	
	UpdateFallSpeed(DeltaTime);
	FVector VerticalDelta = FVector::UpVector * FallSpeed * DeltaTime;
	FHitResult VerticalHit;
	AddActorWorldOffset(VerticalDelta, true, &VerticalHit);
	
	if (VerticalHit.IsValidBlockingHit())
	{
		if (VerticalHit.ImpactNormal.Z > 0.7f)
		{
			// 지상과 충돌했을 경우
			LandStart();
			UE_LOG(LogTemp, Display, TEXT("Land"));
		}
		// else : 벽에 충돌
	}
	else
	{
		// 지상과 충돌하지 않았을 경우 공중이다.

		// 문제1. 첫 프레임에서 공중에서 지상으로 이동하는 버그
		// 문제2. 첫 프레임에서 Fall->Land->Fall로 이동하는 버그
		// 일단은 이동 자체는 바닥과 붙어 있어야 하니까 그대로 하고 공중 판정을 따로 레이캐스트로 진행해서 일정 거리 이하는 전부 지상 판정으로 구현하도록 한다.

		// 추가 문제3. 이전에도 확인했던 스카이 콩콩 문제가 발생
		// 이는 착지 모션이 충분히 나오기 전에 점프를 다시 실행해서 발생하는 문제이다. 다음과 같은 방법을 고려했다.
		// 1. 일정 시간 점프 잠금 : 구현이 용이해서 해당 방법을 선택
		// 2. Animation Notify를 이용해서 Land 모션을 기다림 : Animator마다 설정하는 것은 번거롭기에 다른 방법을 고려
		// 3. 발의 Transform을 찾아서 정확한 착지 순간을 찾기 : 일반적인 상황에서 잘 작동하겠지만 착지 모션이 특이해서 발이 땅에 안 닿을 수 있다.
		
		UE_LOG(LogTemp, Display, TEXT("Air"));
		if (IsOnGround())
		{
			LandStart();
			UE_LOG(LogTemp, Display, TEXT("Land By Line Trace"));
		}
		else
		{
			bIsFall = true;
		}
	}
}

void APracticeCharacter::UpdateFallSpeed(float DeltaTime)
{
	if (FallSpeed >= 0)
	{
		FallSpeed += Gravity * DeltaTime;
	}
	else if (FallSpeed > TerminalSpeed)
	{
		FallSpeed += Gravity * DeltaTime;
		FallSpeed = FallSpeed > TerminalSpeed ? FallSpeed : TerminalSpeed;
	}
}

bool APracticeCharacter::IsOnGround()
{
	FVector StartLocation = GetActorLocation() - FVector(0, 0, CapsuleComponent->GetScaledCapsuleHalfHeight());
	FVector EndLocation = StartLocation - FVector::UpVector * GroundCheckDistance;

	FHitResult GroundHit;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(GroundHit, StartLocation, EndLocation, ECC_Visibility, CollisionParams);
	return bHit;
}

// @To-Do: 속도 / 가속도 이동 방향 계산으로 통합하면 PlaneMove를 통합해서 처리할 수도 있을 것 같다.
void APracticeCharacter::AirPlaneMove(float DeltaTime)
{
	// 속력 계산
	// 공중 이동 시에는 초기 속도는 유지가 된다.
	// 이동 방향에 대해서는 가속도 형식으로 더해준다.
	// 현재는 속력과 이동 방향으로 관리하므로 최종 계산 후에 속력과 이동 방향을 계산해준다.

	FVector CurrentSpeed = Velocity * MoveDirection;
	
	// 문제. 가속도 값만을 제한하니까 최종 속력을 넘어서 빨라질 수 있는 현상이 있다.
	// 최종 속도를 값을 확인해서 최대 속도 이상으로 넘어가지 못하도록 수정
	float AccelAmount = Accel * AirSpeedMultiplier * DeltaTime;
	AccelAmount = FMath::Clamp(AccelAmount, 0.f, MoveSpeed * DeltaTime);
	FVector Acceleration = GetMoveDirectionFromController() * AccelAmount;
	
	FVector NewSpeed = CurrentSpeed + Acceleration;
	if (NewSpeed.Length() > MoveSpeed)
	{
		NewSpeed = NewSpeed.GetSafeNormal() * MoveSpeed;
	}
	MoveDirection = NewSpeed.GetSafeNormal();
	Velocity = NewSpeed.Length();
}

void APracticeCharacter::LandStart()
{
	if (bIsFall)
	{
		bIsLanding = true;

		GetWorldTimerManager().SetTimer(LandingLockTimerHandle, this, &APracticeCharacter::LandEnd, LandingLockTime, false);
	}
	
	// To-Do
	// 1. bIsJumping, bIsFall을 하나로 통합할 수 있는지 확인할 것
	// 2. 객체는 한 번에 하나의 상태를 가지고 있으므로 enum으로 현재 상태를 표현하는 것을 고려할 것.
	// State Machine처럼 고려해도 될 것 같다.
	// 지금 위에서 IsFall을 true로 바꾸는 순간 Land를 실행하는데 이는 State가 Fall에서 Land로 바뀌고 State Enter를 처리하는 것과 유사하다.
	bIsJumping = false;
	bIsFall = false;
}

void APracticeCharacter::LandEnd()
{
	bIsLanding = false;
}

void APracticeCharacter::AddControllerRotation(float Pitch, float Yaw, float Roll)
{
	DeltaCameraRotator.Pitch += Pitch;
	DeltaCameraRotator.Yaw += Yaw;
	DeltaCameraRotator.Roll += Roll;
}

void APracticeCharacter::UpdateControllerRotation(float DeltaTime)
{
	FRotator CurrentDeltaRotator = DeltaCameraRotator * DeltaTime;
	CurrentDeltaRotator.Yaw *= MouseXSensitive;
	CurrentDeltaRotator.Yaw = FMath::Clamp(CurrentDeltaRotator.Yaw, -MaxControllerRotation.Yaw, MaxControllerRotation.Yaw);
	CurrentDeltaRotator.Pitch *= MouseYSensitive;
	CurrentDeltaRotator.Pitch = FMath::Clamp(CurrentDeltaRotator.Pitch, -MaxControllerRotation.Pitch, MaxControllerRotation.Pitch);;
	CurrentDeltaRotator.Roll = FMath::Clamp(CurrentDeltaRotator.Roll, -MaxControllerRotation.Roll, MaxControllerRotation.Roll);
	const FRotator ControllerRotator = Controller->GetControlRotation();
	// 반대로 회전할 수 있나? Normalize가 이를 방지할 수 있다고 알고 있는데 추후에 찾아볼 것.
	FRotator NewRotator = CurrentDeltaRotator + ControllerRotator;
	NewRotator.Normalize();
	Controller->SetControlRotation(NewRotator);
	
	DeltaCameraRotator = FRotator::ZeroRotator;
}

void APracticeCharacter::UpdateCamera()
{
	SpringArmComponent->SetWorldRotation(Controller->GetControlRotation());
}

// Called every frame
void APracticeCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 카메라 방향, 이동, 카메라의 위치 결정 순서로 작성, 추후에 이상이 있을 경우 순서 조정

	UpdateControllerRotation(DeltaTime);

	// 지상 이동
	if (!bIsFall && !bIsJumping)
	{
		FallSpeed = 0;
		PlaneMove(DeltaTime);
	}
	// 공중 이동
	else
	{
		AirPlaneMove(DeltaTime);
	}

	Move(DeltaTime);

	UpdateCamera();
}

