#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "WeatherEffectsTypes.generated.h"

class FRDGBuilder;

UENUM(BlueprintType)
enum class EWeatherState : uint8
{
	Clear,
	Rain,
	Transition
};

USTRUCT(BlueprintType)
struct FEasyRainTextures
{
	GENERATED_BODY()

  // Easy Rain Contents are excluded.

};

struct FEasyRainTexturesRDGRef
{
  // Easy Rain Contents are excluded.
	
	FEasyRainTexturesRDGRef() = default;
	
	FEasyRainTexturesRDGRef(const FEasyRainTextures& InTextureSet, FRDGBuilder& GraphBuilder);
};

struct FRainAccumulationTexturesRDGRef
{
	FRDGTextureRef RainAccumulationRT = nullptr;
	FRDGTextureRef RainAccumulationMask = nullptr;
	
	FRainAccumulationTexturesRDGRef() = default;
	
	FRainAccumulationTexturesRDGRef(const UTextureRenderTarget2D* InAccumulationRT, 
											const UTexture2D* InAccumulationMask,
											FRDGBuilder& GraphBuilder);
};
