// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Engine/SkeletalMeshSocket.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystemComponent.h"

// Sets default values
AShooterCharacter::AShooterCharacter():
	BaseTurnRate(45.f),
	BaseLookupRate(45.f)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create CameraBoom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 500.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 50.f);
	// Create FollowCamera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Configure Character Movement
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;
}

// Called when the game starts or when spawned
void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookupRate", this, &AShooterCharacter::LookupAtRate);
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Lookup", this, &APawn::AddControllerPitchInput);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AShooterCharacter::FireWeapon);

}

void AShooterCharacter::MoveForward(float value)
{
	if(Controller != nullptr && value != 0){
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ 0, Rotation.Yaw, 0 };
		const FVector Direction{ FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X)};
		AddMovementInput(Direction, value);
	}
}

void AShooterCharacter::MoveRight(float value)
{
	if(Controller != nullptr && value != 0){
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ 0, Rotation.Yaw, 0 };
		const FVector Direction{ FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y)};
		AddMovementInput(Direction, value);
	}
}

void AShooterCharacter::TurnAtRate(float rate)
{
	AddControllerYawInput(rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::LookupAtRate(float rate)
{
	AddControllerPitchInput(rate * BaseLookupRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::FireWeapon()
{
	const USkeletalMeshSocket* BarrelSocket = GetMesh()->GetSocketByName("BarrelSocket");
	if(BarrelSocket){
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(GetMesh());

		if(MuzzleFlash){
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, SocketTransform);
		}
		if(FireSound){
			UGameplayStatics::SpawnSoundAtLocation(GetWorld(), FireSound, SocketTransform.GetLocation());
		}

		// Get viewport size
		FVector2D ViewportSize;
		if(GEngine && GEngine->GameViewport){
			GEngine->GameViewport->GetViewportSize(ViewportSize);
		}

		// Get Screen space of crosshair
		FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y /2.f);
		CrosshairLocation.Y -= 50.f;
		FVector CrosshairWorldPosition;
		FVector CrosshairWorldDirection;

		// Get World location of crosshair
		bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0), 
			CrosshairLocation, 
			CrosshairWorldPosition,
			CrosshairWorldDirection);

		if(bScreenToWorld){
			FHitResult ScreenTraceHit;
			const FVector Start(CrosshairWorldPosition);
			const FVector End(CrosshairWorldPosition + CrosshairWorldDirection * 50'000.f);
			// Create End point
			FVector BeamEndPoint(End);
			//Trace
			GetWorld()->LineTraceSingleByChannel(ScreenTraceHit, Start, End, ECollisionChannel::ECC_Visibility);

			if(ScreenTraceHit.bBlockingHit){
				// Set End point to object hit
				BeamEndPoint = ScreenTraceHit.Location;
				if(ImpactParticles){
					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, ScreenTraceHit.Location);
				}
			}
			if(BeamParticles){
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform);
				if(Beam){
					Beam->SetVectorParameter(FName("Target"), BeamEndPoint);
				}
			}
		}
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && HipFireMontage){
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
}

