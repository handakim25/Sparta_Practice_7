// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PracticeCharacter.generated.h"

struct FInputActionValue;
struct FInputActionInstance;

UCLASS()
class SPARTA_PRACTICE_7_API APracticeCharacter : public APawn
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APracticeCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	TObjectPtr<class UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	TObjectPtr<class USkeletalMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	TObjectPtr<class USpringArmComponent> SpringArmComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	TObjectPtr<class UCameraComponent> CameraComponent;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	UFUNCTION()
	void MoveInput(const FInputActionValue& Value);
	
	UFUNCTION()
	void LookInput(const FInputActionValue& Value);

	UFUNCTION()
	void StartJumpInput(const FInputActionValue& Value);

	// 이동 입력 방향(정규화되어 있다.)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	FVector2D InputDirection;	
	
public:
	// Move 아이디어
	// 1. 이동 방향을 Input에서 지정, Tick에서 Input을 기록된 Input을 이용해서 이동 처리
	// 2. 이동 중이라면 속력을 갱신, 가속도에 의해서 속력이 갱신된다. 속력은 선형적으로 증가
	// 3. 충돌 감지 필요(지면이 없어지는 Fall 상황 확인을 하기 위해서는 지속적으로 하방으로 속력을 주어야 한다. 혹은 하방으로 라인 검출을 진행, 요구 사항에 맞추어서 작성할 것)
	// 4. 이동 방향은 Camera를 기준으로 움직인다.
	// 5. Actor의 회전 방향은 정의되지 않았으므로 이동 방향으로 한다.

	// 구현 로직
	// 1. Input Callback에서 현재 Input 방향을 설정
	// 2. 현재 프레임 속력을 계산
	// 3. Controller 방향을 이용해서 이동 속도를 계산, 이동 방향 기록
	// 4. 이동 방향으로 회전
	// 5. Tick의 마지막에서 Player 이동을 실행
	
	// 최대 이동 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
	float MoveSpeed;

	// 가속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
	float Accel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
	float TurnSmoothingDamp;

	// 타고 올라갈 수 있는 경사로 각도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
	float AllowedSlopeAngle;

	// 올라갈 수 있는 계단 높이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
	float StepHeight;
	
protected:
	// 현재 속력
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement)
	float Velocity;

	// 현재 프레임 월드 공간 이동 방향
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement)
	FVector MoveDirection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement)
	FVector2D LastMovementDirection;
	
	// 2차원에서의 일반적인 움직임
	// 속력, 속도를 갱신하고 회전을 한다.
	void PlaneMove(float DeltaTime);
	float CalculateVelocity(float CurrentVelocity, float TargetVelocity, float DeltaTime) const;
	void FaceDirection(FVector NewDirection, float DeltaTime, bool bIsImmediate, float Damp);

	// 현재 속도에 맞추어서 실질적인 이동
	void Move(float DeltaTime);

public:
	// Jump 아이디어
	// 1. Jump를 실행할 경우 Z축 속도를 +로 만들어서 공중으로 올린다. 매 프레임 공중 속도를 계산
	// 2. Fall 상태를 추적하기 위해서는 2가지 방법이 있다. 하방으로 매 프레임 내리는 경우와 하방으로 Ground Check를 하는 것이다. 지금은 구현의 용이성을 위해 매 프레임 중력 값을 적용한다.
	// 3. 충돌을 이용해서 IsFall을 판별 - 공중에서의 이동 제어, Fall Animation 출력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Jump")
	float JumpVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Jump")
	float TerminalSpeed;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Jump")
	float Gravity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Jump")
	float FallSpeed;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Movement|Jump")
	bool IsFall;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Jump")
	bool IsJumping;
	
protected:
	void UpdateFallSpeed(float DeltaTime);
	
public:
	// Look : 3인칭 시점 구현 : Spring Arm이 Controller에 대응되게 회전, Camera 위치 고정
	// 0. 마우스 입력을 회전 값에 더하고 최종 결과를 Tick에서 조정한다.
	// Enhanced Input의 콜백이 Tick 사이에 한 번만 호출되는지는 확인하지 않았기 때문에 여러번 호출될 수 있다고 가정하고 구현
	// 1. Tick에서 Controller의 회전을 계산
	// 2. Controller의 회전에 따라 Camera의 방향을 결정
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float MouseXSensitive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float MouseYSensitive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	FRotator MaxControllerRotation;

	FRotator DeltaCameraRotator;

	UFUNCTION(BlueprintCallable, Category="Input")
	void AddControllerRotation(float Pitch, float Yaw, float Roll);

protected:
	
	// Controller의 방향에 따라 Camera의 회전을 결정
	void UpdateControllerRotation(float DeltaTime);
	void UpdateCamera();
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
