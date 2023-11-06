﻿// TurretGame by Team #1. AlphaNova courses

#include "Player/TGTank.h"
#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/BoxComponent.h"
#include "Components/TGMovementComponent.h"

#include "Gameplay/TGBushStealth.h"

#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

ATGTank::ATGTank()
{
    PrimaryActorTick.bCanEverTick = true;

    Foundation = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Foundation"));
    Foundation->SetSimulatePhysics(true);
    Foundation->SetCollisionObjectType(ECC_Pawn);
    Foundation->SetLinearDamping(1.f);
    SetRootComponent(Foundation);

    BushCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BushCollisionBox"));
    BushCollisionBox->SetupAttachment(GetRootComponent());

    GroundBoxComp->SetupAttachment(GetRootComponent());
    Tower->SetupAttachment(GetRootComponent());
    Gun->SetupAttachment(Tower);
    SpringArmComp->SetupAttachment(GetRootComponent());
    CameraComp->SetupAttachment(SpringArmComp);

    MovementComp = CreateDefaultSubobject<UTGMovementComponent>(TEXT("MovementComp"));
}

void ATGTank::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    BushCollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ATGTank::OnBushCollisionBeginOverlap);
    BushCollisionBox->OnComponentEndOverlap.AddDynamic(this, &ATGTank::OnBushCollisionEndOverlap);
}

void ATGTank::OnBushCollisionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    SetPawnVisibility(OtherActor, EGameplayVisibility::EPGS_Hidden);
}

void ATGTank::OnBushCollisionEndOverlap(
    UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    SetPawnVisibility(OtherActor, EGameplayVisibility::EPGS_Visible);
}

void ATGTank::SetPawnVisibility(AActor* OtherActor, EGameplayVisibility VisibilityState)
{
    if (!OtherActor) return;

    if (!OtherActor->IsA(ATGBushStealth::StaticClass())) return;

    PawnVisibility = VisibilityState;
}

void ATGTank::BeginPlay()
{
    Super::BeginPlay();

    PlayIdleSound();
}

void ATGTank::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* InputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!InputComp) return;

    InputComp->BindAction(Input_Move, ETriggerEvent::Triggered, this, &ATGTank::Move);
    InputComp->BindAction(Input_Move, ETriggerEvent::Completed, this, &ATGTank::StopMove);
}

void ATGTank::Move(const FInputActionInstance& Instance)
{
    if (!MovementComp) return;
    const FVector2D AxisValue = Instance.GetValue().Get<FVector2D>();

    ActivateRoadSmokeSystem();
    PlayStartMoveSound();

    if (AxisValue.X != 0.f)
    {
        MovementComp->AddImpulseRotate(AxisValue.X);
    }

    if (AxisValue.Y != 0.f)
    {
        MovementComp->AddImpulse(AxisValue.Y);
    }
}

void ATGTank::StopMove(const FInputActionInstance& Instance)
{
    DeactivateRoadSmokeSystem();
    PlayIdleSound();
}

void ATGTank::ChangeTowerRotator()
{
    if (!Tower || !SpringArmComp || !Foundation) return;

    FRotator TowerRot = Tower->GetRelativeRotation();
    TowerRot.Yaw = SpringArmComp->GetTargetRotation().Yaw - Foundation->GetRelativeRotation().Yaw;

    Tower->SetRelativeRotation(TowerRot, true);
}

void ATGTank::ActivateRoadSmokeSystem()
{
    if (!RoadSmokeSystem || RoadSmokeComponent || !MovementComp) return;

    RoadSmokeComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(  //
        RoadSmokeSystem,                                                //
        Foundation,                                                     //
        MovementComp->GetBackwardSocketName(),                          //
        GetActorLocation(),                                             //
        GetActorRotation(),                                             //
        EAttachLocation::KeepWorldPosition,                             //
        false                                                           //
    );
}

void ATGTank::DeactivateRoadSmokeSystem()
{
    if (!RoadSmokeComponent) return;

    RoadSmokeComponent->Deactivate();
    RoadSmokeComponent = nullptr;
}

void ATGTank::PlayIdleSound()
{
    if (IdleSound && !IdleSoundComponent)
    {
        IdleSoundComponent = UGameplayStatics::SpawnSoundAttached(IdleSound, GetRootComponent());
    }

    if (StartMoveSoundComponent && MoveSoundComponent)
    {
        StartMoveSoundComponent->Deactivate();
        StartMoveSoundComponent = nullptr;
        MoveSoundComponent->Deactivate();
        MoveSoundComponent = nullptr;
    }
}

void ATGTank::PlayStartMoveSound()
{
    if (StartMoveSound && MoveSound && !StartMoveSoundComponent && !MoveSoundComponent)
    {
        StartMoveSoundComponent = UGameplayStatics::SpawnSoundAttached(StartMoveSound, GetRootComponent());
        MoveSoundComponent = UGameplayStatics::SpawnSoundAttached(MoveSound, GetRootComponent());
    }

    if (IdleSoundComponent)
    {
        IdleSoundComponent->Deactivate();
        IdleSoundComponent = nullptr;
    }
}

float ATGTank::GetSpeedPercent() const
{
    if (!MovementComp) return 0.f;
    return MovementComp->GetPercentPower();
}
