// AFTERLIGHT — place / manage Eclipse Rifts (Splitgate-like, Afterlight-unique)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Afterlight/AfterlightRiftPortal.h"
#include "AfterlightRiftComponent.generated.h"

/**
 * Attach to hero pawns.
 * Q = place/replace Solar rift, E = place/replace Void rift
 * R = Rift Pulse (damage beam between linked rifts) — Afterlight exclusive
 */
UCLASS(ClassGroup = (Afterlight), meta = (BlueprintSpawnableComponent))
class LYRAGAME_API UAfterlightRiftComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAfterlightRiftComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Afterlight|Rifts")
	void PlaceSolarRift();

	UFUNCTION(BlueprintCallable, Category = "Afterlight|Rifts")
	void PlaceVoidRift();

	UFUNCTION(BlueprintCallable, Category = "Afterlight|Rifts")
	void ActivateRiftPulse();

protected:
	void BindInput();
	void PlaceRift(EAfterlightRiftTone Tone);
	bool TracePlacement(FVector& OutLocation, FRotator& OutRotation) const;

	UPROPERTY()
	TObjectPtr<AAfterlightRiftPortal> SolarRift;

	UPROPERTY()
	TObjectPtr<AAfterlightRiftPortal> VoidRift;

	UPROPERTY(EditAnywhere, Category = "Afterlight|Rifts")
	float PlaceRange = 2500.f;

	UPROPERTY(EditAnywhere, Category = "Afterlight|Rifts")
	float PulseCooldown = 8.f;

	UPROPERTY(EditAnywhere, Category = "Afterlight|Rifts")
	float PulseDuration = 2.2f;

	UPROPERTY(EditAnywhere, Category = "Afterlight|Rifts")
	float PulseDPS = 55.f;

	float PulseCooldownLeft = 0.f;
	bool bInputBound = false;
};
