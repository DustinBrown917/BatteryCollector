// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "BatteryCollectorGameMode.h"
#include "BatteryCollectorCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "SpawnVolume.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PawnMovementComponent.h"

ABatteryCollectorGameMode::ABatteryCollectorGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	DecayRate = 0.01f;
}

void ABatteryCollectorGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ABatteryCollectorCharacter* MyCharacter = Cast<ABatteryCollectorCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (MyCharacter) {
		if (MyCharacter->GetCurrentPower() > PowerToWin) {
			SetCurrentState(EBatteryPlayState::EWon);
		}
		else if (MyCharacter->GetCurrentPower() > 0) {
			MyCharacter->UpdatePower(-DecayRate * DeltaTime * (MyCharacter->GetInitialPower()));
		}
		else {
			SetCurrentState(EBatteryPlayState::EGameOver);
		}
	}
}

float ABatteryCollectorGameMode::GetPowerToWin() const
{
	return PowerToWin;
}

void ABatteryCollectorGameMode::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnVolume::StaticClass(), FoundActors);

	for (auto Actor : FoundActors) {
		ASpawnVolume* SpawnVolumeActor = Cast<ASpawnVolume>(Actor);

		if (SpawnVolumeActor) {
			SpawnVolumeActors.AddUnique(SpawnVolumeActor);
		}
	}

	SetCurrentState(EBatteryPlayState::EPlaying);

	ABatteryCollectorCharacter* MyCharacter = Cast<ABatteryCollectorCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (MyCharacter) {
		PowerToWin = MyCharacter->GetInitialPower() * 1.25f;
	}

	if (HUDWidgetClass != nullptr) {
		CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), HUDWidgetClass);
		if (CurrentWidget != nullptr) {
			CurrentWidget->AddToViewport();
		}
	}

}

EBatteryPlayState ABatteryCollectorGameMode::GetCurrentState() const {
	return CurrentState;
}

void ABatteryCollectorGameMode::SetCurrentState(EBatteryPlayState NewState) {
	CurrentState = NewState;
	HandleNewState(NewState);
}

void ABatteryCollectorGameMode::HandleNewState(EBatteryPlayState NewState)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	ACharacter* MyCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);

	switch (NewState)
	{
	case EBatteryPlayState::EPlaying:

		for (ASpawnVolume* Volume : SpawnVolumeActors) {
			Volume->SetSpawningActive(true);
		}

		break;
	case EBatteryPlayState::EGameOver:

		for (ASpawnVolume* Volume : SpawnVolumeActors) {
			Volume->SetSpawningActive(false);
		}

		
		if (PlayerController) {
			PlayerController->SetCinematicMode(true, false, false, true, true);
		}

		if (MyCharacter) {
			MyCharacter->GetMesh()->SetSimulatePhysics(true);
			MyCharacter->GetMovementComponent()->MovementState.bCanJump = false;
		}


		break;
	case EBatteryPlayState::EWon:

		for (ASpawnVolume* Volume : SpawnVolumeActors) {
			Volume->SetSpawningActive(false);
		}

		break;

	default:
	case EBatteryPlayState::EUnknown:
		break;
	}

}
