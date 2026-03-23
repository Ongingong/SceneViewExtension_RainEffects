#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WeatherEffectsTypes.h"
#include "WeatherEffectsSubsystem.generated.h"

class AWeatherController;
class UTexture2D;
class UTextureRenderTarget2D;
class FWeatherEffectsSceneViewExtension;

#define WEATHER_EFFECTS_TEXTURE_BASE_PATH TEXT("/WeatherEffects/Textures/")
#define WEATHER_EFFECTS_MATERIAL_BASE_PATH TEXT("/WeatherEffects/Materials/")
#define ENGINE_DEFAULT_TEXTURE_BASE_PATH TEXT("/Engine/EngineMaterials/")
#define EASY_RAIN_TEXTURE_BASE_PATH TEXT("/Game/EasyRain/System/SystemTextures/Leak/")
namespace WeatherEffectsDefaults
{
	// Wetness Shadow Mask
	inline constexpr const TCHAR* WorldHeightTexturePath = WEATHER_EFFECTS_TEXTURE_BASE_PATH TEXT("T_WeatherEffects_WorldHeight.T_WeatherEffects_WorldHeight");
	
	// Easy Rain Implementation
	inline constexpr const TCHAR* EasyRainTexturePath = EASY_RAIN_TEXTURE_BASE_PATH TEXT("...");  // Easy Rain Contents are excluded.
	
	// Rain Accumulation Effects
	inline constexpr const TCHAR* RainAccumulationTexturePath = WEATHER_EFFECTS_TEXTURE_BASE_PATH TEXT("RT_RainAccumulation.RT_RainAccumulation");
	inline constexpr const TCHAR* RainAccumulationMaskTexturePath = WEATHER_EFFECTS_TEXTURE_BASE_PATH TEXT("T_WeatherEffects_RainAccum_Mask.T_WeatherEffects_RainAccum_Mask");
	
	/* Deprecated - Screen Rain Drop Effects 
	constexpr float RainDropUVScale = 1.0f; 
	constexpr float RainDropDistortionIntensity = 0.05f;
	constexpr float RainDropAnimSpeed = 0.2f;
	inline constexpr const TCHAR* RainDropNormalMaskTexturePath = WEATHER_EFFECTS_TEXTURE_BASE_PATH TEXT("T_WeatherEffects_RainDropNormalMask.T_WeatherEffects_RainDropNormalMask");
	inline constexpr const TCHAR* RainDropNormalMaskTexturePathFallback = ENGINE_DEFAULT_TEXTURE_BASE_PATH TEXT("DefaultNormal_Uncompressed.DefaultNormal_Uncompressed"); 
	*/
}

UCLASS()
class WEATHEREFFECTS_API UWeatherEffectsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
	void SetWeatherState(EWeatherState InWeatherState);
	EWeatherState GetCurrentWeatherState() const { return CurrentWeatherState; }
	float GetTransitionElapsedRatio() const;

	AWeatherController* GetWeatherController() const { return WeatherControllerActor; }
	FVector3f GetCaptureComponentPosition() const;
	float GetCaptureOrthoWide() const;

	UTexture2D* GetRainDepthTexture() const;
	UTexture2D* GetRainAccumulationMaskTexture() const;
	UTextureRenderTarget2D* GetRainAccumulationRT() const;
	FEasyRainTextures GetEasyRainTextures() const;
	
	/* Deprecated - Screen Rain Drop Effects 
	float GetRainDropUVScale() const;
	float GetRainDropDistortionIntensity() const;
	float GetRainDropAnimSpeed() const;
	*/
private:
	void SetWeatherControllerActor(AWeatherController* InWeatherControllerActor);
	AWeatherController* SpawnWeatherController(UWorld* World);

private:
	UPROPERTY()
	TObjectPtr<class AWeatherController> WeatherControllerActor;
	TSharedPtr< class FWeatherEffectsSceneViewExtension, ESPMode::ThreadSafe > WeatherSceneViewExtension;	
	
	EWeatherState CurrentWeatherState = EWeatherState::Clear;
	
	friend class FWeatherEffectsSceneViewExtension;
	friend class AWeatherController;
};
