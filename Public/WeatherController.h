// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "WeatherEffectsModule.h"
#include "WeatherEffectsSubsystem.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeatherController.generated.h"

class UTexture2D;
class UKismetRenderingLibrary;
class UNiagaraComponent;

UCLASS(Blueprintable, BlueprintType)
class WEATHEREFFECTS_API AWeatherController : public AActor
{
	GENERATED_BODY()
	
public:			
	AWeatherController();
	~AWeatherController();

	virtual void Destroyed() override;
	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	float GetTransitionElapsedRatio() const;
	UTexture2D* GetRainDepthTexture() const;
	float GetCaptureOrthoWide() const { return CaptureOrthoWide; }
	FVector3f GetCapturePosition() const { return CapturePosition; }

	UTextureRenderTarget2D* GetRainAccumulationRT() const;
	UTexture2D* GetRainAccumulationMaskTexture() const;
	
	/* Deprecated - Screen Rain Drop Effects
	UTexture2D* GetRainDropNormalMaskTexture() const;
	float GetRainDropUVScale() const { return RainDropUVScale; }
	float GetRainDropDistortionIntensity() const { return RainDropDistortionIntensity; }
	float GetRainDropAnimSpeed() const { return RainDropAnimSpeed; }
	*/

protected:
	// ~ Begin AActor Interface
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	
	// ~ Transition (Temp)

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="01 - Capture", meta = (DisplayName = "Capture Bounds", Delta = "0.01"))
	float CaptureOrthoWide = 5000.0f;
		
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "01 - Capture", meta = (DisplayName = "Capture Position"))
	FVector3f CapturePosition = FVector3f(0.0f, 0.0f, 0.0f);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "01 - Capture", meta = (DisplayName = "Captured World Height Texture"))
	TSoftObjectPtr<UTexture2D> CapturedWorldHeightTexture;
	
	UFUNCTION(CallInEditor, Category = "02 - Controls", meta = (DisplayName = "Clear Weather"))
	void ClearWeather(); 
		
	UFUNCTION(CallInEditor, Category = "02 - Controls", meta = (DisplayName = "Rainy Weather"))
	void RainyWeather();
	
	UFUNCTION(CallInEditor, Category = "02 - Controls", meta = (DisplayName = "Start Transition"))
	void StartTransition();
	
	UFUNCTION(CallInEditor, Category = "02 - Controls", meta = (DisplayName = "End Transition"))
	void EndTransition();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="02 - Controls", meta = (DisplayName = "Transition Duration", Delta = "0.01"))
	float TransitionDuration = 10.0f;
		
	// Easy Rain Implementation
    // Easy Rain Contents are excluded.
	// ~ End Easy Rain Implementation
	
	UPROPERTY()
	TSoftObjectPtr<UTextureRenderTarget2D> RainAccumulationTexture;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "03 - Textures", meta = (DisplayName = "Rain Accumulation Mask"))
	TSoftObjectPtr<UTexture2D> RainAccumulationMaskTexture;
	
	/* Deprecated - Screen Rain Drop Effects 
	// ~ Rain Drop Effects
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "04 - Rain Drops Effects", meta = (DisplayName = "Rain Drop UV Scale", Delta = "0.01"))
	float RainDropUVScale = WeatherEffectsDefaults::RainDropUVScale;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "04 - Rain Drops Effects", meta = (DisplayName = "Rain Drop Distortion Intensity", Delta = "0.01"))
	float RainDropDistortionIntensity = WeatherEffectsDefaults::RainDropDistortionIntensity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "04 - Rain Drops Effects", meta = (DisplayName = "Rain Drop Anim Speed", Delta = "0.01"))
	float RainDropAnimSpeed = WeatherEffectsDefaults::RainDropAnimSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "04 - Rain Drops Effects", meta = (DisplayName = "Rain Drop Normal Mask"))
	TSoftObjectPtr<UTexture2D> RainDropNormalMask;
	// ~ End Rain Drop Effects
	*/
private:
	void SetWeatherEffectsSubsystem(UWeatherEffectsSubsystem* InSubsystem);

private:
	UPROPERTY(Transient);
	TObjectPtr<UWeatherEffectsSubsystem> Subsystem;
	
	bool bIsTransitioning = false;
	FTimerHandle WeatherTransitionTimerHandle;
	FTimerHandle DrawRainSplatterTimerHandle;
	
	UPROPERTY(Transient);
	TObjectPtr<UNiagaraComponent> RainAccumulationNiagaraComponent; 
	
private:
	friend class UWeatherEffectsSubsystem;
	friend class FWeatherEffectsSceneViewExtension;
};
