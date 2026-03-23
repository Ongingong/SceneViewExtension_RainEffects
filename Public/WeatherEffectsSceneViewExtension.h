// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "RHI.h"
#include "RHIResources.h"
#include "SceneRendering.h"
#include "SceneViewExtension.h"
#include "WeatherEffectsModule.h"
#include "WeatherEffectsTypes.h"
#include "WeatherEffectsMaterial.h"

class UWeatherEffectsSubsystem;
class Texture2D;
class FWeatherEffectsWetnessParameters;

class WEATHEREFFECTS_API FWeatherEffectsSceneViewExtension : public FWorldSceneViewExtension
{
public:
	FWeatherEffectsSceneViewExtension(const FAutoRegister& AutoRegister, UWorld* InWorld, UWeatherEffectsSubsystem* InWorldSubsystem);
	~FWeatherEffectsSceneViewExtension();
	
	//~ Begin FSceneViewExtensionBase Interface
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override;
	virtual void PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) override;
	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& InView, FPostProcessingPassDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;
	//~ End FSceneViewExtensionBase Interface
public:

protected:
	/* Draws the rain drop effect. Call through delegate of SubscribeToPostProcessingPass*/
	FScreenPassTexture DrawRainDropPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs);
	
	bool IsTextureValid(UTexture* Texture) const
	{
		if (!Texture)
		{	
			UE_LOG(LogWeatherEffects, Error, TEXT("FWeatherEffectsSceneViewExtension :: Texture is null."));
			return false;
		}
		else if (!Texture->GetResource())
		{
			UE_LOG(LogWeatherEffects, Error, TEXT("FWeatherEffectsSceneViewExtension :: Texture resource is null. Texture Name: %s"), *Texture->GetName());
			return false;
		}
		else if (!Texture->GetResource()->GetTexture2DRHI())
		{
			UE_LOG(LogWeatherEffects, Error, TEXT("FWeatherEffectsSceneViewExtension :: Texture RHI is null. Texture Name: %s"), *Texture->GetName());
			return false;
		}
		else
		{
			return true;
		}
	}

private:
	UWeatherEffectsSubsystem* WeatherEffectsSubsystem;
	
	bool bCachedIsControllerRegistered = false;
	
	UTexture2D* CachedRainDepthTexture;
	FRDGTextureRef CachedRainDepthTextureRDGRef;

	float CachedWetnessFactor = 0.0f;
	float CachedCaptureOrthoWide;
	FVector3f CachedCaptureComponentPosition;
	
	// Global Uniform Buffer
	TUniformBufferRef<FWeatherEffectsEasyRainUniformParameters> EasyRainUniformParametersRef;
	
	// Transition PS parameters cached
	EWeatherState CachedWeatherState;
	bool CachedIsTransitioning = false;
	UTextureRenderTarget2D* CachedRainAccumulationRTTexture;
	UTexture2D* CachedRainAccumulationMaskTexture;
	FRainAccumulationTexturesRDGRef CachedRainAccumulationTexturesRDGRef;

	// Static PS parameters cached
	FEasyRainTextures CachedEasyRainTextures;
	//FEasyRainTexturesRDGRef CachedEasyRainTexturesRDGRef;
	float CachedRainAccumulationBase = 0.0f;
	
	// Screen Rain Drop Effect parameters cached
	float CachedRainDropUVScale = 0.0f;
	float CachedRainDropDistortionIntensity = 0.0f;
	float CachedRainDropAnimSpeed = 0.0f;
	
	//Delete
	UTexture2D* CachedRainRippleTexture;
};
