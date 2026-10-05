#include "Afterlight/AfterlightRiftPortal.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"

AAfterlightRiftPortal::AAfterlightRiftPortal()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->InitSphereRadius(95.f);
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Trigger->SetGenerateOverlapEvents(true);
	RootComponent = Trigger;

	RingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));
	RingMesh->SetupAttachment(RootComponent);
	RingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RingMesh->SetRelativeScale3D(FVector(2.2f, 2.2f, 0.15f));

	DiscMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));
	DiscMesh->SetupAttachment(RootComponent);
	DiscMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DiscMesh->SetRelativeScale3D(FVector(1.8f, 1.8f, 0.05f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		RingMesh->SetStaticMesh(Cylinder.Object);
		RingMesh->SetRelativeScale3D(FVector(2.0f, 2.0f, 0.08f));
	}
	if (Cylinder.Succeeded())
	{
		DiscMesh->SetStaticMesh(Cylinder.Object);
	}

	InitialLifeSpan = 45.f;
}

void AAfterlightRiftPortal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAfterlightRiftPortal, Tone);
	DOREPLIFETIME(AAfterlightRiftPortal, Sibling);
}

void AAfterlightRiftPortal::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AAfterlightRiftPortal::OnOverlap);
	ApplyLook();
}

void AAfterlightRiftPortal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AAfterlightRiftPortal* Other = Sibling.Get())
	{
		Other->Sibling = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AAfterlightRiftPortal::Configure(EAfterlightRiftTone InTone, AAfterlightRiftPortal* InSibling, AActor* InOwnerPawn)
{
	Tone = InTone;
	Sibling = InSibling;
	OwnerPawn = InOwnerPawn;
	ApplyLook();
}

void AAfterlightRiftPortal::SetSibling(AAfterlightRiftPortal* InSibling)
{
	Sibling = InSibling;
}

void AAfterlightRiftPortal::ApplyLook()
{
	const FLinearColor Color = (Tone == EAfterlightRiftTone::Solar)
		? FLinearColor(1.f, 0.55f, 0.08f)
		: FLinearColor(0.15f, 0.85f, 1.f);

	RingMat = RingMesh->CreateAndSetMaterialInstanceDynamic(0);
	DiscMat = DiscMesh->CreateAndSetMaterialInstanceDynamic(0);
	if (RingMat)
	{
		RingMat->SetVectorParameterValue(TEXT("Color"), Color);
		RingMat->SetVectorParameterValue(TEXT("BaseColor"), Color);
		RingMat->SetVectorParameterValue(TEXT("EmissiveColor"), Color * 4.f);
	}
	if (DiscMat)
	{
		FLinearColor Disc = Color;
		Disc.A = 0.35f;
		DiscMat->SetVectorParameterValue(TEXT("Color"), Disc * 0.4f);
		DiscMat->SetVectorParameterValue(TEXT("BaseColor"), Disc * 0.4f);
		DiscMat->SetVectorParameterValue(TEXT("EmissiveColor"), Color * 1.5f);
	}
}

FTransform AAfterlightRiftPortal::GetExitTransform() const
{
	const FVector Loc = GetActorLocation() + GetActorForwardVector() * 120.f;
	return FTransform(GetActorRotation(), Loc);
}

bool AAfterlightRiftPortal::CanTeleport(AActor* OtherActor) const
{
	if (!OtherActor || !Sibling)
	{
		return false;
	}
	if (RecentlyTeleported.Contains(OtherActor))
	{
		return false;
	}
	// Allow pawns and anything with projectile movement
	if (OtherActor->IsA<APawn>() || OtherActor->FindComponentByClass<UProjectileMovementComponent>())
	{
		return true;
	}
	return OtherActor->GetRootComponent() && OtherActor->GetRootComponent()->IsSimulatingPhysics();
}

void AAfterlightRiftPortal::TeleportActor(AActor* OtherActor)
{
	AAfterlightRiftPortal* Dest = Sibling;
	if (!Dest || !OtherActor)
	{
		return;
	}

	const FTransform Exit = Dest->GetExitTransform();
	FVector Velocity = FVector::ZeroVector;
	if (ACharacter* Char = Cast<ACharacter>(OtherActor))
	{
		Velocity = Char->GetVelocity();
	}
	else if (UProjectileMovementComponent* Proj = OtherActor->FindComponentByClass<UProjectileMovementComponent>())
	{
		Velocity = Proj->Velocity;
	}

	// Reorient velocity to destination forward while preserving speed (+ Afterlight boost)
	const float Speed = FMath::Max(Velocity.Size(), 400.f) * 1.15f;
	const FVector NewVel = Exit.GetRotation().GetForwardVector() * Speed;

	OtherActor->TeleportTo(Exit.GetLocation(), Exit.Rotator(), false, true);

	if (ACharacter* Char = Cast<ACharacter>(OtherActor))
	{
		if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
		{
			Move->Velocity = NewVel;
		}
		// Lumen Echo — brief afterimage / speed ghost
		Char->GetMesh()->SetOverlayMaterial(nullptr);
		Char->CustomTimeDilation = 1.15f;
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(Char, [Char]()
		{
			if (IsValid(Char))
			{
				Char->CustomTimeDilation = 1.f;
			}
		}), 0.85f, false);
	}
	else if (UProjectileMovementComponent* Proj = OtherActor->FindComponentByClass<UProjectileMovementComponent>())
	{
		Proj->Velocity = NewVel;
	}

	RecentlyTeleported.Add(OtherActor);
	Dest->RecentlyTeleported.Add(OtherActor);

	FTimerHandle ClearHandle;
	TWeakObjectPtr<AActor> WeakOther(OtherActor);
	GetWorldTimerManager().SetTimer(ClearHandle, FTimerDelegate::CreateWeakLambda(this, [this, Dest, WeakOther]()
	{
		if (AActor* A = WeakOther.Get())
		{
			RecentlyTeleported.Remove(A);
			if (Dest)
			{
				Dest->RecentlyTeleported.Remove(A);
			}
		}
	}), 0.35f, false);
}

void AAfterlightRiftPortal::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (CanTeleport(OtherActor))
	{
		TeleportActor(OtherActor);
	}
}

void AAfterlightRiftPortal::PulseLink(float Duration, float DamagePerSecond)
{
	PulseTimeLeft = Duration;
	PulseDamage = DamagePerSecond;
}

void AAfterlightRiftPortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Spin the ring for presence
	RingMesh->AddLocalRotation(FRotator(0.f, 90.f * DeltaSeconds, 0.f));

	if (PulseTimeLeft > 0.f && Sibling)
	{
		PulseTimeLeft -= DeltaSeconds;
		const FVector A = GetActorLocation();
		const FVector B = Sibling->GetActorLocation();
		DrawDebugLine(GetWorld(), A, B, Tone == EAfterlightRiftTone::Solar ? FColor(255, 160, 20) : FColor(40, 220, 255),
			false, -1.f, 0, 6.f);

		// Damage actors near the beam
		TArray<FHitResult> Hits;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(RiftPulse), true, this);
		Params.AddIgnoredActor(this);
		Params.AddIgnoredActor(Sibling);
		GetWorld()->SweepMultiByChannel(Hits, A, B, FQuat::Identity, ECC_Pawn,
			FCollisionShape::MakeSphere(40.f), Params);
		for (const FHitResult& Hit : Hits)
		{
			if (AActor* Victim = Hit.GetActor())
			{
				if (Victim != OwnerPawn.Get())
				{
					UGameplayStatics::ApplyDamage(Victim, PulseDamage * DeltaSeconds, nullptr, OwnerPawn.Get(), nullptr);
				}
			}
		}
	}
}
