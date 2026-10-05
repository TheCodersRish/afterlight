// AFTERLIGHT — Eclipse Rift portal actor
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AfterlightRiftPortal.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class AAfterlightRiftPortal;

UENUM(BlueprintType)
enum class EAfterlightRiftTone : uint8
{
	Solar UMETA(DisplayName = "Solar (Amber)"),
	Void UMETA(DisplayName = "Void (Cyan)")
};

/**
 * One half of a linked Eclipse Rift pair.
 * Players and projectiles that enter teleport to the sibling with momentum preserved.
 */
UCLASS()
class LYRAGAME_API AAfterlightRiftPortal : public AActor
{
	GENERATED_BODY()

public:
	AAfterlightRiftPortal();

	void Configure(EAfterlightRiftTone InTone, AAfterlightRiftPortal* InSibling, AActor* InOwnerPawn);
	void SetSibling(AAfterlightRiftPortal* InSibling);

	EAfterlightRiftTone GetTone() const { return Tone; }
	AAfterlightRiftPortal* GetSibling() const { return Sibling; }

	/** Exit transform slightly in front of the rift facing outward. */
	FTransform GetExitTransform() const;

	/** Unique Afterlight: fire a damaging lumen beam toward sibling. */
	void PulseLink(float Duration, float DamagePerSecond);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void ApplyLook();
	void TeleportActor(AActor* OtherActor);
	bool CanTeleport(AActor* OtherActor) const;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Trigger;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> RingMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> DiscMesh;

	UPROPERTY(Replicated)
	EAfterlightRiftTone Tone = EAfterlightRiftTone::Solar;

	UPROPERTY(Replicated)
	TObjectPtr<AAfterlightRiftPortal> Sibling;

	UPROPERTY()
	TWeakObjectPtr<AActor> OwnerPawn;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> RingMat;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DiscMat;

	TSet<TWeakObjectPtr<AActor>> RecentlyTeleported;
	float PulseTimeLeft = 0.f;
	float PulseDamage = 0.f;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
