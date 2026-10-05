#include "Afterlight/AfterlightBrandSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CoreDelegates.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"

void UAfterlightBrandSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UAfterlightBrandSubsystem::HandlePostLoadMap);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWorld* World = GI->GetWorld())
		{
			HandlePostLoadMap(World);
		}
	}
}

void UAfterlightBrandSubsystem::Deinitialize()
{
	HideFrontendBrand();
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	Super::Deinitialize();
}

void UAfterlightBrandSubsystem::HandlePostLoadMap(UWorld* World)
{
	if (!World)
	{
		return;
	}

	const FString MapName = World->GetMapName();
	const bool bFrontend =
		MapName.Contains(TEXT("FrontEnd"), ESearchCase::IgnoreCase) ||
		MapName.Contains(TEXT("LyraFrontEnd"), ESearchCase::IgnoreCase);

	if (bFrontend)
	{
		ShowFrontendBrand();
	}
	else
	{
		HideFrontendBrand();
	}
}

void UAfterlightBrandSubsystem::ShowFrontendBrand()
{
	HideFrontendBrand();

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	UGameViewportClient* Viewport = GI->GetGameViewportClient();
	if (!Viewport)
	{
		return;
	}

	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 64);
	const FSlateFontInfo SubFont = FCoreStyle::GetDefaultFontStyle("Regular", 22);

	BrandWidget = SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.f, 48.f, 0.f, 0.f))
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.02f, 0.03f, 0.06f, 0.82f))
			.Padding(FMargin(48.f, 28.f))
			[
				SNew(SBox)
				.MinDesiredWidth(640.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("AFTERLIGHT")))
						.Font(TitleFont)
						.ColorAndOpacity(FLinearColor(1.f, 0.72f, 0.18f, 1.f))
						.Justification(ETextJustify::Center)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(FMargin(0.f, 10.f, 0.f, 0.f))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Eclipse Rifts  ·  Online Arena")))
						.Font(SubFont)
						.ColorAndOpacity(FLinearColor(0.55f, 0.85f, 1.f, 0.95f))
						.Justification(ETextJustify::Center)
					]
				]
			]
		];

	Viewport->AddViewportWidgetContent(BrandWidget.ToSharedRef(), /*ZOrder*/ 10000);

	if (TSharedPtr<SWindow> Window = FSlateApplication::Get().GetActiveTopLevelWindow())
	{
		Window->SetTitle(FText::FromString(TEXT("AFTERLIGHT")));
	}
}

void UAfterlightBrandSubsystem::HideFrontendBrand()
{
	if (!BrandWidget.IsValid())
	{
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGameViewportClient* Viewport = GI->GetGameViewportClient())
		{
			Viewport->RemoveViewportWidgetContent(BrandWidget.ToSharedRef());
		}
	}

	BrandWidget.Reset();
}
