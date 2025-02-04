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
	TerminalSpeed = -500.0f;
	Gravity = -981.0f;
	FallSpeed = 0.0f;
	IsFall = false;
	IsJumping = false;
	
	MouseXSensitive = 180.0f;
	MouseYSensitive = 180.0f;
	MaxControllerRotation = FRotator(180.0f, 180.0f, 180.0f);
}

// Called when the game starts or when spawned
void APracticeCharacter::BeginPlay()
{
	Super::BeginPlay();
	
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

void APracticeCharacter::StartJumpInput(const FInputActionValue& Value)
{
	if (!IsFall)
	{
		IsJumping = true;
		FallSpeed = JumpVelocity;
	}
}


void APracticeCharacter::PlaneMove(float DeltaTime)
{
	// 새로운 속력 계산
	const float TargetVelocity = InputDirection.IsZero() ? 0.0f : MoveSpeed;
	Velocity = CalculateVelocity(Velocity, TargetVelocity, DeltaTime);

	// 새로운 속도를 계산
	// 로테이터의 2차원 회전을 구한 다음에 회전 방향을 기준으로 Forward, Right Vector를 분리한다.
	// 그 후에 각각을 입력 벡터를 이용해서 현재 프레임에서의 월드 기준 이동 방향을 구한다.
	const FRotator ControllerRotator = FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector ControllerForwardVector = FRotationMatrix(ControllerRotator).GetUnitAxis(EAxis::X);
	const FVector ControllerRightVector = FRotationMatrix(ControllerRotator).GetUnitAxis(EAxis::Y);
	MoveDirection = ControllerForwardVector * InputDirection.X + ControllerRightVector * InputDirection.Y;

	// MoveDirection으로 회전
	FaceDirection(MoveDirection, DeltaTime, false, TurnSmoothingDamp);
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

void APracticeCharacter::FaceDirection(FVector NewDirection, float DeltaTime, bool bIsImmediate, float Damp)
{
	if (NewDirection.IsZero())
		return;
	
	FRotator TargetRotator = FRotationMatrix::MakeFromX(NewDirection).Rotator();
	TargetRotator.Pitch = 0.0f;
	TargetRotator.Roll = 0.0f;

	FRotator SmoothRotator = FMath::RInterpTo(GetActorRotation(), TargetRotator, DeltaTime, Damp);
	SetActorRotation(SmoothRotator);
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
			IsJumping = false;
			IsFall = false;
		}
		// else : 벽에 충돌
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("FallSpeed: %f"), FallSpeed);
		IsFall = true;
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

	if (!IsFall && !IsJumping)
	{
		FallSpeed = 0;
	}
	PlaneMove(DeltaTime);
	Move(DeltaTime);

	UpdateCamera();
}

