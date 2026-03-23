#include "WeatherEffectsTypes.h"
#include "RenderGraphBuilder.h"

namespace WeatherEffectsRDGUtils
{
	FRDGTextureRef RegisterRainTexture(FRDGBuilder& GraphBuilder, auto* Texture, const TCHAR* Name)
	{
		if (!Texture || !Texture->GetResource())
		{
			return GraphBuilder.RegisterExternalTexture(CreateRenderTarget(GBlackTexture->GetTextureRHI(), Name));	
		}
		return GraphBuilder.RegisterExternalTexture(CreateRenderTarget(Texture->GetResource()->GetTexture2DRHI(), Name));
	}
}

FEasyRainTexturesRDGRef::FEasyRainTexturesRDGRef(const FEasyRainTextures& InTextureSet, FRDGBuilder& GraphBuilder)
{
  // Easy Rain Contents are excluded.
}

FRainAccumulationTexturesRDGRef::FRainAccumulationTexturesRDGRef(const UTextureRenderTarget2D* InAccumulationRT, const UTexture2D* InAccumulationMask, FRDGBuilder& GraphBuilder)
{
	RainAccumulationRT = WeatherEffectsRDGUtils::RegisterRainTexture(GraphBuilder, InAccumulationRT, TEXT("RainAccumulationRT"));
	RainAccumulationMask = WeatherEffectsRDGUtils::RegisterRainTexture(GraphBuilder, InAccumulationMask, TEXT("RainAccumulationMask"));
}
