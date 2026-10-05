#include "Afterlight/AfterlightRiftComponent.h"

#include "Afterlight/AfterlightRiftPortal.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"

UAfterlightRiftComponent::UAfterlightRiftComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UAfterlightRiftComponent::BeginPlay()
{
	Super::BeginPlay();
	BindInput();
}

void UAfterlightRiftComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PulseCooldownLeft = FMath::Max(0.f, PulseCooldownLeft - DeltaTime);
	if (!bInputBound)
	{
		BindInput();
	}
}

void UAfterlightRiftComponent::BindInput()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}
	APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	if (!PC)
	{
		return;
	}
	UInputComponent* IC = Pawn->InputComponent;
	if (!IC)
	{
		IC = PC->InputComponent;
	}
	if (!IC)
	{
		return;
	}

	IC->BindKey(EKeys::Q, IE_Pressed, this, &UAfterlightRiftComponent::PlaceSolarRift);
	IC->BindKey(EKeys::E, IE_Pressed, this, &UAfterlightRiftComponent::PlaceVoidRift);
	IC->BindKey(EKeys::R, IE_Pressed, this, &UAfterlightRiftComponent::ActivateRiftPulse);
	// Thumb buttons if present
	IC->BindKey(EKeys::ThumbMouseButton, IE_Pressed, this, &UAfterlightRiftComponent::PlaceSolarRift);
	IC->BindKey(EKeys::ThumbMouseButton2, IE_Pressed, this, &UAfterlightRiftComponent::PlaceVoidRift);
	bInputBound = true;
}

void UAfterlightRiftComponent::PlaceSolarRift()
{
	PlaceRift(EAfterlightRiftTone::Solar);
}

void UAfterlightRiftComponent::PlaceVoidRift()
{
	PlaceRift(EAfterlightRiftTone::Void);
}

bool UAfterlightRiftComponent::TracePlacement(FVector& OutLocation, FRotator& OutRotation) const
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		return false;
	}
	APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	FVector Start = Pawn->GetPawnViewLocation();
	FRotator ViewRot = Pawn->GetViewRotation();
	if (PC && PC->PlayerCameraManager)
	{
		Start = PC->PlayerCameraManager->GetCameraLocation();
		ViewRot = PC->PlayerCameraManager->GetCameraRotation();
	}
	const FVector End = Start + ViewRot.Vector() * PlaceRange;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RiftPlace), true, Pawn);
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	if (bHit)
	{
		OutLocation = Hit.ImpactPoint + Hit.ImpactNormal * 40.f;
		OutRotation = Hit.ImpactNormal.Rotation();
	}
	else
	{
		OutLocation = End;
		OutRotation = ViewRot;
	}
	return true;
}

void UAfterlightRiftComponent::PlaceRift(EAfterlightRiftTone Tone)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !GetWorld())
	{
		return;
	}

	FVector Loc;
	FRotator Rot;
	if (!TracePlacement(Loc, Rot))
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = OwnerActor;
	Params.Instigator = Cast<APawn>(OwnerActor);
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (Tone == EAfterlightRiftTone::Solar)
	{
		if (IsValid(SolarRift))
		{
			SolarRift->Destroy();
			SolarRift = nullptr;
		}
	}
	else if (IsValid(VoidRift))
	{
		VoidRift->Destroy();
		VoidRift = nullptr;
	}

	AAfterlightRiftPortal* NewRift = GetWorld()->SpawnActor<AAfterlightRiftPortal>(
		AAfterlightRiftPortal::StaticClass(), Loc, Rot, Params);
	if (!NewRift)
	{
		return;
	}

	if (Tone == EAfterlightRiftTone::Solar)
	{
		SolarRift = NewRift;
	}
	else
	{
		VoidRift = NewRift;
	}
	NewRift->Configure(Tone, nullptr, OwnerActor);

	// Link pair when both exist
	if (IsValid(SolarRift) && IsValid(VoidRift))
	{
		SolarRift->SetSibling(VoidRift);
		VoidRift->SetSibling(SolarRift);
		SolarRift->Configure(EAfterlightRiftTone::Solar, VoidRift, OwnerActor);
		VoidRift->Configure(EAfterlightRiftTone::Void, SolarRift, OwnerActor);
	}
}

void UAfterlightRiftComponent::ActivateRiftPulse()
{
	if (PulseCooldownLeft > 0.f)
	{
		return;
	}
	if (!IsValid(SolarRift) || !IsValid(VoidRift))
	{
		return;
	}
	PulseCooldownLeft = PulseCooldown;
	SolarRift->PulseLink(PulseDuration, PulseDPS);
	VoidRift->PulseLink(PulseDuration, PulseDPS);
}
