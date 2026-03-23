// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ScreenPass.h"
#include "WeatherEffectsTypes.h"

class FRDGBuilder;
class FSceneView;
class FScreenPassTextureViewport;
class FSceneTextureUniformParameters;

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FWeatherEffectsEasyRainUniformParameters, )
  // Easy Rain Contents are exclude.
END_GLOBAL_SHADER_PARAMETER_STRUCT()
	
void DrawWetnessShadowDebugPass_RenderThread(FRDGBuilder& GraphBuilder,
							const FSceneView& View,
							TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures,
							const FRDGTextureRef GBufferATexture,
							const FRDGTextureRef GBufferCTexture,
							const FRDGTextureRef RainDepthTexture,
							float CaptureOrthoWide,
							FVector3f CaptureComponentPosition
							);

void DrawWetnessStaticPass_RenderThread(
							FRDGBuilder& GraphBuilder,
							const FSceneView& View,
							TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures,
							const FRDGTextureRef GBufferATexture,
							const FRDGTextureRef GBufferBTexture,
							const FRDGTextureRef GBufferCTexture, 
							const FRDGTextureRef GBufferDTexture,
							const FRDGTextureRef RainDepthTexture,
							float CaptureOrthoWide,
							FVector3f CaptureComponentPosition,
							float CachedWetnessFactor,
							const TUniformBufferRef<FWeatherEffectsEasyRainUniformParameters> &EasyRainUniformParameters
							);

void DrawWetnessTransitionPass_RenderThread(
							FRDGBuilder& GraphBuilder,
							const FSceneView& View,
							TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures,
							const FRDGTextureRef GBufferATexture,
							const FRDGTextureRef GBufferBTexture,
							const FRDGTextureRef GBufferCTexture, 
							const FRDGTextureRef GBufferDTexture,
							const FRDGTextureRef RainDepthTexture,
							float CaptureOrthoWide,
							FVector3f CaptureComponentPosition,
							float CachedWetnessFactor,
							const FRainAccumulationTexturesRDGRef& RainAccumulationTextures,
							const TUniformBufferRef<FWeatherEffectsEasyRainUniformParameters> &EasyRainUniformParameters
							);

template<typename TPassParameters>
void SetCommonShaderParameters(TPassParameters PassParameters,
	TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures,
	const FSceneView& ViewInfo,
	const FScreenPassTextureViewport& BackbufferTextureViewport,
	const FRDGTextureRef GBufferATexture,
	const FRDGTextureRef GBufferBTexture,
	const FRDGTextureRef GBufferCTexture,
	const FRDGTextureRef GBufferDTexture,
	const FRDGTextureRef RainDepthTexture,
	FRDGTextureRef BackbufferATexture,
	FRDGTextureRef BackbufferBTexture,
	FRDGTextureRef BackbufferCTexture,
	float CaptureOrthoWide,
	FVector3f CaptureComponentPosition,
	float InCachedWetnessFactor
	)
{
	PassParameters->RenderTargets[0] = FRenderTargetBinding(BackbufferATexture, ERenderTargetLoadAction::ENoAction);
	PassParameters->RenderTargets[1] = FRenderTargetBinding(BackbufferBTexture, ERenderTargetLoadAction::ENoAction);
	PassParameters->RenderTargets[2] = FRenderTargetBinding(BackbufferCTexture, ERenderTargetLoadAction::ENoAction);
	
	PassParameters->CommonParameters.SceneTexturesStruct = SceneTextures;
	PassParameters->CommonParameters.View = ViewInfo.ViewUniformBuffer;
	PassParameters->CommonParameters.Output = GetScreenPassTextureViewportParameters(BackbufferTextureViewport);
	PassParameters->CommonParameters.RainDepthTexture = RainDepthTexture;
	PassParameters->CommonParameters.WorldNormalTexture = GBufferATexture;
	PassParameters->CommonParameters.MSRITexture = GBufferBTexture;
	PassParameters->CommonParameters.BaseColorTexture = GBufferCTexture;
	PassParameters->CommonParameters.WeatherEffectsTexture = GBufferDTexture;
	
	PassParameters->CommonParameters.CaptureOrthoWide = CaptureOrthoWide;
	PassParameters->CommonParameters.CaptureComponentPosition = CaptureComponentPosition;
	PassParameters->CommonParameters.WetnessFactor = InCachedWetnessFactor;
	
	PassParameters->CommonParameters.RainDepthTextureSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp>::GetRHI();
}
