// Afterlight branding overlay — replaces Lyra-facing title chrome at runtime.

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "AfterlightBrandSubsystem.generated.h"

class SWidget;

UCLASS()
class UAfterlightBrandSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void HandlePostLoadMap(UWorld* World);
	void ShowFrontendBrand();
	void HideFrontendBrand();

	TSharedPtr<SWidget> BrandWidget;
	FDelegateHandle PostLoadMapHandle;
};
