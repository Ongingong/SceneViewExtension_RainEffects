// Fill out your copyright notice in the Description page of Project Settings.

#include "WeatherController.h"
#include "NiagaraComponent.h"
#include "Engine/TextureRenderTarget2D.h"

class UNiagaraComponent;
// Sets default values
AWeatherController::AWeatherController()
{
	PrimaryActorTick.bCanEverTick = true;
	
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinder<UTexture2D> DefaultWorldHeightTexture;
		
		// Easy Rain Implementation
      // Easy Rain Contents are excluded.
		
		ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> RainAccumulationTexture;
		ConstructorHelpers::FObjectFinder<UTexture2D> RainAccumulationMaskTexture;
		
		/* Deprecated - Screen Rain Drop Effects
		ConstructorHelpers::FObjectFinder<UTexture2D> DefaultRainDropMaskTexture;
		ConstructorHelpers::FObjectFinder<UTexture2D> FallbackRainDropMaskTexture;
		*/
		
		FConstructorStatics()
			: DefaultWorldHeightTexture(WeatherEffectsDefaults::WorldHeightTexturePath)
			
			// Easy Rain Implementation
        // Easy Rain Contents are excluded.

			, RainAccumulationTexture(WeatherEffectsDefaults::RainAccumulationTexturePath)
			, RainAccumulationMaskTexture(WeatherEffectsDefaults::RainAccumulationMaskTexturePath)
			
			/* Deprecated - Screen Rain Drop Effects  
			, DefaultRainDropMaskTexture(WeatherEffectsDefaults::RainDropNormalMaskTexturePath)
			, FallbackRainDropMaskTexture(WeatherEffectsDefaults::RainDropNormalMaskTexturePathFallback)
			*/
		{}
	};

	static FConstructorStatics ConstructorStatics;
	
	auto AssignTexture = [](auto& ConstructorStaticsField, auto& TargetPtr) {
		if (ConstructorStaticsField.Succeeded())
		{
			TargetPtr = ConstructorStaticsField.Object;
		}
		else
		{
			TargetPtr = nullptr;
		}
	};
	AssignTexture(ConstructorStatics.DefaultWorldHeightTexture, CapturedWorldHeightTexture);
	
	AssignTexture(ConstructorStatics.RainDropPackedMaskTexture, RainDropPackedMaskTexture);
	AssignTexture(ConstructorStatics.DropletsMaskTexture, DropletsMaskTexture);
	AssignTexture(ConstructorStatics.DropletsLargeMaskTexture, DropletsLargeMaskTexture);
	AssignTexture(ConstructorStatics.DropletsTemporalMaskTexture, DropletsTemporalMaskTexture);
	AssignTexture(ConstructorStatics.DropletsNormalTexture, DropletsNormalTexture);
	AssignTexture(ConstructorStatics.DripsNormalTexture, DripsNormalTexture);
	
	AssignTexture(ConstructorStatics.RainAccumulationTexture, RainAccumulationTexture);
	AssignTexture(ConstructorStatics.RainAccumulationMaskTexture, RainAccumulationMaskTexture);
	
	/* Deprecated - Screen Rain Drop Effects  
	AssignTexture(ConstructorStatics.DripsNormalTexture, DripsNormalTexture);
	AssignTexture(ConstructorStatics.DefaultRainDropMaskTexture, RainDropNormalMask);
	*/
}

AWeatherController::~AWeatherController()
{
}

void AWeatherController::Destroyed()
{
	if (Subsystem)
	{
		Subsystem->SetWeatherControllerActor(nullptr);
	}
	Super::Destroyed();
}

void AWeatherController::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	UWorld* World = GetWorld();

	if (World)
	{
		Subsystem = World->GetSubsystem<UWeatherEffectsSubsystem>();

		if (Subsystem)
		{
			Subsystem->SetWeatherControllerActor(this);
		}
	}
	
	RainAccumulationNiagaraComponent = GetComponentByClass<UNiagaraComponent>();
	RainAccumulationNiagaraComponent->SetVariableFloat(FName("User.TransitionDuration"), TransitionDuration);
}

#if WITH_EDITOR
void AWeatherController::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	FName PropertyName = (PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None);	
	
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AWeatherController, CapturedWorldHeightTexture))
	{
		if (Subsystem)
		{
			Subsystem->SetWeatherControllerActor(this);
		}
	}
}
#endif

UTexture2D* AWeatherController::GetRainDepthTexture() const
{
	UTexture2D* LoadedTexture = CapturedWorldHeightTexture.Get();

	if(!LoadedTexture)
	{
		LoadedTexture = CapturedWorldHeightTexture.LoadSynchronous();
	}

	return LoadedTexture;
}

void AWeatherController::SetWeatherEffectsSubsystem(UWeatherEffectsSubsystem* InSubsystem)
{
	if (InSubsystem)
	{
		Subsystem = InSubsystem;
		UE_LOG(LogWeatherEffects, Log, TEXT("WeatherController :: Subsystem Registered"));
	}
}

void AWeatherController::StartTransition()
{
	UE_LOG(LogWeatherEffects, Log, TEXT("WeatherController :: Start Transition"));
	bIsTransitioning = true;
	
	GetWorldTimerManager().SetTimer(WeatherTransitionTimerHandle, this, &AWeatherController::EndTransition, TransitionDuration, false);
	Subsystem->SetWeatherState(EWeatherState::Transition);
	
	if(RainAccumulationNiagaraComponent)
	{	
		RainAccumulationNiagaraComponent->Activate();
		RainAccumulationNiagaraComponent->ReinitializeSystem();	
	}
}

void AWeatherController::EndTransition()
{
	UE_LOG(LogWeatherEffects, Log, TEXT("WeatherController :: End Transition"));
	bIsTransitioning = false;
	
	// Update transition state in subsystem.
	GetWorldTimerManager().ClearTimer(WeatherTransitionTimerHandle);
	Subsystem->SetWeatherState(EWeatherState::Rain);
	
	if(RainAccumulationNiagaraComponent)
	{	
		RainAccumulationNiagaraComponent->Deactivate();
	}
}

void AWeatherController::ClearWeather()
{	
	if (GetWorldTimerManager().IsTimerActive(DrawRainSplatterTimerHandle))
	{
		GetWorldTimerManager().ClearTimer(WeatherTransitionTimerHandle);
	}
	bIsTransitioning = false;

	Subsystem->SetWeatherState(EWeatherState::Clear);
	
	if(RainAccumulationNiagaraComponent)
	{	
		RainAccumulationNiagaraComponent->Deactivate();
	}
}

void AWeatherController::RainyWeather()
{	
	if (GetWorldTimerManager().IsTimerActive(DrawRainSplatterTimerHandle))
	{
		GetWorldTimerManager().ClearTimer(WeatherTransitionTimerHandle);
	}
	bIsTransitioning = false;

	Subsystem->SetWeatherState(EWeatherState::Rain);
	
	if(RainAccumulationNiagaraComponent)
	{	
		RainAccumulationNiagaraComponent->Deactivate();
	}
}

UTexture2D* AWeatherController::GetRainAccumulationMaskTexture() const
{
	UTexture2D* LoadedTexture = RainAccumulationMaskTexture.Get();

	if(!LoadedTexture)
	{
		LoadedTexture = RainAccumulationMaskTexture.LoadSynchronous();
	}

	return LoadedTexture;
}

float AWeatherController::GetTransitionElapsedRatio() const
{
	if (bIsTransitioning && GetWorldTimerManager().IsTimerActive(WeatherTransitionTimerHandle))
	{
		return GetWorldTimerManager().GetTimerElapsed(WeatherTransitionTimerHandle) / GetWorldTimerManager().GetTimerRate(WeatherTransitionTimerHandle);
	}
	else
	{
		return 0.0f;
	}
}

UTextureRenderTarget2D* AWeatherController::GetRainAccumulationRT() const
{	
	UTextureRenderTarget2D* LoadedTexture = RainAccumulationTexture.Get();
	
	if(!LoadedTexture)
	{
		LoadedTexture = RainAccumulationTexture.LoadSynchronous();
	}
	
	return LoadedTexture;
}

/* Deprecated - Screen Rain Drop Effects
UTexture2D* AWeatherController::GetRainDropNormalMaskTexture() const
{
	UTexture2D* LoadedTexture = RainDropNormalMask.Get();

	if (!LoadedTexture)
	{
		LoadedTexture = RainDropNormalMask.LoadSynchronous();
	}

	return LoadedTexture;
}
*/
