// Fill out your copyright notice in the Description page of Project Settings.

#include "WeatherEffectsSceneViewExtension.h"
#include "WeatherEffectsModule.h"
#include "WeatherEffectsSubsystem.h"
#include "SceneTexturesConfig.h"
#include "ScreenPass.h"
#include "Engine/Texture2D.h"
#include "PixelShaderUtils.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "TextureResource.h"
#include "WeatherEffectsMaterial.h"
#include "SceneTextureParameters.h"
#include "Engine/TextureRenderTarget2D.h"

namespace
{
	TAutoConsoleVariable<int32> CVarWeatherEffectsEnableRainWetness(
		TEXT("r.WeatherEffects.EnableRainWetness"),
		1,
		TEXT("Enable rain wetness. \n 0: Off\n 1: On"),
		ECVF_RenderThreadSafe);
	
	TAutoConsoleVariable<int32> CVarWeatherEffectsDebugWetnessShadow(
	TEXT("r.WeatherEffects.Debug.WetnessShadow"),
	0,
	TEXT("Show debug view of wetness shadow. \n 0: Off\n 1: On"),
	ECVF_RenderThreadSafe);
	
	TAutoConsoleVariable<int32> CVarWeatherEffectsEnableRainDropPass(
	TEXT("r.WeatherEffects.EnableRainDrops"),
	0,
	TEXT("Enable rain drop post process.\n 0: Off\n 1: On"),
	ECVF_RenderThreadSafe);

}

FWeatherEffectsSceneViewExtension::FWeatherEffectsSceneViewExtension(const FAutoRegister& AutoRegister, UWorld* InWorld, UWeatherEffectsSubsystem* InWorldSubsystem) :
	FWorldSceneViewExtension(AutoRegister, InWorld), WeatherEffectsSubsystem(InWorldSubsystem)
{
	CachedRainDepthTexture = InWorldSubsystem->GetRainDepthTexture();
	CachedCaptureOrthoWide = InWorldSubsystem->GetCaptureOrthoWide();
	CachedCaptureComponentPosition = InWorldSubsystem->GetCaptureComponentPosition();
}

FWeatherEffectsSceneViewExtension::~FWeatherEffectsSceneViewExtension()
{
	ENQUEUE_RENDER_COMMAND(ReleaseWeatherEasyRainBuffer)(
		[BufferToRelease = EasyRainUniformParametersRef](FRHICommandListImmediate& RHICmdList) mutable
		{
			BufferToRelease.SafeRelease();
		});
	
	EasyRainUniformParametersRef.SafeRelease();
}

void FWeatherEffectsSceneViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	// if (CVarWeatherEffectsEnableRainDropPass.GetValueOnAnyThread() != 0)
	// {
	// 	CachedRainDropNormalMaskTexture = WeatherEffectsSubsystem->GetRainDropNormalMaskTexture();
	// 	CachedRainDropUVScale = WeatherEffectsSubsystem->GetRainDropUVScale();
	// 	CachedRainDropDistortionIntensity = WeatherEffectsSubsystem->GetRainDropDistortionIntensity();
	// 	CachedRainDropAnimSpeed = WeatherEffectsSubsystem->GetRainDropAnimSpeed();
	// }
}

void FWeatherEffectsSceneViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	FWorldSceneViewExtension::BeginRenderViewFamily(InViewFamily);
	
	if (CVarWeatherEffectsEnableRainWetness.GetValueOnAnyThread() == 0) return;
	
	if (!IsValid(WeatherEffectsSubsystem))
	{
		bCachedIsControllerRegistered = false;
		return;
	}
	
	// If controller is not registered, return early.
	const bool bIsControllerRegistered = WeatherEffectsSubsystem->GetWeatherController() != nullptr;
	bCachedIsControllerRegistered = bIsControllerRegistered;
	if (!bCachedIsControllerRegistered) return;
	
	CachedRainDepthTexture = WeatherEffectsSubsystem->GetRainDepthTexture();
	CachedCaptureOrthoWide = WeatherEffectsSubsystem->GetCaptureOrthoWide();
	CachedCaptureComponentPosition = WeatherEffectsSubsystem->GetCaptureComponentPosition();

	// If debug wetness shadow is enabled, skip caching wetness textures and params.
	if (CVarWeatherEffectsDebugWetnessShadow.GetValueOnAnyThread() == 1) return;

	CachedWeatherState = WeatherEffectsSubsystem->GetCurrentWeatherState();
	switch (CachedWeatherState)
	{
		case EWeatherState::Clear:
			CachedWetnessFactor = 0.0f;
			break;
		case EWeatherState::Rain:
			CachedWetnessFactor = 1.0f;
			break;
		case EWeatherState::Transition:
			CachedWetnessFactor = WeatherEffectsSubsystem->GetTransitionElapsedRatio();
			CachedRainAccumulationRTTexture = WeatherEffectsSubsystem->GetRainAccumulationRT();
			CachedRainAccumulationMaskTexture = WeatherEffectsSubsystem->GetRainAccumulationMaskTexture();
			CachedRainAccumulationBase = WeatherEffectsSubsystem->GetTransitionElapsedRatio();	
		break;
	}
	CachedEasyRainTextures = WeatherEffectsSubsystem->GetEasyRainTextures();
}

void FWeatherEffectsSceneViewExtension::PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder,
	FSceneViewFamily& InViewFamily)
{
	FWorldSceneViewExtension::PreRenderViewFamily_RenderThread(GraphBuilder, InViewFamily);
	
	if (CVarWeatherEffectsEnableRainWetness.GetValueOnRenderThread() != 1 || !bCachedIsControllerRegistered) return;
	
	{
		FWeatherEffectsEasyRainUniformParameters TempEasyRainUniformParameters;
		FEasyRainTexturesRDGRef TempEasyRainTexturesRDGRef = FEasyRainTexturesRDGRef(CachedEasyRainTextures, GraphBuilder);
     //Easy Rain Contents are excluded
		
		if (!EasyRainUniformParametersRef.IsValid())
		{
			EasyRainUniformParametersRef = TUniformBufferRef<FWeatherEffectsEasyRainUniformParameters>::CreateUniformBufferImmediate(
									TempEasyRainUniformParameters,
									UniformBuffer_MultiFrame
									);	
		}
		
		EasyRainUniformParametersRef.UpdateUniformBufferImmediate(GraphBuilder.RHICmdList, TempEasyRainUniformParameters);
	}
	
	CachedRainAccumulationTexturesRDGRef = FRainAccumulationTexturesRDGRef(CachedRainAccumulationRTTexture, CachedRainAccumulationMaskTexture, GraphBuilder);
}

void FWeatherEffectsSceneViewExtension::PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures)
{
	RDG_EVENT_SCOPE(GraphBuilder, "WeatherEffects_PostRenderBassPassDeferred");

	if (CVarWeatherEffectsEnableRainWetness.GetValueOnRenderThread() != 1 || !bCachedIsControllerRegistered) return; 

	FRDGTextureRef GBufferATexture = RenderTargets[1].GetTexture(); // GBufferA = WorldNormal
	FRDGTextureRef GBufferBTexture = RenderTargets[2].GetTexture(); // GBufferB = Metallic, Specular, Roughness, Shading ID
	FRDGTextureRef GBufferCTexture = RenderTargets[3].GetTexture(); // GBufferC = BaseColor, Alpha
	FRDGTextureRef GBufferDTexture = RenderTargets[4].GetTexture(); // GBufferD = Porosity, PuddleMask

	if (!GBufferATexture || !GBufferBTexture || !GBufferCTexture || !GBufferDTexture)
	{
		UE_LOG(LogWeatherEffects, Warning, TEXT("GBuffer missing: A:%d B:%d C:%d D:%d"), !!GBufferATexture, !!GBufferBTexture, !!GBufferCTexture, !!GBufferDTexture);
		return; 
	}
	
	CachedRainDepthTextureRDGRef = nullptr;
	
	if (!IsTextureValid(CachedRainDepthTexture)) return;

	CachedRainDepthTextureRDGRef = GraphBuilder.RegisterExternalTexture(
		CreateRenderTarget(CachedRainDepthTexture->GetResource()->GetTexture2DRHI(), TEXT("RainDepthTexture"))
	);
	
	if (CVarWeatherEffectsDebugWetnessShadow.GetValueOnRenderThread() == 1)
	{
		DrawWetnessShadowDebugPass_RenderThread(GraphBuilder,
								InView,
								SceneTextures,
								GBufferATexture,
								GBufferCTexture,
								CachedRainDepthTextureRDGRef,
								CachedCaptureOrthoWide,
								CachedCaptureComponentPosition
								);
		return;
	}
	
	if (CachedWeatherState == EWeatherState::Rain)
	{
		DrawWetnessStaticPass_RenderThread(GraphBuilder, 
											InView, 
											SceneTextures, 
											GBufferATexture, 
											GBufferBTexture,
											GBufferCTexture,
											GBufferDTexture,
											CachedRainDepthTextureRDGRef, 
											CachedCaptureOrthoWide, 
											CachedCaptureComponentPosition, 
											CachedWetnessFactor,
											EasyRainUniformParametersRef
											);				

		return;
	}
	
	// Clear to Rain transition pass.
	if (CachedWeatherState == EWeatherState::Transition)
	{	
		FRainAccumulationTexturesRDGRef RainAccumulationTextures = FRainAccumulationTexturesRDGRef(CachedRainAccumulationRTTexture,
		                                                                                           CachedRainAccumulationMaskTexture,
																									GraphBuilder);
		DrawWetnessTransitionPass_RenderThread(GraphBuilder, 
											InView, 
											SceneTextures, 
											GBufferATexture, 
											GBufferBTexture,
											GBufferCTexture,
											GBufferDTexture,
											CachedRainDepthTextureRDGRef, 
											CachedCaptureOrthoWide, 
											CachedCaptureComponentPosition, 
											CachedWetnessFactor,
											RainAccumulationTextures,
											EasyRainUniformParametersRef
											);
		return;
	}
}

void FWeatherEffectsSceneViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& InView, FPostProcessingPassDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
	if (CVarWeatherEffectsEnableRainDropPass.GetValueOnAnyThread() == 0)
	{
		return;
	}

	if (!IsValid(WeatherEffectsSubsystem))
	{
		return;
	}

	if (Pass == EPostProcessingPass::BeforeDOF)
	{
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FWeatherEffectsSceneViewExtension::DrawRainDropPass_RenderThread));
	}
}

FScreenPassTexture FWeatherEffectsSceneViewExtension::DrawRainDropPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessMaterialInputs& Inputs)
{	
	
	Inputs.Validate();

	const FScreenPassTextureSlice& SceneColorSlice = Inputs.GetInput(EPostProcessMaterialInput::SceneColor);
	const FScreenPassTexture SceneColor = FScreenPassTexture(SceneColorSlice);
	const FScreenPassTextureViewport SceneColorViewport(SceneColor);

	if (!SceneColor.IsValid())
	{
		UE_LOG(LogWeatherEffects, Warning, TEXT("SceneColor texture is invalid"));
		return SceneColor;
	}
	
	/*
	RDG_GPU_STAT_SCOPE(GraphBuilder, DrawRainDropPass);
	RDG_EVENT_SCOPE(GraphBuilder, "DrawRainDropPass");

	const FIntRect ViewRect = InView.UnscaledViewRect;
	const FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<FWeatherEffectsRainDropPS> PixelShader(GlobalShaderMap);

	check(InView.bIsViewInfo);
	const FViewInfo& ViewInfo = static_cast<const FViewInfo&>(InView);

	FSamplerStateRHIRef SceneColorSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp>::GetRHI();

	UTexture2D* LocalRainDropNormalMaskTexture = this->CachedRainDropNormalMaskTexture;
	FRDGTextureRef RainDropNormalMaskRDGTexture = nullptr;
	
	FRDGTextureDesc BackbufferDesc = SceneColor.Texture->Desc;
	BackbufferDesc.Flags |= TexCreate_RenderTargetable;
	FRDGTextureRef BackbufferTexture = GraphBuilder.CreateTexture(BackbufferDesc, TEXT("RainDropBackbufferTexture"));
	
	if (LocalRainDropNormalMaskTexture && LocalRainDropNormalMaskTexture->GetResource() && LocalRainDropNormalMaskTexture->GetResource()->GetTexture2DRHI())
	{
		RainDropNormalMaskRDGTexture = GraphBuilder.RegisterExternalTexture(
			CreateRenderTarget(LocalRainDropNormalMaskTexture->GetResource()->GetTexture2DRHI(),
				TEXT("RainDropNormalMaskTexture"))
		);
	}
	else
	{
		UE_LOG(LogWeatherEffects, Warning, TEXT("WeatherEffectsSceneExtensionView :: Fail to load Raindrop Normal Mask Texture or Resource"));
		return SceneColor;
	}

	FSamplerStateRHIRef RainDropNormalMaskSampler = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap>::GetRHI();

	FWeatherEffectsRainDropPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FWeatherEffectsRainDropPS::FParameters>();
	{	
		// RainDrop Parameters
		PassParameters->RainDropUVScale = CachedRainDropUVScale;
		PassParameters->RainDropDistortionIntensity = CachedRainDropDistortionIntensity;
		PassParameters->RainDropAnimSpeed = CachedRainDropAnimSpeed;

		// SceneColor textures
		PassParameters->SceneColorTexture = SceneColor.Texture;
		PassParameters->SceneColorSampler = SceneColorSampler;

		// Rain drop textures
		PassParameters->RainDropNormalMaskTexture = RainDropNormalMaskRDGTexture;
		PassParameters->RainDropNormalMaskSampler = RainDropNormalMaskSampler;

		PassParameters->View = ViewInfo.ViewUniformBuffer;
		PassParameters->Output = GetScreenPassTextureViewportParameters(SceneColorViewport);
		PassParameters->RenderTargets[0] = FRenderTargetBinding(BackbufferTexture, ERenderTargetLoadAction::ENoAction);
	}

	AddDrawScreenPass(
		GraphBuilder, 
		RDG_EVENT_NAME("WeatherEffectsRainDropPass"), 
		ViewInfo, 
		SceneColorViewport, 
		SceneColorViewport, 
		PixelShader, 
		PassParameters
	);
	
	return FScreenPassTexture(BackbufferTexture, SceneColor.ViewRect);
	*/
	return SceneColor;
}







