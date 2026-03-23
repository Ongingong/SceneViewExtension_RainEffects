#include "WeatherEffectsSubsystem.h"
#include "WeatherEffectsModule.h"
#include "WeatherController.h"
#include "EngineUtils.h"
#include "Engine/Texture2D.h"
#include "WeatherEffectsSceneViewExtension.h"

static const FName WeatherControllerName = TEXT("WeatherController");

void UWeatherEffectsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	WeatherSceneViewExtension = FSceneViewExtensions::NewExtension<FWeatherEffectsSceneViewExtension>(GetWorld(), this);
}

void UWeatherEffectsSubsystem::Deinitialize()
{
	if (WeatherSceneViewExtension.IsValid())
	{
		WeatherSceneViewExtension->IsActiveThisFrameFunctions.Empty();

		FSceneViewExtensionIsActiveFunctor IsActiveFunctor;
		IsActiveFunctor.IsActiveFunction = [](const ISceneViewExtension* SceneViewExtension, 
												const FSceneViewExtensionContext& Context)
		{
			return TOptional<bool>(false);
		};

		WeatherSceneViewExtension->IsActiveThisFrameFunctions.Add(IsActiveFunctor);
		
		ENQUEUE_RENDER_COMMAND(ReleaseWeatherSVE)([SVE = MoveTemp(WeatherSceneViewExtension)](FRHICommandListImmediate& RHICmdList) mutable
		{
			SVE.Reset();
		});
	}

	FlushRenderingCommands();
	Super::Deinitialize();
}

void UWeatherEffectsSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	for (TActorIterator<AWeatherController> It(&InWorld); It; ++It)
	{
		SetWeatherControllerActor(*It);
		WeatherControllerActor->SetWeatherEffectsSubsystem(this);
		break;
	}
}

void UWeatherEffectsSubsystem::SetWeatherState(EWeatherState InWeatherState)
{
	if (InWeatherState != CurrentWeatherState)
	{
		CurrentWeatherState = InWeatherState;
	}
}

float UWeatherEffectsSubsystem::GetTransitionElapsedRatio() const
{
	return WeatherControllerActor ? WeatherControllerActor->GetTransitionElapsedRatio() : 0.0f;
}

FVector3f UWeatherEffectsSubsystem::GetCaptureComponentPosition() const
{
	return WeatherControllerActor ? FVector3f(WeatherControllerActor->GetCapturePosition()) : FVector3f::ZeroVector;
}

float UWeatherEffectsSubsystem::GetCaptureOrthoWide() const
{
	return WeatherControllerActor ? WeatherControllerActor->GetCaptureOrthoWide() : 0.0f;
}

UTexture2D* UWeatherEffectsSubsystem::GetRainDepthTexture() const
{
	return WeatherControllerActor ? WeatherControllerActor->GetRainDepthTexture() : nullptr;
}

UTexture2D* UWeatherEffectsSubsystem::GetRainAccumulationMaskTexture() const
{
	return WeatherControllerActor ? WeatherControllerActor->GetRainAccumulationMaskTexture() : nullptr;
}

UTextureRenderTarget2D* UWeatherEffectsSubsystem::GetRainAccumulationRT() const
{
	return WeatherControllerActor ? WeatherControllerActor->GetRainAccumulationRT() : nullptr;
}

FEasyRainTextures UWeatherEffectsSubsystem::GetEasyRainTextures() const
{
	FEasyRainTextures OutTextures;
	
	if (WeatherControllerActor)
	{
    // Easy Rain Contents are excluded
	}
	else
	{
		UE_LOG(LogWeatherEffects, Warning, TEXT("WeatherEffectsSubsystem :: GetEasyRainTextures - WeatherControllerActor is null. Returning empty textures."));
	}
	return OutTextures;
}

void UWeatherEffectsSubsystem::SetWeatherControllerActor(AWeatherController* InWeatherControllerActor)
{
	if (InWeatherControllerActor)
	{
		WeatherControllerActor = InWeatherControllerActor;
		UE_LOG(LogWeatherEffects, Log, TEXT("WeatherEffectsSubsystem :: WeatherController Registered"));
	}
	else
	{
		WeatherControllerActor = nullptr;
		UE_LOG(LogWeatherEffects, Log, TEXT("WeatherEffectsSubsystem :: WeatherController Unregistered"));
	}
}

AWeatherController* UWeatherEffectsSubsystem::SpawnWeatherController(UWorld* World)
{
	if (World)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Name = WeatherControllerName;
		AWeatherController* Actor = World->SpawnActor<AWeatherController>(AWeatherController::StaticClass(), SpawnParams);
		
		return Actor;
	}
	return nullptr;
}

/* Deprecated - Screen Rain Drop Effects  
float UWeatherEffectsSubsystem::GetRainDropUVScale() const
{
	return WeatherControllerActor ? WeatherControllerActor->GetRainDropUVScale() : WeatherEffectsDefaults::RainDropUVScale;
}

float UWeatherEffectsSubsystem::GetRainDropDistortionIntensity() const
{
	return WeatherControllerActor ? WeatherControllerActor->GetRainDropDistortionIntensity() : WeatherEffectsDefaults::RainDropDistortionIntensity;	
}

float UWeatherEffectsSubsystem::GetRainDropAnimSpeed() const
{
	return WeatherControllerActor ? WeatherControllerActor->GetRainDropAnimSpeed() : WeatherEffectsDefaults::RainDropAnimSpeed;
}
*/
