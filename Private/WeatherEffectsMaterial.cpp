// Fill out your copyright notice in the Description page of Project Settings.
#include "WeatherEffectsMaterial.h"
#include "ShaderParameterUtils.h"
#include "RHI.h"
#include "RenderGraphResources.h"
#include "SceneManagement.h"
#include "ScreenPass.h"
#include "SceneTexturesConfig.h"
#include "Engine/Texture.h"
#include "SceneView.h"

#pragma region Shader Parameters
	BEGIN_SHADER_PARAMETER_STRUCT(FWeatherEffectsWetnessParameters, )
		SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTexturesStruct)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Output)

		SHADER_PARAMETER(float, WetnessFactor)
		SHADER_PARAMETER(float, CaptureOrthoWide)
		SHADER_PARAMETER(FVector3f, CaptureComponentPosition)
		SHADER_PARAMETER_ARRAY(FUintVector4, HeightMapPageTableUniform, [2])
		SHADER_PARAMETER(FUintVector4, HeightMapUniform)

		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, RainDepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, WorldNormalTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, MSRITexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BaseColorTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, WeatherEffectsTexture)

		SHADER_PARAMETER_SAMPLER(SamplerState, RainDepthTextureSampler)
	END_SHADER_PARAMETER_STRUCT()

	BEGIN_SHADER_PARAMETER_STRUCT(FWeatherEffectsRainAccumulationParameters, )
		SHADER_PARAMETER_TEXTURE(Texture2D, RainAccumulationRTTexture)
		SHADER_PARAMETER_TEXTURE(Texture2D, RainAccumulationMaskTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, RainAccumulationSampler)
	END_GLOBAL_SHADER_PARAMETER_STRUCT()

	// Wetness Effect
	class FWeatherEffectsWetnessPS : public FGlobalShader
	{	
		DECLARE_GLOBAL_SHADER(FWeatherEffectsWetnessPS);
		static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, 
												FShaderCompilerEnvironment& OutEnvironment)
		{
			FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		}
		
		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT_INCLUDE(FWeatherEffectsWetnessParameters, CommonParameters)
		END_SHADER_PARAMETER_STRUCT()
	};

	class FWeatherEffectsWetnessTransitionPS : public FWeatherEffectsWetnessPS
	{
		DECLARE_GLOBAL_SHADER(FWeatherEffectsWetnessTransitionPS);
		SHADER_USE_PARAMETER_STRUCT(FWeatherEffectsWetnessTransitionPS, FWeatherEffectsWetnessPS)
		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT_INCLUDE(FWeatherEffectsWetnessParameters, CommonParameters)
			SHADER_PARAMETER_STRUCT_INCLUDE(FWeatherEffectsRainAccumulationParameters, RainAccumulationParameters)
			SHADER_PARAMETER_STRUCT_REF(FWeatherEffectsEasyRainUniformParameters, EasyRainParameters)
			RENDER_TARGET_BINDING_SLOTS()
		END_SHADER_PARAMETER_STRUCT()
	};

	class FWeatherEffectsWetnessStaticPS : public FWeatherEffectsWetnessPS
	{
		DECLARE_GLOBAL_SHADER(FWeatherEffectsWetnessStaticPS);
		SHADER_USE_PARAMETER_STRUCT(FWeatherEffectsWetnessStaticPS, FWeatherEffectsWetnessPS)
		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT_INCLUDE(FWeatherEffectsWetnessParameters, CommonParameters)
			SHADER_PARAMETER_STRUCT_REF(FWeatherEffectsEasyRainUniformParameters, EasyRainParameters)
			RENDER_TARGET_BINDING_SLOTS()
		END_SHADER_PARAMETER_STRUCT()
	};

	// Screen rain drop effects
	class FWeatherEffectsRainDropPS : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FWeatherEffectsRainDropPS);
		SHADER_USE_PARAMETER_STRUCT(FWeatherEffectsRainDropPS, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Output)
		
			SHADER_PARAMETER(float, RainDropUVScale)
			SHADER_PARAMETER(float, RainDropDistortionIntensity)
			SHADER_PARAMETER(float, RainDropAnimSpeed)
		
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, RainDropNormalMaskTexture)

			SHADER_PARAMETER_SAMPLER(SamplerState, SceneColorSampler)
			SHADER_PARAMETER_SAMPLER(SamplerState, RainDropNormalMaskSampler)
		
			RENDER_TARGET_BINDING_SLOTS()
		END_SHADER_PARAMETER_STRUCT()
	};

	// Debug
	class FWeatherEffectsDebugWetnessShadowPS : public FGlobalShader
	{	
		DECLARE_GLOBAL_SHADER(FWeatherEffectsDebugWetnessShadowPS);
		SHADER_USE_PARAMETER_STRUCT(FWeatherEffectsDebugWetnessShadowPS, FGlobalShader)
		
		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTexturesStruct)
			SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Output)
		
			SHADER_PARAMETER(float, CaptureOrthoWide)
			SHADER_PARAMETER(FVector3f, CaptureComponentPosition)
		
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, WorldNormalTexture)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BaseColorTexture)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, RainDepthTexture)
			SHADER_PARAMETER_SAMPLER(SamplerState, RainDepthTextureSampler)
		
			RENDER_TARGET_BINDING_SLOTS()
		END_SHADER_PARAMETER_STRUCT()
	};

	IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FWeatherEffectsEasyRainUniformParameters, "EasyRainParameters");
	IMPLEMENT_GLOBAL_SHADER(FWeatherEffectsWetnessPS, "/Plugins/WeatherEffectsShader/WeatherEffectsPixelShader.usf", "WetnessPS", SF_Pixel);
	IMPLEMENT_GLOBAL_SHADER(FWeatherEffectsWetnessStaticPS, "/Plugins/WeatherEffectsShader/WeatherEffectsPixelShader.usf", "WetnessStaticPS", SF_Pixel);
	IMPLEMENT_GLOBAL_SHADER(FWeatherEffectsWetnessTransitionPS, "/Plugins/WeatherEffectsShader/WeatherEffectsPixelShader.usf", "WetnessTransitionPS", SF_Pixel);
	IMPLEMENT_GLOBAL_SHADER(FWeatherEffectsRainDropPS, "/Plugins/WeatherEffectsShader/WeatherEffectsRainDrop.usf", "MainPS", SF_Pixel);
	IMPLEMENT_GLOBAL_SHADER(FWeatherEffectsDebugWetnessShadowPS, "/Plugins/WeatherEffectsShader/WeatherEffectsDebugShader.usf", "DebugWetnessShadowPS", SF_Pixel);
#pragma endregion

#pragma region DrawPass
	void DrawWetnessShadowDebugPass_RenderThread(FRDGBuilder& GraphBuilder, 
												FSceneView& View,
												TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures,
												const FRDGTextureRef GBufferATexture,
												const FRDGTextureRef GBufferCTexture,	
												const FRDGTextureRef RainDepthTexture,
												float CaptureOrthoWide,
												FVector3f CaptureComponentPosition
												)
	{
		RDG_EVENT_SCOPE(GraphBuilder, "WeatherEffects_DebugShadow");
		
		FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
		TShaderMapRef<FWeatherEffectsDebugWetnessShadowPS> PixelShader(GlobalShaderMap);
		
		FIntRect ViewRect = View.UnscaledViewRect;
		
		FRDGTextureDesc BackbufferDesc = GBufferCTexture->Desc;
		BackbufferDesc.Flags = TexCreate_RenderTargetable;
		FRDGTextureRef BackbufferTexture = GraphBuilder.CreateTexture(BackbufferDesc, TEXT("BackbufferTexture"));
		const FScreenPassTextureViewport BackbufferTextureViewport(GBufferCTexture, ViewRect);
		
		auto PassParameters = GraphBuilder.AllocParameters<FWeatherEffectsDebugWetnessShadowPS::FParameters>();
		{	
			PassParameters->RenderTargets[0] = FRenderTargetBinding(BackbufferTexture, ERenderTargetLoadAction::ENoAction);

			PassParameters->SceneTexturesStruct = SceneTextures;
			PassParameters->View = View.ViewUniformBuffer;
			PassParameters->Output = GetScreenPassTextureViewportParameters(BackbufferTextureViewport);
			PassParameters->CaptureOrthoWide = CaptureOrthoWide;
			PassParameters->CaptureComponentPosition = CaptureComponentPosition;
			
			PassParameters->WorldNormalTexture = GBufferATexture;
			PassParameters->BaseColorTexture = GBufferCTexture;
			PassParameters->RainDepthTexture = RainDepthTexture;
			PassParameters->RainDepthTextureSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp>::GetRHI();

			AddDrawScreenPass(
				GraphBuilder,
				RDG_EVENT_NAME("WeatherEffectsDebugWetnessShadowPass"),
				FScreenPassViewInfo(View),
				BackbufferTextureViewport,
				BackbufferTextureViewport,
				PixelShader,
				PassParameters
				);
			
			AddCopyTexturePass(GraphBuilder, BackbufferTexture, GBufferCTexture);
		}
	}

	void DrawWetnessStaticPass_RenderThread(FRDGBuilder& GraphBuilder, 
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
											//const FEasyRainTexturesRDGRef& EasyRainTextures
											)
	{
		FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
		TShaderMapRef<FWeatherEffectsWetnessStaticPS> PixelShader(GlobalShaderMap);

		FIntRect ViewRect = View.UnscaledViewRect;
		
		FRDGTextureDesc BackbufferDesc = GBufferBTexture->Desc;
		BackbufferDesc.Flags = TexCreate_RenderTargetable;
		FRDGTextureRef BackbufferBTexture = GraphBuilder.CreateTexture(BackbufferDesc, TEXT("BackbufferBTexture"));
		const FScreenPassTextureViewport BackbufferTextureViewport(GBufferCTexture, ViewRect);
		
		FRDGTextureDesc BackbufferADesc = GBufferATexture->Desc;
		BackbufferADesc.Flags = TexCreate_RenderTargetable;
		FRDGTextureRef BackbufferATexture = GraphBuilder.CreateTexture(BackbufferADesc, TEXT("BackbufferATexture"));
	
		FRDGTextureDesc BackbufferCDesc = GBufferCTexture->Desc;
		BackbufferCDesc.Flags = TexCreate_RenderTargetable;
		FRDGTextureRef BackbufferCTexture = GraphBuilder.CreateTexture(BackbufferCDesc, TEXT("BackbufferCTexture"));
		
		auto PassParameters = GraphBuilder.AllocParameters<FWeatherEffectsWetnessStaticPS::FParameters>();
		{
			SetCommonShaderParameters(
				PassParameters,
				SceneTextures,
				View,
				BackbufferTextureViewport,
				GBufferATexture,
				GBufferBTexture,
				GBufferCTexture,
				GBufferDTexture,
				RainDepthTexture,
				BackbufferATexture,
				BackbufferBTexture,
				BackbufferCTexture,
				CaptureOrthoWide,
				CaptureComponentPosition,
				CachedWetnessFactor
				);
			
		PassParameters->EasyRainParameters = EasyRainUniformParameters;
		}
		
		AddDrawScreenPass(
				GraphBuilder,
				RDG_EVENT_NAME("WeatherEffectsWetnessStaticPass"),
				View,
				BackbufferTextureViewport,
				BackbufferTextureViewport,
				PixelShader,
				PassParameters
				);
				
		AddCopyTexturePass(GraphBuilder, BackbufferATexture, GBufferATexture);
		AddCopyTexturePass(GraphBuilder, BackbufferBTexture, GBufferBTexture);
		AddCopyTexturePass(GraphBuilder, BackbufferCTexture, GBufferCTexture);
	}

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
								)
	{
		FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
		TShaderMapRef<FWeatherEffectsWetnessTransitionPS> PixelShader(GlobalShaderMap);
		
		FIntRect ViewRect = View.UnscaledViewRect;
		
		FRDGTextureDesc BackbufferDesc = GBufferBTexture->Desc;
		BackbufferDesc.Flags = TexCreate_RenderTargetable;
		FRDGTextureRef BackbufferBTexture = GraphBuilder.CreateTexture(BackbufferDesc, TEXT("BackbufferBTexture"));
		const FScreenPassTextureViewport BackbufferTextureViewport(GBufferCTexture, ViewRect);
		
		FRDGTextureDesc BackbufferADesc = GBufferATexture->Desc;
		BackbufferADesc.Flags = TexCreate_RenderTargetable;
		FRDGTextureRef BackbufferATexture = GraphBuilder.CreateTexture(BackbufferADesc, TEXT("BackbufferATexture"));
	
		FRDGTextureDesc BackbufferCDesc = GBufferCTexture->Desc;
		BackbufferCDesc.Flags = TexCreate_RenderTargetable;
		FRDGTextureRef BackbufferCTexture = GraphBuilder.CreateTexture(BackbufferCDesc, TEXT("BackbufferCTexture"));
		
		auto PassParameters = GraphBuilder.AllocParameters<FWeatherEffectsWetnessTransitionPS::FParameters>();
		{
			SetCommonShaderParameters(
				PassParameters,
				SceneTextures,
				View,
				BackbufferTextureViewport,
				GBufferATexture,
				GBufferBTexture,
				GBufferCTexture,
				GBufferDTexture,
				RainDepthTexture,
				BackbufferATexture,
				BackbufferBTexture,
				BackbufferCTexture,
				CaptureOrthoWide,
				CaptureComponentPosition,
				CachedWetnessFactor
				);
			
		PassParameters->RainAccumulationParameters.RainAccumulationRTTexture = RainAccumulationTextures.RainAccumulationRT->GetRHI();	
		PassParameters->RainAccumulationParameters.RainAccumulationMaskTexture = RainAccumulationTextures.RainAccumulationMask->GetRHI();
		PassParameters->RainAccumulationParameters.RainAccumulationSampler = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap>::GetRHI();
		PassParameters->EasyRainParameters = EasyRainUniformParameters;
		}
		
		AddDrawScreenPass(
				GraphBuilder,
				RDG_EVENT_NAME("WeatherEffectsWetnessTransitionPass"),
				View,
				BackbufferTextureViewport,
				BackbufferTextureViewport,
				PixelShader,
				PassParameters
				);
				
		AddCopyTexturePass(GraphBuilder, BackbufferATexture, GBufferATexture);
		AddCopyTexturePass(GraphBuilder, BackbufferBTexture, GBufferBTexture);
		AddCopyTexturePass(GraphBuilder, BackbufferCTexture, GBufferCTexture);
	}
#pragma endregion

