#pragma once
#include "CoreMinimal.h"
#include "RenderGraphFwd.h"
struct FDeltaLensSettings { float Size=1.0f, Inset=0.06f, Curve=0.22f; FVector2f EyeOffset[2]={FVector2f::ZeroVector,FVector2f::ZeroVector}; };
void DeltaDrawLens(FRDGBuilder& GraphBuilder, FRDGTextureRef BackBuffer, FRDGTextureRef Source, FDeltaLensSettings Settings);
